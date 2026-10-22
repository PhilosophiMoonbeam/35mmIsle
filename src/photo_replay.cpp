#include "photo_replay.hpp"
#include "photo_presentation.hpp"
#include "photo_subjects.hpp"
#include "photo_effects_renderer.hpp"
#include "photo_lighting.hpp"
#include "item_resources.hpp"
#include "course_resources.hpp"
#include "photo_score_source.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <exception>
#include <optional>
#include <stdexcept>
#include <vector>

// Own GAME df8815e75f3d79618466e01e0827c6165b93d11c:
// app_render/47380.c870-925,1039-1133,1245-1343,1646-1680;
// app_render/score.c53-122,154-327,488-674;
// window/847B60.c24-96,184-214; app_render/53AD0.c72-111.
// Score dispatches the complete admitted Source C producer through the shared
// Frame's scoped submit/drain capability; preview retains its guarded GPU loan.
namespace isle::photo {
namespace {
using Range = menu::RamRange;
using Command = menu::model::Command;
using List = menu::model::ListRange;
constexpr std::size_t ram_bytes = 8 * 1024 * 1024;
constexpr std::uint32_t copy_rom = 0x5a180, setup_rom = 0x5a2a8;
constexpr std::uint32_t copy_guest = 0x800ae7d0, setup_guest = 0x800ae8f8;
constexpr std::size_t copy_bytes = setup_rom-copy_rom;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"Replay native operation rejected"); }
void animation(menu::animation::Status status) { require(bool(status),"Replay frozen item animation rejected"); }
bool overlap(Range a, Range b) {
    return a.size && b.size && (a.offset <= b.offset ? b.offset-a.offset < a.size : a.offset-b.offset < b.size);
}
void valid(Range range, std::size_t alignment = 8) {
    require(range.size && range.offset%alignment == 0 && range.size%alignment == 0 &&
        range.offset <= ram_bytes && range.size <= ram_bytes-range.offset,"Replay physical range is invalid");
}
bool contains(Range owner, std::uint32_t address, std::size_t size) {
    return address >= owner.offset && address-owner.offset <= owner.size && size <= owner.size-(address-owner.offset);
}
bool same(float a, float b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
bool same(IsleGameVec3 a, IsleGameVec3 b) { return same(a.x,b.x) && same(a.y,b.y) && same(a.z,b.z); }
bool same(const IslePlayerPhotoData& a, const IslePlayerPhotoData& b) {
    if (a.levelID != b.levelID || a.unk_00_24 != b.unk_00_24 || a.unk_00_16 != b.unk_00_16 ||
        !same(a.unk_04.f32,b.unk_04.f32) || !same(a.unk_08,b.unk_08) || !same(a.unk_14,b.unk_14)) return false;
    for (std::size_t i = 0; i < 12; ++i) {
        const auto& x = a.pokemons[i]; const auto& y = b.pokemons[i];
        if (x.pokemonID != y.pokemonID || x.unk_00_13 != y.unk_00_13 || x.specialID != y.specialID ||
            x.poseID != y.poseID || !same(x.animationTime,y.animationTime) || !same(x.position,y.position) || !same(x.yaw,y.yaw)) return false;
    }
    for (std::size_t i = 0; i < 6; ++i)
        if (a.items[i].itemType != b.items[i].itemType || a.items[i].animationTime != b.items[i].animationTime ||
            !same(a.items[i].pos,b.items[i].pos)) return false;
    for (std::size_t i = 0; i < 32; ++i) {
        const auto& x = a.effects[i]; const auto& y = b.effects[i];
        if (x.textureID != y.textureID || x.dataID != y.dataID || x.bankID != y.bankID || x.unk_03 != y.unk_03 ||
            x.posX != y.posX || x.posY != y.posY || x.posZ != y.posZ || x.size != y.size) return false;
        for (unsigned c = 0; c < 4; ++c) if (x.primColor[c] != y.primColor[c]) return false;
    }
    return true;
}
void finite(IsleGameVec3 v) {
    require(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z),"Nonfinite recorded replay vector");
}
// Same fresh-AObj seek budget as the subject reconstruction. Items have looping
// material streams, so Runtime's default acyclic certificate is not applicable.
std::size_t seek_budget(const menu::Animation& stream, float time,
    std::span<std::optional<float>> scratch) {
    float left = -time;
    std::size_t cursor = 0, count = 0;
    auto visited = scratch.first(stream.commands.size());
    std::fill(visited.begin(),visited.end(),std::nullopt);
    for (;;) {
        require(cursor < stream.commands.size() && count != SIZE_MAX,"Recorded item seek escapes decoded program");
        const auto& command = stream.commands[cursor++]; ++count;
        if (!command.opcode) return count;
        if (command.opcode == 1 || command.opcode == 14) {
            const auto target = command.wire.u32(4);
            if (!target) return count;
            const auto found = std::find_if(stream.commands.begin(),stream.commands.end(),
                [target](const auto& value) { return value.wire.guest_address == target; });
            require(found != stream.commands.end(),"Recorded item seek has no decoded jump target");
            cursor = static_cast<std::size_t>(found-stream.commands.begin());
            if (visited[cursor]) require(left > *visited[cursor],"Recorded item seek cycle cannot advance Source clock");
            visited[cursor] = left;
        }
        switch (command.opcode) {
        case 2: case 3: case 5: case 8: case 10: case 15: case 16: case 17: case 18: case 20: case 23:
            left += static_cast<float>(command.duration); break;
        case 22: left = static_cast<float>(command.duration); break;
        default: break;
        }
        if (left > 0) return count;
    }
}
// Four genuine recorded item families, without the live Item owner, commands,
// physics, callbacks or process simulation. All native retire observers live in
// a fixed array and survive partially completed creation/deletion.
class RecordedItems final {
    struct Slot {
        menu::model::PacketBinding binding;
        menu::animation::InstanceHandle instance;
        bool observed = false, requested = false, joined = false, retiring = false;
    };
    IsleGame* game_;
    const items::Resources& resources_;
    const IslePlayerPhotoData& record_;
    std::uint32_t object_link_, dl_link_;
    std::array<Slot,6> slots_{};
    std::vector<menu::model::PacketBinding> bindings_;
    std::vector<menu::animation::SlotCapacity> capacities_;
    std::vector<IsleGameModelNodeInit> scratch_;
    std::unique_ptr<menu::animation::Runtime> runtime_;
    std::size_t acquired_ = 0;
    static void joined(IsleGameObject*, void* context) noexcept { static_cast<Slot*>(context)->joined = true; }
    const items::Definition& definition(std::size_t i) const {
        switch (record_.items[i].itemType) {
        case 1: return resources_.definition(items::Family::pester_ball);
        case 2: return resources_.definition(items::Family::apple);
        case 3: return resources_.definition(items::Family::water_splash);
        case 4: return resources_.definition(items::Family::lava_splash);
        default: throw std::runtime_error("Inactive recorded item has no definition");
        }
    }
public:
    RecordedItems(IsleGame* game, const items::Resources& resources, const IslePlayerPhotoData& record,
        std::uint32_t object_link = 17, std::uint32_t dl_link = 19)
        : game_(game), resources_(resources), record_(record), object_link_(object_link), dl_link_(dl_link) {
        std::size_t nodes = 0;
        for (std::size_t i = 0; i < 6 && record.items[i].itemType >= 1 && record.items[i].itemType <= 4; ++i) {
            finite(record.items[i].pos);
            const auto capacity = menu::animation::slot_capacity(resources.models()[definition(i).model]);
            capacities_.push_back(capacity); nodes = std::max(nodes,capacity.nodes);
        }
        bindings_.resize(capacities_.size()); scratch_.resize(nodes);
        if (!capacities_.empty()) {
            std::size_t stream_size = 0, maximum = 0;
            for (const auto& stream : resources.animations()) stream_size = std::max(stream_size,stream.commands.size());
            std::vector<std::optional<float>> scratch(stream_size);
            for (std::size_t i = 0; i < capacities_.size(); ++i) {
                std::size_t commands = 0;
                const float time = record.items[i].itemType == 2 ? float(record.items[i].animationTime&15u) :
                    float(record.items[i].animationTime);
                const auto charge = [&](std::size_t stream) {
                    if (stream == menu::no_index) return;
                    const auto count = seek_budget(resources.animations()[stream],time,scratch);
                    require(count <= SIZE_MAX-commands,"Recorded item seek capacity overflow"); commands += count;
                };
                for (const auto& node : resources.models()[definition(i).model].nodes) {
                    charge(node.animation);
                    for (const auto stream : node.material_animations) charge(stream);
                }
                maximum = std::max(maximum,commands);
            }
            auto made = menu::animation::Runtime::create({resources.models(),resources.materials(),resources.animations(),menu::no_index,{}},
                capacities_,{},{maximum,0});
            require(bool(made),"Replay item Runtime admission rejected"); runtime_ = std::move(made.runtime);
        }
    }
    ~RecordedItems() { if (acquired_) std::terminate(); }
    void publish() {
        require(!acquired_,"Recorded items already published");
        for (std::size_t i = 0; i < capacities_.size(); ++i) {
            auto& slot = slots_[i]; const auto& item = record_.items[i];
            const auto& d = definition(i); const auto& model = resources_.models()[d.model];
            const auto acquired = runtime_->acquire(i,d.model,false);
            require(bool(acquired),"Replay item Runtime acquisition rejected");
            slot.instance = acquired.handle; ++acquired_;
            auto state = runtime_->model(slot.instance);
            auto root = state.nodes[0].transform;
            root.position = {item.pos.x,item.pos.y,item.pos.z}; root.scale = {0.1f,0.1f,0.1f};
            animation(runtime_->set_node_transform(slot.instance,0,root));
            const float time = item.itemType == 2 ? float(item.animationTime&15u) : float(item.animationTime);
            animation(runtime_->restart_model(slot.instance,time));
            animation(runtime_->set_model_speed(slot.instance,0));
            animation(runtime_->set_texture_speed(slot.instance,0));
            animation(runtime_->step_model(slot.instance));
            require(runtime_->events().empty(),"Recorded item unexpectedly emitted live callbacks");
            if (item.itemType == 2) {
                root = runtime_->model(slot.instance).nodes[0].transform;
                const auto size = float(item.animationTime>>4);
                root.scale.x = root.scale.x*size/15.0f; root.scale.y = root.scale.y*size/15.0f; root.scale.z = root.scale.z*size/15.0f;
                animation(runtime_->set_node_transform(slot.instance,0,root));
            }
            auto& binding = slot.binding;
            binding.render = {static_cast<std::uint8_t>(dl_link_),0x80000000u,UINT32_MAX};
            binding.model.slot = i; binding.model.runtime = runtime_.get();
            binding.model.state = runtime_->model(slot.instance);
            binding.model.pose = menu::model::PoseSource::native;
            binding.model.root = menu::model::RootRecipe::source;
            native(isle_game_add_object(game_,128,object_link_,0x80000000u,nullptr,nullptr,&binding.model.object));
            native(isle_game_object_set_retire_callback(game_,binding.model.object,joined,&slot)); slot.observed = true;
            require(bool(menu::model::attach_source_model(game_,binding.model.object,model,binding.model.state,
                menu::model::RootRecipe::source,scratch_,&binding.model.model)),"Recorded item native model attachment rejected");
            for (std::size_t n = 0; n < binding.model.state.nodes.size(); ++n)
                native(isle_game_model_set_flags(game_,binding.model.model,n,binding.model.state.nodes[n].flags));
            bindings_[i] = binding;
        }
    }
    bool clear() {
        while (acquired_) {
            auto& slot = slots_[acquired_-1];
            if (slot.binding.model.object) {
                if (!slot.observed) {
                    native(isle_game_object_set_retire_callback(game_,slot.binding.model.object,joined,&slot)); slot.observed = true;
                }
                if (!slot.joined && !slot.requested) { native(isle_game_delete_object(game_,slot.binding.model.object)); slot.requested = true; }
                if (!slot.joined) return false;
                slot.binding.model.object = nullptr; slot.binding.model.model = nullptr;
            }
            if (slot.instance) {
                if (!slot.retiring) { animation(runtime_->begin_retire(slot.instance)); slot.retiring = true; }
                animation(runtime_->release_retired(slot.instance)); slot.instance = {};
            }
            bindings_[acquired_-1] = {}; slot = {}; --acquired_;
        }
        return true;
    }
    bool externally_retired() const noexcept {
        for (std::size_t i = 0; i < acquired_; ++i) if (slots_[i].joined && !slots_[i].requested) return true;
        return false;
    }
    std::span<const menu::model::PacketBinding> bindings() const { return bindings_; }
    std::span<const menu::animation::SlotCapacity> capacities() const { return capacities_; }
};
struct Callback {
    IsleGameObject* object = nullptr;
    scene::RenderMetadata render;
    std::array<List,4> lanes{};
    std::span<const Range> reservations;
    bool resets_camera = false;
    std::size_t insertion = 0;
    std::uint32_t last_draw_frame = 0;
};
}

struct Replay::Impl {
    enum class Phase { idle, prepare_wide, prepare_narrow, prepare_preview, gpu_preview,
                       sound, wait60, wait1, result, stopping, stopped };
    IsleGame* game;
    const rom::Snapshot& snapshot;
    const std::byte* source;
    world::ModuleState& module;
    ReplayGpuAccess access;
    ReplayConfig config;
    std::vector<Range> external, owned, all, world_other, subject_other, item_other, effects_other;
    std::vector<sprite::RamRange> effect_protected;
    struct Asset { std::uint32_t address; std::span<const std::byte> bytes; };
    std::vector<Asset> assets;
    bool assets_ready = false;
    // Transactional CPU work storage: existing producer APIs publish only after
    // their own preflight, not after another producer's. Stage them into this
    // one cold allocation, then publish only the admitted union to actual RAM.
    // Targets/output are NOT copied: their actual RDP contents remain retained.
    std::vector<std::uint32_t> staging;
    std::array<std::vector<Command>,4> lanes;
    std::array<std::size_t,4> lane_counts{};
    std::vector<Callback> order;
    std::size_t order_count = 0;
    std::vector<std::byte> preview;
    LightingInputs lighting;
    menu::EffectResources effects;
    std::shared_ptr<world::Resources> world_resources;
    std::unique_ptr<World> world;
    std::unique_ptr<world::Producer> world_producer;
    std::unique_ptr<items::Resources> item_resources;
    std::unique_ptr<menu::EffectGraphicsResources> effect_graphics;
    std::unique_ptr<photo_effects::Renderer> effect_renderer;
    std::unique_ptr<subjects::Resources> subject_resources;
    std::unique_ptr<subjects::Reconstruction> subjects;
    std::unique_ptr<RecordedItems> items;
    std::unique_ptr<menu::model::PacketProducer> subject_producer, item_producer;
    photo_effects::Packet effect_packet;
    menu::model::CameraBasisState basis;
    IsleGameObject* camera_object = nullptr;
    IsleGameCamera* camera = nullptr;
    bool camera_observed = false, camera_requested = false, camera_joined = false;
    IsleGameObject* effect_object = nullptr;
    bool effect_observed = false, effect_requested = false, effect_joined = false;
    bool resources_current = false;
    const void* presentation_borrower = nullptr;
    ReplayRequest request{};
    IslePlayerPhotoData record{};
    ReplayGpuBinding binding{};
    rsp::CanonicalExecutor::Token token = 0;
    scene::graphics::SourceControl* score_control = nullptr;
    IslePhotoScoreSource score_source{};
    std::uint32_t score_camera_address = 0;
    std::array<std::uint32_t,4> score_lane_begin{},score_lane_end{};
    std::array<std::size_t,4> score_saved{};
    graphics::SourceProgramSelection score_programs{};
    bool score_post_drained = false, score_completed = false;
    std::array<ScoreExecution,2> score_executions{};
    std::size_t score_execution_count = 0;
    photo_feedback::WideCounts wide{};
    photo_feedback::FinalCounts counts{};
    photo_score::Photo score_photo{};
    photo_score::RulesView rules;
    photo_score::Result score{};
    std::array<photo_score::SubjectMap,12> mapping{};
    std::size_t mapping_count = 0, sound_cursor = 0;
    std::uint32_t width = 0, height = 0;
    float source_aspect = 321.0f/211.0f;
    std::size_t setup_bytes = 0;
    std::uint64_t stage_epoch = 0, obligation = 0;
    graphics::PreparedTask prepared_task{};
    Phase phase = Phase::idle;
    bool failed = false, stopping = false, request_live = false;
    std::array<char,512> message{};

    Impl(IsleGame* g, const rom::Snapshot& s, world::ModuleState& m, ReplayGpuAccess a, ReplayConfig c)
        : game(g),snapshot(s),source(s.canonical().data()),module(m),access(a),config(c),
          external(c.external_reservations.begin(),c.external_reservations.end()),staging(ram_bytes/4),lighting(s),effects(s) {
        require(game && access.acquire && access.release,"Replay needs actual Game and guarded GPU access");
        require(c.dl_link < 32 && c.world_object_link < 32 && c.photo_object_link < 32,"Replay Source link tuple is invalid");
        require(c.program == graphics::SourceProgram::f3dex2_non_fifo,"Replay requires original included F3DEX2_NoN_fifo program");
        require(c.buffer_width == 320 && c.buffer_height == 210,"Replay requires Source 320x210 preview backing storage");
        config.external_reservations = external;
        auto& l = config.memory;
        require(l.workspace.size >= 0x7100 && l.commands.size >= 0x1000 && l.output.size >= 0x1000,
            "Replay task/reset/output capacity is insufficient");
        require(l.feedback.size == photo_feedback::buffer_bytes && l.preview_color.size == 0x20d00 && l.preview_depth.size == 0x20d00,
            "Replay exact Source feedback/preview allocation extents changed");
        require(l.feedback.offset == l.preview_color.offset &&
            l.preview_depth.offset == l.preview_color.offset+l.preview_color.size,
            "Replay must preserve Source score/preview backing storage alias");
        const Range photo_storage{l.preview_color.offset,l.preview_color.size+l.preview_depth.size};
        valid(photo_storage,128);
        require(l.effects.size >= photo_effects::Renderer::max_words*4,"Replay effect list capacity is insufficient");
        l.subjects.framebuffers = l.items.framebuffers = {l.preview_color,l.preview_depth};
        owned = {l.commands,l.workspace,l.output,l.world.payloads,l.world.lists,l.world.materials,
            l.subjects.payloads,l.subjects.lists,l.subjects.materials,l.items.payloads,l.items.lists,l.items.materials,
            photo_storage,l.effects,l.effect_assets};
        for (std::size_t i = 0; i < owned.size(); ++i) {
            valid(owned[i],64);
            require(!overlap(owned[i],{0,graphics::bss_end}),"Replay storage overlaps immutable main/BSS");
            for (std::size_t j = 0; j < i; ++j) require(!overlap(owned[i],owned[j]),"Replay retained storage overlaps");
            for (const auto r : external) { valid(r,1); require(!overlap(owned[i],r),"Replay storage aliases ordinary display"); }
        }
        all = owned; all.insert(all.end(),external.begin(),external.end());
        const auto other = [&](std::span<const Range> excluded, std::vector<Range>& output) {
            for (const auto r : all) {
                bool omit = false;
                for (const auto e : excluded) if (r.offset == e.offset && r.size == e.size) omit = true;
                if (!omit) output.push_back(r);
            }
        };
        const std::array wr{l.world.payloads,l.world.lists,l.world.materials}; other(wr,world_other);
        const std::array sr{l.subjects.payloads,l.subjects.lists,l.subjects.materials,photo_storage}; other(sr,subject_other);
        const std::array ir{l.items.payloads,l.items.lists,l.items.materials,photo_storage}; other(ir,item_other);
        const std::array er{l.effects,l.effect_assets}; other(er,effects_other);
        for (const auto range : effects_other)
            effect_protected.push_back({static_cast<std::uint32_t>(range.offset),static_cast<std::uint32_t>(range.size)});
        const auto capacity = (l.commands.size-0x400)/8;
        for (auto& lane : lanes) lane.resize(capacity);
        order.resize(capacity/8);
        preview.resize(l.preview_color.size);
        const auto canonical = snapshot.canonical();
        require(photo_score::RulesView::borrow(canonical.subspan(0x46270,0x5bf20-0x46270),rules),"Replay score rules unavailable");
        // These are the original bounded own-GAME lists, never SDK payloads.
        require(canonical.size() >= setup_rom+8,"Replay setup source extent unavailable");
        for (std::size_t at = setup_rom; at+8 <= 0x5a480; at += 8) {
            const auto byte = [&](std::size_t n) { return std::to_integer<std::uint32_t>(canonical[at+n]); };
            const auto w0 = (byte(0)<<24)|(byte(1)<<16)|(byte(2)<<8)|byte(3);
            if (w0 == 0xdf000000) { setup_bytes = at+8-setup_rom; break; }
        }
        require(setup_bytes,"Original score setup lacks bounded EndDL");
    }
    void fail(const char* detail) noexcept {
        if (!failed) {
            failed = true;
            const auto n = std::min(std::strlen(detail),message.size()-1);
            std::memcpy(message.data(),detail,n); message[n] = 0;
        }
        stopping = true;
    }
    bool current() const noexcept {
        return request_live && request.record.data && request.record.revision &&
            *request.record.revision == request.record.expected_revision && same(*request.record.data,record);
    }
    void require_current() { require(current(),"Replay Source record/revision capability changed"); }
    bool release_binding() noexcept {
        if (!binding.executor) return true;
        if (!access.release(access.context,binding)) { fail("Replay guarded GPU admission release failed"); return false; }
        binding = {};
        return true;
    }
    void camera_create() {
        if (camera) return;
        if (!camera_object) native(isle_game_add_object(game,0x200,5,5,nullptr,nullptr,&camera_object));
        if (!camera_observed) {
            native(isle_game_object_set_retire_callback(game,camera_object,[](IsleGameObject*,void* user) noexcept {
                static_cast<Impl*>(user)->camera_joined = true;
            },this)); camera_observed = true;
        }
        native(isle_game_attach_camera(game,camera_object,&camera));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
        native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT));
    }
    bool transient_clear() {
        effect_packet = {};
        if (effect_object) {
            if (!effect_observed) {
                native(isle_game_object_set_retire_callback(game,effect_object,[](IsleGameObject*,void* user) noexcept {
                    static_cast<Impl*>(user)->effect_joined = true;
                },this)); effect_observed = true;
            }
            if (!effect_joined && !effect_requested) { native(isle_game_delete_object(game,effect_object)); effect_requested = true; }
            if (!effect_joined) return false;
            effect_object = nullptr; effect_observed = effect_requested = effect_joined = false;
        }
        if (subject_producer) { subject_producer->retire(); subject_producer.reset(); }
        if (item_producer) { item_producer->retire(); item_producer.reset(); }
        if (items && !items->clear()) return false;
        items.reset();
        if (subjects) {
            if (subjects->failed()) fail("Replay subject native owner retired outside its request");
            const auto status = subjects->clear();
            native(status);
            if (!subjects->cleared()) return false;
        }
        subjects.reset();
        return true;
    }
    bool course_clear() {
        if (!transient_clear()) return false;
        assets.clear(); assets_ready = false;
        subject_resources.reset(); resources_current = false;
        if (world_producer) { world_producer->retire(); world_producer.reset(); }
        effect_renderer.reset(); effect_graphics.reset(); item_resources.reset();
        if (world && !world->clear()) {
            fail(world->error().empty() ? "Replay World native retirement failed" : world->error().c_str());
            return false;
        }
        if (world && !world->cleared()) {
            fail("Replay World clear returned without confirmed native retirement");
            return false;
        }
        world.reset(); world_resources.reset(); return true;
    }
    bool native_clear() {
        if (!course_clear()) return false;
        if (camera_object) {
            if (!camera_observed) {
                native(isle_game_object_set_retire_callback(game,camera_object,[](IsleGameObject*,void* user) noexcept {
                    static_cast<Impl*>(user)->camera_joined = true;
                },this)); camera_observed = true;
            }
            if (!camera_joined && !camera_requested) { native(isle_game_delete_object(game,camera_object)); camera_requested = true; }
            if (!camera_joined) return false;
            camera_object = nullptr; camera = nullptr;
        }
        return true;
    }
    bool prepare_course_resources(std::int32_t level, bool need_camera = true) {
        if (world_resources && world_resources->course_id() != level && !course_clear()) return false;
        if (!world_resources) {
            world_resources = std::make_shared<world::Resources>(snapshot,level);
            world = std::make_unique<World>(game,*world_resources,module,config.world_object_link,config.dl_link);
            if (config.background_rgb) *config.background_rgb=world_resources->setup().background_rgb;
            require(world->publish(),"Replay World native publication rejected");
        }
        if (!item_resources) {
            const auto& setup = world_resources->setup();
            item_resources = std::make_unique<items::Resources>(snapshot,setup.fog_rgb,setup.fog_min,setup.fog_max);
            const std::array banks{&effects.bank(menu::EffectGroup::common),&effects.bank(menu::effect_group(level))};
            effect_graphics = std::make_unique<menu::EffectGraphicsResources>(snapshot,banks,config.memory.effect_assets);
            effect_renderer = std::make_unique<photo_effects::Renderer>(effects,*effect_graphics,level);
            require(bool(effect_renderer->status()),"Recorded effect resource admission rejected");
        }
        if (need_camera) {
            camera_create();
            if (!world_producer) world_producer = std::make_unique<world::Producer>(game,camera,basis,*world_resources,world->render_view(),config.memory.world,world_other);
        }
        return true;
    }
    bool reconstruct() {
        require_current(); camera_create();
        if (world_resources && world_resources->course_id() != record.levelID) {
            if (!course_clear()) return false;
        } else if (!transient_clear()) return false;
        if (stopping) return false;
        if (!resources_current) {
            assets.clear(); assets_ready = false;
            subject_resources.reset(); resources_current = true;
        }
        if (!prepare_course_resources(record.levelID)) return false;
        require(world->reconstruct({record.unk_00_16,record.unk_04.f32,record.unk_08}),"Replay World reconstruction rejected");
        if (!subject_resources) subject_resources = std::make_unique<subjects::Resources>(snapshot,*world_resources,record);
        subjects = std::make_unique<subjects::Reconstruction>(game,*subject_resources,subjects::LinkOwnership{static_cast<std::uint8_t>(config.photo_object_link),static_cast<std::uint8_t>(config.dl_link),UINT32_MAX});
        native(subjects->publish());
        items = std::make_unique<RecordedItems>(game,*item_resources,record,config.photo_object_link,config.dl_link); items->publish();
        // Actual Source recorded effect GObj, created after the item objects.
        native(isle_game_add_object(game,128,config.photo_object_link,0x80000000u,nullptr,nullptr,&effect_object));
        native(isle_game_object_set_retire_callback(game,effect_object,[](IsleGameObject*,void* user) noexcept {
            static_cast<Impl*>(user)->effect_joined = true;
        },this)); effect_observed = true;
        if (request.kind == ReplayKind::preview)
            require(world->pause_thumbnail_processes(),"Replay thumbnail process pause rejected");
        if (!subject_resources->instances().empty())
            subject_producer = std::make_unique<menu::model::PacketProducer>(game,camera,basis,subject_resources->packet_resources(),
                subject_resources->capacities(),subjects->bindings(),config.memory.subjects,subject_other);
        if (!items->capacities().empty())
            item_producer = std::make_unique<menu::model::PacketProducer>(game,camera,basis,
                menu::model::PacketResources{item_resources->models(),item_resources->materials(),item_resources->installation_spans()},
                items->capacities(),items->bindings(),config.memory.items,item_other);
        return true;
    }
    void camera_configure() {
        IsleGamePerspective perspective{30.0f/request.preview_scale,float(width+1)/float(height+1),10,25600,1};
        const IsleGameCameraParameters* saved=nullptr;native(isle_game_camera_parameters(game,camera,&saved));
        auto viewport=saved->viewport;
        viewport.scale[0]=viewport.translate[0]=static_cast<std::int16_t>(((width+1)/2)*4);
        viewport.scale[1]=viewport.translate[1]=static_cast<std::int16_t>(((height+1)/2)*4);
        const IsleGameLookAt look{{record.unk_08.x,record.unk_08.y,record.unk_08.z},
            {record.unk_14.x,record.unk_14.y,record.unk_14.z},{0,1,0}};
        native(isle_game_camera_set_perspective(game,camera,&perspective));
        native(isle_game_camera_set_look_at(game,camera,&look));
        native(isle_game_camera_set_viewport(game,camera,&viewport));
        native(isle_game_camera_compute_all(game,camera));
    }
    void check_asset(std::uint32_t address, std::span<const std::byte> bytes, bool fx = false) {
        require(address <= ram_bytes && bytes.size() <= ram_bytes-address,"Replay immutable domain escapes RDRAM");
        const Range asset{address,bytes.size()};
        for (const auto r : all) {
            if (fx && r.offset == config.memory.effect_assets.offset && r.size == config.memory.effect_assets.size) continue;
            require(!overlap(asset,r),"Replay immutable domain aliases retained runtime/display storage");
        }
        require(!overlap(asset,{0,graphics::bss_end}),"Replay course asset aliases cold main/BSS");
    }
    void preflight_assets() {
        require(snapshot.canonical().data() == source && snapshot.canonical().size() == rom::us_size,"Replay Snapshot moved");
        world_resources->require_live(); subject_resources->require_live(); item_resources->require_live();
        if (assets_ready) return;
        assets.clear();
        const auto add = [&](std::uint32_t address, std::span<const std::byte> bytes, bool fx = false) {
            if (bytes.empty()) return;
            check_asset(address,bytes,fx); assets.push_back({address,bytes});
        };
        for (const auto& span : world_resources->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for (const auto& span : item_resources->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for (const auto& span : subject_resources->installation_spans()) add(span.guest_address-0x80000000u,span.bytes);
        for (const auto& span : lighting.installation_spans())
            if (span.guest_address != lighting.normal_setup_guest()) add(span.guest_address-0x80000000u,span.bytes);
        for (const auto& span : effect_graphics->installation_spans()) add(span.address,std::as_bytes(span.bytes),true);
        add(copy_guest-0x80000000u,snapshot.canonical().subspan(copy_rom,copy_bytes));
        add(setup_guest-0x80000000u,snapshot.canonical().subspan(setup_rom,setup_bytes));
        std::sort(assets.begin(),assets.end(),[](const Asset& a,const Asset& b) { return a.address < b.address; });
        // Prove every shared immutable address has identical provenance bytes,
        // and retain a disjoint upload union. No later installer may silently
        // overwrite another producer's texture, list or egg-vertex payload.
        Asset frontier{}; std::size_t emitted = 0;
        for (std::size_t i = 0; i < assets.size(); ++i) {
            const auto next = assets[i];
            const auto end = std::size_t(frontier.address)+frontier.bytes.size();
            if (!emitted || next.address >= end) {
                assets[emitted++] = next; frontier = next; continue;
            }
            const auto overlap_bytes = std::min(end-next.address,next.bytes.size());
            const auto begin = frontier.bytes.begin()+static_cast<std::ptrdiff_t>(next.address-frontier.address);
            require(std::equal(next.bytes.begin(),next.bytes.begin()+static_cast<std::ptrdiff_t>(overlap_bytes),begin),
                "Replay immutable owners disagree at a shared canonical address");
            if (overlap_bytes < next.bytes.size()) {
                assets[emitted++] = {static_cast<std::uint32_t>(next.address+overlap_bytes),next.bytes.subspan(overlap_bytes)};
                frontier = next;
            }
        }
        assets.resize(emitted); assets_ready = true;
    }
    void upload_assets(std::span<std::uint32_t> words) {
        graphics::Memory memory(words);
        for (const auto& span : assets) memory.copy(span.address,span.bytes);
        // The normal-preview list borrows a photo_check overlay address which
        // may be occupied by a different course. Its exact admitted commands
        // are placed in this owner's reset domain, not installed over models.
        memory.copy(static_cast<std::uint32_t>(config.memory.commands.offset+0x100),lighting.normal_setup().bytes);
        lighting.update_direction(std::as_writable_bytes(words),record.unk_08,record.unk_14);
    }
    void command(unsigned lane, std::uint32_t a, std::uint32_t b) {
        require(lane < 4 && lane_counts[lane] < lanes[lane].size(),"Replay composed lane exceeds cold command capacity");
        lanes[lane][lane_counts[lane]++] = {a,b};
    }
    void call(unsigned lane, std::uint32_t address) { command(lane,0xde000000,address); }
    void segment(unsigned id, std::uint32_t address) { command(0,0xdb060000|(id*4),address); }
    void raw(unsigned lane, List list, std::span<const Range> reservations) {
        if (!list.commands) return;
        require(!(list.address&7u) && list.address <= ram_bytes && list.commands <= (ram_bytes-list.address)/8,
            "Replay callback fragment escapes RDRAM");
        bool owned_range = false;
        for (const auto r : reservations) if (contains(r,list.address,std::size_t(list.commands)*8)) owned_range = true;
        require(owned_range,"Replay callback fragment escapes producer command domain");
        for (std::size_t i = 0; i < list.commands; ++i) {
            const auto at = list.address/4+i*2;
            require(staging[at] != 0xdf000000,"Replay raw callback unexpectedly contains EndDL");
            command(lane,staging[at],staging[at+1]);
        }
    }
    template<class Frame> void collect(const Frame& frame) {
        for (const auto& object : frame.objects) {
            require(order_count < order.size(),"Replay source callback count exceeds cold capacity");
            auto& entry = order[order_count];
            entry = {object.object,object.render,object.lanes,frame.reservations,false,order_count};
            if constexpr (requires { object.resets_camera; }) entry.resets_camera = object.resets_camera;
            require(entry.object && entry.render.link == config.dl_link,"Replay callback has wrong Source identity/link");
            for (std::size_t i = 0; i < order_count; ++i) require(order[i].object != entry.object,"Replay object belongs to two producers");
            ++order_count;
        }
    }
    menu::EffectCamera effect_camera() {
        const IsleGameCameraMatrix *projection = nullptr, *view = nullptr;
        native(isle_game_camera_matrix(game,camera,0,&projection)); native(isle_game_camera_matrix(game,camera,1,&view));
        menu::EffectCamera result;
        // guMtxCatF(view, projection), with its original left-associated f32 sum.
        for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
            volatile float p0 = view->floating[row][0]*projection->floating[0][col];
            volatile float p1 = view->floating[row][1]*projection->floating[1][col];
            volatile float p2 = view->floating[row][2]*projection->floating[2][col];
            volatile float p3 = view->floating[row][3]*projection->floating[3][col];
            volatile float s0 = p0+p1; volatile float s1 = s0+p2; result.projection[row][col] = s1+p3;
        }
        const IsleGameCameraParameters* parameters = nullptr; native(isle_game_camera_parameters(game,camera,&parameters));
        for (unsigned i = 0; i < 3; ++i) {
            result.viewport_scale[i] = parameters->viewport.scale[i]; result.viewport_translate[i] = parameters->viewport.translate[i];
        }
        return result;
    }
#include "photo_score_render_core.inc"
    void compose_preview() {
        camera_configure();preflight_assets();
        require(stage_epoch != UINT64_MAX,"Replay stage incarnation exhausted"); ++stage_epoch;
        graphics::validate_cold_inputs(staging,snapshot,source,static_cast<std::uint32_t>(config.memory.workspace.offset+0x7000));
        graphics::install_cold_inputs(staging,snapshot,static_cast<std::uint32_t>(config.memory.workspace.offset+0x7000));
        upload_assets(staging);
        const auto wf = world_producer->prepare(staging,stage_epoch);
        const auto sf = subject_producer ? subject_producer->prepare(staging,stage_epoch) : menu::model::PacketFrame{};
        const auto it = item_producer ? item_producer->prepare(staging,stage_epoch) : menu::model::PacketFrame{};
        auto effect_result = effect_renderer->prepare(record,effect_camera(),{staging,
            {static_cast<std::uint32_t>(config.memory.effects.offset),static_cast<std::uint32_t>(config.memory.effects.size)},stage_epoch,effect_protected});
        require(bool(effect_result.status) && bool(effect_result.packet),"Recorded effect projection/list preflight rejected");
        effect_packet = std::move(effect_result.packet);
        lane_counts.fill(0); order_count = 0; collect(wf); collect(sf); collect(it);
        // Source DL insertion is descending priority, after existing equal ties.
        // Each provider's object array is native creation order; World is created
        // before scenery/subjects, which precede items and the priority0 FX GObj.
        std::sort(order.begin(),order.begin()+static_cast<std::ptrdiff_t>(order_count),[](const auto& a,const auto& b) {
            if (a.render.link != b.render.link) return a.render.link < b.render.link;
            if (a.render.priority != b.render.priority) return a.render.priority > b.render.priority;
            return a.insertion < b.insertion;
        });
        const auto& layout = config.memory;
        graphics::Memory memory(staging);
        const auto payload = static_cast<std::uint32_t>(layout.commands.offset+0x300);
        const IsleGameCameraParameters* parameters = nullptr; native(isle_game_camera_parameters(game,camera,&parameters));
        for (unsigned i = 0; i < 4; ++i) {
            memory.half(payload+i*2,static_cast<std::uint16_t>(parameters->viewport.scale[i]));
            memory.half(payload+8+i*2,static_cast<std::uint16_t>(parameters->viewport.translate[i]));
        }
        const IsleGameFixedMatrix identity{{{0x00010000,0,1,0},{0,0x00010000,0,1},{0,0,0,0},{0,0,0,0}}};
        memory.matrix(payload+64,identity);
        const auto color=static_cast<std::uint32_t>(layout.preview_color.offset);
        const auto depth=static_cast<std::uint32_t>(layout.preview_depth.offset);
        const auto color_bytes=static_cast<std::uint32_t>(layout.preview_color.size);
        const auto depth_bytes=static_cast<std::uint32_t>(layout.preview_depth.size);
        const auto reset = graphics::reset_commands({color,depth,payload,payload+64,
            lighting.pre_render_guest()-0x80000000u,color_bytes,depth_bytes});
        for (std::size_t i = 0; i < reset.size(); ++i) memory.word(static_cast<std::uint32_t>(layout.commands.offset+i*4),reset[i]);
        memory.word(static_cast<std::uint32_t>(layout.commands.offset+reset.size()*4),0xdf000000);
        memory.word(static_cast<std::uint32_t>(layout.commands.offset+reset.size()*4+4),0);
        call(0,static_cast<std::uint32_t>(layout.commands.offset));
        // renInitCameraEx clears depth; only Cave's initObjectsOnPhoto leaves
        // CAMERA_FLAG_2 set, so other courses preserve the real color bytes.
        const auto xmax=std::min(config.buffer_width,static_cast<std::uint32_t>(
            parameters->viewport.translate[0]/4+parameters->viewport.scale[0]/4));
        const auto ymax=std::min(config.buffer_height,static_cast<std::uint32_t>(
            parameters->viewport.translate[1]/4+parameters->viewport.scale[1]/4));
        command(0,0xdc080008,payload);
        command(0,0xed000000,(xmax*4<<12)|(ymax*4));
        command(0,0xe7000000,0);command(0,0xe3000a01,3u<<20);command(0,0xe200001c,0);
        command(0,0xff100000|(config.buffer_width-1),depth);command(0,0xf7000000,0xfffcfffc);
        command(0,0xf6000000|((xmax-1)*4<<12)|((ymax-1)*4),0);
        if(record.levelID==course::cave_id) {
            command(0,0xe7000000,0);command(0,0xff100000|(config.buffer_width-1),color);
            command(0,0xf7000000,0x00410041); // native RGBA8888 05080401 -> RGBA5551 0041
            command(0,0xf6000000|((xmax-1)*4<<12)|((ymax-1)*4),0);
        }
        command(0,0xe7000000,0);command(0,0xff100000|(config.buffer_width-1),color);command(0,0xfe000000,depth);
        command(0,0xe3000a01,0);command(0,0xe200001c,0x00552078);
        call(0,static_cast<std::uint32_t>(layout.commands.offset+0x100));call(0,wf.camera.address);
        // Every lane starts with the actual source camera. Lanes 2/1/3 execute
        // only at the original combine-all boundary, not once per object.
        for (unsigned lane = 1; lane < 4; ++lane) call(lane,wf.camera.address);
        const auto draw = [&](const Callback& object) {
            for (unsigned lane = 0; lane < 4; ++lane) raw(lane,object.lanes[lane],object.reservations);
            if (object.resets_camera) call(0,wf.camera.address);
        };
        for (const auto& object : std::span(order).first(order_count)) {
            IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(game,object.object,&metadata));
            if ((metadata.flags&1u) || !object.render.camera_tag) continue;
            draw(object);
        }
        // The actual effect callback setup precedes gtlCombineAllDLists. Its
        // body runs only after lanes0,2,1,3 have really executed in that order.
        call(0,effect_packet.setup().address);
        for (auto& count : lane_counts) require(count < lanes[0].size(),"Replay lane terminal capacity exhausted");
        for (unsigned lane = 0; lane < 4; ++lane) command(lane,0xdf000000,0);
        const auto root = static_cast<std::uint32_t>(layout.commands.offset+0x200);
        std::size_t cursor = layout.commands.offset+0x400;
        const std::array<unsigned,4> lane_order{0,2,1,3};
        std::array<std::uint32_t,4> addresses{};
        for (const auto lane : lane_order) {
            require(cursor <= layout.commands.offset+layout.commands.size && lane_counts[lane]*8 <= layout.commands.offset+layout.commands.size-cursor,
                "Replay combined source lanes exceed owned commands");
            addresses[lane] = static_cast<std::uint32_t>(cursor);
            for (std::size_t i = 0; i < lane_counts[lane]; ++i) {
                memory.word(static_cast<std::uint32_t>(cursor),lanes[lane][i].w0);
                memory.word(static_cast<std::uint32_t>(cursor+4),lanes[lane][i].w1); cursor += 8;
            }
        }
        auto at = root;
        const auto root_command = [&](std::uint32_t a,std::uint32_t b) { memory.word(at,a); memory.word(at+4,b); at += 8; };
        for (const auto lane : lane_order) root_command(0xde000000,addresses[lane]);
        root_command(0xde000000,effect_packet.body().address);
        root_command(0xe9000000,0); root_command(0xdf000000,0);
        require(at <= layout.commands.offset+0x300,"Replay root aliases viewport payload");
        // Validate the borrowed program now, before any actual canonical write.
        const auto descriptor = binding.programs->descriptor(config.program);
        require(descriptor.program == config.program && layout.output.size >= descriptor.minimum_fifo_bytes,
            "Replay borrowed task program/output contract rejected");
        prepared_task = graphics::finish_task(staging,snapshot,static_cast<std::uint32_t>(layout.workspace.offset+0x7000),
            static_cast<std::uint32_t>(layout.workspace.offset),static_cast<std::uint32_t>(layout.workspace.offset+0x4000),
            static_cast<std::uint32_t>(layout.workspace.offset+0x6000),static_cast<std::uint32_t>(layout.output.offset),
            static_cast<std::uint32_t>(layout.output.offset+layout.output.size),root,config.program);
        require_current();
    }
    void commit(std::span<std::uint32_t> words) {
        require(words.size_bytes() == ram_bytes && words.data() != staging.data(),"Replay actual canonical RAM extent/identity rejected");
        graphics::validate_cold_inputs(words,snapshot,source,static_cast<std::uint32_t>(config.memory.workspace.offset+0x7000));
        // All cross-producer root/lane/domain/capacity validation completed on
        // private storage. Canonical publication below consists only of bounded
        // copies; no native/producer operation capable of rejection intervenes.
        const auto copy = [&](Range range) {
            std::copy_n(staging.begin()+static_cast<std::ptrdiff_t>(range.offset/4),range.size/4,
                words.begin()+static_cast<std::ptrdiff_t>(range.offset/4));
        };
        copy({graphics::main_begin,graphics::bss_end-graphics::main_begin});
        for (const auto range : owned) {
            if (range.offset == config.memory.output.offset || range.offset == config.memory.preview_color.offset ||
                range.offset == config.memory.effect_assets.offset) continue;
            copy(range);
        }
        if (request.kind == ReplayKind::score) copy(config.memory.feedback);
        // Immutable byte ranges need not be word-aligned; preserve adjacent RAM.
        const auto copy_asset = [&](std::uint32_t address,std::span<const std::byte> bytes) {
            auto destination = std::as_writable_bytes(words); const auto staged = std::as_bytes(std::span(staging));
            for (std::size_t i = 0; i < bytes.size(); ++i) destination[(address+i)^3u] = staged[(address+i)^3u];
        };
        for (const auto& span : assets) copy_asset(span.address,span.bytes);
    }
    bool acquire() {
        if (binding.executor) return true;
        ReplayGpuBinding granted{};
        const auto result = access.acquire(access.context,granted);
        if (granted.executor) binding = granted;
        if (result == ReplayAcquire::pending) { require(!granted.executor,"Pending replay admission returned an executor"); return false; }
        require(result == ReplayAcquire::admitted,"Replay guarded GPU admission failed");
        require(binding.frame&&binding.executor&&binding.programs&&binding.epoch&&binding.admission&&binding.executor->quiescent(),
            "Replay GPU binding lacks its required Frame/exclusive quiescence/epoch/program authority");
        return true;
    }
    void submit_preview() {
        compose_preview();
        auto preparation = binding.executor->begin_preparation();
        try {
            { const auto words = preparation.words(); commit(words); }
            token = binding.executor->begin(std::move(preparation),prepared_task.task,prepared_task.boot);
            require(token != 0,"Replay executor returned an invalid submission");
        } catch (...) { if (preparation) preparation.cancel(); throw; }
    }
    bool complete() {
        require(binding.executor && token,"Replay completion has no actual GPU submission");
        if (!binding.executor->poll(token)) return false;
        auto completed = binding.executor->completed(token); token = 0;
        if (!stopping) {
            require_current();
            const auto bytes = completed.rdram();
            require(phase==Phase::gpu_preview,"Replay direct completion is not a preview task");
            const auto base=config.memory.preview_color.offset;
            require(base<=bytes.size()&&preview.size()<=bytes.size()-base,"Completed replay preview extent rejected");
            for(std::size_t i=0;i<preview.size();++i)preview[i]=bytes[(base+i)^3u];
        }
        completed.release(); effect_packet = {};
        release_binding(); return true;
    }
    void score_input() {
        score_photo = {}; score_photo.level_id = record.levelID; score_photo.flags = record.unk_00_16;
        score_photo.has_dash_engine = request.has_dash_engine;
        score_photo.eye_x = record.unk_08.x; score_photo.eye_z = record.unk_08.z;
        score_photo.at_x = record.unk_14.x; score_photo.at_z = record.unk_14.z;
        for (std::size_t i = 0; i < 12; ++i) {
            const auto& s = record.pokemons[i];
            require(s.pokemonID >= INT16_MIN && s.pokemonID <= INT16_MAX,"Replay subject ID escapes Source signed halfword");
            score_photo.subjects[i] = {static_cast<std::int16_t>(s.pokemonID),s.unk_00_13,s.poseID,s.specialID,s.yaw};
        }
    }
    bool shortcut() {
        photo_score::Result early{};
        require(photo_score::evaluate(rules,score_photo,counts,{},early),"Replay score metadata/rules rejected");
        // Positive focus without a selected photo index is precisely the Source
        // sign/egg/ineligible-pose return before func_800A007C, not a fake score.
        if (early.pokemon_in_focus > 0 && early.selected_photo_index == 0xff) { score = early; return true; }
        return false;
    }
    void signal(Phase next) {
        require(obligation != UINT64_MAX,"Replay Source obligation identity exhausted"); ++obligation; phase = next;
    }
    void next_sound() {
        while (sound_cursor < mapping_count) {
            const auto i = sound_cursor++;
            // score.c321 intentionally indexes the raw record by render count,
            // NOT photo_PokemonIndexes. Preserve that original observable quirk.
            if (record.pokemons[i].animationTime < 0) { signal(Phase::sound); return; }
        }
        signal(Phase::wait1);
    }
    ReplayStep step() const noexcept {
        if (phase == Phase::stopped) return {failed ? ReplaySignal::failed : ReplaySignal::cancelled};
        switch (phase) {
        case Phase::idle: return {ReplaySignal::idle};
        case Phase::sound: return {ReplaySignal::play_sound85,obligation};
        case Phase::wait60: return {ReplaySignal::wait60,obligation};
        case Phase::wait1: return {ReplaySignal::wait1,obligation};
        case Phase::result: return {ReplaySignal::result};
        default: return {ReplaySignal::pending};
        }
    }
};

Replay::Replay(IsleGame* game, const rom::Snapshot& snapshot, world::ModuleState& module,
    ReplayGpuAccess access, ReplayConfig config) : impl_(std::make_unique<Impl>(game,snapshot,module,access,config)) {}
Replay::~Replay() noexcept {
    const auto& s = *impl_;
    if (s.token || s.binding.executor || s.world || s.subjects || s.items || s.camera_object || s.effect_object || s.request_live || s.presentation_borrower) std::terminate();
}
bool Replay::initialize_preview_camera(std::uint32_t background,IsleGameObject** object,
    IsleGameCamera** camera) noexcept {
    if (object) *object=nullptr;
    if (camera) *camera=nullptr;
    auto& s=*impl_;
    if (!object || !camera || s.failed || s.stopping || s.request_live ||
        s.presentation_borrower || s.camera || s.camera_object) return false;
    try {
        s.camera_create();
        const IsleGameCameraParameters* saved=nullptr;
        native(isle_game_camera_parameters(s.game,s.camera,&saved));
        auto viewport=saved->viewport;
        viewport.scale[0]=viewport.translate[0]=640;
        viewport.scale[1]=viewport.translate[1]=420;
        const IsleGamePerspective perspective{33.0f,321.0f/211.0f,62.5f,8000.0f,1.0f};
        const IsleGameLookAt look{{-1,0,0},{0,0,0},{0,0,1}};
        const IsleGameCameraRenderState render{0x17u,background,UINT32_MAX};
        native(isle_game_camera_set_perspective(s.game,s.camera,&perspective));
        native(isle_game_camera_set_look_at(s.game,s.camera,&look));
        native(isle_game_camera_set_viewport(s.game,s.camera,&viewport));
        native(isle_game_camera_set_render_state(s.game,s.camera,&render));
        native(isle_game_camera_compute_all(s.game,s.camera));
        *object=s.camera_object; *camera=s.camera;
        return true;
    } catch (const std::exception& error) { s.fail(error.what()); }
      catch (...) { s.fail("Original thumbnail camera initialization failed"); }
    return false;
}
bool Replay::begin(const ReplayRequest& request) noexcept {
    auto& s = *impl_;
    if (s.failed || s.stopping || s.phase != Impl::Phase::idle || s.request_live || s.presentation_borrower) return false;
    try {
        require(request.kind == ReplayKind::score || request.kind == ReplayKind::preview,"Invalid replay operation kind");
        require(request.record.data && request.record.revision && *request.record.revision == request.record.expected_revision,
            "Invalid replay Source record/revision capability");
        s.request = request; s.record = *request.record.data; s.request_live = true;
        (void)course::layout(s.record.levelID);
        require(std::isfinite(s.record.unk_04.f32) && s.record.unk_04.f32 >= 0,"Replay recorded global time is invalid");
        finite(s.record.unk_08); finite(s.record.unk_14);
        require(std::isfinite(request.preview_scale) && request.preview_scale > 0,"Replay preview scale is invalid");
        s.width = static_cast<std::uint32_t>(std::clamp(request.width,16,320));
        s.height = static_cast<std::uint32_t>(std::clamp(request.height,16,210));
        s.wide = {}; s.counts = {}; s.mapping = {}; s.mapping_count = 0; s.sound_cursor = 0; s.score = {};
        s.score_executions={};s.score_execution_count=0;
        s.resources_current = false; s.score_input(); s.require_current();
        if (request.kind == ReplayKind::score && s.shortcut()) s.phase = Impl::Phase::result;
        else s.phase = request.kind == ReplayKind::score ? Impl::Phase::prepare_wide : Impl::Phase::prepare_preview;
        return true;
    } catch (const std::exception& error) { s.fail(error.what()); }
      catch (...) { s.fail("Replay request admission failed"); }
    return false;
}
ReplayStep Replay::poll() noexcept {
    auto& s = *impl_;
    try {
        if (s.request_live && !s.current()) s.fail("Replay Source record/revision capability changed");
        if (!s.stopping && ((s.world && !s.world->available()) || (s.subjects && s.subjects->failed()) ||
            (s.items && s.items->externally_retired()) || (s.camera_joined && !s.camera_requested) ||
            (s.effect_joined && !s.effect_requested))) s.fail("Replay native ownership changed outside its admitted lifetime");
        if (s.token) {
            try { if (!s.complete()) return {ReplaySignal::pending}; }
            catch (...) { s.token = 0; s.effect_packet = {}; s.release_binding(); throw; }
            if(!s.stopping)s.phase=Impl::Phase::result;
            // Completion and a new acquisition are separate bounded leaves.
            if (!s.stopping) return s.step();
        }
        if (s.stopping) {
            if (s.presentation_borrower) return {ReplaySignal::pending};
            s.effect_packet = {}; if (!s.release_binding()) return {ReplaySignal::pending};
            if (!s.native_clear()) return {ReplaySignal::pending};
            s.request_live = false; s.request = {}; s.phase = Impl::Phase::stopped; return s.step();
        }
        const auto phase = s.phase;
        if (phase == Impl::Phase::prepare_wide || phase == Impl::Phase::prepare_narrow || phase == Impl::Phase::prepare_preview) {
            s.require_current();
            if(!s.acquire())return {ReplaySignal::pending};
            if(s.request.kind==ReplayKind::score) {
                s.submit_score(phase==Impl::Phase::prepare_wide);
                if(phase==Impl::Phase::prepare_wide)s.phase=Impl::Phase::prepare_narrow;
                else s.next_sound();
            } else {
                if(!s.reconstruct())return {ReplaySignal::pending};
                s.submit_preview();s.phase=Impl::Phase::gpu_preview;
            }
            return {ReplaySignal::pending};
        }
        return s.step();
    } catch (const std::exception& error) { s.fail(error.what()); }
      catch (...) { s.fail("Replay preparation/execution/native retirement failed"); }
    // Failure remains pending until the real submitted work and every native
    // retire observer have drained. A failed status is never an early fence.
    if (!s.token) { s.effect_packet = {}; s.release_binding(); }
    return {ReplaySignal::pending};
}
bool Replay::acknowledge(std::uint64_t obligation) noexcept {
    auto& s = *impl_;
    if (s.failed || s.stopping) return false;
    try {
        s.require_current();
        require(!s.token && !s.binding.executor && obligation && obligation == s.obligation,
            "Replay obligation acknowledged before GPU closure or with stale identity");
        if (s.phase == Impl::Phase::sound) s.signal(Impl::Phase::wait60);
        else if (s.phase == Impl::Phase::wait60) s.next_sound();
        else if (s.phase == Impl::Phase::wait1) {
            require(photo_score::evaluate(s.rules,s.score_photo,s.counts,std::span(s.mapping).first(s.mapping_count),s.score),
                "Actual replay feedback score evaluation rejected");
            s.phase = Impl::Phase::result;
        } else throw std::runtime_error("Replay has no acknowledgeable Source obligation");
        return true;
    } catch (const std::exception& error) { s.fail(error.what()); }
      catch (...) { s.fail("Replay obligation acknowledgment failed"); }
    return false;
}
ReplayResult Replay::result() const noexcept {
    const auto& s = *impl_;
    if (s.failed || s.stopping || s.phase != Impl::Phase::result || !s.current()) return {};
    if (s.request.kind == ReplayKind::score) return {ReplayKind::score,&s.score,{},0,0,0};
    return {ReplayKind::preview,nullptr,s.preview,s.width,s.height,s.config.buffer_width*2};
}
bool Replay::acknowledge_result() noexcept {
    auto& s = *impl_;
    if (s.failed || s.stopping || s.phase != Impl::Phase::result || s.token || s.binding.executor) return false;
    if (!s.current()) { s.fail("Replay Source record changed before result consumption"); return false; }
    s.request_live = false; s.request = {}; s.phase = Impl::Phase::idle; return true;
}
void Replay::cancel() noexcept { auto& s = *impl_; if (s.phase != Impl::Phase::stopped) s.stopping = true; }
bool Replay::retire() noexcept {
    cancel(); (void)poll();
    return impl_->phase == Impl::Phase::stopped;
}
bool Replay::prepare_course(std::int32_t level) noexcept {
    auto& s = *impl_;
    if (s.failed || s.stopping || s.request_live || s.token || s.binding.executor || s.phase != Impl::Phase::idle || s.presentation_borrower) return false;
    try { return s.prepare_course_resources(level,false); }
    catch (const std::exception& error) { s.fail(error.what()); }
    catch (...) { s.fail("Replay Source course preparation failed"); }
    return false;
}
ReplayAcquire Replay::take_presentation_world(IsleGame* game,const void* borrower,std::int32_t level,
    std::uint32_t dl,std::uint32_t world_link,std::uint32_t photo_link,ReplayWorldHandoff& output) noexcept {
    auto& s=*impl_;
    if(!borrower || game!=s.game || output.resources || output.world || s.failed || s.stopping ||
        s.presentation_borrower || dl!=s.config.dl_link || world_link!=s.config.world_object_link || photo_link!=s.config.photo_object_link)
        return ReplayAcquire::failed;
    if(s.request_live || s.token || s.binding.executor || s.phase!=Impl::Phase::idle) return ReplayAcquire::pending;
    try {
        if(!s.transient_clear()) return ReplayAcquire::pending;
        if(!s.prepare_course_resources(level,false)) return ReplayAcquire::pending;
        if(s.world_producer) {s.world_producer->retire();s.world_producer.reset();}
        s.subject_resources.reset();s.resources_current=false;s.assets.clear();s.assets_ready=false;
        output.resources=std::move(s.world_resources);output.world=std::move(s.world);
        s.presentation_borrower=borrower;
        return ReplayAcquire::admitted;
    } catch(const std::exception& error) {s.fail(error.what());} catch(...) {s.fail("Replay World handoff rejected");}
    return ReplayAcquire::failed;
}
bool Replay::return_presentation_world(const void* borrower,ReplayWorldHandoff&& input) noexcept {
    auto& s=*impl_;
    if(!borrower || s.presentation_borrower!=borrower || !input.resources || !input.world || s.world_resources || s.world) return false;
    // These are CPU-only resources: every transient native/effect borrow was
    // joined before take. World/native callbacks move without changing address.
    s.effect_renderer.reset();s.effect_graphics.reset();s.item_resources.reset();
    s.assets.clear();s.assets_ready=false;s.resources_current=false;
    s.world_resources=std::move(input.resources);s.world=std::move(input.world);
    s.presentation_borrower=nullptr;
    return true;
}
bool Replay::busy() const noexcept {
    const auto& s = *impl_; return s.phase != Impl::Phase::idle && s.phase != Impl::Phase::stopped;
}
bool Replay::gpu_busy() const noexcept { return impl_->token != 0 || impl_->binding.executor != nullptr; }
const char* Replay::error() const noexcept { return impl_->failed ? impl_->message.data() : ""; }
std::span<const Range> Replay::reservations() const noexcept { return impl_->owned; }
std::span<const ScoreExecution> Replay::score_executions() const noexcept {
    return std::span(impl_->score_executions).first(impl_->score_execution_count);
}
} // namespace isle::photo

#include "photo_presentation_core.inc"
