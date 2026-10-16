#include "beach_world_producer.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "GAME Beach transforms require strict binary32 arithmetic"
#endif

// GAME authority: df8815e75f3d79618466e01e0827c6165b93d11c, exact staged
// world/block.c:120-207,249-267,439-505,649-661; sys/render.c:838-853,
// 1086-1114,2106-2168,2355-2376; app_render/4CDB0.c:15-30,81-115;
// beach/55C110.c:79-84; app_level/player.c:2936-2975.
// Public numeric ABI: https://hack64.net/wiki/doku.php?id=f3dex2
// No SDK producer source, opaque layouts, callback emulation or R4300 execution.
namespace isle::beach {
namespace {
using graphics::Memory;
constexpr std::uint32_t identity_address = StartProducer::matrices_address + 128;
constexpr std::uint32_t terrain_matrix = StartProducer::matrices_address + 192;
constexpr std::uint32_t sky_matrix = StartProducer::matrices_address + 256;
constexpr std::uint32_t geometry_base = 0x00200405; // Z, shade, back cull, smooth.
constexpr std::uint32_t opaque_mode = 0x00552078, fog_mode = 0xc8112078;
struct Range { std::uint32_t begin, end; };
constexpr std::array<Range, 3> reserved{{
    {0, graphics::bss_end}, {StartProducer::commands_address, StartProducer::runtime_end},
    {StartProducer::output_address, StartProducer::output_end}
}};
bool overlap(Range a, Range b) { return a.begin < b.end && b.begin < a.end; }
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void check(IsleGameResult result) { require(result == ISLE_GAME_OK, "Native Beach camera/model operation failed"); }
bool equal(const IsleGameViewport& a, const IsleGameViewport& b) {
    for (unsigned i = 0; i < 4; ++i)
        if (a.scale[i] != b.scale[i] || a.translate[i] != b.translate[i]) return false;
    return true;
}
IsleGameModelTransform translated(IsleGameVec3 position) {
    return {position, {0, 0, 0}, {1, 1, 1}};
}
void targets_valid(FrameTargets t, const StartResources& resources) {
    require(t.width > 0 && t.width <= 1023 && !(t.width & 1u) && t.height > 0 && t.height <= 1023,
            "Invalid Beach RGBA16 target dimensions");
    const auto bytes = std::uint32_t(t.width) * t.height * 2;
    require(!(t.color & 63u) && !(t.depth & 63u) && t.color <= rdram_size - bytes &&
            t.depth <= rdram_size - bytes, "Beach frame target is unaligned/outside RDRAM");
    const std::array<Range, 2> planes{{{t.color, t.color + bytes}, {t.depth, t.depth + bytes}}};
    require(!overlap(planes[0], planes[1]), "Beach color and depth planes overlap");
    for (const auto plane : planes) {
        for (const auto region : reserved)
            require(!overlap(plane, region), "Beach target overlaps cold/runtime/FIFO owner");
        for (const auto& span : resources.installation_spans()) {
            const auto begin = span.guest_address - 0x80000000;
            require(!overlap(plane, {begin, begin + static_cast<std::uint32_t>(span.bytes.size())}),
                    "Beach target overlaps immutable source span");
        }
    }
}
std::uint32_t fog_factor(Fog f) {
    require(f.minimum < f.maximum && f.maximum <= 1000, "Invalid Beach fog interval");
    const auto difference = std::int32_t(f.maximum) - f.minimum;
    const auto multiplier = 128000 / difference;
    const auto offset = (500 - std::int32_t(f.minimum)) * 256 / difference;
    require(multiplier <= 32767 && offset >= -32768 && offset <= 32767,
            "Beach fog factors exceed public signed 16-bit wire range");
    return (std::uint32_t(multiplier) << 16) | (std::uint32_t(offset) & 0xffffu);
}
std::uint32_t rgba(std::array<std::uint8_t, 3> rgb) {
    return (std::uint32_t(rgb[0]) << 24) | (std::uint32_t(rgb[1]) << 16) | (std::uint32_t(rgb[2]) << 8);
}
std::uint32_t background(std::array<std::uint8_t, 3> rgb) {
    const auto color = (std::uint32_t(rgb[0] >> 3) << 11) | (std::uint32_t(rgb[1] >> 3) << 6) |
                       (std::uint32_t(rgb[2] >> 3) << 1) | 1u;
    return (color << 16) | color;
}
class List final {
public:
    explicit List(Memory& memory) : memory_(memory) {}
    void command(std::uint32_t a, std::uint32_t b) {
        // Current fixed sequence is below 100 commands for every layer choice;
        // this guard also fences future maintenance errors within its owner.
        require(next_ <= StartProducer::commands_end - 8, "Beach command capacity exceeded");
        memory_.word(next_, a); memory_.word(next_ + 4, b); next_ += 8;
    }
    void high(unsigned shift, unsigned bits, std::uint32_t value) {
        command(0xe3000000u | ((32 - shift - bits) << 8) | (bits - 1), value);
    }
    void low(unsigned shift, unsigned bits, std::uint32_t value) {
        command(0xe2000000u | ((32 - shift - bits) << 8) | (bits - 1), value);
    }
    void model(std::uint32_t matrix, std::uint32_t guest) {
        command(0xda380000, matrix); // Type A root: push, multiply, modelview.
        // Actual GAME graphics microcode combined-MVP recalculation interface.
        command(0xd5000001, 0);
        command(0xde000000, guest); // Original list/data pointers remain unchanged.
        command(0xd8380002, 64);
    }
    void fill(FrameTargets t, std::uint32_t target, std::uint32_t color) {
        command(0xe7000000, 0); high(20, 2, 3u << 20); low(3, 29, 0);
        command(0xff100000 | (t.width - 1), target);
        command(0xf7000000, color);
        command(0xed000000, (std::uint32_t(t.width) * 4 << 12) | (std::uint32_t(t.height) * 4));
        command(0xf6000000 | (std::uint32_t(t.width - 1) * 4 << 12) |
                (std::uint32_t(t.height - 1) * 4), 0);
    }
private:
    Memory& memory_;
    std::uint32_t next_ = StartProducer::commands_address;
};
static_assert(100 * 8 <= StartProducer::commands_end - StartProducer::commands_address);
static_assert(sky_matrix + 64 <= StartProducer::reflection_address);
static_assert(StartProducer::reflection_address + 32 <= StartProducer::viewport_address);
} // namespace

StartProducer::StartProducer(IsleGame* game, IsleGameCamera* camera,
                             const rom::Snapshot& snapshot, const StartResources& resources)
    : game_(game), camera_(camera), snapshot_(snapshot), resources_(resources),
      source_data_(snapshot.canonical().data()) {
    require(game && camera, "Beach producer requires live native game/camera owners");
    validate_owner();
    const auto& block = resources.start_block();
    const auto& sky = resources.sky();
    require(block.render_callback_guest == 0x800a1530 && block.display_list_guest == 0x8011cf00 &&
            block.materials_guest == 0 && block.texture_animation_guest == 0 &&
            sky.render_callback_guest == 0x800e1ca4 && sky.display_list_guest == 0x8011cc28 &&
            sky.materials_guest == 0 && sky.animation_guest == 0,
            "Beach producer input is not the admitted static Type-A block and sky");
    std::size_t count = 0;
    check(isle_game_camera_matrix_count(game, camera, &count));
    require(count == 2, "Beach main camera requires exactly two matrices");
    for (std::size_t i = 0; i < 2; ++i) {
        const IsleGameCameraMatrix* matrix = nullptr;
        check(isle_game_camera_matrix(game, camera, i, &matrix));
        require(matrix->kind == (i == 0 ? ISLE_GAME_CAMERA_PERSPECTIVE_FAST : ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL),
                "Beach main camera matrix order must be GAME kinds 3,14");
    }
    try {
        const IsleGameModelNodeInit initial{std::numeric_limits<std::uint32_t>::max(),
                                            translated({0, 0, 0}), ISLE_GAME_MODEL_TRANSLATE};
        for (std::size_t i = 0; i < 2; ++i) {
            check(isle_game_add_object(game, i == 0 ? 100 : 99, 9, 0x80000000, nullptr, nullptr, &objects_[i]));
            check(isle_game_attach_model(game, objects_[i], &initial, 1, &models_[i]));
        }
    } catch (...) {
        for (auto object : objects_) if (object) static_cast<void>(isle_game_delete_object(game, object));
        throw;
    }
}
StartProducer::~StartProducer() {
    for (auto object : objects_) if (object) static_cast<void>(isle_game_delete_object(game_, object));
}
void StartProducer::validate_owner() const {
    const auto canonical = snapshot_.canonical();
    require(canonical.size() == rom::us_size && canonical.data() == source_data_, "Moved Beach Snapshot owner");
    const auto& world = resources_.world().wire;
    require(world.rom_offset < canonical.size() && world.bytes.size() <= canonical.size() - world.rom_offset &&
            world.bytes.data() == canonical.data() + world.rom_offset, "Beach resources borrow another Snapshot owner");
    for (const auto& span : resources_.installation_spans()) {
        require(span.guest_address >= 0x80000000 && span.rom_offset <= canonical.size() &&
                span.bytes.size() <= canonical.size() - span.rom_offset &&
                span.bytes.data() == canonical.data() + span.rom_offset, "Invalid Beach source provenance");
        const auto begin = span.guest_address - 0x80000000;
        require(begin <= rdram_size && span.bytes.size() <= rdram_size - begin,
                "Beach immutable source escapes RDRAM");
        for (const auto region : reserved)
            require(!overlap({begin, begin + static_cast<std::uint32_t>(span.bytes.size())}, region),
                    "Beach immutable source overlaps cold/runtime/FIFO owner");
    }
}
FrameSettings StartProducer::source_settings() const noexcept {
    const auto& world = resources_.world();
    return {resources_.start_block().world_position, {world.fog_min, world.fog_max, world.fog_rgb},
            Layers::terrain_and_sky};
}
const IsleGameModelNode& StartProducer::node(std::size_t index) const {
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    check(isle_game_model_nodes(game_, models_[index], &nodes, &count));
    require(count == 1, "Beach native model root ownership changed");
    return nodes[0];
}
const IsleGameModelNode& StartProducer::terrain_node() const { return node(0); }
const IsleGameModelNode& StartProducer::sky_node() const { return node(1); }
void StartProducer::install_inputs(std::span<std::uint32_t> words, std::uint64_t epoch) {
    require(epoch != 0 && (words.data() != resident_memory_ || epoch != resident_epoch_),
            "Beach cold installation requires a new nonzero pointer/epoch");
    validate_owner();
    graphics::validate_cold_inputs(words, snapshot_, source_data_, boot_address);
    constexpr std::array<RamReservation, 1> cold{{{0, graphics::bss_end}}};
    // The resource transaction checks every source/reservation before any write.
    resources_.install(std::as_writable_bytes(words), cold);
    graphics::install_cold_inputs(words, snapshot_, boot_address);
    resident_memory_ = words.data(); resident_epoch_ = epoch;
    matrix_revisions_.fill(0); reflection_revision_ = 0; payloads_resident_ = false;
}
graphics::PreparedTask StartProducer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch,
                                             FrameTargets targets, const FrameSettings& input) {
    Memory memory(words);
    require(words.data() == resident_memory_ && epoch != 0 && epoch == resident_epoch_,
            "Beach canonical inputs are not resident at pointer/epoch");
    validate_owner();
    targets_valid(targets, resources_);
    const FrameSettings settings = input;
    const auto layers = static_cast<unsigned>(settings.layers);
    require(layers >= 1 && layers <= 3, "Invalid Beach layer selection");
    const auto factor = fog_factor(settings.fog);
    const auto& reference = settings.current_block_world_position;
    require(std::isfinite(reference.x) && std::isfinite(reference.y) && std::isfinite(reference.z),
            "Nonfinite Beach current-block world position");
    const IsleGameCameraParameters* parameters = nullptr;
    check(isle_game_camera_parameters(game_, camera_, &parameters));
    const auto& viewport = parameters->viewport;
    require(viewport.scale[0] > 0 && viewport.scale[1] > 0 && viewport.scale[2] == 511 &&
            viewport.translate[2] == 511 && viewport.scale[3] == 0 && viewport.translate[3] == 0 &&
            viewport.translate[0] >= viewport.scale[0] && viewport.translate[1] >= viewport.scale[1] &&
            viewport.translate[0] + viewport.scale[0] <= targets.width * 4 &&
            viewport.translate[1] + viewport.scale[1] <= targets.height * 4,
            "Beach camera viewport escapes target or source depth range");
    const auto world = resources_.start_block().world_position;
    const auto terrain = translated({(world.x - reference.x) * 100.0f,
                                    (world.y - reference.y) * 100.0f, (world.z - reference.z) * 100.0f});
    const auto& eye = parameters->look_at.eye;
    const auto sky = translated({eye[0], eye[1], eye[2]});
    check(isle_game_model_validate_transform(game_, models_[0], 0, &terrain));
    check(isle_game_model_validate_transform(game_, models_[1], 0, &sky));
    // GAME render.c divides the translation and scale separately BEFORE
    // addition/subtraction; quarter-pixel sums have different edge rounding.
    const auto left = viewport.translate[0] / 4 - viewport.scale[0] / 4;
    const auto top = viewport.translate[1] / 4 - viewport.scale[1] / 4;
    const auto right = viewport.translate[0] / 4 + viewport.scale[0] / 4;
    const auto bottom = viewport.translate[1] / 4 + viewport.scale[1] / 4;
    require(left < right && top < bottom, "Beach source-rounded scissor is empty");
    // No ordinary input/transform rejection remains. Batch computation keeps
    // every camera payload/revision unpublished if ANY matrix is invalid.
    check(isle_game_camera_compute_all(game_, camera_));
    std::array<const IsleGameCameraMatrix*, 2> camera{};
    for (std::size_t i = 0; i < camera.size(); ++i)
        check(isle_game_camera_matrix(game_, camera_, i, &camera[i]));
    const IsleGameCameraReflection* reflection = nullptr;
    check(isle_game_camera_reflection(game_, camera_, 1, &reflection));
    std::uint16_t norm = 0;
    check(isle_game_camera_perspective_normalization(game_, camera_, &norm));
    // Native records own source model transforms and stable revisions.
    // Both models explicitly select GAME hal_translate: no RTS/trig work.
    // Native validation/compute for BOTH models precedes ALL canonical writes.
    check(isle_game_model_set_transform(game_, models_[0], 0, &terrain));
    check(isle_game_model_set_transform(game_, models_[1], 0, &sky));
    const std::array<const IsleGameModelNode*, 2> nodes{&node(0), &node(1)};
    for (std::size_t i = 0; i < camera.size(); ++i) {
        if (!payloads_resident_ || matrix_revisions_[i] != camera[i]->revision) {
            memory.matrix(matrices_address + static_cast<std::uint32_t>(i * 64), camera[i]->fixed);
            matrix_revisions_[i] = camera[i]->revision;
        }
        if (!payloads_resident_ || matrix_revisions_[i + 2] != nodes[i]->revision) {
            memory.matrix(i == 0 ? terrain_matrix : sky_matrix, nodes[i]->fixed);
            matrix_revisions_[i + 2] = nodes[i]->revision;
        }
    }
    if (!payloads_resident_) {
        const IsleGameFixedMatrix identity{{{0x00010000, 0, 1, 0}, {0, 0x00010000, 0, 1},
                                            {0, 0, 0, 0}, {0, 0, 0, 0}}};
        memory.matrix(identity_address, identity);
    }
    if (!payloads_resident_ || reflection_revision_ != reflection->revision) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            const auto address = reflection_address + axis * 16;
            for (unsigned i = 0; i < 16; ++i) memory.byte(address + i, 0);
            for (unsigned c = 0; c < 3; ++c) {
                memory.byte(address + c, reflection->color[axis][c]);
                memory.byte(address + 4 + c, reflection->color_copy[axis][c]);
                memory.byte(address + 8 + c, static_cast<std::uint8_t>(reflection->direction[axis][c]));
            }
        }
        reflection_revision_ = reflection->revision;
    }
    if (!payloads_resident_ || !equal(resident_viewport_, viewport)) {
        for (unsigned i = 0; i < 4; ++i) {
            memory.half(viewport_address + i * 2, static_cast<std::uint16_t>(viewport.scale[i]));
            memory.half(viewport_address + 8 + i * 2, static_cast<std::uint16_t>(viewport.translate[i]));
        }
        resident_viewport_ = viewport;
    }
    payloads_resident_ = true;
    List list(memory);
    list.command(0xdb060000, 0); // Segment zero: original physical/KSEG0 operands.
    list.fill(targets, targets.depth, 0xfffcfffc);
    list.fill(targets, targets.color, background(resources_.world().background_rgb));
    list.command(0xe7000000, 0); list.command(0xfe000000, targets.depth);
    list.command(0xed000000 | (std::uint32_t(left * 4) << 12) | std::uint32_t(top * 4),
                 (std::uint32_t(right * 4) << 12) | std::uint32_t(bottom * 4));
    list.command(0xd9000000, geometry_base); // Explicit complete geometry state.
    list.command(0xdb040004, 1); list.command(0xdb04000c, 1);
    list.command(0xdb040014, 0xffff); list.command(0xdb04001c, 0xffff);
    list.command(0xd7000000, 0);
    list.high(23, 1, 0); list.high(16, 1, 0); list.high(14, 2, 0);
    list.high(17, 2, 0); list.high(19, 1, 1u << 19); list.high(12, 2, 2u << 12);
    list.high(9, 3, 6u << 9); list.high(8, 1, 0); list.high(6, 2, 0); list.high(4, 2, 0);
    list.low(0, 2, 0); list.low(2, 1, 0);
    list.command(0xdc080008, viewport_address);
    list.command(0xda380007, matrices_address); list.command(0xdb0e0000, norm);
    list.command(0xdc08000a, reflection_address); list.command(0xdc08030a, reflection_address + 16);
    list.command(0xda380005, matrices_address + 64); list.command(0xda380003, identity_address);
    if (layers & static_cast<unsigned>(Layers::sky)) {
        // Callback identity source-equivalent to GAME drawSkyBox1Cycle proven
        // by bounded own-GAME instruction observations; see scoped manifest.
        list.command(0xe7000000, 0); list.high(20, 2, 0); list.low(3, 29, 0x00552048);
        list.command(0xd9fefffe, 0);
        list.model(sky_matrix, resources_.sky().display_list_guest);
        list.command(0xe7000000, 0); list.command(0xd9ffffff, 0x00010001);
    }
    if (layers & static_cast<unsigned>(Layers::terrain)) {
        // Exact renderModelTypeAFogged -> Type A root -> original list -> restore.
        list.command(0xe7000000, 0); list.command(0xdb080000, factor);
        list.command(0xf8000000, rgba(settings.fog.rgb));
        list.high(20, 2, 1u << 20); list.low(3, 29, fog_mode);
        list.command(0xd9ffffff, 0x00010001);
        list.model(terrain_matrix, resources_.start_block().display_list_guest);
        list.command(0xe7000000, 0); list.high(20, 2, 0); list.low(3, 29, opaque_mode);
        list.command(0xd9feffff, 0);
    }
    list.command(0xe9000000, 0); list.command(0xdf000000, 0);
    return graphics::finish_task(words, snapshot_, boot_address, stack_address, yield_address,
                                 task_address, output_address, output_end, commands_address);
}
} // namespace isle::beach
