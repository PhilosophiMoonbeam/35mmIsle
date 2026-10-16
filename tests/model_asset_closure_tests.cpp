#include "model_asset_closure.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace a = isle::model_assets;
namespace m = isle::menu;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F&& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::exception&) { rejected = true; }
    check(rejected, "Expected bounded closure rejection");
}
// All packets and bytes below are authored boundary inputs, not ROM fixtures.
struct Input {
    static constexpr std::uint32_t base = 0x80010000;
    std::vector<std::byte> bytes = std::vector<std::byte>(0x4000);
    std::array<a::Mapping, 1> maps{{{base, 0, 0x4000}}};
    a::Reader reader{bytes, maps};
    std::uint32_t address(std::size_t offset) const { return base + static_cast<std::uint32_t>(offset); }
    void word(std::size_t offset, std::uint32_t value) {
        for (std::size_t i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<std::byte>(value >> ((3 - i) * 8));
    }
    void packet(std::size_t offset, std::uint32_t w0, std::uint32_t w1) { word(offset, w0); word(offset + 4, w1); }
    void end(std::size_t offset) { packet(offset, 0xdf000000, 0); }
    void set(std::size_t offset, std::size_t image, unsigned format = 4, unsigned size = 2, unsigned width = 1) {
        packet(offset, 0xfd000000 | (format << 21) | (size << 19) | (width - 1), address(image));
    }
    void block(std::size_t offset, unsigned units = 2) { packet(offset, 0xf3000000, 0x07000000 | ((units - 1) << 12)); }
    void call(std::size_t offset, std::size_t target, bool push = true) { packet(offset, push ? 0xde000000 : 0xde010000, address(target)); }
    m::Model model(std::size_t root, unsigned lane = 0) {
        m::Model result{}; result.render_type = m::RenderType::d_fogged;
        m::Node n{}; n.payloads.push_back({{}, lane, 0, address(root)}); result.nodes.push_back(std::move(n));
        return result;
    }
    a::Closure close(const m::Model& model, std::span<const m::Material> materials = {},
                     std::span<const std::uint8_t> visibility = {}, a::Limits limits = {}) {
        const std::array plans{a::ModelPlan{&model, visibility}};
        return a::Closure(reader, plans, materials, {}, {}, limits);
    }
};
bool demand(const a::Closure& closure, std::uint32_t command, std::uint32_t source,
            std::size_t bytes, unsigned format, unsigned size, unsigned width) {
    return std::any_of(closure.image_loads().begin(), closure.image_loads().end(), [&](const auto& value) {
        return value.command_identity == command && value.wire.guest_address == source && value.wire.bytes.size() == bytes &&
            value.format == format && value.source_size_code == size && value.source_width == width;
    });
}
void cull_returns_full_tuple() {
    Input in;
    in.set(0, 0x1000, 4, 2, 1); in.call(8, 0x100); in.block(16); in.end(24);
    in.packet(0x100, 0x03000000, 14); in.set(0x108, 0x1100, 0, 1, 8); in.end(0x110);
    const auto closure = in.close(in.model(0)); closure.require_complete();
    check(closure.image_loads().size() == 2 && demand(closure, in.address(16), in.address(0x1000), 4, 4, 2, 1) &&
          demand(closure, in.address(16), in.address(0x1100), 2, 0, 1, 8), "CULL lost early-return tuple or invented independent field combinations");
}
void branch_is_nopush_and_returns_to_caller() {
    Input in;
    in.call(0, 0x100); in.block(8); in.end(16);
    in.packet(0x100, 0xe1000000, in.address(0x200));
    in.packet(0x108, 0x04005002, 0); // index1 encoded as 1*5 and 1*2.
    in.set(0x110, 0x1100); in.end(0x118);
    in.set(0x200, 0x1200, 0, 0); in.end(0x208);
    const auto closure = in.close(in.model(0)); closure.require_complete();
    check(closure.image_loads().size() == 2 && demand(closure, in.address(8), in.address(0x1100), 4, 4, 2, 1) &&
          demand(closure, in.address(8), in.address(0x1200), 1, 0, 0, 1), "Taken branch was treated as PUSH or discarded on caller return");
    in.packet(0x108, 0x04004002, 0);
    rejects([&] { in.close(in.model(0)); });
}
void latch_survives_callee_and_tail_transfer() {
    Input in;
    in.call(0, 0x100); in.packet(8, 0x04000000, 0); in.set(16, 0x1100); in.block(24); in.end(32);
    in.packet(0x100, 0xe1000000, in.address(0x200)); in.call(0x108, 0x180, false);
    in.end(0x180);
    in.set(0x200, 0x1200); in.block(0x208); in.end(0x210);
    const auto closure = in.close(in.model(0)); closure.require_complete();
    check(demand(closure, in.address(24), in.address(0x1100), 4, 4, 2, 1) &&
          demand(closure, in.address(0x208), in.address(0x1200), 4, 4, 2, 1), "E1 latch lost across call/tail return");
    in.packet(0x100, 0xe7000000, 0);
    const auto unknown = in.close(in.model(0));
    check(std::any_of(unknown.obligations().begin(), unknown.obligations().end(), [](const auto& o) { return o.parameter == a::Parameter::branch_target; }),
          "Unbound branch target falsely declared complete");
    rejects([&] { unknown.installation_spans(); });
}
void source_j_precedes_current_binding_and_null_retains() {
    Input in;
    in.packet(0, 0xde000000, 0x0e000000); in.block(8); in.end(16);
    in.packet(0x100, 0xde000000, 0x0e000000); in.block(0x108); in.end(0x110);
    in.packet(0x200, 0xde000000, 0x0e000000); in.block(0x208); in.end(0x210);
    std::array<m::Material, 2> materials{};
    for (unsigned i = 0; i < materials.size(); ++i) {
        materials[i].flags = 1; materials[i].format = 4; materials[i].size_code = static_cast<std::uint8_t>(i);
        materials[i].images.push_back(in.address(0x1000 + i * 0x100));
    }
    auto model = in.model(0); model.render_type = m::RenderType::j_fogged; model.nodes[0].materials = {0};
    m::Node second{}; second.materials = {1}; second.payloads.push_back({{}, 0, in.address(0x100), in.address(0x200)});
    model.nodes.push_back(std::move(second));
    m::Node third{}; third.payloads.push_back({{}, 0, 0, in.address(0x100)}); model.nodes.push_back(std::move(third));
    const auto closure = in.close(model, materials); closure.require_complete();
    check(demand(closure, in.address(0x108), in.address(0x1000), 1, 4, 0, 1) &&
          demand(closure, in.address(0x108), in.address(0x1100), 2, 4, 1, 1) &&
          demand(closure, in.address(0x208), in.address(0x1100), 2, 4, 1, 1),
          "J pre-list incorrectly used current binding, NULL attachment reset Segment14, or main size was converted to16");
    model.nodes.erase(model.nodes.begin());
    const auto unbound_pre = in.close(model, materials);
    check(std::any_of(unbound_pre.obligations().begin(), unbound_pre.obligations().end(), [](const auto& o) { return o.parameter == a::Parameter::segment14; }),
          "J pre-list fabricated its current material binding");
}
void local_dominator_and_visibility_obligations() {
    Input in;
    // Source1006 boundary: setter is in caller BEFORE the callee's cull.
    in.set(0, 0x1000); in.call(8, 0x100); in.end(16);
    in.packet(0x100, 0x03000000, 14); in.block(0x108, 192); in.end(0x110);
    in.packet(0x200, 0xde000000, 0x0e000000); in.block(0x208, 192); in.end(0x210);
    auto model = in.model(0, 1);
    m::Node sibling{}; sibling.materials = {0}; sibling.payloads.push_back({{}, 1, 0, in.address(0x200)});
    model.nodes.push_back(std::move(sibling));
    std::array<m::Material, 1> materials{}; materials[0].flags = 0x200;
    const auto closure = in.close(model, materials); closure.require_complete();
    check(demand(closure, in.address(0x208), in.address(0x1000), 384, 4, 2, 1), "Local dominating setter lost on CULL return or color-only material");
    const std::array<std::uint8_t, 2> omit_supplier{3, 1};
    const auto unresolved = in.close(model, materials, omit_supplier);
    check(std::any_of(unresolved.obligations().begin(), unresolved.obligations().end(), [&](const auto& o) {
        return o.parameter == a::Parameter::image && o.command_identity == in.address(0x208) && o.lane == 1;
    }), "Legitimate independently hidden supplier silently ignored");
    rejects([&] { unresolved.require_complete(); }); rejects([&] { unresolved.installation_spans(); });
    // Different lanes are not a justified adjacent producer/consumer relation.
    model.nodes[0].payloads[0].display_list_id = 0;
    const auto cross_lane = in.close(model, materials);
    check(!cross_lane.obligations().empty(), "Cross-lane state guessed without whole-task composition");
}
void material_auxiliary_rounding_and_nonimage_flags() {
    Input in;
    in.packet(0, 0xde000000, 0x0e000000); in.block(8); in.end(16);
    auto model = in.model(0); model.nodes[0].materials = {0};
    std::array<m::Material, 1> materials{};
    auto& material = materials[0];
    material.flags = 3; material.format = 4; material.size_code = 0;
    material.auxiliary_format = 4; material.auxiliary_size_code = 0;
    material.block_width = 1; material.block_height = 1;
    material.images = {in.address(0x1000), in.address(0x1100)};
    const auto closure = in.close(model, materials); closure.require_complete();
    check(closure.image_loads().size() == 4 &&
          demand(closure, in.address(0), in.address(0x1000), 2, 4, 2, 1) &&
          demand(closure, in.address(8), in.address(0x1000), 1, 4, 0, 1) &&
          demand(closure, in.address(8), in.address(0x1100), 1, 4, 0, 1),
          "Auxiliary loader rounding leaked into primary size or lost bounded image alternatives");
    // Source Snorlax/gate also have NULL-image descriptors with tile-only flags.
    in.set(0, 0x1200); in.packet(8, 0xde000000, 0x0e000000); in.block(16); in.end(24);
    material.flags = 0x20; material.images.clear();
    const auto retained = in.close(model, materials); retained.require_complete();
    check(demand(retained, in.address(16), in.address(0x1200), 4, 4, 2, 1),
          "Non-image material flags failed to preserve incoming tuple");
}
void subtree_hiding_does_not_hide_sibling() {
    Input in;
    in.set(0, 0x1000); in.end(8); in.block(0x100); in.end(0x108);
    auto model = in.model(0);
    m::Node child{}; child.parent = 0; child.payloads.push_back({{}, 0, 0, in.address(0x100)});
    model.nodes.push_back(std::move(child));
    const std::array<std::uint8_t, 2> subtree_hidden{4, 1};
    const auto hidden = in.close(model, {}, subtree_hidden); hidden.require_complete();
    check(hidden.image_loads().empty(), "flags2 failed to omit subtree");
    model.nodes[1].parent = m::no_index;
    const auto sibling = in.close(model, {}, subtree_hidden);
    check(!sibling.obligations().empty(), "flags2 incorrectly omitted sibling");
    model.nodes[1].parent = 0;
    const std::array<std::uint8_t, 2> payload_hidden{2, 1};
    const auto child_runs = in.close(model, {}, payload_hidden);
    check(!child_runs.obligations().empty(), "flags1 incorrectly omitted descendants");
}
void footprint_and_malformed_boundaries() {
    Input in;
    in.set(0, 0x1000, 0, 2, 1); in.packet(8, 0xf0000000, 0x0503c000); in.end(16);
    const auto palette = in.close(in.model(0)); palette.require_complete();
    check(demand(palette, in.address(8), in.address(0x1000), 32, 0, 2, 1), "TLUT tuple footprint wrong");
    in.set(0, 0x1000, 4, 1, 8); in.packet(8, 0xf4004004, 0x0700c008);
    const auto tile = in.close(in.model(0)); tile.require_complete();
    check(demand(tile, in.address(8), in.address(0x1000), 20, 4, 1, 8), "LOADTILE failed to retain source row-stride prefix");
    in.packet(8, 0xf4004004, 0x07030008); rejects([&] { in.close(in.model(0)); });
    in.packet(8, 0xdc08060a, in.address(0x1000)); rejects([&] { in.close(in.model(0)); });
    in.packet(8, 0x01001001, in.address(0x1000)); rejects([&] { in.close(in.model(0)); });
    in.call(8, 0); rejects([&] { in.close(in.model(0)); });
    in.end(8);
    a::Limits limits{}; limits.commands = 1; rejects([&] { in.close(in.model(0), {}, {}, limits); });
    limits = {}; limits.evaluations = 1; rejects([&] { in.close(in.model(0), {}, {}, limits); });
    in.packet(8, 0x03000000, 14); in.set(16, 0x1100); in.end(24);
    limits = {}; limits.alternatives = 1; rejects([&] { in.close(in.model(0), {}, {}, limits); });
    rejects([&] { in.reader.view(in.address(0x3ff8), 16, 8); });
    rejects([&] { in.reader.view(in.address(1), 8, 8); });
    const std::array<a::Mapping, 2> ambiguous{{{Input::base, 0, 32}, {Input::base + 16, 64, 32}}};
    rejects([&] { a::Reader invalid(in.bytes, ambiguous); });
    const std::array<a::Mapping, 1> overflow{{{0xfffffff8, 0, 32}}};
    rejects([&] { a::Reader invalid(in.bytes, overflow); });
    in.call(0, 0x100); in.end(8); in.end(0x100);
    limits = {}; limits.call_depth = 1; rejects([&] { in.close(in.model(0), {}, {}, limits); });
    in.block(0); in.block(8); in.end(16);
    limits = {}; limits.obligations = 1; rejects([&] { in.close(in.model(0), {}, {}, limits); });
    const std::array<a::Mapping, 2> rom_alias{{{Input::base, 0, 32}, {Input::base + 64, 16, 32}}};
    rejects([&] { a::Reader invalid(in.bytes, rom_alias); });
}
void complete_union_and_provenance_admission() {
    Input in; in.set(0, 0x1000); in.block(8); in.end(16);
    auto model = in.model(0);
    const std::array plans{a::ModelPlan{&model}};
    const std::array extras{in.reader.view(in.address(0x2000), 24, 8)};
    const a::Closure complete(in.reader, plans, {}, {}, extras);
    const auto spans = complete.installation_spans();
    check(std::any_of(spans.begin(), spans.end(), [&](const auto& span) {
        return span.guest_address == in.address(0x2000) && span.bytes.size() == 24;
    }), "Caller setup/light span lost from immutable union");
    auto wrong = extras; ++wrong[0].rom_offset;
    rejects([&] { a::Closure invalid(in.reader, plans, {}, {}, wrong); });
    in.packet(0, 0xe7000000, 0);
    const a::Closure partial(in.reader, plans, {}, {}, extras);
    check(!partial.obligations().empty() && !partial.display_lists().empty(), "Unbound load did not retain explicit partial diagnostics");
    rejects([&] { partial.installation_spans(); });
}
} // namespace
int main() {
    try {
        cull_returns_full_tuple(); branch_is_nopush_and_returns_to_caller(); latch_survives_callee_and_tail_transfer();
        source_j_precedes_current_binding_and_null_retains(); local_dominator_and_visibility_obligations();
        material_auxiliary_rounding_and_nonimage_flags();
        subtree_hiding_does_not_hide_sibling(); footprint_and_malformed_boundaries(); complete_union_and_provenance_admission();
        std::cout << "Shared model asset closure boundaries passed\n"; return EXIT_SUCCESS;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
}
