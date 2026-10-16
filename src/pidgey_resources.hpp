#ifndef ISLE_PIDGEY_RESOURCES_HPP
#define ISLE_PIDGEY_RESOURCES_HPP

#include "rom_snapshot.hpp"
#include "rom_view.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace isle::pidgey {
inline constexpr std::size_t node_count = 17;
inline constexpr std::size_t tree_record_count = 18; // Includes the depthFlags == 18 sentinel.
inline constexpr std::size_t payload_count = 15;
inline constexpr std::size_t display_list_count = 23;
inline constexpr std::size_t texture_count = 21;
inline constexpr std::size_t vertex_count = 602;
inline constexpr std::size_t installation_span_count = 42;
inline constexpr std::size_t rdram_size = 8 * 1024 * 1024;
inline constexpr std::uint8_t no_index = 0xff;


struct Vec3 { float x, y, z; };
struct Node {
    std::uint32_t depth_flags = 0;
    std::uint32_t payload_guest = 0;
    Vec3 position{}, rotation{}, scale{};
    std::uint8_t parent = no_index;
    std::uint8_t payload_index = no_index;
};
struct Payload {
    std::uint32_t guest_address = 0;
    std::uint32_t pre_guest = 0;
    std::uint32_t draw_guest = 0;
    std::uint8_t pre_index = no_index;
    std::uint8_t draw_index = no_index;
};
struct Command { std::uint32_t w0, w1; };
struct DisplayList {
    rom::GuestView wire;
    std::size_t command_count() const noexcept { return wire.bytes.size() / 8; }
    Command command(std::size_t index) const;
};
struct Vertex {
    std::array<std::int16_t, 3> position;
    std::uint16_t flag;
    std::array<std::int16_t, 2> st;
    std::array<std::uint8_t, 4> color_or_normal;
};
enum class TextureKind { rgba16_palette, ci4, ci8 };
struct TextureView {
    rom::GuestView wire;
    TextureKind kind;
    std::uint16_t width; // Palette entry count for rgba16_palette.
    std::uint16_t height;
};

// Native scalar view of GAME's Texture (include/sys/om.h:423-467), not an SDK
// structure. Retain all static fields, including the GAME's unnamed parameters.
struct Material {
    rom::GuestView wire;
    std::uint16_t pad00;
    std::uint8_t format, palette_bit_depth;
    std::uint32_t images_guest;
    std::array<std::uint32_t, 2> image_guests;
    std::array<std::uint8_t, 2> image_indices;
    std::uint16_t scale, unk0a, width_main, height_main;
    std::int32_t unk10;
    float unk14, unk18, scale_s, scale_t, unk24, unk28;
    std::uint32_t palettes_guest;
    std::uint16_t flags;
    std::uint8_t image_format, image_bit_depth;
    std::uint16_t block_width, block_height, width_aux, height_aux;
    float unk3c, unk40, unk44;
    std::array<std::uint8_t, 4> unk48;
    std::uint32_t unk4c;
    std::array<std::uint8_t, 4> prim_rgba, lod_fields, env_rgba, blend_rgba;
    std::array<std::uint8_t, 4> light_color1, light_color2;
    std::array<std::int32_t, 4> trailing;
};
struct PhotoSelection {
    rom::GuestView render_record; // Only the selected D_800ADBEC record, not the whole table.
    rom::GuestView animation_set_record; // Only the selected D_800AD4B8 record.
    std::uint32_t pokemon_id, model_guest, materials_guest, render_callback_guest;
    float scale;
    std::uint32_t pose_table_guest, pose_count;
};
struct Pose0 {
    rom::GuestView record; // First D_800AC414 record; poses 1-3 are deliberately not installed.
    rom::GuestView model_table;
    float pose_id;
    std::uint32_t material_animation_guest;
    // Index corresponds to the 17 live nodes. Null table entries have empty views.
    // Each nonempty stream includes its END word; these are not evaluated here.
    std::array<rom::GuestView, node_count> model_streams;
};
struct ExternalDisplayList {
    std::uint32_t command_guest;
    std::uint32_t target; // Segmented address, not a missing immutable ROM resource.
};

// Checked, immutable HD photo-Pidgey resources. Fixed-size native descriptors;
// no image/vertex/list copies, per-pass allocations, relocation, or renderer.
// The Snapshot AND its storage must outlive this object and every returned view:
// do not move the Snapshot while borrowed. Construction rejects moved-from input.
class HdResources final {
public:
    explicit HdResources(const rom::Snapshot& snapshot);
    HdResources(rom::Snapshot&&) = delete;
    HdResources(const HdResources&) = delete;
    HdResources& operator=(const HdResources&) = delete;
    HdResources(HdResources&&) = delete;
    HdResources& operator=(HdResources&&) = delete;

    const std::array<Node, node_count>& nodes() const noexcept { return nodes_; }
    const std::array<Payload, payload_count>& payloads() const noexcept { return payloads_; }
    const std::array<DisplayList, display_list_count>& display_lists() const noexcept { return lists_; }
    const std::array<TextureView, texture_count>& textures() const noexcept { return textures_; }
    const Material& material() const noexcept { return material_; }
    const std::array<std::uint32_t, node_count>& node_material_guests() const noexcept { return node_materials_; }
    const PhotoSelection& photo_selection() const noexcept { return selection_; }
    const Pose0& pose0() const noexcept { return pose0_; }
    const rom::GuestView& tree_wire() const noexcept { return tree_; }
    const rom::GuestView& vertex_wire() const noexcept { return vertices_; }
    Vertex vertex(std::size_t index) const;

    // hd_part7_draw calls segment 0x0E+0: its dynamic material list is the real
    // producer's responsibility. No fake terminator/list is installed for it.
    const ExternalDisplayList& external_material_list() const noexcept { return external_; }

    // Exact disjoint writes, useful for caller RAM reservations and guard checks.
    // Includes only selected lookup records, not other species/poses or padding
    // between texture objects. Nothing else in the caller's memory is reset.
    const std::array<rom::GuestView, installation_span_count>& installation_spans() const noexcept { return install_; }
    std::size_t installed_byte_count() const noexcept;
    // Requires exactly caller-owned 8 MiB in the backend's word-swapped layout.
    // Copies original bytes to (guest - 0x80000000) ^ 3, preserving all pointers.
    // Throws before writing for a wrong extent or moved-from Snapshot. Snapshot
    // destruction while borrowed violates the lifetime precondition above.
    void install(std::span<std::byte> word_swapped_rdram) const;

private:
    const rom::Snapshot* snapshot_;
    rom::GuestView tree_, vertices_;
    std::array<Node, node_count> nodes_{};
    std::array<Payload, payload_count> payloads_{};
    std::array<DisplayList, display_list_count> lists_{};
    std::array<TextureView, texture_count> textures_{};
    Material material_{};
    std::array<std::uint32_t, node_count> node_materials_{};
    PhotoSelection selection_{};
    Pose0 pose0_{};
    ExternalDisplayList external_{};
    std::array<rom::GuestView, installation_span_count> install_{};
};
} // namespace isle::pidgey
#endif
