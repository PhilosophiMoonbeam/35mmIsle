// Included once by photo_replay.cpp to share recorded-item and projection mechanics.
namespace isle::photo {
struct Presentation::Impl {
    struct Course {
        std::shared_ptr<world::Resources> resources;
        std::unique_ptr<World> world;
        std::unique_ptr<world::Producer> producer;
        std::unique_ptr<items::Resources> items;
        std::unique_ptr<menu::EffectGraphicsResources> graphics;
        std::unique_ptr<photo_effects::Renderer> effects;
        bool published = false;
    };
    struct Plan {
        IslePlayerPhotoData record{};
        Course* course = nullptr;
        // assets/Subjects borrow these immutable installation bytes even if
        // Replay later switches courses and releases its current Resources.
        std::shared_ptr<world::Resources> world_resource_owner;
        std::unique_ptr<subjects::Resources> resources;
        std::unique_ptr<subjects::Reconstruction> subjects;
        std::unique_ptr<RecordedItems> items;
        std::unique_ptr<menu::model::PacketProducer> subject_producer, item_producer;
        struct Asset { std::uint32_t address; std::span<const std::byte> bytes; };
        std::vector<Asset> assets;
        std::vector<sprite::AssetSpan> immutable;
    };
    IsleGame* game;
    const rom::Snapshot& snapshot;
    world::ModuleState& module;
    application::Photos& photos;
    PresentationConfig config;
    Presentation* display_owner = nullptr;
    bool display_storage_active = false;
    LightingInputs lighting;
    menu::EffectResources effects;
    photo_effects::Packet effect_packet;
    menu::model::CameraBasisState basis;
    std::vector<Range> external, owned, render_owned, all, world_other, subject_other, item_other;
    std::vector<sprite::RamRange> effect_protected;
    std::vector<std::unique_ptr<Plan>> plans;
    std::array<Course,7> courses;
    std::array<std::vector<Command>,4> lanes;
    std::array<std::size_t,4> counts{};
    std::vector<Callback> order;
    std::size_t order_count = 0;
    Plan* active = nullptr;
    Course* current_course = nullptr;
    IsleGameCamera* camera = nullptr;
    IsleGameObject* effect_object = nullptr;
    bool effect_observed = false, effect_requested = false, effect_joined = false;
    bool retained = false, canonical = false, ready = false, clearing = false, retiring = false, retired = false;
    bool failed = false, publication_started = false;
    bool handoff_started = false;
    ReplayWorldHandoff returning_world;
    IslePhotoRecord canonical_record{};
    IslePhotoWorkingRead working_read{};
    const std::uint64_t* revision = nullptr;
    const IslePlayerPhotoData* source_record = nullptr;
    std::uint64_t expected_revision = 0, epoch = 0, resource_revision = 0;
    std::array<char,512> message{};
    Impl(IsleGame* g, const rom::Snapshot& s, world::ModuleState& m, application::Photos& p, PresentationConfig c)
        : game(g),snapshot(s),module(m),photos(p),config(c),lighting(s),effects(s),
          external(c.external_reservations.begin(),c.external_reservations.end()),plans(c.record_capacity) {
        require(game && c.frame && c.thumbnail_replay && c.record_capacity && c.record_capacity <= 256,"Presentation needs actual Game/Frame/shared Replay and bounded cold capacity");
        require(c.dl_link < 32 && c.world_object_link < 32 && c.photo_object_link < 32,
            "Presentation Source link tuple is invalid");
        config.external_reservations = external;
        auto& l = config.memory;
        require(l.commands.size >= 0x1000 && l.effects.size >= photo_effects::Renderer::max_words*4,
            "Presentation command/effect reservation is insufficient");
        l.subjects.framebuffers = l.items.framebuffers = c.framebuffers;
        owned = {l.commands,l.world.payloads,l.world.lists,l.world.materials,l.subjects.payloads,l.subjects.lists,
            l.subjects.materials,l.items.payloads,l.items.lists,l.items.materials,l.effects,l.effect_assets};
        for (std::size_t i=0;i<owned.size();++i) {
            valid(owned[i],64);
            require(!overlap(owned[i],{0,graphics::bss_end}),"Presentation reservation aliases cold image");
            for (std::size_t j=0;j<i;++j) require(!overlap(owned[i],owned[j]),"Presentation reservations overlap");
            for (const auto r:external) { valid(r,1); require(!overlap(owned[i],r),"Presentation aliases Frame or scene storage"); }
        }
        for (const auto fb:c.framebuffers) {
            valid(fb,64); bool found=false;
            for (const auto r:external) if (r.offset==fb.offset && r.size==fb.size) found=true;
            require(found,"Presentation framebuffer missing from complete external plan");
        }
        all=owned; all.insert(all.end(),external.begin(),external.end());
        for(const auto r:owned) if(r.offset!=l.effect_assets.offset) render_owned.push_back(r);
        const auto other=[&](std::span<const Range> excluded,std::vector<Range>& output,bool omit_fb) {
            for (const auto r:all) {
                bool omit=false;
                for (const auto e:excluded) if(r.offset==e.offset && r.size==e.size) omit=true;
                if(omit_fb) for(const auto f:c.framebuffers) if(r.offset==f.offset && r.size==f.size) omit=true;
                if(!omit) output.push_back(r);
            }
        };
        const std::array wr{l.world.payloads,l.world.lists,l.world.materials}; other(wr,world_other,false);
        const std::array sr{l.subjects.payloads,l.subjects.lists,l.subjects.materials}; other(sr,subject_other,true);
        const std::array ir{l.items.payloads,l.items.lists,l.items.materials}; other(ir,item_other,true);
        for(const auto r:all) if(!((r.offset==l.effects.offset && r.size==l.effects.size) ||
            (r.offset==l.effect_assets.offset && r.size==l.effect_assets.size)))
            effect_protected.push_back({static_cast<std::uint32_t>(r.offset),static_cast<std::uint32_t>(r.size)});
        for(auto& lane:lanes) lane.resize(l.commands.size/8);
        order.resize(l.commands.size/64);
    }
    void fail(const char* text) noexcept {
        if(!failed) { const auto n=std::min(std::strlen(text),message.size()-1); std::memcpy(message.data(),text,n); message[n]=0; }
        failed=true; clearing=true; ready=false;
    }
    bool current() const noexcept {
        if(!retained || !source_record || !revision || *revision!=expected_revision) return false;
        if(!canonical && isle_photo_working_validate_read(&working_read)!=ISLE_GAME_OK) return false;
        return !active || same(*source_record,active->record);
    }
    void release() {
        if(!retained) return;
        if(canonical) require(!photos.release_replay(this),"Presentation canonical lease release rejected");
        else native(isle_photo_working_release_read(&working_read));
        retained=false; canonical=false; source_record=nullptr; revision=nullptr; canonical_record={}; expected_revision=0;
    }
    bool take_shared_world() {
        if(handoff_started) return true;
        ReplayWorldHandoff handoff;
        const auto result=config.thumbnail_replay->take_presentation_world(game,this,source_record->levelID,
            config.dl_link,config.world_object_link,config.photo_object_link,handoff);
        if(result==ReplayAcquire::pending) return false;
        require(result==ReplayAcquire::admitted,"Presentation shared World admission rejected");
        const auto index=static_cast<std::size_t>(menu::effect_group(source_record->levelID))-static_cast<std::size_t>(menu::EffectGroup::beach);
        auto& c=courses[index];
        c.resources=std::move(handoff.resources);c.world=std::move(handoff.world);c.published=true;
        current_course=&c;handoff_started=true;
        require(config.frame->begin_source_display_storage(display_owner),
            "Presentation shared RAM handoff rejected by actual Frame");
        display_storage_active=true;
        return true;
    }
    void return_display_storage() {
        if(!display_storage_active)return;
        require(config.frame->end_source_display_storage(display_owner),
            "Presentation shared RAM return still has actual Frame borrowers");
        display_storage_active=false;
    }
    Course& course(std::int32_t id) {
        const auto index=static_cast<std::size_t>(menu::effect_group(id))-static_cast<std::size_t>(menu::EffectGroup::beach);
        require(index<courses.size(),"Presentation course cache index escapes original courses");
        auto& c=courses[index];
        if(!c.resources) {
            c.resources=std::make_shared<world::Resources>(snapshot,id);
            c.world=std::make_unique<World>(game,*c.resources,module,config.world_object_link,config.dl_link);
        }
        if(!c.items) {
            const auto& setup=c.resources->setup();
            c.items=std::make_unique<items::Resources>(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
            const std::array banks{&effects.bank(menu::EffectGroup::common),&effects.bank(menu::effect_group(id))};
            c.graphics=std::make_unique<menu::EffectGraphicsResources>(snapshot,banks,config.memory.effect_assets);
            c.effects=std::make_unique<photo_effects::Renderer>(effects,*c.graphics,id);
            require(bool(c.effects->status()),"Presentation effect bank admission rejected");
        }
        return c;
    }
    Plan& admit() {
        require(current(),"Presentation photo read capability changed before admission");
        for(auto& p:plans) if(p && same(p->record,*source_record)) return *p;
        auto slot=std::find_if(plans.begin(),plans.end(),[](const auto& p){return !p;});
        require(slot!=plans.end(),"Presentation cold record cache capacity exhausted");
        auto p=std::make_unique<Plan>(); p->record=*source_record; p->course=&course(p->record.levelID);
        require(std::isfinite(p->record.unk_04.f32) && p->record.unk_04.f32>=0,"Presentation time is invalid");
        finite(p->record.unk_08); finite(p->record.unk_14);
        p->world_resource_owner=p->course->resources;
        p->resources=std::make_unique<subjects::Resources>(snapshot,*p->world_resource_owner,p->record);
        p->subjects=std::make_unique<subjects::Reconstruction>(game,*p->resources,subjects::LinkOwnership{static_cast<std::uint8_t>(config.photo_object_link),static_cast<std::uint8_t>(config.dl_link),UINT32_MAX});
        p->items=std::make_unique<RecordedItems>(game,*p->course->items,p->record,config.photo_object_link,config.dl_link);
        if(!p->resources->instances().empty()) p->subject_producer=std::make_unique<menu::model::PacketProducer>(game,camera,basis,
            p->resources->packet_resources(),p->resources->capacities(),p->subjects->bindings(),config.memory.subjects,subject_other);
        if(!p->items->capacities().empty()) p->item_producer=std::make_unique<menu::model::PacketProducer>(game,camera,basis,
            menu::model::PacketResources{p->course->items->models(),p->course->items->materials(),p->course->items->installation_spans()},
            p->items->capacities(),p->items->bindings(),config.memory.items,item_other);
        const auto add=[&](std::uint32_t address,std::span<const std::byte> bytes,bool fx=false) {
            if(bytes.empty()) return;
            require(address<=ram_bytes && bytes.size()<=ram_bytes-address,"Presentation asset escapes RDRAM");
            const Range asset{address,bytes.size()};
            require(!overlap(asset,{0,graphics::bss_end}),"Presentation asset aliases cold image");
            for(const auto r:all) {
                if(fx && r.offset==config.memory.effect_assets.offset && r.size==config.memory.effect_assets.size) continue;
                require(!overlap(asset,r),"Presentation immutable resource aliases runtime reservation");
            }
            p->assets.push_back({address,bytes});
        };
        for(const auto& span:p->course->resources->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for(const auto& span:p->course->items->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for(const auto& span:p->resources->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for(const auto& span:lighting.installation_spans()) if(span.guest_address!=lighting.normal_setup_guest()) add(span.guest_address-0x80000000u,span.bytes);
        for(const auto& span:p->course->graphics->installation_spans()) add(span.address,std::as_bytes(span.bytes),true);
        std::sort(p->assets.begin(),p->assets.end(),[](const auto& a,const auto& b){return a.address<b.address;});
        for(std::size_t i=0;i<p->assets.size();++i) for(std::size_t j=0;j<i;++j) {
            const auto& a=p->assets[j]; const auto& b=p->assets[i];
            if(b.address>=a.address+a.bytes.size()) continue;
            const auto n=std::min(a.address+a.bytes.size()-b.address,b.bytes.size());
            require(std::equal(b.bytes.begin(),b.bytes.begin()+static_cast<std::ptrdiff_t>(n),a.bytes.begin()+static_cast<std::ptrdiff_t>(b.address-a.address)),
                "Presentation shared immutable resource provenance differs");
        }
        // Publish a disjoint identical-provenance union to the Frame's own
        // immutable admission, rather than hiding our uploads from its planner.
        std::size_t emitted=0;
        for(const auto asset:p->assets) {
            auto next=asset;
            if(emitted) {
                const auto& previous=p->assets[emitted-1];
                const auto end=previous.address+previous.bytes.size();
                if(next.address<end) {
                    const auto skip=end-next.address;
                    if(skip>=next.bytes.size()) continue;
                    next.address=static_cast<std::uint32_t>(end); next.bytes=next.bytes.subspan(skip);
                }
            }
            p->assets[emitted++]=next;
        }
        p->assets.resize(emitted);
        p->immutable.reserve(emitted);
        for(const auto& asset:p->assets) p->immutable.push_back({asset.address,
            {reinterpret_cast<const std::uint8_t*>(asset.bytes.data()),asset.bytes.size()}});
        *slot=std::move(p); return **slot;
    }
    bool course_ready(Course& c) {
        if(current_course && current_course!=&c) {
            if(!current_course->world->clear()) {
                require(current_course->world->error().empty(),current_course->world->error().c_str()); return false;
            }
            current_course->published=false; current_course=nullptr;
        }
        if(!c.published) {
            if(c.world->cleared()) require(c.world->rearm(),"Presentation World rearm rejected");
            current_course=&c;
            if(config.background_rgb)*config.background_rgb=c.resources->setup().background_rgb;
            require(c.world->publish(),"Presentation World publication rejected"); c.published=true;
        }
            if(!c.producer) c.producer=std::make_unique<world::Producer>(game,camera,basis,*c.resources,c.world->render_view(),config.memory.world,world_other);
        return true;
    }
    bool publish() {
        require(active && current(),"Presentation retained record changed");
        if(!course_ready(*active->course)) return false;
        if(!publication_started) {
            publication_started=true;
            require(active->course->world->reconstruct({active->record.unk_00_16,active->record.unk_04.f32,active->record.unk_08}),"Presentation World reconstruction rejected");
            if(active->subjects->cleared()) require(active->subjects->rearm(),"Presentation subjects rearm rejected");
            native(active->subjects->publish()); active->items->publish();
            native(isle_game_add_object(game,128,config.photo_object_link,0x80000000u,nullptr,nullptr,&effect_object));
            native(isle_game_object_set_retire_callback(game,effect_object,[](IsleGameObject*,void* context) noexcept {
                static_cast<Impl*>(context)->effect_joined=true;
            },this)); effect_observed=true;
            const IsleGameCameraParameters* saved=nullptr; native(isle_game_camera_parameters(game,camera,&saved));
            auto perspective=saved->perspective; perspective.fovy=30; perspective.near_plane=10; perspective.far_plane=25600;
            native(isle_game_camera_set_perspective(game,camera,&perspective));
            const auto& r=active->record;
            const IsleGameLookAt look{{r.unk_08.x,r.unk_08.y,r.unk_08.z},{r.unk_14.x,r.unk_14.y,r.unk_14.z},{0,1,0}};
            native(isle_game_camera_set_look_at(game,camera,&look)); native(isle_game_camera_compute_all(game,camera));
            IsleGameCameraRenderState state{}; native(isle_game_camera_render_state(game,camera,&state));
            if(r.levelID==course::cave_id) { state.flags|=2u; state.background_rgba=0x05080401; } else state.flags&=~2u;
            native(isle_game_camera_set_render_state(game,camera,&state));
        }
        ready=true; return true;
    }
    bool photo_clear() {
        effect_packet = {}; // Caller has already joined actual Frame leases.
        if(effect_object) {
            if(!effect_observed) {
                native(isle_game_object_set_retire_callback(game,effect_object,[](IsleGameObject*,void* context) noexcept {static_cast<Impl*>(context)->effect_joined=true;},this));
                effect_observed=true;
            }
            if(!effect_joined && !effect_requested) { native(isle_game_delete_object(game,effect_object)); effect_requested=true; }
            if(!effect_joined) return false;
            effect_object=nullptr; effect_observed=effect_requested=effect_joined=false;
        }
        if(active) {
            if(!active->items->clear()) return false;
            native(active->subjects->clear()); if(!active->subjects->cleared()) return false;
            require(resource_revision!=UINT64_MAX,"Presentation resource incarnation exhausted");
            ++resource_revision;
        }
        release(); active=nullptr; publication_started=false; ready=false; clearing=false; return true;
    }
    void command(unsigned lane,std::uint32_t a,std::uint32_t b) {
        require(lane<4 && counts[lane]<lanes[lane].size(),"Presentation composed lane capacity exhausted");
        lanes[lane][counts[lane]++]={a,b};
    }
    template<class Frame> void collect(const Frame& f) {
        for(const auto& object:f.objects) {
            require(order_count<order.size(),"Presentation callback capacity exhausted");
            auto& o=order[order_count]; o={object.object,object.render,object.lanes,f.reservations,false,order_count};
            if constexpr(requires{object.resets_camera;}) o.resets_camera=object.resets_camera;
            for(std::size_t i=0;i<order_count;++i) require(order[i].object!=o.object,"Presentation duplicate producer object");
            ++order_count;
        }
    }
    void raw(unsigned lane,List list,std::span<const Range> reservations,std::span<const std::uint32_t> words) {
        if(!list.commands) return;
        bool contained=false;
        for(const auto r:reservations) if(contains(r,list.address,std::size_t(list.commands)*8)) contained=true;
        require(contained && !(list.address&7u),"Presentation callback escapes producer reservation");
        for(std::size_t i=0;i<list.commands;++i) {
            const auto at=list.address/4+i*2;
            require(words[at]!=0xdf000000,"Presentation raw callback contains unexpected EndDL");
            command(lane,words[at],words[at+1]);
        }
    }
    menu::EffectCamera effect_camera() {
        const IsleGameCameraMatrix *projection=nullptr,*view=nullptr;
        native(isle_game_camera_matrix(game,camera,0,&projection)); native(isle_game_camera_matrix(game,camera,1,&view));
        menu::EffectCamera result;
        for(unsigned row=0;row<4;++row) for(unsigned col=0;col<4;++col) {
            volatile float p0=view->floating[row][0]*projection->floating[0][col];
            volatile float p1=view->floating[row][1]*projection->floating[1][col];
            volatile float p2=view->floating[row][2]*projection->floating[2][col];
            volatile float p3=view->floating[row][3]*projection->floating[3][col];
            volatile float s0=p0+p1; volatile float s1=s0+p2; result.projection[row][col]=s1+p3;
        }
        const IsleGameCameraParameters* p=nullptr; native(isle_game_camera_parameters(game,camera,&p));
        for(unsigned i=0;i<3;++i) {result.viewport_scale[i]=p->viewport.scale[i];result.viewport_translate[i]=p->viewport.translate[i];}
        return result;
    }
    bool prepare(std::span<std::uint32_t> words,std::uint64_t e,const scene::render::Pass& pass,scene::render::Prepared& output) {
        require(ready && !clearing && !retiring && !failed && current() && active,"Presentation prepare lacks live retained photo");
        require(pass.camera==camera && pass.kind==scene::render::Kind::compiled && e && e>=epoch && words.size_bytes()==ram_bytes,
            "Presentation prepare camera/epoch/RAM capability rejected");
        require(active->course->world->available() && !active->subjects->failed() && !active->items->externally_retired() && !effect_joined,
            "Presentation native owner retired outside Source cleanup");
        // Frame already admitted/installed our immutable union and owns the
        // exclusive preparation lease. Native producers publish directly into
        // that lease; rejection cancels the real preparation, never submits it.
        graphics::Memory memory(words);
        lighting.update_direction(std::as_writable_bytes(words),active->record.unk_08,active->record.unk_14);
        const auto wf=active->course->producer->prepare(words,e,pass);
        const auto sf=active->subject_producer?active->subject_producer->prepare(words,e,pass):menu::model::PacketFrame{};
        const auto it=active->item_producer?active->item_producer->prepare(words,e,pass):menu::model::PacketFrame{};
        // A new Frame preparation cannot exist until its previous actual
        // GPU/readback/map leases have retired. Keep the old effect borrow
        // until this point, even across hidden-camera Source waits.
        effect_packet = {};
        auto fx=active->course->effects->prepare(active->record,effect_camera(),{words,
            {static_cast<std::uint32_t>(config.memory.effects.offset),static_cast<std::uint32_t>(config.memory.effects.size)},e,effect_protected});
        require(bool(fx.status) && bool(fx.packet),"Presentation recorded effect projection rejected");
        counts.fill(0); order_count=0; collect(wf); collect(sf); collect(it);
        std::sort(order.begin(),order.begin()+static_cast<std::ptrdiff_t>(order_count),[](const auto& a,const auto& b) {
            if(a.render.link!=b.render.link) return a.render.link<b.render.link;
            if(a.render.priority!=b.render.priority) return a.render.priority>b.render.priority;
            return a.insertion<b.insertion;
        });
        for(const auto& object:std::span(order).first(order_count)) {
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(game,object.object,&metadata));
            if((metadata.flags&1u) || !pass.includes(object.render)) continue;
            for(unsigned lane=0;lane<4;++lane) raw(lane,object.lanes[lane],object.reservations,words);
            if(object.resets_camera) command(0,0xde000000,wf.camera.address);
        }
        if(pass.includes({static_cast<std::uint8_t>(config.dl_link),0,UINT32_MAX})) {
            command(0,0xde000000,fx.packet.setup().address);
            command(3,0xde000000,fx.packet.body().address);
        }
        scene::render::Prepared prepared{};
        prepared.camera={wf.camera.address,std::size_t(wf.camera.commands)*2};
        std::size_t cursor=config.memory.commands.offset;
        for(unsigned lane=0;lane<4;++lane) {
            command(lane,0xdf000000,0);
            require(cursor<=config.memory.commands.offset+config.memory.commands.size && counts[lane]*8<=config.memory.commands.offset+config.memory.commands.size-cursor,
                "Presentation composed lists exceed fixed command reservation");
            prepared.lanes[lane]={static_cast<std::uint32_t>(cursor),counts[lane]*2};
            for(std::size_t i=0;i<counts[lane];++i) {memory.word(static_cast<std::uint32_t>(cursor),lanes[lane][i].w0);memory.word(static_cast<std::uint32_t>(cursor+4),lanes[lane][i].w1);cursor+=8;}
        }
        fx.packet.close_cpu_view();
        effect_packet = std::move(fx.packet);
        output=prepared; epoch=e; return true;
    }
};
Presentation::Presentation(IsleGame* game,const rom::Snapshot& snapshot,world::ModuleState& module,application::Photos& photos,PresentationConfig config)
    :impl_(std::make_unique<Impl>(game,snapshot,module,photos,config)) {
    auto& s=*impl_;
    std::vector<sprite::RamRange> shared;
    shared.reserve(s.owned.size());
    for(const auto range:s.owned)
        shared.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
    require(config.frame->register_source_display_storage(this,shared),
        "Presentation shared Replay RAM plan is not admitted by actual Frame");
    s.display_owner=this;
}
Presentation::~Presentation() noexcept {if(!impl_->retired) std::terminate();}
bool Presentation::begin(IsleGameCamera* camera,const IslePhotoRecord& record) noexcept {
    auto& s=*impl_;
    if(!camera || s.failed || s.retiring || s.retired || s.retained || s.active || !s.config.frame->quiescent() || (s.camera && s.camera!=camera)) return false;
    try {
        require(record.owner==s.photos.native_source() && record.photo,"Presentation canonical owner/record mismatch");
        require(!s.photos.acquire_replay(record,&s,&s.revision),"Presentation canonical replay lease rejected");
        s.retained=true; s.canonical=true; s.canonical_record=record; s.source_record=record.photo; s.expected_revision=record.revision; s.camera=camera;
        return true;
    } catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation canonical admission failed");}
    return false;
}
bool Presentation::begin_working(IsleGameCamera* camera,IslePhotoWorking* working) noexcept {
    auto& s=*impl_;
    if(!camera || !working || s.failed || s.retiring || s.retired || s.retained || s.active || !s.config.frame->quiescent() || (s.camera && s.camera!=camera)) return false;
    try {
        native(isle_photo_working_acquire_read(working,&s,&s.working_read)); s.retained=true;
        require(s.working_read.photos==s.photos.native_source(),"Presentation working photo belongs to another Photos");
        s.canonical=false; s.source_record=s.working_read.photo; s.revision=s.working_read.revision_address; s.expected_revision=s.working_read.revision; s.camera=camera;
        return true;
    } catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation working admission failed");}
    return false;
}
IslePhotoPresentationStatus Presentation::poll() noexcept {
    auto& s=*impl_;
    if(s.retired) return ISLE_PHOTO_PRESENTATION_RETIRED;
    if(s.retiring) return retire();
    if(s.clearing) return clear();
    if(!s.retained) return s.failed?ISLE_PHOTO_PRESENTATION_FAILED:ISLE_PHOTO_PRESENTATION_IDLE;
    try {
        require(s.current(),"Presentation retained photo changed");
        if(!s.ready) {
            if(!s.config.frame->quiescent()) return ISLE_PHOTO_PRESENTATION_PENDING;
            if(!s.active) {
                if(!s.take_shared_world()) return ISLE_PHOTO_PRESENTATION_PENDING;
                require(s.resource_revision!=UINT64_MAX,"Presentation resource incarnation exhausted");
                s.active=&s.admit();
                ++s.resource_revision;
            }
            if(!s.publish()) return ISLE_PHOTO_PRESENTATION_PENDING;
        }
        return ISLE_PHOTO_PRESENTATION_READY;
    } catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation native publication failed");}
    return ISLE_PHOTO_PRESENTATION_PENDING;
}
IslePhotoPresentationStatus Presentation::clear() noexcept {
    auto& s=*impl_; if(s.retired) return ISLE_PHOTO_PRESENTATION_RETIRED;
    s.clearing=true; s.ready=false;
    if(!s.config.frame->quiescent()) return ISLE_PHOTO_PRESENTATION_PENDING;
    try {if(!s.photo_clear()) return ISLE_PHOTO_PRESENTATION_PENDING; return s.failed?ISLE_PHOTO_PRESENTATION_FAILED:ISLE_PHOTO_PRESENTATION_IDLE;}
    catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation photo retirement failed");}
    return ISLE_PHOTO_PRESENTATION_PENDING;
}
IslePhotoPresentationStatus Presentation::release_to_replay() noexcept {
    auto& s=*impl_;
    if(s.retired || s.retiring) return ISLE_PHOTO_PRESENTATION_FAILED;
    // clear() retains every Photo/native owner until actual Frame/GPU/readback
    // quiescence. The caller's pure-C pending driver services real completions.
    const auto status=clear();
    if(status==ISLE_PHOTO_PRESENTATION_PENDING) return status;
    if(status==ISLE_PHOTO_PRESENTATION_RETIRED) return ISLE_PHOTO_PRESENTATION_FAILED;
    try {
        if(!s.handoff_started) {
            s.return_display_storage();
            return s.failed?ISLE_PHOTO_PRESENTATION_FAILED:ISLE_PHOTO_PRESENTATION_IDLE;
        }
        require(s.config.frame->quiescent(),"Presentation World return still has Frame borrowers");
        auto* returned=s.current_course;
        if(!returned) for(auto& c:s.courses) if(c.world) {returned=&c;break;}
        // Other cached courses are native-inactive before the sole live World
        // is returned. Their immutable resource/record caches remain stable.
        for(auto& c:s.courses) if(c.world && &c!=returned) {
            if(!c.world->clear()) {
                require(c.world->error().empty(),c.world->error().c_str());
                return ISLE_PHOTO_PRESENTATION_PENDING;
            }
            c.published=false;
        }
        if(!s.returning_world.world) {
            require(returned && returned->resources && returned->world,"Presentation lost shared World ownership");
            // This producer borrows this exact World's render_view. It cannot
            // follow the World into Replay, whose camera/basis/layout differ.
            // Subject/item packet producers and all immutable Plans survive.
            if(returned->producer) {
                returned->producer->retire();
                returned->producer.reset();
            }
            s.returning_world={returned->resources,std::move(returned->world)};
            returned->published=false;
        }
        if(!s.config.thumbnail_replay->return_presentation_world(&s,std::move(s.returning_world))) {
            s.fail("Presentation shared World return rejected");
            return ISLE_PHOTO_PRESENTATION_FAILED;
        }
        s.handoff_started=false;
        s.current_course=nullptr;
        s.return_display_storage();
        return s.failed?ISLE_PHOTO_PRESENTATION_FAILED:ISLE_PHOTO_PRESENTATION_IDLE;
    } catch(const std::exception& e) {s.fail(e.what());}
      catch(...) {s.fail("Presentation nonterminal World return failed");}
    return ISLE_PHOTO_PRESENTATION_FAILED;
}
IslePhotoPresentationStatus Presentation::retire() noexcept {
    auto& s=*impl_; if(s.retired) return ISLE_PHOTO_PRESENTATION_RETIRED;
    s.retiring=true;
    const auto status=clear(); if(status==ISLE_PHOTO_PRESENTATION_PENDING) return status;
    try {
        auto* returned=s.current_course;
        if(s.handoff_started && !returned) for(auto& c:s.courses) if(c.world) {returned=&c;break;}
        for(auto& c:s.courses) if(c.world && &c!=returned) {
            if(!c.world->clear()) {require(c.world->error().empty(),c.world->error().c_str());return ISLE_PHOTO_PRESENTATION_PENDING;}
            c.published=false;
        }
        for(auto& p:s.plans) if(p) {if(p->subject_producer)p->subject_producer->retire();if(p->item_producer)p->item_producer->retire();}
        for(auto& c:s.courses) if(c.producer) {c.producer->retire();c.producer.reset();}
        if(s.handoff_started) {
            if(!s.returning_world.world) {
                require(returned && returned->resources && returned->world,"Presentation lost shared World ownership");
                s.returning_world={std::move(returned->resources),std::move(returned->world)};
                returned->published=false;
            }
            if(!s.config.thumbnail_replay->return_presentation_world(&s,std::move(s.returning_world))) {
                s.fail("Presentation shared World return rejected");
                return ISLE_PHOTO_PRESENTATION_PENDING;
            }
            s.handoff_started=false;
        }
        s.return_display_storage();
        s.current_course=nullptr; s.retired=true; return ISLE_PHOTO_PRESENTATION_RETIRED;
    } catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation complete retirement failed");}
    return ISLE_PHOTO_PRESENTATION_PENDING;
}
bool Presentation::pause_links(std::uint64_t mask,bool paused) noexcept {
    auto& s=*impl_;
    // Exactly the Source 18/17 pair; no implicit End8 pause and no invented
    // resume. Callers invoke these returning leaves where their C body does.
    if(!s.ready || s.failed || s.retiring || !s.config.frame->quiescent() || (mask & ~((UINT64_C(1)<<17)|(UINT64_C(1)<<18)))) return false;
    if(mask&(UINT64_C(1)<<18)) if(isle_game_pause_link_processes(s.game,18,paused)!=ISLE_GAME_OK) {s.fail("Presentation link18 pause rejected");return false;}
    if(mask&(UINT64_C(1)<<17)) if(isle_game_pause_link_processes(s.game,17,paused)!=ISLE_GAME_OK) {s.fail("Presentation link17 pause rejected");return false;}
    return true;
}
IslePhotoPresentationStatus Presentation::prepare_course(const IslePhotoRecord& record) noexcept {
    auto& s=*impl_;
    if(s.retired || s.retiring || s.retained || s.active || !s.config.thumbnail_replay || record.owner!=s.photos.native_source() || !record.photo) return ISLE_PHOTO_PRESENTATION_FAILED;
    if(!s.config.frame->quiescent()) return ISLE_PHOTO_PRESENTATION_PENDING;
    const std::uint64_t* revision=nullptr;
    if(s.photos.acquire_replay(record,&s,&revision)) return ISLE_PHOTO_PRESENTATION_FAILED;
    const bool prepared=s.config.thumbnail_replay->prepare_course(record.photo->levelID);
    const bool released=!s.photos.release_replay(&s);
    if(!released) {
        s.retained=true; s.canonical=true; s.canonical_record=record;
        s.source_record=record.photo; s.revision=revision; s.expected_revision=record.revision;
        s.fail("Presentation course canonical lease release rejected");
        return ISLE_PHOTO_PRESENTATION_PENDING;
    }
    if(s.config.thumbnail_replay->error()[0]) return ISLE_PHOTO_PRESENTATION_FAILED;
    return prepared?ISLE_PHOTO_PRESENTATION_READY:ISLE_PHOTO_PRESENTATION_PENDING;
}
bool Presentation::prepare_pass(void* context,std::span<std::uint32_t> words,std::uint64_t epoch,const scene::render::Pass& pass,scene::render::Prepared& result) {
    if(!context) return false;
    auto& s=*static_cast<Presentation*>(context)->impl_;
    try {return s.prepare(words,epoch,pass,result);}
    catch(const std::exception& e) {s.fail(e.what());} catch(...) {s.fail("Presentation compiled Prepare rejected");}
    return false;
}
std::span<const Range> Presentation::reservations() const noexcept {return impl_->render_owned;}
std::span<const sprite::AssetSpan> Presentation::immutable() const noexcept {
    return impl_->active ? std::span<const sprite::AssetSpan>(impl_->active->immutable) : std::span<const sprite::AssetSpan>{};
}
std::uint32_t Presentation::pre_render_address() const noexcept {return impl_->lighting.pre_render_guest()-0x80000000u;}
std::uint64_t Presentation::resource_revision() const noexcept {return impl_->resource_revision;}
const char* Presentation::error() const noexcept {return impl_->failed?impl_->message.data():"";}
} // namespace isle::photo
