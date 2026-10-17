#include "player_cart_producer.hpp"
#include "graphics_task.hpp"
#include <algorithm>
#include <stdexcept>

namespace isle::player::cart {
namespace {
using Range=menu::RamRange;
using Command=menu::model::Command;
void require(bool value,const char* message) { if (!value) throw std::invalid_argument(message); }
void native(IsleGameResult result) { require(result==ISLE_GAME_OK,"Cart producer native operation rejected"); }
bool overlap(Range a,Range b) { return a.size && b.size && a.offset<b.offset+b.size && b.offset<a.offset+a.size; }
void valid(Range range,std::size_t alignment) {
    require(range.size && range.offset%alignment==0 && range.size%alignment==0 &&
        range.offset<=menu::rdram_size && range.size<=menu::rdram_size-range.offset,
        "Cart producer range is empty, unaligned or outside RDRAM");
}
}
struct Producer::Impl {
    IsleGame* game;
    IsleGameCamera* camera;
    menu::model::CameraBasisState& basis;
    const CartResources& resources;
    std::span<const RenderBinding> bindings;
    ProducerLayout layout;
    std::array<Range,3> owned;
    std::vector<menu::model::BorrowedBinding> staged;
    std::vector<ProducerObject> staged_objects,objects;
    std::size_t staged_count=0,object_count=0;
    std::unique_ptr<menu::model::BorrowedKernel> kernel;
    std::array<std::vector<Command>,4> lanes;
    std::vector<Command> pending;
    std::array<std::size_t,4> counts{};
    std::array<menu::model::ListRange,4> ranges{};
    std::array<Command,9> camera_commands{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::uint64_t reflection_revision=0,revision=0,epoch=0;
    const std::uint32_t* ram=nullptr;
    bool retired=false;
    Impl(IsleGame* g,IsleGameCamera* c,menu::model::CameraBasisState& b,const CartResources& r,
         std::span<const RenderBinding> bindings_,ProducerLayout l)
        : game(g),camera(c),basis(b),resources(r),bindings(bindings_),layout(l),owned{l.payloads,l.lists,l.materials} {}
    std::uint32_t payload(std::size_t offset) const { return static_cast<std::uint32_t>(layout.payloads.offset+offset); }
    void command(unsigned lane,std::uint32_t a,std::uint32_t b) {
        require(lane<4 && counts[lane]<lanes[lane].size(),"Cart command capacity exceeded");
        lanes[lane][counts[lane]++]={a,b};
    }
    // app_render J-fogged wrapper: opaque AND translucent deferred lanes each
    // receive their source fog setup/restore around the actual J traversal.
    void fog(unsigned lane,const menu::Model& model,bool enable) {
        command(lane,0xe7000000,0);
        if (enable) {
            const auto difference=std::int32_t(model.fog_max)-model.fog_min;
            const auto multiplier=128000/difference;
            const auto offset=(500-std::int32_t(model.fog_min))*256/difference;
            command(lane,0xdb080000,(std::uint32_t(multiplier)<<16)|(std::uint32_t(offset)&65535));
            command(lane,0xf8000000,(std::uint32_t(model.fog_rgb[0])<<24)|
                (std::uint32_t(model.fog_rgb[1])<<16)|(std::uint32_t(model.fog_rgb[2])<<8));
        }
        command(lane,0xe3000a01,enable?0x00100000:0);
        command(lane,0xe200001c,enable?(lane==1?0xc81049d8:0xc8112078):(lane==1?0x005049d8:0x00552078));
        command(lane,enable?0xd9ffffff:0xd9feffff,enable?0x10000:0);
    }
    void stage(const scene::render::Pass* pass,const menu::model::RenderContext& context) {
        staged_count=0;
        for (const auto& binding:bindings) {
            auto b=binding.model;
            if (!b.object && !b.model && !b.runtime && b.state.instance==menu::animation::InstanceHandle{}) continue;
            require(b.runtime && binding.render.link==3 && b.pose==menu::model::PoseSource::native &&
                b.root==menu::model::RootRecipe::source,"Cart active binding lacks actual native source pose/runtime");
            b.state=b.runtime->model(b.state.instance);
            require(bool(b.state) && b.state.resource_model==0,"Cart stale instance or wrong immutable model");
            require(staged_count<staged.size(),"Cart bindings exceed explicit physical capacity");
            b.draw=b.draw && (!pass || pass->includes(binding.render));
            staged[staged_count]=b; staged_objects[staged_count++]={b.object,binding.render,{}};
        }
        require(bool(kernel->stage(std::span(staged).first(staged_count))),"Cart kernel rejected native pose/material/physical capacity");
        counts.fill(0);
        const auto input=resources.animation_resources();
        const auto& model=input.models[0];
        for (std::size_t i=0;i<staged_count;++i) {
            const auto& b=staged[i]; auto& object=staged_objects[i]; const auto starts=counts;
            if (kernel->draws(b.slot)) {
                fog(0,model,true); fog(1,model,true);
                menu::model::HierarchyBuffer buffer{{std::span(lanes[0]),std::span(lanes[1]),std::span(lanes[2]),std::span(lanes[3])},counts,pending};
                require(bool(kernel->encode(b.slot,context,buffer)),"Cart source J hierarchy/context/list capacity rejected");
                fog(0,model,false); fog(1,model,false);
            }
            for (unsigned lane=0;lane<4;++lane)
                object.lanes[lane]={static_cast<std::uint32_t>(starts[lane]),static_cast<std::uint32_t>(counts[lane]-starts[lane])};
        }
        std::size_t cursor=layout.lists.offset+camera_commands.size()*8;
        for (unsigned lane=0;lane<4;++lane) {
            command(lane,0xdf000000,0);
            require(cursor<=layout.lists.offset+layout.lists.size && counts[lane]*8<=layout.lists.offset+layout.lists.size-cursor,
                "Cart EndDL sublists exceed explicit list reservation");
            ranges[lane]={static_cast<std::uint32_t>(cursor),static_cast<std::uint32_t>(counts[lane])};
            for (std::size_t i=0;i<staged_count;++i)
                staged_objects[i].lanes[lane].address=static_cast<std::uint32_t>(cursor+staged_objects[i].lanes[lane].address*8);
            cursor+=counts[lane]*8;
        }
    }
};
Producer::Producer(IsleGame* game,IsleGameCamera* camera,menu::model::CameraBasisState& basis,
    const CartResources& resources,std::span<const menu::animation::SlotCapacity> capacities,
    std::span<const RenderBinding> bindings,ProducerLayout layout,std::span<const Range> other)
    : impl_(std::make_unique<Impl>(game,camera,basis,resources,bindings,layout)) {
    auto& s=*impl_;
    require(game && camera && !capacities.empty() && !bindings.empty() && !other.empty(),
        "Cart producer requires actual owners, physical slots/bindings and complete external RAM plan");
    resources.require_live();
    std::vector<Range> ranges{{0,graphics::bss_end}};
    for (std::size_t i=0;i<s.owned.size();++i) { valid(s.owned[i],i==0?64:8); ranges.push_back(s.owned[i]); }
    for (const auto range:layout.framebuffers) { valid(range,64); ranges.push_back(range); }
    for (const auto range:other) { valid(range,1); ranges.push_back(range); }
    for (std::size_t i=0;i<ranges.size();++i) {
        for (std::size_t j=0;j<i;++j) require(!overlap(ranges[i],ranges[j]),"Cart complete RAM owner ranges overlap");
        for (const auto& span:resources.installation_spans()) {
            require(span.guest_address>=0x80000000 && span.guest_address-0x80000000<=menu::rdram_size &&
                span.bytes.size()<=menu::rdram_size-(span.guest_address-0x80000000),"Cart immutable asset extent invalid");
            require(!overlap(ranges[i],{span.guest_address-0x80000000,span.bytes.size()}),"Cart complete RAM plan overlaps immutable assets");
        }
    }
    const auto inputs=resources.animation_resources(); const auto& model=inputs.models[0];
    require(inputs.models.size()==1 && model.nodes.size()==43 && model.render_type==menu::RenderType::j_fogged &&
        model.matrix_kinds==std::array<std::uint8_t,3>{28,0,0},"Cart source J/tree/root policy changed");
    const auto difference=std::int32_t(model.fog_max)-model.fog_min;
    require(difference>0 && model.fog_max<=1000,"Cart source World fog interval invalid");
    const auto multiplier=128000/difference,offset=(500-std::int32_t(model.fog_min))*256/difference;
    require(multiplier<=32767 && offset>=-32768 && offset<=32767,"Cart fog numeric extent invalid");
    require(layout.payloads.size>=256,"Cart camera payload reservation exhausted");
    std::size_t matrix=layout.payloads.offset+256,material=layout.materials.offset,nodes=0;
    std::vector<menu::model::KernelSlot> slots; slots.reserve(capacities.size());
    for (const auto capacity:capacities) {
        require(capacity.nodes && capacity.nodes<=(layout.payloads.offset+layout.payloads.size-matrix)/64 &&
            capacity.material_attachments<=(layout.materials.offset+layout.materials.size-material)/264,
            "Cart explicit slot matrix/material capacity exceeds RAM plan");
        slots.push_back({capacity,static_cast<std::uint32_t>(matrix),static_cast<std::uint32_t>(material)});
        matrix+=capacity.nodes*64; material+=capacity.material_attachments*264; nodes+=capacity.nodes;
    }
    s.kernel=std::make_unique<menu::model::BorrowedKernel>(game,inputs.models,inputs.materials,slots,
        Range{matrix,layout.payloads.offset+layout.payloads.size-matrix});
    s.staged.resize(capacities.size()); s.staged_objects.resize(capacities.size()); s.objects.resize(capacities.size());
    for (auto& lane:s.lanes) lane.resize(layout.lists.size/8);
    s.pending.resize(nodes*15+capacities.size());
}
Producer::~Producer()=default;
ProducerFrame Producer::prepare_filtered(std::span<std::uint32_t> words,std::uint64_t epoch,const scene::render::Pass* pass) {
    auto& s=*impl_;
    require(!s.retired && words.data() && words.size_bytes()==menu::rdram_size && epoch,
        "Cart prepare requires live exclusive canonical RAM and nonzero epoch");
    require(!pass || (pass->kind==scene::render::Kind::compiled && pass->camera==s.camera),"Cart compiled pass camera/policy differs from actual owner");
    require(s.revision!=UINT64_MAX,"Cart frame revision exhausted"); s.resources.require_live();
    const auto begin=reinterpret_cast<std::uintptr_t>(words.data());
    require(begin%alignof(std::uint32_t)==0 && begin<=UINTPTR_MAX-words.size_bytes(),"Cart RAM pointer extent invalid");
    const auto end=begin+words.size_bytes();
    const auto disjoint=[&](const void* pointer,std::size_t bytes) {
        if (!bytes) return;
        const auto at=reinterpret_cast<std::uintptr_t>(pointer);
        require(pointer && at<=UINTPTR_MAX-bytes && (end<=at || at+bytes<=begin),"Cart RAM aliases preparation owner");
    };
    disjoint(this,sizeof(*this)); disjoint(&s,sizeof(s)); disjoint(&s.basis,sizeof(s.basis));
    disjoint(s.game,1); disjoint(s.camera,1); disjoint(&s.resources,sizeof(s.resources));
    disjoint(s.bindings.data(),s.bindings.size_bytes()); disjoint(s.staged.data(),s.staged.size()*sizeof(menu::model::BorrowedBinding));
    disjoint(s.staged_objects.data(),s.staged_objects.size()*sizeof(ProducerObject)); disjoint(s.objects.data(),s.objects.size()*sizeof(ProducerObject));
    disjoint(s.pending.data(),s.pending.size()*sizeof(Command)); if (pass) disjoint(pass,sizeof(*pass));
    for (const auto& lane:s.lanes) disjoint(lane.data(),lane.size()*sizeof(Command));
    for (const auto& span:s.resources.installation_spans()) disjoint(span.bytes.data(),span.bytes.size());
    std::size_t count=0; native(isle_game_camera_matrix_count(s.game,s.camera,&count));
    require(count==2,"Cart main camera requires actual source kinds3/14");
    native(isle_game_camera_validate_disjoint(s.game,s.camera,words.data(),words.size_bytes()));
    std::array<const IsleGameCameraMatrix*,2> camera{};
    for (std::size_t i=0;i<camera.size();++i) {
        native(isle_game_camera_peek_matrix(s.game,s.camera,i,&camera[i]));
        require(camera[i]->kind==(i==0?ISLE_GAME_CAMERA_PERSPECTIVE_FAST:ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL),"Cart main camera matrix kinds/order changed");
        disjoint(camera[i],sizeof(*camera[i]));
    }
    const IsleGameCameraParameters* parameters=nullptr; native(isle_game_camera_parameters(s.game,s.camera,&parameters));
    disjoint(parameters,sizeof(*parameters));
    menu::model::RenderContext context;
    constexpr std::array<menu::model::CameraMatrixOperation,2> operations{{{3,0},{14,0}}};
    require(bool(menu::model::prepare_render_context(s.basis.value,*parameters,operations,s.basis.selector,context)),"Cart source camera context rejected");
    require(bool(s.kernel->validate_upload(words,epoch)),"Cart kernel fixed scratch aliases RAM");
    s.stage(pass,context);
    require(bool(s.kernel->validate_upload(words,epoch)),"Cart kernel borrowed pose aliases RAM");
    native(isle_game_camera_compute_all(s.game,s.camera));
    const IsleGameCameraReflection* reflection=nullptr; native(isle_game_camera_reflection(s.game,s.camera,1,&reflection));
    std::uint16_t norm=0; native(isle_game_camera_perspective_normalization(s.game,s.camera,&norm));
    s.kernel->commit_pose();
    s.camera_commands={{{0xda380007,s.payload(0)},{0xdb0e0000,norm},{0xdc08000a,s.payload(192)},
        {0xdc08030a,s.payload(208)},{0xda380005,s.payload(64)},{0xda380003,s.payload(128)},
        {0xdb060000,0},{0xd5000001,0},{0xdf000000,0}}};
    graphics::Memory memory(words); const bool fresh=s.ram!=words.data() || s.epoch!=epoch;
    for (std::size_t i=0;i<camera.size();++i) if (fresh || s.camera_revisions[i]!=camera[i]->revision) {
        memory.matrix(s.payload(i*64),camera[i]->fixed); s.camera_revisions[i]=camera[i]->revision;
    }
    if (fresh) {
        const IsleGameFixedMatrix identity{{{0x10000,0,1,0},{0,0x10000,0,1},{0,0,0,0},{0,0,0,0}}};
        memory.matrix(s.payload(128),identity);
    }
    if (fresh || s.reflection_revision!=reflection->revision) {
        for (unsigned axis=0;axis<2;++axis) {
            const auto at=s.payload(192+axis*16);
            for (unsigned b=0;b<16;++b) memory.byte(at+b,0);
            for (unsigned c=0;c<3;++c) {
                memory.byte(at+c,reflection->color[axis][c]); memory.byte(at+4+c,reflection->color_copy[axis][c]);
                memory.byte(at+8+c,static_cast<std::uint8_t>(reflection->direction[axis][c]));
            }
        }
        s.reflection_revision=reflection->revision;
    }
    s.kernel->upload(words,epoch);
    const auto write=[&](std::uint32_t at,Command command) {
        if (fresh || words[at/4]!=command.w0) memory.word(at,command.w0);
        if (fresh || words[at/4+1]!=command.w1) memory.word(at+4,command.w1);
    };
    for (std::size_t i=0;i<s.camera_commands.size();++i) write(static_cast<std::uint32_t>(s.layout.lists.offset+i*8),s.camera_commands[i]);
    for (unsigned lane=0;lane<4;++lane) for (std::size_t i=0;i<s.counts[lane];++i)
        write(s.ranges[lane].address+static_cast<std::uint32_t>(i*8),s.lanes[lane][i]);
    s.objects.swap(s.staged_objects); s.object_count=s.staged_count; s.ram=words.data(); s.epoch=epoch; s.basis.value=context; ++s.revision;
    return {{static_cast<std::uint32_t>(s.layout.lists.offset),static_cast<std::uint32_t>(s.camera_commands.size())},
        s.ranges,s.owned,s.revision,std::span(s.objects).first(s.object_count)};
}
ProducerFrame Producer::prepare(std::span<std::uint32_t> words,std::uint64_t epoch) { return prepare_filtered(words,epoch,nullptr); }
ProducerFrame Producer::prepare(std::span<std::uint32_t> words,std::uint64_t epoch,const scene::render::Pass& pass) { return prepare_filtered(words,epoch,&pass); }
bool Producer::prepare_pass(void* user,std::span<std::uint32_t> words,std::uint64_t epoch,const scene::render::Pass& pass,scene::render::Prepared& out) {
    require(user!=nullptr,"Cart compiled pass requires actual producer owner");
    const auto begin=reinterpret_cast<std::uintptr_t>(words.data()),output=reinterpret_cast<std::uintptr_t>(&out);
    require(begin<=UINTPTR_MAX-words.size_bytes() && output<=UINTPTR_MAX-sizeof(out) &&
        (begin+words.size_bytes()<=output || output+sizeof(out)<=begin),"Cart prepared output aliases canonical RAM");
    const auto frame=static_cast<Producer*>(user)->prepare(words,epoch,pass);
    scene::render::Prepared staged; staged.camera={frame.camera.address,static_cast<std::size_t>(frame.camera.commands)*2};
    for (unsigned lane=0;lane<4;++lane) staged.lanes[lane]={frame.lanes[lane].address,static_cast<std::size_t>(frame.lanes[lane].commands)*2};
    out=staged; return true;
}
std::span<const Range> Producer::reservations() const noexcept { return impl_->owned; }
std::span<const menu::model::WorldMatrix> Producer::world_nodes(menu::animation::InstanceHandle instance) const noexcept {
    return impl_->retired?std::span<const menu::model::WorldMatrix>{}:impl_->kernel->world_nodes(instance);
}
std::span<const IsleGameModelNode> Producer::native_nodes(menu::animation::InstanceHandle instance) const noexcept {
    return impl_->retired?std::span<const IsleGameModelNode>{}:impl_->kernel->native_nodes(instance);
}
void Producer::retire() noexcept { impl_->retired=true; }
} // namespace isle::player::cart
