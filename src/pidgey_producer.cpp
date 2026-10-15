#include "pidgey_producer.hpp"

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
constexpr std::uint32_t main_begin = 0x400, main_rom_begin = 0x1000;
constexpr std::uint32_t main_rom_end = 0x46270, bss_begin = 0x45670, bss_end = 0x9a8c0;
constexpr std::uint32_t graphics_text = 0x3e7a0, graphics_data = 0x44e60;
constexpr std::uint32_t boot_rom = 0xb70, boot_bytes = 0x100;
constexpr std::uint32_t identity_address = PidgeyProducer::matrices_address + 128;
constexpr std::uint32_t node_matrices = identity_address + 64;
constexpr std::uint32_t absent = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint32_t geometry_score = 0x00200405; // Z, shade, back cull, smooth.
constexpr std::uint32_t geometry_clear = 0x00bf0605; // score.c:100 incl. LOD/clipping.
constexpr std::uint32_t opaque_mode = 0x00552078;
constexpr std::uint32_t fog_mode = 0xc8112078;
static_assert(node_matrices + node_count * 64 == PidgeyProducer::material_address);
static_assert(main_begin + main_rom_end - main_rom_begin == bss_begin);

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
void sample(const RomView& stream, float animation_time,
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
constexpr std::array<Range, 9> reserved{{
    {main_begin, bss_end},
    {PidgeyProducer::commands_address, PidgeyProducer::material_address + 0x28},
    {PidgeyProducer::viewport_address, PidgeyProducer::viewport_address + 0x40},
    {PidgeyProducer::stack_address, PidgeyProducer::stack_address + 0x400},
    {PidgeyProducer::yield_address, PidgeyProducer::yield_address + 0xc00},
    {PidgeyProducer::task_address, PidgeyProducer::task_address + 64},
    {PidgeyProducer::boot_address, PidgeyProducer::boot_address + boot_bytes},
    {PidgeyProducer::output_address, PidgeyProducer::output_end},
    {0, main_begin}
}};
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
    for (const auto target : targets_ranges) {
        for (const auto region : reserved)
            require(!overlap(target, region), "Pidgey target overlaps code/runtime/FIFO reservation");
        for (const auto& resource : resources.installation_spans()) {
            const std::uint32_t begin = resource.guest_address - 0x80000000;
            require(!overlap(target, {begin, begin + static_cast<std::uint32_t>(resource.bytes.size())}),
                    "Pidgey target overlaps immutable resource");
        }
    }
}
class Memory {
public:
    explicit Memory(std::span<std::uint32_t> words) : words_(words) {
        require(words.size_bytes() == rdram_size, "Pidgey producer requires exactly 8 MiB");
    }
    void word(std::uint32_t address, std::uint32_t value) { words_[address / 4] = value; }
    void half(std::uint32_t address, std::uint16_t value) {
        auto& w = words_[address / 4];
        const auto shift = (address & 2u) ? 0u : 16u;
        w = (w & ~(0xffffu << shift)) | (std::uint32_t(value) << shift);
    }
    void copy(std::uint32_t address, std::span<const std::byte> bytes) {
        for (std::size_t i = 0; i < bytes.size(); i += 4)
            word(address + static_cast<std::uint32_t>(i),
                 std::to_integer<std::uint32_t>(bytes[i]) << 24 |
                 std::to_integer<std::uint32_t>(bytes[i + 1]) << 16 |
                 std::to_integer<std::uint32_t>(bytes[i + 2]) << 8 |
                 std::to_integer<std::uint32_t>(bytes[i + 3]));
    }
    void matrix(std::uint32_t address, const IsleGameFixedMatrix& m) {
        for (unsigned row = 0; row < 4; ++row)
            for (unsigned col = 0; col < 4; ++col) word(address + (row * 4 + col) * 4, m.m[row][col]);
    }
private:
    std::span<std::uint32_t> words_;
};
class List {
public:
    explicit List(Memory& memory) : memory_(memory) {}
    void command(std::uint32_t w0, std::uint32_t w1) {
        require(next_ <= PidgeyProducer::commands_end - 8, "Pidgey command capacity exceeded");
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
    void node(std::size_t index, std::span<const IsleGameModelNode> nodes, const HdResources& resources) {
        const auto& n = nodes[index];
        if (!(n.flags & 2u)) {
            const auto payload_index = resources.nodes()[index].payload_index;
            const Payload* payload = payload_index == no_index ? nullptr : &resources.payloads()[payload_index];
            if (payload && payload->pre_guest && !(n.flags & 1u)) call(payload->pre_guest);
            const bool push = n.parent == absent || n.next_sibling != absent;
            command(push ? 0xda380000 : 0xda380001,
                    node_matrices + static_cast<std::uint32_t>(index * 64));
            if (payload && payload->draw_guest && !(n.flags & 1u)) {
                if (resources.node_material_guests()[index]) segment(14, PidgeyProducer::material_address);
                call(payload->draw_guest);
            }
            for (auto child = n.first_child; child != absent; child = nodes[child].next_sibling)
                node(child, nodes, resources);
            if (push) command(0xd8380002, 64);
        }
    }
private:
    Memory& memory_;
    std::uint32_t next_ = PidgeyProducer::commands_address;
};
} // namespace

PidgeyProducer::PidgeyProducer(IsleGame* game, IsleGameCamera* camera,
                             const rom::Snapshot& snapshot, const HdResources& resources)
    : game_(game), camera_(camera), snapshot_(snapshot), resources_(resources) {
    require(game && camera && snapshot.canonical().size() == rom::us_size,
            "Pidgey producer requires live native and immutable input owners");
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
                   initial(resources.nodes()[i])};
    check(isle_game_add_object(game_, 128, 0, 0x80000000, nullptr, nullptr, &object_));
    try {
        check(isle_game_attach_model(game_, object_, init.data(), init.size(), &model_));
        set_photo_pose(0, {0, 0, 0}, 0);
    } catch (...) {
        static_cast<void>(isle_game_delete_object(game_, object_));
        object_ = nullptr;
        throw;
    }
}
PidgeyProducer::~PidgeyProducer() {
    if (object_) static_cast<void>(isle_game_delete_object(game_, object_));
}
std::span<const IsleGameModelNode> PidgeyProducer::model_nodes() const {
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    check(isle_game_model_nodes(game_, model_, &nodes, &count));
    require(count == node_count, "Native Pidgey node count changed");
    return {nodes, count};
}
void PidgeyProducer::set_photo_pose(float animation_time, IsleGameVec3 position, float yaw) {
    require(std::isfinite(animation_time) && std::isfinite(position.x) && std::isfinite(position.y) &&
            std::isfinite(position.z) && std::isfinite(yaw), "Nonfinite Pidgey photo pose");
    if (have_pose_ && animation_time == animation_time_ && equal(position, position_) && yaw == yaw_) return;
    const bool resample = !have_pose_ || animation_time != animation_time_;
    std::array<IsleGameModelTransform, node_count> sampled_pose;
    std::array<std::uint32_t, node_count> sampled_flags;
    if (resample) {
        sampled_flags.fill(0);
        for (std::size_t i = 0; i < node_count; ++i) {
            sampled_pose[i] = initial(resources_.nodes()[i]);
            sample(resources_.pose0().model_streams[i], animation_time, sampled_pose[i], sampled_flags[i]);
        }
    }
    const auto& pose = resample ? sampled_pose : unbaked_;
    const auto& flags = resample ? sampled_flags : flags_;
    IsleGameModelTransform root{};
    check(isle_game_model_bake_photo_transform(&pose[0], &position, yaw,
                                              resources_.photo_selection().scale, &root));
    const bool previous_valid = have_pose_;
    // On any rejected native mutation, leave the pose cache invalid so the next
    // request resamples from immutable streams instead of treating a partial
    // native mutation as a committed pose.
    have_pose_ = false;
    for (std::size_t i = 0; i < node_count; ++i) {
        const auto& transform = i == 0 ? root : pose[i];
        const auto& previous = i == 0 ? baked_root_ : unbaked_[i];
        if (!previous_valid || !equal(previous, transform))
            check(isle_game_model_set_transform(game_, model_, i, &transform));
        if (!previous_valid || flags_[i] != flags[i])
            check(isle_game_model_set_flags(game_, model_, i, flags[i]));
    }
    if (resample) { unbaked_ = sampled_pose; flags_ = sampled_flags; }
    baked_root_ = root;
    animation_time_ = animation_time; position_ = position; yaw_ = yaw; have_pose_ = true;
}
void PidgeyProducer::install_inputs(std::span<std::uint32_t> words, std::uint64_t epoch) {
    Memory memory(words);
    require(words.data() != resident_memory_ || epoch != resident_epoch_, "Pidgey inputs already installed for this epoch");
    const auto rom = snapshot_.canonical();
    require(rom.size() == rom::us_size, "Moved-from Pidgey ROM owner");
    memory.copy(main_begin, rom.subspan(main_rom_begin, main_rom_end - main_rom_begin));
    std::fill(words.begin() + bss_begin / 4, words.begin() + bss_end / 4, 0);
    memory.copy(boot_address, rom.subspan(boot_rom, boot_bytes));
    resources_.install(std::as_writable_bytes(words));
    resident_memory_ = words.data(); resident_epoch_ = epoch;
    resident_revisions_.fill(0); resident_payloads_ = false;
}
PreparedPidgeyTask PidgeyProducer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch, PhotoTargets targets) {
    Memory memory(words);
    require(words.data() == resident_memory_ && epoch == resident_epoch_, "Pidgey canonical inputs are not resident at pointer+epoch");
    require(have_pose_, "No complete Pidgey photo pose");
    validate_targets(targets, resources_);
    const auto nodes = model_nodes();
    std::array<const IsleGameCameraMatrix*, 2> camera{};
    for (std::size_t i = 0; i < camera.size(); ++i) {
        check(isle_game_camera_matrix(game_, camera_, i, &camera[i]));
        if (!resident_payloads_ || resident_revisions_[i] != camera[i]->revision) {
            memory.matrix(matrices_address + static_cast<std::uint32_t>(i * 64), camera[i]->fixed);
            resident_revisions_[i] = camera[i]->revision;
        }
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!resident_payloads_ || resident_revisions_[i + 2] != nodes[i].revision) {
            memory.matrix(node_matrices + static_cast<std::uint32_t>(i * 64), nodes[i].fixed);
            resident_revisions_[i + 2] = nodes[i].revision;
        }
    }
    const IsleGameCameraParameters* parameters = nullptr;
    check(isle_game_camera_parameters(game_, camera_, &parameters));
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
    std::uint16_t norm = 0;
    check(isle_game_camera_perspective_normalization(game_, camera_, &norm));
    List list(memory);
    list.segment(0, 0);
    list.segment(15, targets.color); list.segment(12, targets.depth);
    list.segment(10, targets.depth); list.segment(11, targets.depth);
    list.command(0xe7000000, 0);
    list.high(20, 2, 3u << 20); list.low(3, 29, 0); // FILL, NOOP blender.
    list.command(0xff100000 | (targets.width - 1), targets.depth);
    list.command(0xf7000000, 0xfffcfffc);
    list.command(0xed000000, (std::uint32_t(targets.width) * 4 << 12) | (std::uint32_t(targets.height) * 4));
    const auto rectangle = 0xf6000000 | (std::uint32_t(targets.width - 1) * 4 << 12) |
                           (std::uint32_t(targets.height - 1) * 4);
    list.command(rectangle, 0); list.command(0xe7000000, 0);
    list.command(0xff100000 | (targets.width - 1), targets.color);
    list.command(0xfe000000, targets.depth);
    list.command(0xf7000000, 0x00010001); list.command(rectangle, 0);
    list.command(0xe7000000, 0);
    list.command(0xd9000000 | (~geometry_clear & 0x00ffffff), 0);
    // Exact GAME FRUSTRATIO_1 wire order, observed in the source-selected
    // score setup list at US ROM 0x5a330..0x5a350. Reversing these signs
    // rejects every otherwise visible model triangle in the actual RSP.
    list.command(0xdb040004, 1); list.command(0xdb04000c, 1);
    list.command(0xdb040014, 0x0000ffff); list.command(0xdb04001c, 0x0000ffff);
    list.command(0xd7000000, 0);
    list.command(0xd9ffffff, geometry_score);
    list.command(0xda380007, identity_address); list.command(0xda380003, identity_address);
    list.high(23, 1, 1u << 23);
    // G_CC_SHADE in both cycles: (0-0)*0+SHADE, likewise alpha.
    list.command(0xfcffffff, 0xfffe793c);
    list.high(16, 1, 0); list.high(14, 2, 0); list.high(17, 2, 0);
    list.high(19, 1, 1u << 19); list.high(12, 2, 2u << 12);
    list.high(9, 3, 6u << 9); list.high(8, 1, 0);
    list.low(0, 2, 0); list.high(6, 2, 0); list.high(4, 2, 0);
    list.high(20, 2, 0); list.low(3, 29, opaque_mode); list.low(2, 1, 0);
    list.command(0xe7000000, 0);
    list.command(0xdc080008, viewport_address);
    // Original GAME camera placement: perspective LOAD, normalize, ordinary
    // look-at MUL into PROJECTION. MODELVIEW remains the identity loaded above.
    list.command(0xda380007, matrices_address);
    list.command(0xdb0e0000, norm);
    list.command(0xda380005, matrices_address + 64);
    // Genuine renderModelTypeIFogged wrapper, 968..990, not invented lighting.
    list.command(0xe7000000, 0);
    constexpr std::int32_t fog_multiplier = 128000 / (990 - 968);
    constexpr std::int32_t fog_offset = (500 - 968) * 256 / (990 - 968);
    list.command(0xdb080000, (std::uint32_t(fog_multiplier) << 16) | (std::uint32_t(fog_offset) & 0xffff));
    list.command(0xf8000000, 0x3c6eb400);
    list.high(20, 2, 1u << 20); list.low(3, 29, fog_mode);
    list.command(0xd9ffffff, 0x00010000);
    list.node(0, nodes, resources_);
    list.command(0xe7000000, 0);
    list.high(20, 2, 0); list.low(3, 29, opaque_mode);
    list.command(0xd9feffff, 0);
    list.command(0xe9000000, 0); list.command(0xdf000000, 0);
    // GAME gtl.c sets data_size (command size) to zero; DL termination controls
    // traversal. Loaded CPU main bytes remain inert; only graphics/boot execute.
    const rsp::Task task{1, 4, boot_address, boot_bytes, graphics_text, 0x1000,
                         graphics_data, 0x800, stack_address, 0x400,
                         output_address, output_end, commands_address, 0, yield_address, 0xc00};
    const std::array<std::uint32_t, 16> fields{
        task.type, task.flags, task.boot_address, task.boot_size, task.text_address, task.text_size,
        task.data_address, task.data_size, task.stack_address, task.stack_size, task.output_address,
        task.output_end, task.commands_address, task.commands_size, task.yield_address, task.yield_size};
    for (std::size_t i = 0; i < fields.size(); ++i)
        memory.word(task_address + static_cast<std::uint32_t>(i * 4), fields[i]);
    return {task, snapshot_.canonical().subspan(boot_rom, boot_bytes)};
}
} // namespace isle::pidgey
