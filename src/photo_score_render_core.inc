// Included inside Replay::Impl: returning native leaves for the privately
// admitted complete score_Render producer, never an alternate score algorithm.
    template<class Function> static IsleGameResult score_leaf(void* raw,Function&& function) noexcept {
        if(!raw)return ISLE_GAME_INVALID_ARGUMENT;
        auto& self=*static_cast<Impl*>(raw);
        try {
            require(!self.failed&&self.score_control&&self.binding.frame&&self.current(),
                "Source score leaf escaped its live record/Frame capability");
            function(self);return ISLE_GAME_OK;
        } catch(const std::exception& error) {self.fail(error.what());}
          catch(...) {self.fail("Source score returning native leaf failed");}
        return ISLE_GAME_EXECUTION_FAILURE;
    }
    Callback& score_object(IsleGameObject* object) {
        for(auto& entry:std::span(order).first(order_count))if(entry.object==object)return entry;
        throw std::runtime_error("Source score object escaped admitted producer membership");
    }
    void score_initialize(IslePhotoScoreSource& native_source,IsleGameObject* object,const IslePlayerPhotoData* photo) {
        require(object==camera_object&&photo==&record&&&native_source==&score_source,
            "Source score initObjectsOnPhoto changed its canonical camera/photo identity");
        require(reconstruct(),"Source score native photo reconstruction did not join");
        // Source47380.c1106-1133 leaves aspect, scale and viewport Z unchanged.
        const IsleGameCameraParameters* saved=nullptr;
        native(isle_game_camera_parameters(game,camera,&saved));
        auto perspective=saved->perspective;
        source_aspect=perspective.aspect;
        perspective.fovy=30;perspective.near_plane=10;perspective.far_plane=25600;
        native(isle_game_camera_set_perspective(game,camera,&perspective));
        const IsleGameLookAt look{{record.unk_08.x,record.unk_08.y,record.unk_08.z},
            {record.unk_14.x,record.unk_14.y,record.unk_14.z},{0,1,0}};
        native(isle_game_camera_set_look_at(game,camera,&look));
        IsleGameCameraRenderState render{};native(isle_game_camera_render_state(game,camera,&render));
        if(record.levelID==course::cave_id) {render.flags|=2u;render.background_rgba=0x05080401u;}
        else render.flags&=~2u;
        native(isle_game_camera_set_render_state(game,camera,&render));
        preflight_assets();
        require(stage_epoch!=UINT64_MAX,"Source score producer incarnation exhausted");++stage_epoch;
        graphics::validate_cold_inputs(staging,snapshot,source,static_cast<std::uint32_t>(config.memory.workspace.offset+0x7000));
        graphics::install_cold_inputs(staging,snapshot,static_cast<std::uint32_t>(config.memory.workspace.offset+0x7000));
        upload_assets(staging);
        std::fill_n(staging.begin()+static_cast<std::ptrdiff_t>(config.memory.feedback.offset/4),photo_feedback::buffer_bytes/4,0u);
        lane_counts.fill(0);order_count=0;mapping_count=0;score_post_drained=false;
        // Only lane0's scalar Source prefix exists before matrix/provider
        // preparation. Its final disjoint physical partition is planned from
        // actual admitted callback sizes before any join address is generated.
        score_lane_begin.fill(static_cast<std::uint32_t>(config.memory.commands.offset+0x400));
        score_lane_end.fill(static_cast<std::uint32_t>(config.memory.commands.offset+config.memory.commands.size));
        score_saved.fill(0);score_programs={config.program,config.program,false,false};
        graphics::Memory memory(staging);
        const auto payload=static_cast<std::uint32_t>(config.memory.commands.offset+0x300);
        const IsleGameFixedMatrix identity{{{0x00010000,0,1,0},{0,0x00010000,0,1},{0,0,0,0},{0,0,0,0}}};
        memory.matrix(payload+64,identity);
        const auto reset=graphics::reset_commands({native_source.image_buffer,native_source.current_z_buffer,payload,payload+64,
            lighting.pre_render_guest()-0x80000000u,photo_feedback::plane_bytes,photo_feedback::plane_bytes});
        for(std::size_t i=0;i<reset.size();++i)memory.word(static_cast<std::uint32_t>(config.memory.commands.offset+i*4),reset[i]);
        memory.word(static_cast<std::uint32_t>(config.memory.commands.offset+reset.size()*4),0xdf000000u);
        memory.word(static_cast<std::uint32_t>(config.memory.commands.offset+reset.size()*4+4),0);
        const auto slots=subjects->scored_slots();
        require(slots.size()<=12,"Source score reconstructed too many Pokemon slots");
        std::fill(std::begin(native_source.photo_objects),std::end(native_source.photo_objects),nullptr);
        for(std::size_t i=0;i<slots.size();++i)native_source.photo_objects[i]=slots[i].object;
        native_source.photo_count=static_cast<std::int32_t>(slots.size());
    }
    void score_viewport(IsleGameCamera* value) {
        require(value==camera,"Source score viewport changed its actual camera");
        const IsleGameCameraParameters* parameters=nullptr;native(isle_game_camera_parameters(game,camera,&parameters));
        const auto payload=static_cast<std::uint32_t>(config.memory.commands.offset+0x300);
        graphics::Memory memory(staging);
        for(unsigned i=0;i<4;++i) {
            memory.half(payload+i*2,static_cast<std::uint16_t>(parameters->viewport.scale[i]));
            memory.half(payload+8+i*2,static_cast<std::uint16_t>(parameters->viewport.translate[i]));
        }
        score_command(0,0xdc080008u,payload);
    }
    void score_prepare_camera(IsleGameCamera* value) {
        require(value==camera,"Source score matrix preparation changed its actual camera");
        native(isle_game_camera_compute_all(game,camera));
        const auto wf=world_producer->prepare(staging,stage_epoch);
        const auto sf=subject_producer?subject_producer->prepare(staging,stage_epoch):menu::model::PacketFrame{};
        const auto it=item_producer?item_producer->prepare(staging,stage_epoch):menu::model::PacketFrame{};
        score_camera_address=wf.camera.address;
        collect(wf);collect(sf);collect(it);
        require(order_count<order.size(),"Source score effect callback exceeds cold membership capacity");
        order[order_count]={effect_object,{static_cast<std::uint8_t>(config.dl_link),0,UINT32_MAX},{},{},false,order_count};++order_count;
        std::sort(order.begin(),order.begin()+static_cast<std::ptrdiff_t>(order_count),[](const auto& a,const auto& b) {
            if(a.render.link!=b.render.link)return a.render.link<b.render.link;
            if(a.render.priority!=b.render.priority)return a.render.priority>b.render.priority;
            return a.insertion<b.insertion;
        });
        score_plan_lanes();
        score_call(0,score_camera_address);
    }
    void score_plan_lanes() {
        const auto capacity=static_cast<std::size_t>((config.memory.commands.size-0x400)/8);
        auto required=lane_counts;
        const auto add=[&](unsigned lane,std::size_t commands) {
            require(required[lane]<=capacity&&commands<=capacity-required[lane],
                "Source score actual provider demand exceeds its cold command union");
            required[lane]+=commands;
        };
        add(0,1); // The actual renPrepareCameraMatrix call appended next.
        std::array<bool,4> auxiliary_pending{};
        std::size_t pokemon=0;
        for(const auto& entry:std::span(order).first(order_count)) {
            IsleGameObjectMetadata metadata{};native(isle_game_object_metadata(game,entry.object,&metadata));
            if((metadata.flags&1u)||!(score_source.camera_tag&entry.render.camera_tag))continue;
            const bool scored=pokemon<static_cast<std::size_t>(score_source.photo_count)&&
                entry.object==score_source.photo_objects[pokemon];
            if(scored) {
                ++pokemon;
                add(0,5); // A/B segments, copy-list call, PipeSync, C depth restore.
            }
            const unsigned draws=scored?2u:1u; // Original isolated + cumulative fnRender.
            if(entry.object==effect_object) {
                add(0,2); // Genuine FX setup and post-combine body calls.
                auxiliary_pending.fill(false);
                continue;
            }
            for(unsigned lane=0;lane<4;++lane)if(entry.lanes[lane].commands) {
                if(lane&&!auxiliary_pending[lane]) {
                    add(lane,1); // Restore only an actually populated auxiliary.
                    auxiliary_pending[lane]=true;
                }
                add(lane,std::size_t(entry.lanes[lane].commands)*draws);
            }
            if(entry.resets_camera)add(0,draws);
        }
        // At most FX combine, camera-post combine and actual process. Reserve
        // the shared planner's bounded packet envelope, not a quarter arena.
        for(unsigned lane=0;lane<4;++lane)
            add(lane,3*(graphics::SourceJoinPlan::lane_word_capacity/2));
        std::size_t total=0;
        for(const auto commands:required) {
            require(commands<=capacity-total,
                "Source score populated lanes exhaust the shared cold command union");
            total+=commands;
        }
        // Lane0 owns every unused command; lane0-heavy genuine photos retain
        // the whole admitted union rather than an arbitrary per-lane quarter.
        required[0]+=capacity-total;
        auto address=static_cast<std::uint32_t>(config.memory.commands.offset+0x400);
        for(unsigned lane=0;lane<4;++lane) {
            score_lane_begin[lane]=address;
            address+=static_cast<std::uint32_t>(required[lane]*8);
            score_lane_end[lane]=address;
        }
    }
    void score_draw(IsleGameObject* object) {
        const auto& entry=score_object(object);
        require(score_source.rendered_object==object,"Source score callback lost omRenderedObject identity");
        if(object==effect_object) {
            native(isle_game_camera_compute_all(game,camera));
            auto result=effect_renderer->prepare(record,effect_camera(),{staging,
                {static_cast<std::uint32_t>(config.memory.effects.offset),static_cast<std::uint32_t>(config.memory.effects.size)},
                stage_epoch,effect_protected});
            require(bool(result.status)&&bool(result.packet),"Source score genuine effect callback rejected");
            effect_packet=std::move(result.packet);
            score_call(0,effect_packet.setup().address);
            graphics::SourceJoinPlan combined;
            score_join(false,combined);
            score_call(0,effect_packet.body().address);return;
        }
        for(unsigned lane=0;lane<4;++lane)if(entry.lanes[lane].commands) {
            if(lane&&lane_counts[lane]==score_saved[lane])score_call(lane,score_camera_address);
            require(entry.lanes[lane].commands<=(score_lane_end[lane]-score_lane_begin[lane])/8-lane_counts[lane],
                "Source score actual provider lane exceeds its cold arena");
            raw(lane,entry.lanes[lane],entry.reservations);
        }
        if(entry.resets_camera)score_call(0,score_camera_address);
    }
    void score_command(unsigned lane,std::uint32_t a,std::uint32_t b) {
        require(lane<4&&lane_counts[lane]<(score_lane_end[lane]-score_lane_begin[lane])/8,
            "Source score command exceeded its bounded native lane");
        command(lane,a,b);
    }
    void score_call(unsigned lane,std::uint32_t address) {score_command(lane,0xde000000u,address);}
    void score_join(bool process,graphics::SourceJoinPlan& plan) {
        std::array<graphics::SourceLaneCursor,4> cursors{};
        for(unsigned lane=0;lane<4;++lane)cursors[lane]={
            score_lane_begin[lane]+static_cast<std::uint32_t>(score_saved[lane]*8),
            score_lane_begin[lane]+static_cast<std::uint32_t>(lane_counts[lane]*8),
            score_lane_end[lane],lane_counts[lane]!=score_saved[lane]};
        graphics::plan_source_join(*binding.programs,score_programs,cursors,
            {static_cast<std::uint32_t>(config.memory.commands.offset),graphics::reset_command_words/2+1},process,plan);
        if(plan.empty)return;
        for(unsigned lane=0;lane<4;++lane)
            for(std::size_t i=0;i<plan.word_counts[lane];i+=2)
                score_command(lane,plan.words[lane][i],plan.words[lane][i+1]);
        if(!process) {
            for(unsigned lane=1;lane<4;++lane)score_saved[lane]=lane_counts[lane];
            score_programs.alternate_active=false;
        }
    }
    void score_process() {
        bool pending=false;
        for(unsigned lane=0;lane<4;++lane)pending|=lane_counts[lane]!=score_saved[lane];
        if(!pending)return;
        score_control->reserve_task();
        graphics::SourceJoinPlan plan;
        score_join(true,plan);
        graphics::Memory memory(staging);
        for(unsigned lane=0;lane<4;++lane) {
            auto cursor=score_lane_begin[lane];
            for(std::size_t i=0;i<lane_counts[lane];++i) {
                memory.word(cursor,lanes[lane][i].w0);memory.word(cursor+4,lanes[lane][i].w1);cursor+=8;
            }
        }
        score_control->with_ram(this,+[](void* raw,std::span<std::uint32_t> words){static_cast<Impl*>(raw)->commit(words);});
        score_control->submit_task({plan.root.address,plan.root.commands*2},
            {plan.terminal.address,plan.terminal.commands*2},plan.program);
        score_saved=lane_counts;
    }
    void score_post(IsleGameCamera* value) {
        require(value==camera,"Source score post changed its actual native camera");
        IsleGameCameraRenderState render{};native(isle_game_camera_render_state(game,camera,&render));
        // Independent Source post ordering; the FX callback has already used
        // this same native combine convention before appending its own body.
        if(render.flags&4u){graphics::SourceJoinPlan combined;score_join(false,combined);}
        if(render.flags&0x10u) {
            score_process();score_control->drain_tasks();score_control->reset_task_slots();score_post_drained=true;
            lane_counts.fill(0);score_saved.fill(0);score_programs.alternate_active=false;
        }
        if(render.flags&0x40u)score_process();
        const auto slots=subjects->scored_slots();
        require(score_source.pokemon_count<=slots.size(),"Source score count escaped canonical photo slots");
        mapping_count=score_source.pokemon_count;
        for(std::size_t i=0;i<mapping_count;++i)
            mapping[i]={slots[i].record_index,static_cast<std::uint8_t>(i)};
    }
    static const IslePhotoScoreNative& score_native() {
        static const IslePhotoScoreNative native{
            +[](void* raw,IslePhotoScoreSource* source,IsleGameObject* object,const IslePlayerPhotoData* photo) noexcept {
                return score_leaf(raw,[&](Impl& s){s.score_initialize(*source,object,photo);});},
            +[](void* raw,IsleGameCamera* camera) noexcept {return score_leaf(raw,[&](Impl& s){s.score_viewport(camera);});},
            +[](void* raw,std::uint32_t a,std::uint32_t b) noexcept {return score_leaf(raw,[&](Impl& s){s.score_command(0,a,b);});},
            +[](void* raw,IsleGameCamera* camera) noexcept {return score_leaf(raw,[&](Impl& s){s.score_prepare_camera(camera);});},
            +[](void* raw,std::int32_t link,IsleGameObject** output) noexcept {
                if(output)*output=nullptr;
                return score_leaf(raw,[&](Impl& s){require(output&&link>=0&&link<32,"Source score DL link invalid");
                    for(const auto& entry:std::span(s.order).first(s.order_count))if(entry.render.link==unsigned(link)){*output=entry.object;break;}});},
            +[](void* raw,IsleGameObject* object,IsleGameObject** output) noexcept {
                if(output)*output=nullptr;
                return score_leaf(raw,[&](Impl& s){require(output,"Source score next output missing");
                    const auto& entry=s.score_object(object);const auto index=static_cast<std::size_t>(&entry-s.order.data());
                    if(index+1<s.order_count&&s.order[index+1].render.link==entry.render.link)*output=s.order[index+1].object;});},
            +[](void* raw,IsleGameObject* object,std::uint32_t* output) noexcept {
                if(output)*output=0;
                return score_leaf(raw,[&](Impl& s){require(output,"Source score tag output missing");*output=s.score_object(object).render.camera_tag;});},
            +[](void* raw,IsleGameObject* object) noexcept {return score_leaf(raw,[&](Impl& s){s.score_draw(object);});},
            +[](void* raw,IsleGameObject* object,std::uint32_t frame) noexcept {
                return score_leaf(raw,[&](Impl& s){s.score_object(object).last_draw_frame=frame;});},
            +[](void* raw,IsleGameCamera* camera) noexcept {return score_leaf(raw,[&](Impl& s){s.score_post(camera);});}
        };
        return native;
    }
    void submit_score(bool wide_pass) {
        require(binding.frame&&binding.executor&&score_execution_count<score_executions.size(),
            "Source score lacks its exact shared Frame or bounded receipt slot");
        camera_create();score_source={};score_source.game=game;score_source.context=this;score_source.native=&score_native();
        score_source.camera_object=camera_object;score_source.current_photo=&record;
        score_source.ready=1;score_source.wide=wide_pass;
        score_source.link_mask=score_source.buffer_mask=1u<<config.dl_link;score_source.camera_tag=UINT32_MAX;
        score_source.image_buffer=static_cast<std::uint32_t>(config.memory.feedback.offset);
        score_source.current_z_buffer=score_source.image_buffer+photo_feedback::plane_bytes;
        for(unsigned i=0;i<12;++i) {
            score_source.pokemon_z_buffer[i]=score_source.image_buffer+(2+i)*photo_feedback::plane_bytes;
            score_source.z_buffer_before[i]=score_source.image_buffer+(14+i)*photo_feedback::plane_bytes;
        }
        score_source.setup_list=setup_guest;score_source.copy_list=copy_guest;
        score_completed=false;
        const scene::graphics::SourceGpuBinding authority{binding.executor,binding.programs,binding.epoch,binding.admission};
        const bool ran=binding.frame->run_source_control(authority,this,+[](void* raw,scene::graphics::SourceControl& control) {
            auto& self=*static_cast<Impl*>(raw);self.score_control=&control;
            struct Clear {Impl& owner;~Clear(){owner.score_control=nullptr;}} clear{self};
            const auto result=isle_photo_score_call(&self.score_source);
            require(result==ISLE_GAME_OK,"complete Source score_Render C producer failed");
            return true;
        },{this,+[](void* raw,rsp::CanonicalExecutor::MemoryLease& completed,std::uint32_t) {
            auto& self=*static_cast<Impl*>(raw);self.require_current();
            const auto bytes=completed.rdram();photo_feedback::PlanesView view;
            require(photo_feedback::PlanesView::borrow({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()},
                static_cast<std::uint32_t>(self.config.memory.feedback.offset),view),"Source score completed planes unavailable");
            if(self.score_source.wide)require(photo_feedback::count_wide(view,self.wide),"Source score wide reduction failed");
            else require(photo_feedback::count_narrow(view,self.wide,self.counts),"Source score narrow reduction failed");
            self.score_completed=true;
        }});
        require(ran,binding.frame->message());
        const auto receipts=binding.frame->task_receipts();
        require(receipts.size()==1&&receipts[0].completed&&!receipts[0].failed&&score_completed&&score_post_drained,
            "Natural score_Render did not complete its one real post10 submission");
        const IsleGameCameraParameters* parameters=nullptr;native(isle_game_camera_parameters(game,camera,&parameters));
        IsleGameCameraRenderState render{};native(isle_game_camera_render_state(game,camera,&render));
        const bool restored=!(render.flags&0x10u)&&same(parameters->perspective.fovy,30.0f)&&
            same(parameters->perspective.aspect,source_aspect);
        require(restored&&!score_source.ready,"Source score C suffix did not restore its actual native camera");
        score_executions[score_execution_count++]={receipts[0],wide_pass,true,restored,score_source.pokemon_count};
        effect_packet={};require(release_binding(),"Source score exact Frame binding release failed");
    }
