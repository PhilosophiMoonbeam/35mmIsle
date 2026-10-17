#include "player_cart_resources.hpp"
#include "model_data_decoder.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace isle::player {
namespace {
constexpr std::uint32_t ram_base=0x80000000;
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
// Pin splat.yaml:5318..5639: initialized DATA only, never app_level code/BSS.
constexpr std::array<model_assets::Mapping,2> mappings{{
    {0x80366ba0,0x506fb0,0x44620},
    {0x8011b050,0xa74e0,0x952a0}}};
// Exact initialized image-array extents; the repeated fourth LOD image is real,
// not a NULL terminator. Primitive-only Texture records have no image table.
constexpr std::array<model_assets::MaterialExtent,8> material_extents{{
    {0x8039da08,0,0,0},{0x8039da80,0,0,0},{0x8039daf8,0,0,0},
    {0x8039db70,0x8039d9e4,2,2},{0x8039dbe8,0x8039d9ec,4,4},
    {0x8039dc60,0,0,0},{0x8039dcd8,0x8039d9fc,2,2},{0x8039dd50,0,0,0}}};
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
CartResources::CartResources(const rom::Snapshot& snapshot, CartFogProfile fog)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Cart requires admitted US Snapshot");
    require(fog.min<fog.max,"Cart requires actual World fog profile");
    const model_assets::Reader reader(snapshot,mappings);
    model_assets::DataDecoder decoder(reader,materials_,animations_,material_extents,&paths_);
    models_.push_back(decoder.model(0x803aaa30,0x8039d938,43,menu::RenderType::j_fogged));
    auto& model=models_.front();
    model.matrix_kinds={28,0,0}; model.fog_rgb=fog.rgb; model.fog_min=fog.min; model.fog_max=fog.max;
    // No animation is implicitly selected; original Beach explicitly binds and
    // sets both model/material speed to0.5. Player's camera speed1 is owner policy.
    visibility_.assign(model.nodes.size(),std::uint8_t{1});
    auto clip=[&](CartClip& result, std::uint32_t model_table, std::uint32_t material_table,
                  std::uint32_t camera) {
        result.model_animation_table=reader.view(model_table,model.nodes.size()*4);
        result.material_animation_table=reader.view(material_table,model.nodes.size()*4);
        result.nodes.reserve(model.nodes.size()); result.model_bindings.reserve(model.nodes.size());
        for (std::size_t i=0; i<model.nodes.size(); ++i) {
            const auto& node=model.nodes[i]; CartNodeBinding binding;
            binding.model_animation=decoder.animation(result.model_animation_table.u32(i*4),menu::AnimationKind::model);
            result.model_bindings.push_back(binding.model_animation);
            if (binding.model_animation!=menu::no_index)
                for (const auto& command:animations_[binding.model_animation].commands)
                    if (command.opcode==15) visibility_[i]|=static_cast<std::uint8_t>(1u<<(command.mask&3u));
            if (const auto list=result.material_animation_table.u32(i*4)) {
                require(!node.materials.empty(),"Cart material stream without actual attachment");
                binding.material_animation_table=reader.view(list,node.materials.size()*4);
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    const auto stream=decoder.animation(binding.material_animation_table.u32(j*4),menu::AnimationKind::texture);
                    decoder.validate_material_animation(stream,node.materials[j]);
                    binding.material_animations.push_back(stream);
                }
            }
            if (binding.material_animations.empty())
                result.material_bindings.insert(result.material_bindings.end(),node.materials.size(),menu::no_index);
            else result.material_bindings.insert(result.material_bindings.end(),binding.material_animations.begin(),binding.material_animations.end());
            result.nodes.push_back(std::move(binding));
        }
        result.camera_identity=camera;
        result.camera_animation=decoder.animation(camera,menu::AnimationKind::camera,false);
    };
    clip(intro_,0x8013c580,0x8013cea0,0x8013da90);
    clip(outro_,0x8013b030,0x8013baa0,0x8013c530);
    require(materials_.size()==8 && animations_.size()==92 && paths_.size()==6,
            "Incomplete actual cart/Beach resource closure");
    const std::array<model_assets::ModelPlan,1> plans{{{&model,visibility_}}};
    closure_.emplace(reader,plans,materials_); closure_->require_complete();
}
void CartResources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,"Cart borrows moved-from Snapshot");
}
menu::animation::Resources CartResources::animation_resources() const {
    require_live(); return {models_,materials_,animations_,menu::no_index,paths_};
}
const CartClip& CartResources::intro() const { require_live(); return intro_; }
const CartClip& CartResources::outro() const { require_live(); return outro_; }
std::span<const std::uint8_t> CartResources::visibility_domains() const { require_live(); return visibility_; }
std::span<const menu::DisplayList> CartResources::display_lists() const { require_live(); return closure_->display_lists(); }
std::span<const menu::VertexLoad> CartResources::vertex_loads() const { require_live(); return closure_->vertex_loads(); }
std::span<const menu::ImageLoad> CartResources::image_loads() const { require_live(); return closure_->image_loads(); }
std::span<const menu::DynamicMaterialCall> CartResources::dynamic_material_calls() const { require_live(); return closure_->dynamic_material_calls(); }
std::span<const rom::GuestView> CartResources::installation_spans() const { require_live(); return closure_->installation_spans(); }
void CartResources::install(std::span<std::byte> ram, std::span<const menu::RamRange> reservations) const {
    require_live(); closure_->require_complete();
    if (ram.size()!=menu::rdram_size) throw std::invalid_argument("Cart install requires word-swapped8MiB RAM");
    const auto source=snapshot_->canonical(); const std::less<const std::byte*> less;
    if (less(ram.data(),source.data()+source.size()) && less(source.data(),ram.data()+ram.size()))
        throw std::invalid_argument("Cart install aliases Snapshot");
    for (const auto range:reservations)
        if (range.offset>ram.size() || range.size>ram.size()-range.offset)
            throw std::invalid_argument("Cart reservation escapes RAM");
    const auto spans=closure_->installation_spans();
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        require(span.guest_address>=ram_base && offset<=ram.size() && span.bytes.size()<=ram.size()-offset &&
                span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Invalid cart install provenance/extent");
        for (const auto range:reservations)
            if (range.size && offset<range.offset+range.size && range.offset<offset+span.bytes.size())
                throw std::invalid_argument("Cart resource overlaps actual runtime reservation union");
    }
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        for (std::size_t i=0; i<span.bytes.size(); ++i) ram[(offset+i)^3u]=span.bytes[i];
    }
}
} // namespace isle::player
