#include "actor_producer.hpp"
#include "actor_source.h"
#include "graphics_task.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>

// Own GAME authority: df8815e75f3d79618466e01e0827c6165b93d11c.
// Policy only: the sole BorrowedKernel owns hierarchy/material/source54/48 math.
// See actor-producer-inputs.json for exact source identities and limits.
namespace isle::actor {
namespace {
using Range = menu::RamRange;
using Command = menu::model::Command;
void require(bool value, const char* message) { if (!value) throw std::invalid_argument(message); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"Actor producer native operation rejected"); }
bool overlap(Range a, Range b) {
    return a.size && b.size && a.offset < b.offset+b.size && b.offset < a.offset+a.size;
}
void valid(Range r, std::size_t alignment) {
    require(r.size && r.offset%alignment == 0 && r.size%alignment == 0 &&
            r.offset <= beach::rdram_size && r.size <= beach::rdram_size-r.offset,
            "Actor producer range is empty, unaligned or outside RDRAM");
}
constexpr scene::RenderMetadata prefix_render{5,0x80000001u,UINT32_MAX};
struct Policy { bool fog = false, transparent = false, gate = false, geometry = false; };
Policy policy(const beach::ActorInitialization& init, const menu::Model& model) {
    Policy p;
    menu::RenderType type;
    switch (init.render_callback_identity) {
    case 0x80014f98: type = menu::RenderType::b; break;
    case 0x803594dc: type = menu::RenderType::b_fogged; p.fog = p.gate = true; break;
    case 0x800a1608: type = menu::RenderType::d_fogged; p.fog = p.transparent = true; break;
    case 0x80359534: type = menu::RenderType::d_fogged; p.fog = p.transparent = p.gate = true; break;
    case 0x8035942c: type = menu::RenderType::i_fogged; p.fog = p.gate = true; break;
    case 0x80359484: type = menu::RenderType::j_fogged; p.fog = p.transparent = p.gate = true; break;
    default: throw std::invalid_argument("Unadmitted actual actor source render callback");
    }
    require(model.render_type == type,"Actor callback and source hierarchy disagree");
    for (const auto& node : model.nodes) p.geometry |= !node.payloads.empty();
    return p;
}
}
struct Producer::Impl {
    IsleGame* game;
    IsleGameCamera* camera;
    menu::model::CameraBasisState& basis;
    const beach::ActorResources& resources;
    std::span<const ActorRenderBinding> bindings;
    ProducerLayout layout;
    std::array<Range,3> owned;
    std::vector<Policy> policies;
    std::vector<menu::model::BorrowedBinding> staged;
    std::vector<scene::RenderMetadata> staged_render;
    std::vector<focus::Subject> staged_subjects;
    std::vector<ProducerFragment> staged_fragments, fragments;
    std::size_t staged_fragment_count = 0, fragment_count = 0;
    DetectorBinding* detector;
    std::array<focus::Subject,focus::history_capacity> previous_subjects{}, current_subjects{};
    std::size_t previous_count = 0, appended_count = 0, current_count = 0;
    std::uint16_t source_context = 0;
    bool prefix_staged = false;
    focus::Photo staged_photo{};
    std::unique_ptr<menu::model::BorrowedKernel> kernel;
    std::array<std::vector<Command>,4> lanes;
    std::vector<Command> pending;
    std::array<std::size_t,4> counts{};
    std::array<menu::model::ListRange,4> ranges{};
    std::array<Command,9> camera_commands{};
    std::array<std::uint64_t,2> camera_revisions{};
    std::uint64_t reflection_revision = 0, revision = 0, epoch = 0;
    const std::uint32_t* ram = nullptr;
    bool retired = false;
    Impl(IsleGame* g, IsleGameCamera* c, menu::model::CameraBasisState& b,
         const beach::ActorResources& r, std::span<const ActorRenderBinding> bindings_, ProducerLayout l,
         DetectorBinding* detector_)
        : game(g),camera(c),basis(b),resources(r),bindings(bindings_),layout(l),
          owned{l.payloads,l.lists,l.materials},detector(detector_) {}
    std::uint32_t payload(std::size_t offset) const { return static_cast<std::uint32_t>(layout.payloads.offset+offset); }
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && counts[lane] < lanes[lane].size(),"Actor producer command capacity exceeded");
        lanes[lane][counts[lane]++] = {a,b};
    }
    void fog(unsigned lane, const menu::Model& model, bool enable) {
        command(lane,0xe7000000,0);
        if (enable) {
            const auto difference = std::int32_t(model.fog_max)-model.fog_min;
            const auto multiplier = 128000/difference;
            const auto offset = (500-std::int32_t(model.fog_min))*256/difference;
            command(lane,0xdb080000,(std::uint32_t(multiplier)<<16)|(std::uint32_t(offset)&65535));
            command(lane,0xf8000000,(std::uint32_t(model.fog_rgb[0])<<24)|
                (std::uint32_t(model.fog_rgb[1])<<16)|(std::uint32_t(model.fog_rgb[2])<<8));
        }
        command(lane,0xe3000a01,enable ? 0x00100000 : 0);
        command(lane,0xe200001c,enable ? (lane == 1 ? 0xc81049d8 : 0xc8112078) :
            (lane == 1 ? 0x005049d8 : 0x00552078));
        command(lane,enable ? 0xd9ffffff : 0xd9feffff,enable ? 0x10000 : 0);
    }
    void capture(focus::Subject subject) {
        const auto row=prefix_staged?current_count:previous_count+appended_count;
        require(row<focus::history_capacity,"Actor source focus history capacity exceeded");
        const auto emitted=detector->copies.append_region((lanes[0].size()-counts[0])*2,
            static_cast<unsigned>(row),detector->scissor,
            [this](std::uint32_t a,std::uint32_t b) { command(0,a,b); });
        require(bool(emitted),"Actor source focus copy/scissor capacity rejected");
        if (prefix_staged) current_subjects[current_count++]=subject;
        else previous_subjects[appended_count++]=subject;
    }
    void start_prefix() {
        const auto starts = counts;
        prefix_staged=true;
        capture({0,-1,0});
        auto& fragment = staged_fragments[staged_fragment_count++];
        fragment = {FragmentKind::detector_prefix,nullptr,prefix_render,{}};
        for (unsigned lane = 0; lane < 4; ++lane) {
            fragment.lanes[lane] = {static_cast<std::uint32_t>(starts[lane]),
                static_cast<std::uint32_t>(counts[lane]-starts[lane])};
            if (fragment.lanes[lane].commands) command(lane,0,0);
        }
    }
    void commit_detector() noexcept {
        if (!detector) return;
        // Retention was admitted before membership; capture() bounded every
        // staged row against the authoritative history before native/RAM
        // publication. Exclusive ownership keeps this a no-fail commit.
        if (appended_count && detector->history.append(source_context,
            std::span(previous_subjects).first(appended_count))!=focus::Error::none)
            std::terminate(); // A violated exclusive-owner preflight cannot report success.
        if (prefix_staged) {
            const auto destination=static_cast<std::uint16_t>(1-source_context);
            if (detector->history.stage(destination,
                std::span(current_subjects).first(current_count),staged_photo)!=focus::Error::none)
                std::terminate();
            detector->context=destination;
            detector->photo=staged_photo;
        }
    }
    void stage(const scene::render::Pass* pass, const menu::model::RenderContext& context,
               std::uintptr_t ram_begin,std::uintptr_t ram_end,bool prefix_visible) {
        prefix_staged=false; appended_count=0; current_count=0; staged_fragment_count=0;
        const bool draw_prefix=detector && prefix_visible && (!pass || pass->includes(prefix_render));
        if (detector) {
            require(detector->context<2,"Actor source detector context is invalid");
            source_context=detector->context;
            previous_count=detector->history.history(source_context).size();
            if (draw_prefix) {
                const auto destination=static_cast<std::uint16_t>(1-source_context);
                require(detector->history.context_available(destination),
                    "Actor source detector prefix context is retained");
                require(detector->history.generation(destination)!=UINT64_MAX,
                    "Actor source detector prefix generation exhausted");
            }
        }
        bool membership_prepared=false;
        const auto prepare_membership = [&] {
            if (detector->prepare_membership)
                native(detector->prepare_membership(detector->prefix_user,staged_photo));
            else staged_photo=detector->photo;
            membership_prepared=true;
        };
        std::size_t active = 0;
        for (const auto& binding : bindings) {
            auto b = binding.model;
            if (!b.object && !b.model && !b.runtime && b.state.instance == menu::animation::InstanceHandle{}) continue;
            require(b.runtime && binding.render.link < 64,"Actor active binding lacks source/runtime metadata");
            // Resolve fresh active incarnation, never trust a copied successful view.
            b.state = b.runtime->model(b.state.instance);
            require(bool(b.state) && b.state.resource_model < policies.size(),"Actor stale instance or resource binding");
            const auto& p = policies[b.state.resource_model];
            IsleActorSource* source=nullptr;
            native(isle_actor_source_get(game,b.object,&source));
            const auto source_at=reinterpret_cast<std::uintptr_t>(source);
            require(source_at<=UINTPTR_MAX-sizeof(*source) &&
                (ram_end<=source_at || source_at+sizeof(*source)<=ram_begin),"Actor RAM aliases canonical source payload");
            require(active < staged.size(),"Actor bindings exceed explicit slot count");
            if (detector && active) {
                const auto previous=staged_render[active-1];
                require(previous.link<binding.render.link ||
                    (previous.link==binding.render.link && previous.priority>=binding.render.priority),
                    "Actor detector bindings are not in source DL/priority order");
            }
            if (draw_prefix && !membership_prepared &&
                (binding.render.link>prefix_render.link ||
                 (binding.render.link==prefix_render.link && binding.render.priority<=prefix_render.priority)))
                prepare_membership();
            staged_subjects[active]={reinterpret_cast<focus::Object>(b.object),source->id,source->flags};
            staged_render[active]=binding.render;
            b.draw = b.draw && p.geometry && (!p.gate || !(staged_subjects[active].flags&0x100u)) &&
                (!pass || pass->includes(binding.render));
            if (detector && detector->enabled && p.gate && b.draw && !membership_prepared &&
                !detector->history.context_available(source_context)) {
                // Only actual pre-prefix captures append to the old context.
                // Consult native/Runtime visibility, as the kernel does, before
                // the prefix hook can publish canonical membership flags.
                IsleGameObjectMetadata metadata{};
                native(isle_game_object_metadata(game,b.object,&metadata));
                require((metadata.flags&1u) ||
                    (b.pose!=menu::model::PoseSource::native && (b.state.object_flags&1u)),
                    "Actor source detector append context is retained");
            }
            staged[active++] = b;
        }
        if (draw_prefix && !membership_prepared) prepare_membership();
        require(bool(kernel->stage(std::span(staged).first(active))),"Actor kernel rejected native pose/material/slot capacity");
        counts.fill(0);
        const auto models = resources.models();
        for (std::size_t i = 0; i < active; ++i) {
            const auto& b = staged[i];
            const auto render=staged_render[i];
            if (draw_prefix && !prefix_staged &&
                (render.link>prefix_render.link || (render.link==prefix_render.link && render.priority<=prefix_render.priority)))
                start_prefix();
            auto& fragment = staged_fragments[staged_fragment_count++];
            fragment = {FragmentKind::object,b.object,render,{}};
            const auto starts = counts;
            if (kernel->draws(b.slot)) {
                const auto& p = policies[b.state.resource_model];
                const auto& model = models[b.state.resource_model];
                if (p.fog) { fog(0,model,true); if (p.transparent) fog(1,model,true); }
                menu::model::HierarchyBuffer buffer{{std::span(lanes[0]),std::span(lanes[1]),
                    std::span(lanes[2]),std::span(lanes[3])},counts,pending};
                require(bool(kernel->encode(b.slot,context,buffer)),"Actor source hierarchy/context/list capacity rejected");
                if (p.fog) { fog(0,model,false); if (p.transparent) fog(1,model,false); }
                if (detector && detector->enabled && p.gate) capture(staged_subjects[i]);
            }
            for (unsigned lane = 0; lane < 4; ++lane) {
                fragment.lanes[lane] = {static_cast<std::uint32_t>(starts[lane]),
                    static_cast<std::uint32_t>(counts[lane]-starts[lane])};
                if (fragment.lanes[lane].commands) command(lane,0,0);
            }
        }
        if (draw_prefix && !prefix_staged) start_prefix();
        std::size_t cursor = layout.lists.offset+camera_commands.size()*8;
        for (unsigned lane = 0; lane < 4; ++lane) {
            command(lane,0xdf000000,0);
            require(cursor <= layout.lists.offset+layout.lists.size &&
                    counts[lane]*8 <= layout.lists.offset+layout.lists.size-cursor,
                    "Actor EndDL sublists exceed explicit list reservation");
            ranges[lane] = {static_cast<std::uint32_t>(cursor),static_cast<std::uint32_t>(counts[lane])};
            cursor += counts[lane]*8;
        }
        for (auto& fragment : std::span(staged_fragments).first(staged_fragment_count))
            for (unsigned lane = 0; lane < 4; ++lane) {
                auto& range = fragment.lanes[lane];
                range.address = ranges[lane].address+range.address*8;
                fragment.continuation_slots[lane] = range.commands ? range.address+range.commands*8 : 0;
            }
    }
};
Producer::Producer(IsleGame* game, IsleGameCamera* camera, menu::model::CameraBasisState& basis,
    const beach::ActorResources& resources, std::span<const menu::animation::SlotCapacity> capacities,
    std::span<const ActorRenderBinding> bindings, ProducerLayout layout, std::span<const Range> other,
    DetectorBinding* detector)
    : impl_(std::make_unique<Impl>(game,camera,basis,resources,bindings,layout,detector)) {
    auto& s = *impl_;
    require(!detector || detector->context<2,"Actor source detector context is invalid");
    require(game && camera && !capacities.empty() && !bindings.empty() && !other.empty(),
            "Actor producer requires actual native owners, explicit slots/bindings and complete external RAM plan");
    if (detector) for (const auto r:detector->copies.reservations())
        require(std::any_of(other.begin(),other.end(),[&](Range owner) {
            return owner.offset<=r.offset && owner.size>=r.size && r.offset-owner.offset<=owner.size-r.size;
        }),"Actor complete RAM union omits detector capture storage");
    resources.require_live();
    // Validate the FULL supplied union against assets, including task/output and
    // framebuffer spans. Isolated ActorResources.install({}) proves no such union.
    std::vector<Range> ranges{{0,graphics::bss_end}};
    for (std::size_t i = 0; i < s.owned.size(); ++i) { valid(s.owned[i],i == 0 ? 64 : 8); ranges.push_back(s.owned[i]); }
    for (const auto r : layout.framebuffers) { valid(r,64); ranges.push_back(r); }
    for (const auto r : other) { valid(r,1); ranges.push_back(r); }
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j)
            require(!overlap(ranges[i],ranges[j]),"Actor complete RAM owner ranges overlap");
        for (const auto& span : resources.installation_spans()) {
            require(span.guest_address >= 0x80000000 && span.guest_address-0x80000000 <= beach::rdram_size &&
                    span.bytes.size() <= beach::rdram_size-(span.guest_address-0x80000000),"Actor source asset extent invalid");
            require(!overlap(ranges[i],{span.guest_address-0x80000000,span.bytes.size()}),
                    "Actor complete RAM plan overlaps actual immutable assets");
        }
    }
    const auto models = resources.models();
    s.policies.resize(models.size()); std::vector<bool> seen(models.size());
    for (const auto& init : resources.initializations()) {
        require(init.model < models.size() && !seen[init.model],"Actor source initialization/model identity changed");
        s.policies[init.model] = policy(init,models[init.model]); seen[init.model] = true;
    }
    require(std::all_of(seen.begin(),seen.end(),[](bool v) { return v; }),"Actor model lacks actual source callback policy");
    for (const auto& model : models) {
        const auto difference = std::int32_t(model.fog_max)-model.fog_min;
        require(difference > 0 && model.fog_max <= 1000,"Actor source World fog interval invalid");
        const auto multiplier = 128000/difference, offset = (500-std::int32_t(model.fog_min))*256/difference;
        require(multiplier <= 32767 && offset >= -32768 && offset <= 32767,"Actor source fog numeric extent invalid");
    }
    require(layout.payloads.size >= 256,"Actor camera payload reservation exhausted");
    std::size_t matrix = layout.payloads.offset+256, material = layout.materials.offset, nodes = 0;
    std::vector<menu::model::KernelSlot> slots; slots.reserve(capacities.size());
    for (const auto capacity : capacities) {
        require(capacity.nodes && capacity.nodes <= (layout.payloads.offset+layout.payloads.size-matrix)/64 &&
                capacity.material_attachments <= (layout.materials.offset+layout.materials.size-material)/264,
                "Actor explicit slot matrix/material capacity exceeds RAM plan");
        slots.push_back({capacity,static_cast<std::uint32_t>(matrix),static_cast<std::uint32_t>(material)});
        matrix += capacity.nodes*64; material += capacity.material_attachments*264; nodes += capacity.nodes;
    }
    s.kernel = std::make_unique<menu::model::BorrowedKernel>(game,models,resources.materials(),slots,
        Range{matrix,layout.payloads.offset+layout.payloads.size-matrix});
    s.staged.resize(capacities.size());
    s.staged_render.resize(capacities.size()); s.staged_subjects.resize(capacities.size());
    s.staged_fragments.resize(capacities.size()+1); s.fragments.resize(capacities.size()+1);
    for (auto& lane : s.lanes) lane.resize(layout.lists.size/8);
    // Every node can append a matrix + source48's13 words and a placement.
    s.pending.resize(nodes*15+capacities.size());
}
Producer::~Producer() = default;
ProducerFrame Producer::prepare_filtered(std::span<std::uint32_t> words, std::uint64_t epoch,
                                        const scene::render::Pass* pass,bool prefix_visible) {
    auto& s = *impl_;
    require(!s.retired && words.data() && words.size_bytes() == beach::rdram_size && epoch,
            "Actor prepare requires live exclusive canonical RAM and nonzero epoch");
    require(!pass || (pass->kind == scene::render::Kind::compiled && pass->camera == s.camera),
            "Actor compiled pass camera/policy differs from actual owner");
    require(s.revision != UINT64_MAX,"Actor frame revision exhausted");
    s.resources.require_live();
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
    require(begin%alignof(std::uint32_t) == 0 && begin <= UINTPTR_MAX-words.size_bytes(),"Actor RAM pointer extent invalid");
    const auto end = begin+words.size_bytes();
    auto disjoint = [&](const void* pointer, std::size_t bytes) {
        if (!bytes) return;
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        require(pointer && at <= UINTPTR_MAX-bytes && (end <= at || at+bytes <= begin),"Actor RAM aliases preparation owner");
    };
    disjoint(this,sizeof(*this)); disjoint(&s,sizeof(s)); disjoint(&s.basis,sizeof(s.basis));
    disjoint(s.game,1); disjoint(s.camera,1); disjoint(&s.resources,sizeof(s.resources));
    disjoint(s.bindings.data(),s.bindings.size_bytes());
    disjoint(s.policies.data(),s.policies.size()*sizeof(Policy));
    disjoint(s.staged.data(),s.staged.size()*sizeof(menu::model::BorrowedBinding));
    disjoint(s.staged_render.data(),s.staged_render.size()*sizeof(scene::RenderMetadata));
    disjoint(s.staged_subjects.data(),s.staged_subjects.size()*sizeof(focus::Subject));
    disjoint(s.staged_fragments.data(),s.staged_fragments.size()*sizeof(ProducerFragment));
    disjoint(s.fragments.data(),s.fragments.size()*sizeof(ProducerFragment));
    if (s.detector) {
        disjoint(s.detector,sizeof(*s.detector));
        disjoint(&s.detector->copies,sizeof(s.detector->copies));
        disjoint(&s.detector->history,sizeof(s.detector->history));
    }
    disjoint(s.pending.data(),s.pending.size()*sizeof(Command));
    if (pass) disjoint(pass,sizeof(*pass));
    for (const auto& lane : s.lanes) disjoint(lane.data(),lane.size()*sizeof(Command));
    for (const auto& span : s.resources.installation_spans()) disjoint(span.bytes.data(),span.bytes.size());
    std::size_t count = 0; native(isle_game_camera_matrix_count(s.game,s.camera,&count));
    require(count == 2,"Actor Beach camera requires source kinds3/14");
    native(isle_game_camera_validate_disjoint(s.game,s.camera,words.data(),words.size_bytes()));
    std::array<const IsleGameCameraMatrix*,2> camera{};
    for (std::size_t i = 0; i < camera.size(); ++i) {
        native(isle_game_camera_peek_matrix(s.game,s.camera,i,&camera[i]));
        require(camera[i]->kind == (i == 0 ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL),
                "Actor Beach camera matrix kinds/order changed");
        disjoint(camera[i],sizeof(*camera[i]));
    }
    const IsleGameCameraParameters* parameters = nullptr;
    native(isle_game_camera_parameters(s.game,s.camera,&parameters)); disjoint(parameters,sizeof(*parameters));
    menu::model::RenderContext context;
    constexpr std::array<menu::model::CameraMatrixOperation,2> operations{{{3,0},{14,0}}};
    require(bool(menu::model::prepare_render_context(s.basis.value,*parameters,operations,s.basis.selector,context)),
            "Actor source camera context rejected");
    require(bool(s.kernel->validate_upload(words,epoch)),"Actor kernel fixed scratch aliases RAM");
    s.stage(pass,context,begin,end,prefix_visible);
    require(bool(s.kernel->validate_upload(words,epoch)),"Actor kernel borrowed pose aliases RAM");
    // Last fallible computation: the native camera batch is failure-atomic.
    // Membership may already have changed canonical visibility flags, including
    // on a rejected preparation. No pose/cache, RAM, detector metadata/context,
    // basis or frame revision has yet been published.
    native(isle_game_camera_compute_all(s.game,s.camera));
    const IsleGameCameraReflection* reflection = nullptr;
    native(isle_game_camera_reflection(s.game,s.camera,1,&reflection));
    std::uint16_t norm = 0; native(isle_game_camera_perspective_normalization(s.game,s.camera,&norm));
    s.kernel->commit_pose();
    s.camera_commands = {{{0xda380007,s.payload(0)},{0xdb0e0000,norm},{0xdc08000a,s.payload(192)},
        {0xdc08030a,s.payload(208)},{0xda380005,s.payload(64)},{0xda380003,s.payload(128)},
        {0xdb060000,0},{0xd5000001,0},{0xdf000000,0}}};
    graphics::Memory memory(words);
    const bool fresh = s.ram != words.data() || s.epoch != epoch;
    for (std::size_t i = 0; i < camera.size(); ++i) if (fresh || s.camera_revisions[i] != camera[i]->revision) {
        memory.matrix(s.payload(i*64),camera[i]->fixed); s.camera_revisions[i] = camera[i]->revision;
    }
    if (fresh) {
        const IsleGameFixedMatrix identity{{{0x10000,0,1,0},{0,0x10000,0,1},{0,0,0,0},{0,0,0,0}}};
        memory.matrix(s.payload(128),identity);
    }
    if (fresh || s.reflection_revision != reflection->revision) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            const auto at = s.payload(192+axis*16);
            for (unsigned b = 0; b < 16; ++b) memory.byte(at+b,0);
            for (unsigned c = 0; c < 3; ++c) {
                memory.byte(at+c,reflection->color[axis][c]); memory.byte(at+4+c,reflection->color_copy[axis][c]);
                memory.byte(at+8+c,static_cast<std::uint8_t>(reflection->direction[axis][c]));
            }
        }
        s.reflection_revision = reflection->revision;
    }
    s.kernel->upload(words,epoch);
    const auto write = [&](std::uint32_t at, Command c) {
        if (fresh || words[at/4] != c.w0) memory.word(at,c.w0);
        if (fresh || words[at/4+1] != c.w1) memory.word(at+4,c.w1);
    };
    for (std::size_t i = 0; i < s.camera_commands.size(); ++i)
        write(static_cast<std::uint32_t>(s.layout.lists.offset+i*8),s.camera_commands[i]);
    for (unsigned lane = 0; lane < 4; ++lane) for (std::size_t i = 0; i < s.counts[lane]; ++i)
        write(s.ranges[lane].address+static_cast<std::uint32_t>(i*8),s.lanes[lane][i]);
    s.ram = words.data(); s.epoch = epoch; s.basis.value = context; ++s.revision;
    s.commit_detector();
    s.fragments.swap(s.staged_fragments); s.fragment_count = s.staged_fragment_count;
    return {{static_cast<std::uint32_t>(s.layout.lists.offset),static_cast<std::uint32_t>(s.camera_commands.size())},
        s.ranges,s.owned,s.revision,std::span(s.fragments).first(s.fragment_count)};
}
ProducerFrame Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch) { return prepare_filtered(words,epoch,nullptr,true); }
ProducerFrame Producer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch, const scene::render::Pass& pass,
                               bool prefix_visible) {
    return prepare_filtered(words,epoch,&pass,prefix_visible);
}
bool Producer::prepare_pass(void* user, std::span<std::uint32_t> words, std::uint64_t epoch,
    const scene::render::Pass& pass, scene::render::Prepared& out) {
    require(user != nullptr,"Actor compiled pass requires actual producer owner");
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
    const auto output = reinterpret_cast<std::uintptr_t>(&out);
    require(begin <= UINTPTR_MAX-words.size_bytes() && output <= UINTPTR_MAX-sizeof(out) &&
            (begin+words.size_bytes() <= output || output+sizeof(out) <= begin),
            "Actor prepared output aliases canonical RAM");
    const auto frame = static_cast<Producer*>(user)->prepare(words,epoch,pass);
    scene::render::Prepared staged;
    staged.camera = {frame.camera.address,static_cast<std::size_t>(frame.camera.commands)*2};
    for (unsigned lane = 0; lane < 4; ++lane)
        staged.lanes[lane] = {frame.lanes[lane].address,static_cast<std::size_t>(frame.lanes[lane].commands)*2};
    out = staged; return true;
}
std::span<const Range> Producer::reservations() const noexcept { return impl_->owned; }
std::span<const menu::model::WorldMatrix> Producer::world_nodes(menu::animation::InstanceHandle h) const noexcept {
    return impl_->retired ? std::span<const menu::model::WorldMatrix>{} : impl_->kernel->world_nodes(h);
}
std::span<const IsleGameModelNode> Producer::native_nodes(menu::animation::InstanceHandle h) const noexcept {
    return impl_->retired ? std::span<const IsleGameModelNode>{} : impl_->kernel->native_nodes(h);
}
void Producer::retire() noexcept { impl_->retired = true; }
} // namespace isle::actor
