#include "item_resources.hpp"
#include "model_data_decoder.hpp"

#include <stdexcept>
#include <utility>

namespace isle::items {
namespace {
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
constexpr std::array<model_assets::Mapping,3> mappings{{
    {0x800e18a0,0x5f050,0x14160},
    {0x800f5d90,0x731b0,0xa200},
    {0x800fff90,0x7d3b0,0x1b0c0},
}};
// Only the initialized source-consumed image domain is admitted. Ball's
// STEP stream reaches0..7 and apple's0..5; no NULL-table scan/array ABI guess.
constexpr std::array<model_assets::MaterialExtent,4> material_extents{{
    {0x800e8ee0,0x800e8ec0,8,8},
    {0x800eac78,0x800eac60,6,6},
    {0x800eb520,0,0,0},
    {0x800edba0,0,0,0},
}};
struct SourceFact {
    Family family;
    std::uint32_t tree, materials, model_animations, material_animations;
    std::size_t nodes;
    menu::RenderType render;
    float model_speed, texture_speed;
};
constexpr std::array<SourceFact,4> facts{{
    {Family::pester_ball,0x800e9138,0x800e8eb8,0,0x800e91c0,2,menu::RenderType::b_fogged,1.0f,0.4f},
    {Family::apple,0x800eaed0,0x800eac58,0,0x800eaf60,2,menu::RenderType::b_fogged,1.0f,0.4f},
    {Family::water_splash,0x800eb430,0x800eb510,0x800eafb0,0x800eb0c0,4,menu::RenderType::d_fogged,0.9f,0.9f},
    {Family::lava_splash,0x800edab0,0x800edb90,0x800ed5b0,0x800ed6b0,4,menu::RenderType::d_fogged,0.9f,0.9f},
}};
void require(bool condition,const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
Resources::Resources(const rom::Snapshot& snapshot,std::array<std::uint8_t,3> fog_rgb,
                     std::uint16_t fog_min,std::uint16_t fog_max)
    : snapshot_(&snapshot),source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Item resources require admitted US Snapshot");
    require(fog_min<fog_max,"Invalid explicit item fog profile");
    models_.reserve(facts.size()); materials_.reserve(material_extents.size()); animations_.reserve(11);
    const model_assets::Reader reader(snapshot,mappings);
    model_assets::DataDecoder decoder(reader,materials_,animations_,material_extents);
    for (const auto& fact:facts) {
        auto model=decoder.model(fact.tree,fact.materials,fact.nodes,fact.render);
        model.animation_speed=fact.model_speed;
        model.matrix_kinds={0x1c,0,0}; // Own anim_func80010230 source RTS recipe.
        model.fog_rgb=fog_rgb; model.fog_min=fog_min; model.fog_max=fog_max;
        // items.c never installs fnAnimCallback; even water's root opcode16
        // must not acquire the Pokemon callback/FX policy of the shared model.
        decoder.bind_initial_animations(model,fact.model_animations,fact.material_animations,false);
        definitions_[models_.size()]={fact.family,models_.size(),fact.texture_speed,{0.1f,0.1f,0.1f}};
        models_.push_back(std::move(model));
    }
    std::array<model_assets::ModelPlan,4> plans{};
    for (std::size_t i=0;i<plans.size();++i) plans[i]={&models_[i],{}};
    closure_.emplace(reader,plans,materials_);
    closure_->require_complete();
}
void Resources::require_live() const {
    const auto source=snapshot_->canonical();
    require(source.size()==rom::us_size && source.data()==source_data_,"Item resources borrow moved-from Snapshot");
}
std::span<const Definition> Resources::definitions() const { require_live();return definitions_; }
const Definition& Resources::definition(Family family) const {
    require_live();
    const auto index=static_cast<std::size_t>(family);
    if (index>=definitions_.size() || definitions_[index].family!=family)
        throw std::out_of_range("Unknown admitted item family");
    return definitions_[index];
}
std::span<const menu::Model> Resources::models() const { require_live();return models_; }
std::span<const menu::Material> Resources::materials() const { require_live();return materials_; }
std::span<const menu::Animation> Resources::animations() const { require_live();return animations_; }
std::span<const menu::DisplayList> Resources::display_lists() const { require_live();return closure_->display_lists(); }
std::span<const menu::VertexLoad> Resources::vertex_loads() const { require_live();return closure_->vertex_loads(); }
std::span<const menu::ImageLoad> Resources::image_loads() const { require_live();return closure_->image_loads(); }
std::span<const menu::DynamicMaterialCall> Resources::dynamic_material_calls() const { require_live();return closure_->dynamic_material_calls(); }
std::span<const rom::GuestView> Resources::installation_spans() const { require_live();return closure_->installation_spans(); }
} // namespace isle::items
