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
// Source pin df8815e75f3d79618466e01e0827c6165b93d11c; splat.yaml
// blob d2690bd3d2cb0aeac5c648db20666c5a42e5ba45:5219..5231,5318..5639.
// Initialized DATA only. Course overlays overlap VRAM and must never be united.
struct ClipSpec { std::uint32_t model, material, camera; };
constexpr std::array<ClipSpec,2> beach_clips{{
    {0x8013c580,0x8013cea0,0x8013da90},{0x8013b030,0x8013baa0,0x8013c530}}};
constexpr std::array<ClipSpec,3> tunnel_clips{{
    {0x8013d920,0x8013e3d0,0x8013ee60},{0x8013c230,0x8013ceb0,0x8013d8d0},
    {0x8013eee0,0x8013fdd0,0x801419e4}}};
constexpr std::array<ClipSpec,2> volcano_clips{{
    {0x801174e0,0x801182f0,0x80118d90},{0x80115e80,0x80116a80,0x8011749c}}};
constexpr std::array<ClipSpec,3> river_clips{{
    {0x8014a660,0x8014b450,0x8014bf30},{0x8014bfb0,0x8014cec0,0x8014ea10},
    {0x80148f30,0x80149be0,0x8014a600}}};
constexpr std::array<ClipSpec,2> cave_clips{{
    {0x80147540,0x80148420,0x80148ea0},{0x80145e30,0x80146ab0,0x801474ec}}};
constexpr std::array<ClipSpec,3> valley_clips{{
    {0x80120520,0x801212a0,0x80121d30},{0x8011c5a0,0x8011cfb0,0x8011da1c},
    {0x8011da60,0x8011ea20,0x801204a0}}};
constexpr std::array<std::uint32_t,1> valley_cameras{{0x8011c550}};
constexpr std::array<ClipSpec,2> rainbow_clips{{
    {0x80119ae0,0x8011a970,0x8011b3e0},{0x801183e0,0x80119050,0x80119a8c}}};
struct Catalog {
    std::int32_t course;
    std::array<model_assets::Mapping,2> mappings;
    std::span<const ClipSpec> clips;
    std::span<const std::uint32_t> cameras;
    std::size_t animations, paths;
};
// Only selected initialized DATA is admitted. New courses are closed by their
// actual Source-requested tables/streams/paths, not guessed inventory counts.
// Old Beach/Rainbow numerical inventory checks stay unchanged.
constexpr std::array<Catalog,7> catalogs{{
    {0, {{{0x80366ba0,0x506fb0,0x44620},{0x8011b050,0xa74e0,0x952a0}}},beach_clips,{},92,6},
    {1, {{{0x80366ba0,0x506fb0,0x44620},{0x8011cb60,0x162cc0,0x1d1d90-0x162cc0}}},tunnel_clips,{},menu::no_index,menu::no_index},
    {2, {{{0x80366ba0,0x506fb0,0x44620},{0x800fffa0,0x326c20,0x3d0560-0x326c20}}},volcano_clips,{},menu::no_index,menu::no_index},
    {3, {{{0x80366ba0,0x506fb0,0x44620},{0x8012a0d0,0x29a1a0,0x30af90-0x29a1a0}}},river_clips,{},menu::no_index,menu::no_index},
    {4, {{{0x80366ba0,0x506fb0,0x44620},{0x8012a0d0,0x1f5e80,0x27ab80-0x1f5e80}}},cave_clips,{},menu::no_index,menu::no_index},
    {5, {{{0x80366ba0,0x506fb0,0x44620},{0x800fffa0,0x3f63e0,0x47cf30-0x3f63e0}}},valley_clips,valley_cameras,menu::no_index,menu::no_index},
    {6, {{{0x80366ba0,0x506fb0,0x44620},{0x800f5d90,0x4a8160,0x43ea0}}},rainbow_clips,{},100,6}}};
const Catalog& catalog(std::int32_t course) {
    const auto found=std::find_if(catalogs.begin(),catalogs.end(),
        [course](const auto& entry) { return entry.course==course; });
    if (found==catalogs.end()) throw std::runtime_error("Cart course catalog unavailable");
    return *found;
}
// Exact initialized image-array extents; the repeated fourth LOD image is real,
// not a NULL terminator. Primitive-only Texture records have no image table.
constexpr std::array<model_assets::MaterialExtent,8> material_extents{{
    {0x8039da08,0,0,0},{0x8039da80,0,0,0},{0x8039daf8,0,0,0},
    {0x8039db70,0x8039d9e4,2,2},{0x8039dbe8,0x8039d9ec,4,4},
    {0x8039dc60,0,0,0},{0x8039dcd8,0x8039d9fc,2,2},{0x8039dd50,0,0,0}}};
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
CartResources::CartResources(const rom::Snapshot& snapshot, std::int32_t courseID, CartFogProfile fog)
    : course_id_(courseID), snapshot_(&snapshot), source_data_(snapshot.canonical().data()) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Cart requires admitted US Snapshot");
    require(fog.min<fog.max,"Cart requires actual World fog profile");
    const auto& selected=catalog(courseID);
    const model_assets::Reader reader(snapshot,selected.mappings);
    model_assets::DataDecoder decoder(reader,materials_,animations_,material_extents,&paths_);
    models_.push_back(decoder.model(0x803aaa30,0x8039d938,43,menu::RenderType::j_fogged));
    auto& model=models_.front();
    model.matrix_kinds={28,0,0}; model.fog_rgb=fog.rgb; model.fog_min=fog.min; model.fog_max=fog.max;
    // Common PlayerModel_Init, render_player.c:3..15 (blob
    // a87c007297022d577f629d5850a8ec76ac69a845), proves the same full
    // ZerOnePlayer tree/materials and JFogged matrix recipe for both courses.
    // No initial animation/speed policy is selected by this DATA resource.
    visibility_.assign(model.nodes.size(),std::uint8_t{1});
    require(selected.clips.size()>=2,"Cart requires actual intro/outro tables");
    clips_.resize(selected.clips.size());
    cameras_.reserve(selected.clips.size()+selected.cameras.size());
    auto camera_program=[&](std::uint32_t identity) {
        for (const auto& existing:cameras_) if (existing.identity==identity) return existing.animation;
        const auto animation=decoder.animation(identity,menu::AnimationKind::camera,false);
        require(identity && animation!=menu::no_index,"Cart camera identity must name a real program");
        cameras_.push_back({identity,animation});
        return animation;
    };
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
        result.camera_animation=camera_program(camera);
    };
    // Rainbow intro.c:4..6,107..110,155 (blob
    // 370dd2fb22d8bed3ccc898fc8dd0b0d4c40d9c9d); gate/code.c:7..10,148..157
    // (blob ee5d0b94feada6dcafa0df3a8b07c36baa5b10d1).
    // Identity-proven US ROM SHA256
    // a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc:
    // intro model/material/camera ROM 4CBEB0/4CCD40/4CD7B0;
    // outro model/material/camera ROM 4CA7B0/4CB420/4CBE5C.
    // Full 43-entry DFS tables (including NULL preservation slots), all
    // attachment streams and camera jump fragments yield 100 programs/6 paths.
    // Each Rainbow clip: 43 distinct model streams, 6 texture streams, one
    // camera program and 3 interpolation paths; no stream identity is shared
    // across these two clips. The common 43-node/8-material model is unchanged.
    // Rainbow path descriptors: 801184D8,80119A10,80119A74,80119BBC,
    // 8011B380,8011B3C8. Beach remains 92 programs/6 paths, 8 materials.
    for (std::size_t i=0;i<selected.clips.size();++i) {
        const auto& spec=selected.clips[i];
        clip(clips_[i],spec.model,spec.material,spec.camera);
        require(clips_[i].nodes.size()==43,"Incomplete actual Cart DFS table");
    }
    for (const auto identity:selected.cameras) camera_program(identity);
    require(model.nodes.size()==43 && materials_.size()==8,"Incomplete common Cart tree/material closure");
    if (selected.animations!=menu::no_index)
        require(animations_.size()==selected.animations && paths_.size()==selected.paths,
                "Incomplete actual course cart resource closure");
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
const CartClip& CartResources::intro() const { require_live(); return clips_[0]; }
const CartClip& CartResources::outro() const { require_live(); return clips_[1]; }
std::span<const CartClip> CartResources::clips() const { require_live(); return clips_; }
std::size_t CartResources::camera_animation(std::uint32_t identity) const {
    require_live();
    for (const auto& camera:cameras_) if (camera.identity==identity) return camera.animation;
    return menu::no_index;
}
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
