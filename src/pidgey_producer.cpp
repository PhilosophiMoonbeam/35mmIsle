#include "pidgey_producer.hpp"
#include "photo_lighting.hpp"
#include "35mmisle/photo_feedback.hpp"
#include "graphics_task.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

// GAME authority: df8815e75f3d79618466e01e0827c6165b93d11c.
// Original bounded native ownership/wire implementation; semantic sources:
// sys/anim.c:108-151,217-518,581-666,1163-1200; sys/om.c:965-980,
// 1061-1091; sys/render.c:838-853,856-911,913-1084,1647-1706,
// 2360-2376; app_render/47380.c:1135-1192; score.c:80-122;
// app_render/4CDB0.c:15-30,145-149; sys/gtl.c:325-364.
// Public wire ABI, not SDK producer source/headers:
// https://hack64.net/wiki/doku.php?id=f3dex2
// https://hack64.net/wiki/doku.php?id=rcpstructs
// No console pointer union, native pointer cast, SDK layout or SDK code is used.
namespace isle::pidgey {
namespace {
using graphics::bss_end;
using graphics::Memory;
constexpr std::uint32_t identity_address = PidgeyProducer::matrices_address + 128;
constexpr std::uint32_t copy_rom_begin = 0x5a180, copy_rom_end = 0x5a2a8;
constexpr std::uint32_t absent = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint32_t geometry_score = 0x00200405; // Z, shade, back cull, smooth.
constexpr std::uint32_t geometry_clear = 0x00bf0605; // score.c:100 incl. LOD/clipping.
constexpr std::uint32_t opaque_mode = 0x00552078;
constexpr std::uint32_t fog_mode = 0xc8112078;
static_assert(PidgeyProducer::model_lists_address + max_instances * PidgeyProducer::model_list_bytes ==
              PidgeyProducer::node_matrices_address);
static_assert(PidgeyProducer::node_matrices_address + max_instances * node_count * 64 == 0x10e300);
static_assert(PidgeyProducer::copy_list_address + copy_rom_end - copy_rom_begin <= 0x10ec00);

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
void check(IsleGameResult result) {
    require(result == ISLE_GAME_OK, "Native Pidgey model/camera operation failed");
}
bool equal(IsleGameVec3 a, IsleGameVec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
bool equal(const IsleGameModelTransform& a, const IsleGameModelTransform& b) {
    return equal(a.position, b.position) && equal(a.rotation, b.rotation) && equal(a.scale, b.scale);
}
bool equal(const IsleGameViewport& a, const IsleGameViewport& b) {
    for (std::size_t i = 0; i < 4; ++i)
        if (a.scale[i] != b.scale[i] || a.translate[i] != b.translate[i]) return false;
    return true;
}
IsleGameVec3 native(Vec3 v) { return {v.x, v.y, v.z}; }
IsleGameModelTransform initial(const Node& node) {
    return {native(node.position), native(node.rotation), native(node.scale)};
}

// omDObjAddAObj starts every new track at zero, NOT at the bind-pose channel.
struct Track {
    unsigned kind = 0;
    float inv_duration = 1, time = 0, initial = 0, target = 0, rate = 0, target_rate = 0;
};
float value(const Track& a) {
    if (a.kind == 2) return a.initial + a.time * a.rate;
    if (a.kind == 1) return a.inv_duration <= a.time ? a.target : a.initial;
    require(a.kind == 3, "Unadmitted Pidgey animation track kind");
    // Keep animUpdateModelAnimatedParams' binary32 grouping. Compile this TU
    // without fast-math or contraction, just like the admitted GAME math TU.
    const float t16 = a.inv_duration * a.inv_duration;
    const float t12 = a.time * a.time;
    const float t18 = a.inv_duration * t12;
    const float t14 = a.time * t12 * t16;
    const float t20 = 2.0f * t14 * a.inv_duration;
    const float t22 = 3.0f * t12 * t16;
    const float t24 = t14 - t18;
    return a.initial * ((t20 - t22) + 1.0f) + a.target * (t22 - t20) +
           a.rate * ((t24 - t18) + a.time) + a.target_rate * t24;
}
void sample(const rom::GuestView& stream, float animation_time,
            IsleGameModelTransform& transform, std::uint32_t& flags) {
    if (stream.bytes.empty()) return; // GAME ANIMATION_DISABLED (null node entry).
    std::array<Track, 10> tracks{};
    float left = -animation_time; // CHANGED transition; animSpeed is exactly zero.
    std::size_t cursor = 0;
    auto scalar = [&]() {
        const float f = stream.f32(cursor);
        cursor += 4;
        require(std::isfinite(f), "Nonfinite admitted animation scalar");
        return f;
    };
    do {
        const auto word = stream.u32(cursor);
        cursor += 4;
        const auto opcode = word >> 25;
        auto mask = (word >> 15) & 0x3ffu;
        const float duration = static_cast<float>(word & 0x7fffu);
        if (opcode == 0) {
            require(word == 0 && cursor == stream.bytes.size(), "Unadmitted Pidgey animation END");
            for (auto& track : tracks) if (track.kind != 0) track.time += left;
            // FINISHED -> apply once -> DISABLED, with frozen speed zero.
            break;
        }
        if (opcode == 2 || opcode == 15 || opcode == 16 || opcode == 17) {
            if (opcode == 15) flags = mask;
            // 16 and 17 deliver callbacks only when fnAnimCallback != NULL;
            // this constructor has no callback. Still consume 17's operands.
            if (opcode == 17)
                for (unsigned i = 0; i < 10; ++i, mask >>= 1)
                    if (mask & 1u) static_cast<void>(scalar());
            left += duration;
        } else {
            require((opcode >= 3 && opcode <= 12), "Opcode outside admitted pose-0 stream contract");
            for (unsigned channel = 0; channel < tracks.size() && mask != 0; ++channel, mask >>= 1) {
                if (!(mask & 1u)) continue;
                auto& a = tracks[channel];
                if (opcode == 12) {
                    a.time += duration; // No operand and no kind change.
                    continue;
                }
                const float next = scalar();
                if (opcode == 7) {
                    a.target_rate = next;
                    continue;
                }
                a.initial = a.target;
                a.target = next;
                if (opcode == 3 || opcode == 4) {
                    a.kind = 2;
                    if (duration != 0) a.rate = (a.target - a.initial) / duration;
                    a.target_rate = 0;
                } else if (opcode == 10 || opcode == 11) {
                    a.kind = 1;
                    a.inv_duration = duration;
                    a.target_rate = 0;
                } else {
                    a.kind = 3;
                    a.rate = a.target_rate;
                    a.target_rate = (opcode == 5 || opcode == 6) ? scalar() : 0.0f;
                    if (duration != 0) a.inv_duration = 1.0f / duration;
                }
                a.time = -left; // -timeLeft - animSpeed, with frozen speed zero.
            }
            if (opcode == 3 || opcode == 5 || opcode == 8 || opcode == 10) left += duration;
        }
        require(cursor < stream.bytes.size(), "Pidgey stream exhausted without END");
    } while (left <= 0.0f);
    for (unsigned channel = 0; channel < tracks.size(); ++channel) {
        if (tracks[channel].kind == 0) continue;
        const float v = value(tracks[channel]);
        require(std::isfinite(v), "Nonfinite sampled Pidgey channel");
        switch (channel) {
        case 0: transform.rotation.x = v; break;
        case 1: transform.rotation.y = v; break;
        case 2: transform.rotation.z = v; break;
        case 3:
            // Exact ROM inventory: all active masks are 0x007 (rotation) or
            // 0x070 (root position). No interpolation-pointer opcode 13 or
            // channel 4 is reachable; never emulate it as an ignored value.
            throw std::runtime_error("Pose-0 unexpectedly requests interpolation-pointer channel 4");
        case 4: transform.position.x = v; break;
        case 5: transform.position.y = v; break;
        case 6: transform.position.z = v; break;
        case 7: transform.scale.x = v; break;
        case 8: transform.scale.y = v; break;
        case 9: transform.scale.z = v; break;
        }
    }
}

struct Range { std::uint32_t begin, end; };
bool overlap(Range a, Range b) { return a.begin < b.end && b.begin < a.end; }
constexpr std::array<Range, 6> reserved{{
    {0, bss_end},
    {PidgeyProducer::commands_address, PidgeyProducer::runtime_end},
    {PidgeyProducer::output_address, PidgeyProducer::output_end},
    // Actual GAME light DMA union, global pre-render and normal photo setup.
    {0x0aebd0, 0x0aebe8}, {0x0aec18, 0x0aec58}, {0x1e53a0, 0x1e5410}
}};
void validate_range(Range target, const HdResources& resources) {
    for (const auto region : reserved)
        require(!overlap(target, region), "Pidgey target overlaps code/runtime/FIFO reservation");
    for (const auto& resource : resources.installation_spans()) {
        const std::uint32_t begin = resource.guest_address - 0x80000000;
        require(!overlap(target, {begin, begin + static_cast<std::uint32_t>(resource.bytes.size())}),
                "Pidgey target overlaps immutable resource");
    }
}
void validate_targets(PhotoTargets targets, const HdResources& resources) {
    require(targets.width != 0 && targets.width <= 1023 && targets.height != 0 && targets.height <= 1023,
            "Pidgey target dimensions exceed RDP/scissor extent");
    require((targets.width & 1u) == 0, "RGBA16 fill target width must be even");
    const std::uint32_t bytes = std::uint32_t(targets.width) * targets.height * 2;
    require((targets.color & 63u) == 0 && (targets.depth & 63u) == 0 &&
            targets.color <= rdram_size - bytes && targets.depth <= rdram_size - bytes,
            "Pidgey color/Z target is unaligned or outside 8 MiB");
    const std::array<Range, 2> targets_ranges{{{targets.color, targets.color + bytes},
                                             {targets.depth, targets.depth + bytes}}};
    require(!overlap(targets_ranges[0], targets_ranges[1]), "Pidgey color/Z targets overlap");
    for (const auto target : targets_ranges) validate_range(target, resources);
}
class List {
public:
    explicit List(Memory& memory, std::uint32_t begin = PidgeyProducer::commands_address,
                  std::uint32_t end = PidgeyProducer::commands_end)
        : memory_(memory), next_(begin), end_(end) {}
    void command(std::uint32_t w0, std::uint32_t w1) {
        require(next_ <= end_ - 8, "Pidgey command capacity exceeded");
        memory_.word(next_, w0); memory_.word(next_ + 4, w1); next_ += 8;
    }
    void high(unsigned shift, unsigned bits, std::uint32_t value) {
        command(0xe3000000u | ((32 - shift - bits) << 8) | (bits - 1), value);
    }
    void low(unsigned shift, unsigned bits, std::uint32_t value) {
        command(0xe2000000u | ((32 - shift - bits) << 8) | (bits - 1), value);
    }
    void segment(unsigned index, std::uint32_t address) { command(0xdb060000u | (index * 4), address); }
    void call(std::uint32_t guest) { command(0xde000000, guest); }
    void node(std::size_t index, std::span<const IsleGameModelNode> nodes,
              const HdResources& resources, std::uint32_t matrices) {
        const auto& n = nodes[index];
        if (!(n.flags & 2u)) {
            const auto payload_index = resources.nodes()[index].payload_index;
            const Payload* payload = payload_index == no_index ? nullptr : &resources.payloads()[payload_index];
            if (payload && payload->pre_guest && !(n.flags & 1u)) call(payload->pre_guest);
            const bool push = n.parent == absent || n.next_sibling != absent;
            command(push ? 0xda380000 : 0xda380001,
                    matrices + static_cast<std::uint32_t>(index * 64));
            if (payload && payload->draw_guest && !(n.flags & 1u)) {
                if (resources.node_material_guests()[index]) segment(14, PidgeyProducer::material_address);
                call(payload->draw_guest);
            }
            for (auto child = n.first_child; child != absent; child = nodes[child].next_sibling)
                node(child, nodes, resources, matrices);
            if (push) command(0xd8380002, 64);
        }
    }
    void model(std::span<const IsleGameModelNode> nodes, const HdResources& resources,
               std::uint32_t matrices) {
        // Genuine renderModelTypeIFogged wrapper, shared by preview and scoring.
        command(0xe7000000, 0);
        constexpr std::int32_t fog_multiplier = 128000 / (990 - 968);
        constexpr std::int32_t fog_offset = (500 - 968) * 256 / (990 - 968);
        command(0xdb080000, (std::uint32_t(fog_multiplier) << 16) | (std::uint32_t(fog_offset) & 0xffff));
        command(0xf8000000, 0x3c6eb400);
        high(20, 2, 1u << 20); low(3, 29, fog_mode);
        command(0xd9ffffff, 0x00010000);
        node(0, nodes, resources, matrices);
        command(0xe7000000, 0);
        high(20, 2, 0); low(3, 29, opaque_mode);
        command(0xd9feffff, 0);
        command(0xdf000000, 0);
    }
private:
    Memory& memory_;
    std::uint32_t next_, end_;
};
std::uint32_t matrices_for(std::size_t instance) {
    return PidgeyProducer::node_matrices_address + static_cast<std::uint32_t>(instance * node_count * 64);
}
std::uint32_t list_for(std::size_t instance) {
    return PidgeyProducer::model_lists_address +
           static_cast<std::uint32_t>(instance * PidgeyProducer::model_list_bytes);
}
// At most matrix/pre-list/material/draw/pop per node, plus fog wrapper.
static_assert((node_count * 5 + 16) * 8 <= PidgeyProducer::model_list_bytes);
static_assert((80 + max_instances * 8) * 8 <=
              PidgeyProducer::commands_end - PidgeyProducer::commands_address);

class CameraRestore {
public:
    CameraRestore(IsleGame* game, IsleGameCamera* camera) : game_(game), camera_(camera) {
        const IsleGameCameraParameters* parameters = nullptr;
        check(isle_game_camera_parameters(game, camera, &parameters));
        saved_ = *parameters;
    }
    ~CameraRestore() {
        static_cast<void>(isle_game_camera_set_perspective(game_, camera_, &saved_.perspective));
        static_cast<void>(isle_game_camera_set_look_at(game_, camera_, &saved_.look_at));
        static_cast<void>(isle_game_camera_set_viewport(game_, camera_, &saved_.viewport));
    }
    const IsleGameCameraParameters& saved() const noexcept { return saved_; }
private:
    IsleGame* game_;
    IsleGameCamera* camera_;
    IsleGameCameraParameters saved_{};
};
void set_camera(IsleGame* game, IsleGameCamera* camera,
                const IsleGameCameraParameters& parameters, FeedbackPass pass) {
    require(pass == FeedbackPass::wide || pass == FeedbackPass::narrow, "Invalid feedback camera pass");
    auto perspective = parameters.perspective;
    perspective.fovy = pass == FeedbackPass::wide ? 60.0f : 30.0f;
    check(isle_game_camera_set_perspective(game, camera, &perspective));
    check(isle_game_camera_set_look_at(game, camera, &parameters.look_at));
    check(isle_game_camera_set_viewport(game, camera, &parameters.viewport));
    for (std::size_t i = 0; i < 2; ++i) {
        const IsleGameCameraMatrix* matrix = nullptr;
        check(isle_game_camera_matrix(game, camera, i, &matrix));
    }
}

void emit_setup(List& list, PhotoTargets targets, std::uint16_t norm,
                unsigned depth_planes, const photo::LightingInputs* lighting) {
    list.segment(0, 0);
    list.segment(15, targets.color); list.segment(12, targets.depth);
    list.segment(10, targets.depth); list.segment(11, targets.depth);
    list.command(0xe7000000, 0);
    list.high(20, 2, 3u << 20); list.low(3, 29, 0);
    list.command(0xff100000 | (targets.width - 1), targets.depth);
    list.command(0xf7000000, 0xfffcfffc);
    const auto depth_height = std::uint32_t(targets.height) * depth_planes;
    list.command(0xed000000, (std::uint32_t(targets.width) * 4 << 12) | (depth_height * 4));
    list.command(0xf6000000 | (std::uint32_t(targets.width - 1) * 4 << 12) |
                 ((depth_height - 1) * 4), 0);
    list.command(0xe7000000, 0);
    list.command(0xff100000 | (targets.width - 1), targets.color);
    list.command(0xfe000000, targets.depth);
    list.command(0xed000000, (std::uint32_t(targets.width) * 4 << 12) |
                              (std::uint32_t(targets.height) * 4));
    list.command(0xf7000000, 0x00010001);
    list.command(0xf6000000 | (std::uint32_t(targets.width - 1) * 4 << 12) |
                 (std::uint32_t(targets.height - 1) * 4), 0);
    list.command(0xe7000000, 0);
    list.command(0xd9000000 | (~geometry_clear & 0x00ffffff), 0);
    // Exact source-selected GAME FRUSTRATIO_1 at US ROM 0x5a330..0x5a350.
    list.command(0xdb040004, 1); list.command(0xdb04000c, 1);
    list.command(0xdb040014, 0x0000ffff); list.command(0xdb04001c, 0x0000ffff);
    list.command(0xd7000000, 0);
    list.command(0xd9ffffff, geometry_score);
    list.command(0xda380007, identity_address); list.command(0xda380003, identity_address);
    list.high(23, 1, 1u << 23);
    list.command(0xfcffffff, 0xfffe793c); // SHADE in both cycles.
    list.high(16, 1, 0); list.high(14, 2, 0); list.high(17, 2, 0);
    list.high(19, 1, 1u << 19); list.high(12, 2, 2u << 12);
    list.high(9, 3, 6u << 9); list.high(8, 1, 0);
    list.low(0, 2, 0); list.high(6, 2, 0); list.high(4, 2, 0);
    list.high(20, 2, 0); list.low(3, 29, opaque_mode); list.low(2, 1, 0);
    list.command(0xe7000000, 0);
    if (lighting) list.call(lighting->pre_render_guest());
    list.command(0xdc080008, PidgeyProducer::viewport_address);
    list.command(0xda380007, PidgeyProducer::matrices_address);
    list.command(0xdb0e0000, norm);
    list.command(0xda380005, PidgeyProducer::matrices_address + 64);
    if (lighting) {
        list.call(lighting->normal_setup_guest());
        list.high(6, 2, 0);
        list.command(0xe7000000, 0);
        list.low(3, 29, 0x0f5f2078);
        list.command(0xfc1219ff, 0xfffffe38);
    }
}
} // namespace

PidgeyProducer::PidgeyProducer(IsleGame* game, IsleGameCamera* camera,
                             const rom::Snapshot& snapshot, const HdResources& resources,
                             std::size_t capacity)
    : game_(game), camera_(camera), snapshot_(snapshot), resources_(resources), capacity_(capacity),
      snapshot_source_(snapshot.canonical().data()) {
    require(game && camera && snapshot.canonical().size() == rom::us_size,
            "Pidgey producer requires live native and immutable input owners");
    require(capacity >= 1 && capacity <= max_instances, "Pidgey instance capacity must be 1..12");
    // Exact private US ROM, GAME score.c:53-79 at the pinned revision. This
    // self-contained 37-command static list is copied, never macro-recreated.
    // Its only memory operands are C+[0,C00], B+[0,C00], F and A segments.
    const rom::GuestView copy{0, copy_rom_begin,
                       snapshot.canonical().subspan(copy_rom_begin, copy_rom_end - copy_rom_begin)};
    for (std::size_t i = 0; i < copy.bytes.size(); i += 8) {
        const auto w0 = copy.u32(i), w1 = copy.u32(i + 4), op = w0 >> 24;
        switch (op) {
        case 0xfd: require(w1 == 0x0c000000 || w1 == 0x0c000c00, "Copy source escapes C"); break;
        case 0xff: require(w1 == 0x0b000000 || w1 == 0x0b000c00 || w1 == 0x0f000000,
                           "Copy color target escapes B/F"); break;
        case 0xfe: require(w1 == 0x0a000000, "Copy depth target escapes A"); break;
        case 0xdf: require(i + 8 == copy.bytes.size() && w1 == 0, "Copy terminator is not final"); break;
        case 0xe7: case 0xe3: case 0xe2: case 0xed: case 0xf5: case 0xe6:
        case 0xf3: case 0xf2: case 0xe4: case 0xe1: case 0xf1: break;
        default: throw std::runtime_error("Original GAME copy list has an external reference/unsupported opcode");
        }
    }
    require(copy.u32(9 * 8) == 0xf3000000 && copy.u32(9 * 8 + 4) == 0x075ff080 &&
            copy.u32(21 * 8) == 0xf3000000 && copy.u32(21 * 8 + 4) == 0x075ff080 &&
            copy.u32(36 * 8) == 0xdf000000, "Original GAME two-half LoadBlock bounds differ");
    std::size_t camera_count = 0;
    check(isle_game_camera_matrix_count(game_, camera_, &camera_count));
    require(camera_count == 2, "Photo camera must own perspective then look-at matrices");
    for (std::size_t i = 0; i < 2; ++i) {
        const IsleGameCameraMatrix* matrix = nullptr;
        check(isle_game_camera_matrix(game_, camera_, i, &matrix));
        require(matrix->kind == (i == 0 ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT),
                "Photo camera matrix order is not perspective/look-at");
    }
    // Validate ALL immutable installation spans against every runtime owner,
    // before any mutable object is attached or any canonical memory is written.
    for (const auto& resource : resources.installation_spans()) {
        const auto begin = resource.guest_address - 0x80000000;
        const Range range{begin, begin + static_cast<std::uint32_t>(resource.bytes.size())};
        for (const auto reservation : reserved)
            require(!overlap(range, reservation), "Pidgey immutable input overlaps runtime reservation");
    }
    std::array<IsleGameModelNodeInit, node_count> init{};
    for (std::size_t i = 0; i < node_count; ++i)
        init[i] = {resources.nodes()[i].parent == no_index ? absent : resources.nodes()[i].parent,
                   initial(resources.nodes()[i]), ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE};
    try {
        for (std::size_t i = 0; i < capacity_; ++i) {
            auto& instance = instances_[i];
            check(isle_game_add_object(game_, 128, 0, 0x80000000, nullptr, nullptr, &instance.object));
            check(isle_game_attach_model(game_, instance.object, init.data(), init.size(), &instance.model));
            set_photo_pose(i, 0, {0, 0, 0}, 0);
        }
    } catch (...) {
        for (auto& instance : instances_)
            if (instance.object) static_cast<void>(isle_game_delete_object(game_, instance.object));
        throw;
    }
}
PidgeyProducer::~PidgeyProducer() {
    for (auto& instance : instances_)
        if (instance.object) static_cast<void>(isle_game_delete_object(game_, instance.object));
}
PidgeyProducer::Instance& PidgeyProducer::instance_at(std::size_t instance) {
    require(instance < capacity_, "Pidgey instance index outside constructed capacity");
    return instances_[instance];
}
const PidgeyProducer::Instance& PidgeyProducer::instance_at(std::size_t instance) const {
    require(instance < capacity_, "Pidgey instance index outside constructed capacity");
    return instances_[instance];
}
std::span<const IsleGameModelTransform> PidgeyProducer::unbaked_pose(std::size_t instance) const {
    return instance_at(instance).unbaked;
}
std::span<const IsleGameModelNode> PidgeyProducer::model_nodes(std::size_t instance) const {
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    check(isle_game_model_nodes(game_, instance_at(instance).model, &nodes, &count));
    require(count == node_count, "Native Pidgey node count changed");
    return {nodes, count};
}
void PidgeyProducer::set_photo_pose(std::size_t index, float animation_time, IsleGameVec3 position, float yaw) {
    auto& instance = instance_at(index);
    require(std::isfinite(animation_time) && std::isfinite(position.x) && std::isfinite(position.y) &&
            std::isfinite(position.z) && std::isfinite(yaw), "Nonfinite Pidgey photo pose");
    if (instance.have_pose && animation_time == instance.animation_time &&
        equal(position, instance.position) && yaw == instance.yaw) return;
    const bool resample = !instance.have_pose || animation_time != instance.animation_time;
    std::array<IsleGameModelTransform, node_count> sampled_pose;
    std::array<std::uint32_t, node_count> sampled_flags;
    if (resample) {
        sampled_flags.fill(0);
        for (std::size_t i = 0; i < node_count; ++i) {
            sampled_pose[i] = initial(resources_.nodes()[i]);
            sample(resources_.pose0().model_streams[i], animation_time, sampled_pose[i], sampled_flags[i]);
        }
    }
    const auto& pose = resample ? sampled_pose : instance.unbaked;
    const auto& flags = resample ? sampled_flags : instance.flags;
    IsleGameModelTransform root{};
    check(isle_game_model_bake_photo_transform(&pose[0], &position, yaw,
                                              resources_.photo_selection().scale, &root));
    const bool previous_valid = instance.have_pose;
    // On any rejected native mutation, leave the pose cache invalid so the next
    // request resamples from immutable streams instead of treating a partial
    // native mutation as a committed pose.
    instance.have_pose = false;
    for (std::size_t i = 0; i < node_count; ++i) {
        const auto& transform = i == 0 ? root : pose[i];
        const auto& previous = i == 0 ? instance.baked_root : instance.unbaked[i];
        if (!previous_valid || !equal(previous, transform))
            check(isle_game_model_set_transform(game_, instance.model, i, &transform));
        if (!previous_valid || instance.flags[i] != flags[i])
            check(isle_game_model_set_flags(game_, instance.model, i, flags[i]));
    }
    if (resample) { instance.unbaked = sampled_pose; instance.flags = sampled_flags; }
    instance.baked_root = root;
    instance.animation_time = animation_time; instance.position = position;
    instance.yaw = yaw; instance.have_pose = true;
}
void PidgeyProducer::install_inputs(std::span<std::uint32_t> words, std::uint64_t epoch) {
    graphics::validate_cold_inputs(words, snapshot_, snapshot_source_, boot_address);
    require(words.data() != resident_memory_ || epoch != resident_epoch_, "Pidgey inputs already installed for this epoch");
    // Resource install validates all immutable spans before its first write.
    // The remaining shared boot/image writes have already been preflighted.
    resources_.install(std::as_writable_bytes(words));
    graphics::install_cold_inputs(words, snapshot_, boot_address);
    Memory memory(words);
    memory.copy(copy_list_address, snapshot_.canonical().subspan(copy_rom_begin, copy_rom_end - copy_rom_begin));
    resident_memory_ = words.data(); resident_epoch_ = epoch;
    resident_camera_revisions_.fill(0); resident_payloads_ = false;
    for (auto& instance : instances_) {
        instance.resident_revisions.fill(0);
        instance.resident_payloads = false;
    }
    preview_inputs_ = preview_direction_ = false;
}
PreparedPidgeyTask PidgeyProducer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch,
                                         std::size_t instance, PhotoTargets targets) {
    return prepare_impl(words, epoch, instance, targets, nullptr);
}
PreparedPidgeyTask PidgeyProducer::prepare_preview(std::span<std::uint32_t> words,
                                                std::uint64_t epoch, std::size_t instance,
                                                PhotoTargets targets,
                                                const photo::LightingInputs& lighting) {
    return prepare_impl(words, epoch, instance, targets, &lighting);
}
std::uint16_t PidgeyProducer::upload_payloads(std::span<std::uint32_t> words,
                                            std::span<const std::size_t> indices,
                                            const photo::LightingInputs* lighting) {
    Memory memory(words);
    // Force all native fixed-matrix range checks before canonical writes.
    std::array<const IsleGameCameraMatrix*, 2> camera{};
    for (std::size_t i = 0; i < camera.size(); ++i)
        check(isle_game_camera_matrix(game_, camera_, i, &camera[i]));
    std::uint16_t norm = 0;
    check(isle_game_camera_perspective_normalization(game_, camera_, &norm));
    std::array<std::span<const IsleGameModelNode>, max_instances> models{};
    for (const auto index : indices) {
        require(instance_at(index).have_pose, "No complete Pidgey photo pose");
        models[index] = model_nodes(index);
    }
    for (std::size_t i = 0; i < camera.size(); ++i) {
        if (!resident_payloads_ || resident_camera_revisions_[i] != camera[i]->revision) {
            memory.matrix(matrices_address + static_cast<std::uint32_t>(i * 64), camera[i]->fixed);
            resident_camera_revisions_[i] = camera[i]->revision;
        }
    }
    for (const auto index : indices) {
        auto& instance = instance_at(index);
        const auto nodes = models[index];
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (!instance.resident_payloads || instance.resident_revisions[i] != nodes[i].revision) {
                memory.matrix(matrices_for(index) + static_cast<std::uint32_t>(i * 64), nodes[i].fixed);
                instance.resident_revisions[i] = nodes[i].revision;
            }
        }
        instance.resident_payloads = true;
    }
    const IsleGameCameraParameters* parameters = nullptr;
    check(isle_game_camera_parameters(game_, camera_, &parameters));
    if (lighting) {
        if (!preview_inputs_) {
            lighting->install(std::as_writable_bytes(words));
            preview_inputs_ = true;
        }
        const auto& view = parameters->look_at;
        const std::array<float, 4> camera_xz{view.eye[0], view.eye[2], view.at[0], view.at[2]};
        if (!preview_direction_ || preview_camera_xz_ != camera_xz) {
            lighting->update_direction(std::as_writable_bytes(words),
                                       {view.eye[0], 0, view.eye[2]}, {view.at[0], 0, view.at[2]});
            preview_camera_xz_ = camera_xz;
            preview_direction_ = true;
        }
    }
    if (!resident_payloads_ || !equal(resident_viewport_, parameters->viewport)) {
        for (unsigned i = 0; i < 4; ++i) {
            memory.half(viewport_address + i * 2, static_cast<std::uint16_t>(parameters->viewport.scale[i]));
            memory.half(viewport_address + 8 + i * 2, static_cast<std::uint16_t>(parameters->viewport.translate[i]));
        }
        resident_viewport_ = parameters->viewport;
    }
    if (!resident_payloads_) {
        // Actual split-half fixed identity, independent of native SDK layout.
        IsleGameFixedMatrix identity{};
        identity.m[0][0] = 0x00010000; identity.m[0][2] = 1;
        identity.m[1][1] = 0x00010000; identity.m[1][3] = 1;
        memory.matrix(identity_address, identity);
        const auto& material = resources_.material();
        require(material.flags == 0xa1 && material.image_guests[0] == 0x8019f820 &&
                material.format == 2 && material.palette_bit_depth == 2 &&
                material.width_main == 32 && material.height_main == 32 && material.scale == 32 &&
                material.scale_s == 1 && material.scale_t == 1 && material.unk10 == 0 &&
                material.unk14 == 0 && material.unk18 == 0 && material.unk0a == 0,
                "Pidgey A1 material differs from admitted five-command object");
        const std::array<Command, 5> body{{
            {0xde010000, material_address + 8},
            {0xfd500000, material.image_guests[0]}, // CI, 16-bit loading, width 1.
            {0xf2000000, (124u << 12) | 124u},
            {0xd7000002, 0xffffffff},
            {0xdf000000, 0}
        }};
        for (std::size_t i = 0; i < body.size(); ++i) {
            memory.word(material_address + static_cast<std::uint32_t>(i * 8), body[i].w0);
            memory.word(material_address + static_cast<std::uint32_t>(i * 8 + 4), body[i].w1);
        }
    }
    resident_payloads_ = true;
    return norm;
}
PreparedPidgeyTask PidgeyProducer::prepare_impl(std::span<std::uint32_t> words,
                                             std::uint64_t epoch, std::size_t instance,
                                             PhotoTargets targets,
                                             const photo::LightingInputs* lighting) {
    Memory memory(words);
    require(words.data() == resident_memory_ && epoch == resident_epoch_,
            "Pidgey canonical inputs are not resident at pointer+epoch");
    instance_at(instance);
    validate_targets(targets, resources_);
    const std::array indices{instance};
    const auto norm = upload_payloads(words, indices, lighting);
    List model(memory, list_for(instance), list_for(instance) + model_list_bytes);
    model.model(model_nodes(instance), resources_, matrices_for(instance));
    List list(memory);
    emit_setup(list, targets, norm, 1, lighting);
    list.call(list_for(instance));
    list.command(0xe9000000, 0); list.command(0xdf000000, 0);
    return finish_task(words);
}
PreparedPidgeyTask PidgeyProducer::finish_task(std::span<std::uint32_t> words) {
    const auto prepared = graphics::finish_task(words, snapshot_, boot_address, stack_address,
                                                yield_address, task_address, output_address,
                                                output_end, commands_address);
    return {prepared.task, prepared.boot};
}
FrozenFeedbackRequest PidgeyProducer::freeze_feedback_request(const FeedbackRequest& input) {
    require(input.actors.size() <= capacity_, "Feedback actor count exceeds constructed capacity");
    require((input.planes_base & 63u) == 0 &&
            input.planes_base <= rdram_size - photo_feedback::buffer_bytes,
            "Feedback whole-plane block is unaligned/outside 8 MiB");
    validate_range({input.planes_base,
                    input.planes_base + static_cast<std::uint32_t>(photo_feedback::buffer_bytes)}, resources_);
    FrozenFeedbackRequest request;
    request.owner_ = this;
    request.count_ = input.actors.size();
    request.planes_base_ = input.planes_base;
    std::copy(input.actors.begin(), input.actors.end(), request.actors_.begin());
    std::array<bool, max_instances> used_instances{}, used_slots{};
    for (std::size_t i = 0; i < request.count_; ++i) {
        const auto& actor = request.actors_[i];
        require(actor.instance < capacity_ && !used_instances[actor.instance],
                "Feedback instance is duplicate/outside capacity");
        used_instances[actor.instance] = true;
        if (actor.score_slot) {
            require(*actor.score_slot < max_instances && !used_slots[*actor.score_slot],
                    "Feedback score slot is duplicate/outside 0..11");
            used_slots[*actor.score_slot] = true;
        }
        require(std::isfinite(actor.animation_time) && std::isfinite(actor.position.x) &&
                std::isfinite(actor.position.y) && std::isfinite(actor.position.z) &&
                std::isfinite(actor.yaw), "Nonfinite feedback actor pose");
    }
    CameraRestore restore(game_, camera_);
    request.camera_ = restore.saved();
    request.camera_.look_at = input.look_at;
    require(input.look_at.up[0] == 0 && input.look_at.up[1] == 1 && input.look_at.up[2] == 0,
            "Feedback photograph requires GAME world-up (0,1,0)");
    request.camera_.perspective = {30.0f, 4.0f / 3.0f, 10.0f, 25600.0f, 1.0f};
    // score_Render changes only XY. Preserve source depth/reserved lanes.
    request.camera_.viewport.scale[0] = request.camera_.viewport.translate[0] = 128;
    request.camera_.viewport.scale[1] = request.camera_.viewport.translate[1] = 96;
    set_camera(game_, camera_, request.camera_, FeedbackPass::wide);
    set_camera(game_, camera_, request.camera_, FeedbackPass::narrow);
    for (std::size_t i = 0; i < request.count_; ++i) {
        const auto& actor = request.actors_[i];
        set_photo_pose(actor.instance, actor.animation_time, actor.position, actor.yaw);
        static_cast<void>(model_nodes(actor.instance));
    }
    return request;
}
PreparedPidgeyTask PidgeyProducer::prepare_feedback_pass(std::span<std::uint32_t> words,
                                                        std::uint64_t epoch,
                                                        const FrozenFeedbackRequest& request,
                                                        FeedbackPass pass) {
    Memory memory(words); // Extent guard before any canonical write.
    require(request.owner_ == this, "Frozen feedback request belongs to another scene owner");
    require(words.data() == resident_memory_ && epoch == resident_epoch_,
            "Pidgey canonical inputs are not resident at pointer+epoch");
    CameraRestore restore(game_, camera_);
    set_camera(game_, camera_, request.camera_, pass);
    std::array<std::size_t, max_instances> indices{};
    std::array<bool, max_instances> active_slots{};
    for (std::size_t i = 0; i < request.count_; ++i) {
        const auto& actor = request.actors_[i];
        indices[i] = actor.instance;
        if (actor.score_slot) active_slots[*actor.score_slot] = true;
        set_photo_pose(actor.instance, actor.animation_time, actor.position, actor.yaw);
        static_cast<void>(model_nodes(actor.instance));
    }
    const auto norm = upload_payloads(words, {indices.data(), request.count_}, nullptr);
    for (std::size_t slot = 0; slot < max_instances; ++slot) {
        if (!active_slots[slot]) {
            const auto begin = request.planes_base_ + (14 + slot) * photo_feedback::plane_bytes;
            std::fill_n(words.begin() + static_cast<std::ptrdiff_t>(begin / 4),
                        photo_feedback::plane_bytes / 4, 0u);
        }
    }
    for (std::size_t i = 0; i < request.count_; ++i) {
        const auto instance = indices[i];
        List model(memory, list_for(instance), list_for(instance) + model_list_bytes);
        model.model(model_nodes(instance), resources_, matrices_for(instance));
    }
    List list(memory);
    const auto base = request.planes_base_;
    emit_setup(list, {base, base + static_cast<std::uint32_t>(photo_feedback::plane_bytes), 64, 48},
               norm, 13, nullptr);
    for (std::size_t i = 0; i < request.count_; ++i) {
        const auto& actor = request.actors_[i];
        if (actor.score_slot) {
            const auto slot = std::uint32_t(*actor.score_slot);
            list.segment(10, base + (2 + slot) * static_cast<std::uint32_t>(photo_feedback::plane_bytes));
            list.segment(11, base + (14 + slot) * static_cast<std::uint32_t>(photo_feedback::plane_bytes));
            list.call(copy_list_address); // Prefix C -> B; restores F color, A depth.
            list.call(list_for(actor.instance)); // Genuine isolated draw, also writes F.
            list.command(0xe7000000, 0);
            list.command(0xfe000000, 0x0c000000);
        }
        list.call(list_for(actor.instance)); // Same body, cumulative C (unscored: only draw).
    }
    list.command(0xe9000000, 0); list.command(0xdf000000, 0);
    return finish_task(words);
}
} // namespace isle::pidgey
