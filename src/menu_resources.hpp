#ifndef ISLE_MENU_RESOURCES_HPP
#define ISLE_MENU_RESOURCES_HPP

#include "scene_resources.hpp"
#include "rom_snapshot.hpp"
#include "rom_view.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace isle::menu {
inline constexpr std::uint32_t bank_guest = 0x802b5000;
inline constexpr std::size_t rdram_size = 8 * 1024 * 1024;
inline constexpr std::size_t no_index = static_cast<std::size_t>(-1);
// Decompressed views have no one-to-one ROM offset. Their owner and compressed
// input range, rather than GuestView::rom_offset, provide provenance.
inline constexpr std::size_t decompressed_rom_offset = static_cast<std::size_t>(-1);
enum class Scene { logo, title_menu };
enum class Error {
    none, invalid_snapshot, extent, malformed, callback, resource_failure,
    reservation, relocation_required, aliasing
};
struct Status {
    Error error = Error::none;
    std::uint32_t address = 0; // Failing guest descriptor/command; ROM offset for relative effect-bank data.
    explicit operator bool() const noexcept { return error == Error::none; }
};
// Numeric selectors, not function pointers, and never guest-to-host casts.
enum class Callback : std::uint32_t {
    update_all = 0x8000affc, draw_all = 0x8000ada0, controller = 0x80004860,
    logo_pre_render = 0x800e1b30, logo_init = 0x800e1b54,
    menu_pre_render = 0x800e4610, menu_init = 0x800e4634,
    title_effect_event = 0x800e48e0
};
Status callback_selector(std::uint32_t identity, Callback& output) noexcept;
struct SceneSetupFacts {
    rom::GuestView wire;
    Callback update{}, draw{}, pre_render{}, input{}, initialize{};
    std::uint16_t flags = 0, flags30 = 0;
    std::uint32_t heap_guest = 0, initialized_heap_size = 0, entry_heap_size = 0;
    std::uint32_t setup_count = 0, contexts = 0;
    std::array<std::uint32_t, 4> display_list_bytes{};
    std::uint32_t graphics_heap_bytes = 0, rdp_output_bytes = 0;
    std::uint32_t threads = 0, thread_stack_bytes = 0, stacks = 0, unk4c = 0;
    std::uint32_t processes = 0, objects = 0, matrices = 0;
    std::uint32_t animation_objects = 0, materials = 0, model_nodes = 0;
    std::uint32_t sprites = 0, cameras = 0;
    // Source GAME strides are provenance only. Native pools MUST use native
    // sizeof/alignment, including dynamically allocated overflow nodes.
    std::uint32_t source_object_stride = 0, source_model_stride = 0;
    std::uint32_t source_sprite_stride = 0, source_camera_stride = 0;
    std::uint32_t update_interval = 1, draw_interval = 1; // gtlInit defaults
    bool disable_near_clipping = true;
};
struct SceneSetupResult {
    Status status;
    SceneSetupFacts facts;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
// Decode an initialized 0x8c GAME record with its exact overlay mapping. The
// supplied borrowed bytes must remain live; native sizeof values are not read.
// Re-entry/restoration consumers can reject short/wrong-callback images before
// replacing any initialized native scene state.
SceneSetupResult inspect_setup(rom::GuestView wire, Scene scene) noexcept;
struct ScreenFacts {
    rom::GuestView wire;
    std::array<std::uint32_t, 3> framebuffer_guests{};
    std::uint32_t initialized_depth_guest = 0, width = 0, height = 0, flags = 0;
    std::size_t allocated_depth_bytes = 0; // Both scenes allocate depth storage post-init, not initialized data.
};
struct PreRenderFacts {
    rom::GuestView display_list, lights;
    std::array<std::uint8_t, 3> ambient{}, diffuse{};
    std::array<std::int8_t, 3> direction{};
};
struct MenuInitializedFacts {
    rom::GuestView wire; // Only source scalar configuration at A0F660..A0F68C.
    std::int8_t initial_variant = 0;
    std::array<std::int32_t, 4> pan{};
    std::array<std::array<std::int16_t, 4>, 3> y_positions{};
    std::uint32_t source_bss_guest = 0x800e82a0, source_bss_bytes = 0x100;
};
struct BankFacts {
    sprite::BankOwner owner;
    std::size_t rom_begin = 0, rom_end = 0, compressed_bytes_consumed = 0;
    std::uint32_t guest_begin = bank_guest;
};
struct Vec3 { float x, y, z; };
enum class RenderType : std::uint32_t {
    b = 0x80014f98,
    a_fogged = 0x800a1530,
    b_fogged = 0x800a15d8, d_fogged = 0x800a1608,
    i_fogged = 0x800a1650, j_fogged = 0x800a16b0
};
struct Payload {
    rom::GuestView wire;
    std::uint32_t display_list_id = 0;
    std::uint32_t pre_list = 0, draw_list = 0;
};
struct Node {
    rom::GuestView wire;
    std::uint32_t depth_flags = 0;
    std::size_t parent = no_index;
    Vec3 position{}, rotation{}, scale{};
    std::vector<Payload> payloads;
    std::vector<std::size_t> materials;
    std::size_t animation = no_index;
    std::vector<std::size_t> material_animations;
};
struct Model {
    rom::GuestView tree, material_table, animation_table, material_animation_table;
    RenderType render_type{};
    std::vector<Node> nodes;
    float animation_speed = 0.5f;
    std::array<std::uint8_t, 3> matrix_kinds{0x1c, 0, 0};
    std::array<std::uint8_t, 3> fog_rgb{120, 120, 150};
    std::uint16_t fog_min = 989, fog_max = 1000;
    bool receives_effect_callback = false;
};
// GAME Texture, not an SDK descriptor. All scalar fields remain available in
// wire; pointer tables are separately resolved to bounded borrowed views.
struct Material {
    rom::GuestView wire, image_table, palette_table;
    std::vector<std::uint32_t> images, palettes;
    std::uint8_t format = 0, size_code = 0, auxiliary_format = 0, auxiliary_size_code = 0;
    std::uint16_t flags = 0, scale = 0, image_origin = 0;
    std::uint16_t width = 0, height = 0, block_width = 0, block_height = 0;
    std::uint16_t auxiliary_width = 0, auxiliary_height = 0;
    std::array<float, 9> parameters{}; // 14,18,1c,20,24,28,3c,40,44
    std::array<std::uint32_t, 10> color_and_trailing_words{}; // 50..74
};
// GAME InterpData, decoded into native vectors; no console pointers/layout.
struct AnimationPath {
    rom::GuestView wire;
    std::int32_t kind = 0;
    float duration = 0, tension = 0;
    std::size_t segment_point_count = 0;
    std::vector<Vec3> points;
    std::vector<float> timestamps, speed_polynomials;
};
enum class AnimationKind { model, texture, camera };
struct AnimationCommand {
    rom::GuestView wire; // Exactly this command and its operands, not next data.
    std::uint8_t opcode = 0;
    std::uint16_t mask = 0, duration = 0;
    std::size_t path_index = no_index; // Model13 / camera13 eye's decoded path.
    std::size_t camera_at_path_index = no_index; // Camera13 bit7, independent of bit3.
};
struct Animation {
    rom::GuestView wire; // Exact reachable stream words; commands retain operand framing.
    AnimationKind kind{};
    bool callback_installed = false; // Source fnAnimCallback may be NULL even with opcode16.
    std::vector<AnimationCommand> commands;
};
struct DisplayList { rom::GuestView wire; };
struct VertexLoad {
    rom::GuestView wire;
    std::uint32_t command_identity;
    std::uint8_t first_slot, count;
};
enum class ImageKind { texture_block, texture_tile, palette };
struct ImageLoad {
    rom::GuestView wire;
    std::uint32_t command_identity;
    ImageKind kind;
    std::uint8_t format, source_size_code;
    std::uint16_t source_width;
};
struct DynamicMaterialCall {
    std::uint32_t command_identity, segmented_target;
    std::size_t material_index;
};
struct EffectInstruction {
    // branch_offset is informational: no_index also denotes multiple register
    // targets. Execution uses the source loop/return registers, never this hint.
    std::size_t offset = 0, size = 0, branch_offset = no_index;
    std::uint8_t opcode = 0;
};
struct EffectScript {
    rom::GuestView header, bytecode;
    std::uint16_t kind = 0, texture = 0, effect_lifetime = 0, particle_lifetime = 0;
    std::uint32_t flags = 0;
    std::array<float, 9> parameters{}; // gravity through size, 0c..2c
    std::vector<EffectInstruction> instructions;
};
struct EffectImage {
    rom::GuestView header;
    std::uint32_t format = 0, size_code = 0, width = 0, height = 0, flags = 0;
    std::vector<rom::GuestView> frames, palettes;
};
struct EffectBank {
    rom::GuestView script_table, image_table;
    std::vector<EffectScript> scripts;
    std::vector<EffectImage> images;
    std::uint32_t bank_id = 0;
};
struct RamRange { std::size_t offset, size; };
struct Collision { rom::GuestView source; RamRange reservation; };
struct InstallationPlan {
    Status status;
    std::vector<rom::GuestView> spans; // Only GBI/vertex/image users, never full bank/code overlay.
    std::vector<Collision> collisions;
    std::size_t bytes = 0;
};
struct HydrateResult;
// Owns immutable decompression and native descriptor storage. Snapshot storage
// must outlive Catalog and all borrowed views; moving it while borrowed rejects
// subsequent access. No per-update work, hidden RAM writes or callback execution.
class Catalog final {
public:
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;
    Catalog(Catalog&&) = delete;
    Catalog& operator=(Catalog&&) = delete;
    const SceneSetupFacts& setup(Scene scene) const;
    const ScreenFacts& screen(Scene scene) const;
    const PreRenderFacts& pre_render(Scene scene) const;
    const MenuInitializedFacts& menu_initialized() const;
    // Exact GAME report-photo mapping, not a save/report array bounds check.
    // Signed table values and special indices beyond a caller's capacity are
    // returned unchanged; the UI must apply its own storage-domain bounds.
    std::int32_t report_photo_index(std::int32_t sourcePokemonID) const;
    const BankFacts& bank(Scene scene) const;
    std::span<const scene::SpriteResource> sprites(Scene scene) const;
    const sprite::Sprite& sprite(Scene scene, std::uint32_t identity) const;
    std::span<const Model> models() const;
    std::span<const Material> materials() const;
    std::span<const Animation> animations() const;
    std::size_t camera_animation() const;
    const EffectBank& effects() const;
    std::span<const DisplayList> display_lists() const;
    std::span<const VertexLoad> vertex_loads() const;
    std::span<const ImageLoad> image_loads() const;
    std::span<const DynamicMaterialCall> dynamic_material_calls() const;
    // Fixed source framebuffers always reserved. Additional runtime/producer
    // ranges MUST be supplied. A collision requires real address relocation or
    // a new producer RAM layout; there is no overwrite/fake-success fallback.
    InstallationPlan plan_install(Scene scene, std::span<const RamRange> runtime_ranges) const noexcept;
    Status install(Scene scene, std::span<std::byte> word_swapped_rdram,
                   std::span<const RamRange> runtime_ranges) const noexcept;
private:
    explicit Catalog(const rom::Snapshot& snapshot);
    void require_live() const;
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    std::array<SceneSetupFacts, 2> setups_;
    std::array<ScreenFacts, 2> screens_;
    std::array<PreRenderFacts, 2> pre_render_;
    MenuInitializedFacts menu_initialized_;
    rom::GuestView report_index_table_;
    std::array<BankFacts, 2> banks_;
    std::array<std::vector<scene::SpriteResource>, 2> sprites_;
    std::vector<Model> models_;
    std::vector<Material> materials_;
    std::vector<Animation> animations_;
    std::size_t camera_animation_ = no_index;
    EffectBank effects_;
    std::vector<DisplayList> display_lists_;
    std::vector<VertexLoad> vertex_loads_;
    std::vector<ImageLoad> image_loads_;
    std::vector<DynamicMaterialCall> dynamic_calls_;
    std::vector<rom::GuestView> install_;
    std::vector<rom::GuestView> logo_install_;
    friend struct Builder;
    friend HydrateResult hydrate(const rom::Snapshot&) noexcept;
};
struct HydrateResult {
    Status status;
    std::unique_ptr<Catalog> catalog;
    explicit operator bool() const noexcept { return static_cast<bool>(status); }
};
HydrateResult hydrate(const rom::Snapshot& snapshot) noexcept;
HydrateResult hydrate(rom::Snapshot&&) = delete;
} // namespace isle::menu
#endif
