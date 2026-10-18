#include "actor_resources.hpp"
#include "actor_resource_catalog.hpp"
#include "source_matrix_configuration.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::actor {
namespace {
constexpr std::uint32_t ram_base = 0x80000000;
constexpr std::array<std::uint8_t, 20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf
};
void require(bool condition, const char* message, std::uint32_t address = 0) {
    if (!condition) throw std::runtime_error(std::string(message) + " at guest " + std::to_string(address));
}
float finite(const rom::GuestView& wire, std::size_t offset) {
    const auto value = wire.f32(offset);
    require(std::isfinite(value), "Nonfinite actor scalar", wire.guest_address);
    return value;
}
menu::Vec3 vector3(const rom::GuestView& wire, std::size_t offset) {
    return {finite(wire,offset),finite(wire,offset+4),finite(wire,offset+8)};
}
menu::RenderType render_type(std::uint32_t identity) {
    switch (identity) {
    case 0x80014f98: return menu::RenderType::b;
    case 0x800a15d8: case 0x803594dc: return menu::RenderType::b_fogged;
    case 0x800a1608: case 0x80359534: return menu::RenderType::d_fogged;
    case 0x8035942c: return menu::RenderType::i_fogged;
    case 0x80359484: return menu::RenderType::j_fogged;
    case 0x8035958c: return menu::RenderType::i_no_fog;
    // Pinned volcano/moltres/code.c:186-196 delegates the hierarchy to
    // renderPokemonModelTypeI; volcano_smoke/code.c:62-66 delegates to
    // renderModelTypeDFogged. Classify payload shape only: the raw callbacks
    // remain in ActorInitialization and neither custom wrapper executes here.
    case 0x802de26c: return menu::RenderType::i_no_fog;
    case 0x802dfae4: return menu::RenderType::d_fogged;
    default: throw std::runtime_error("Unknown actor render callback identity");
    }
}
void valid_reservation(menu::RamRange range) {
    if (range.offset > menu::rdram_size || range.size > menu::rdram_size-range.offset)
        throw std::invalid_argument("Actor reservation escapes 8 MiB RDRAM");
}
void disjoint(const rom::GuestView& wire, menu::RamRange range) {
    const auto offset = static_cast<std::size_t>(wire.guest_address-ram_base);
    if (range.size && offset < range.offset+range.size && range.offset < offset+wire.bytes.size())
        throw std::invalid_argument("Actor resource overlaps runtime reservation");
}
template<class T> const T& lookup(const std::vector<T>& values, std::uint32_t identity) {
    const auto found=std::find_if(values.begin(),values.end(),
        [identity](const T& value) { return value.wire.guest_address==identity; });
    if (found==values.end()) throw std::out_of_range("Unknown admitted actor resource identity");
    return *found;
}
} // namespace

// The existing ActorResourceBuilder, with course facts supplied by the closed
// catalog. Models, materials, programs, paths and GBI use the shared decoders.
struct ActorResourceBuilder {
    Resources& out;
    const model_assets::Reader& reader;
    FogProfile fog;
    model_assets::DataDecoder decoder{reader,out.materials_,out.animations_,out.catalog_->material_extents,&out.paths_};
    rom::GuestView at(std::uint32_t address, std::size_t bytes, std::size_t alignment = 4) const {
        return reader.view(address,bytes,alignment);
    }
    void model(const detail::InitializationSpec& fact) {
        ActorInitialization initialization{};
        initialization.wire=at(fact.identity,52); initialization.actor_id=fact.actor_id;
        const auto& w=initialization.wire;
        require(w.u32(0)==fact.model_identity && w.u32(4)==fact.material_table_identity &&
                w.u32(8)==fact.render_identity && w.u32(12)==fact.setup_identity,
                "Actor initialization differs from closed course catalog",fact.identity);
        initialization.render_callback_identity=w.u32(8);
        initialization.textures_present=w.u32(4)!=0;
        initialization.scale=vector3(w,16); initialization.collision_center=vector3(w,28);
        initialization.radius=finite(w,40); initialization.flags=w.u16(44);
        initialization.configured_matrix_kinds={w.u8(46),w.u8(47),w.u8(48)};
        require(initialization.scale.x!=0 && initialization.scale.y!=0 && initialization.scale.z!=0 &&
                initialization.radius>=0,"Invalid actor initialization scale/radius",fact.identity);
        auto& setup=initialization.setup; setup.wire=at(fact.setup_identity,20);
        setup.initial_state_identity=setup.wire.u32(4); setup.process_kind=std::bit_cast<std::int8_t>(setup.wire.u8(8));
        setup.unknown_0c_identity=setup.wire.u32(12); setup.transition_graph_identity=setup.wire.u32(16);
        require(setup.wire.u32(0)==fact.initial_header_identity &&
                setup.initial_state_identity==fact.initial_state_identity &&
                setup.process_kind==fact.process_kind && setup.unknown_0c_identity==fact.unknown_0c_identity &&
                setup.transition_graph_identity==fact.transition_graph_identity,
                "Actor setup differs from closed course catalog",fact.setup_identity);
        initialization.animations_enabled=setup.wire.u32(0)!=0;
        auto result=decoder.model(w.u32(0),w.u32(4),fact.nodes,render_type(initialization.render_callback_identity));
        result.animation_speed=0; // A NULL initial header starts disabled; Source may bind a reachable header later.
        result.matrix_kinds=menu::model::select_source_matrix_kinds(initialization.flags,initialization.configured_matrix_kinds);
        result.fog_rgb=fog.rgb; result.fog_min=fog.min; result.fog_max=fog.max;
        initialization.model=out.models_.size();
        out.models_.push_back(std::move(result)); out.initializations_.push_back(std::move(initialization));
        out.visibility_.emplace_back(fact.nodes,std::uint8_t{1});
    }
    void header(std::size_t initialization_index, std::uint32_t address) {
        auto& initialization=out.initializations_[initialization_index];
        auto& model=out.models_[initialization.model];
        ActorAnimationHeader header{}; header.wire=at(address,20);
        header.initialization=initialization_index; header.speed=finite(header.wire,0); header.unknown_04=finite(header.wire,4);
        if (const auto table=header.wire.u32(8)) header.model_animation_table=at(table,model.nodes.size()*4);
        if (const auto table=header.wire.u32(12)) header.material_animation_table=at(table,model.nodes.size()*4);
        header.nodes.reserve(model.nodes.size());
        header.model_bindings.reserve(model.nodes.size());
        if (!header.material_animation_table.bytes.empty()) {
            std::size_t attachments=0;
            for (const auto& node:model.nodes) attachments+=node.materials.size();
            header.material_bindings.reserve(attachments);
        }
        std::size_t sound_count=0;
        for (std::size_t i=0; i<model.nodes.size(); ++i) {
            const auto& node=model.nodes[i]; ActorNodeBinding binding{};
            if (!header.model_animation_table.bytes.empty())
                binding.model_animation=decoder.animation(header.model_animation_table.u32(i*4),menu::AnimationKind::model);
            if (binding.model_animation!=actor_no_index) for (const auto& command:out.animations_[binding.model_animation].commands) {
                if (command.opcode==15)
                    out.visibility_[initialization.model][i]|=static_cast<std::uint8_t>(1u<<(command.mask&3u));
                if (command.opcode==16 && node.parent==actor_no_index && (command.mask>>8)<=3) {
                    const auto argument=command.mask&255u;
                    if (argument) sound_count=std::max(sound_count,static_cast<std::size_t>(argument));
                }
            }
            // Source advances the outer array, but never dereferences a list
            // when this DObj has no attached MObjs (anim.c:160-171).
            if (!header.material_animation_table.bytes.empty() && !node.materials.empty()) if (const auto list=header.material_animation_table.u32(i*4)) {
                binding.material_animation_table=at(list,node.materials.size()*4);
                binding.material_animations.reserve(node.materials.size());
                for (std::size_t j=0; j<node.materials.size(); ++j) {
                    const auto stream=decoder.animation(binding.material_animation_table.u32(j*4),menu::AnimationKind::texture);
                    decoder.validate_material_animation(stream,node.materials[j]);
                    binding.material_animations.push_back(stream); // NULL preserves this slot, never terminates.
                }
            }
            header.model_bindings.push_back(binding.model_animation);
            if (!header.material_animation_table.bytes.empty()) {
                if (binding.material_animation_table.bytes.empty())
                    header.material_bindings.insert(header.material_bindings.end(),node.materials.size(),actor_no_index);
                else
                    header.material_bindings.insert(header.material_bindings.end(),
                                                    binding.material_animations.begin(),binding.material_animations.end());
            }
            header.nodes.push_back(std::move(binding));
        }
        if (const auto ids=header.wire.u32(16); ids && sound_count) {
            header.sound_id_table=at(ids,sound_count*4); header.sound_ids.reserve(sound_count);
            for (std::size_t i=0; i<sound_count; ++i) header.sound_ids.push_back(std::bit_cast<std::int32_t>(header.sound_id_table.u32(i*4)));
        }
        if (initialization.setup.wire.u32(0)==address) {
            require(initialization.setup.initial_header==actor_no_index,"Duplicate actor initial header",address);
            initialization.setup.initial_header=out.headers_.size();
            model.animation_speed=header.speed; model.animation_table=header.model_animation_table;
            model.material_animation_table=header.material_animation_table;
            for (std::size_t i=0; i<model.nodes.size(); ++i) {
                model.nodes[i].animation=header.nodes[i].model_animation;
                model.nodes[i].material_animations=header.nodes[i].material_animations;
            }
        }
        out.headers_.push_back(std::move(header));
    }
    void definition(const detail::DefinitionSpec& fact) {
        ActorDefinition result{}; result.wire=at(fact.identity,16); result.singleton=fact.singleton;
        result.actor_id=result.wire.u32(0); result.factory_identity=result.wire.u32(4);
        result.change_block_identity=result.wire.u32(8); result.remove_identity=result.wire.u32(12);
        require(result.actor_id==fact.actor_id && result.factory_identity==fact.factory_identity &&
                result.change_block_identity==fact.change_block_identity && result.remove_identity==fact.remove_identity,
                "Actor definition differs from closed course catalog",fact.identity);
        if (result.actor_id) {
            require(fact.initialization<out.initializations_.size() &&
                    out.initializations_[fact.initialization].actor_id==result.actor_id &&
                    result.factory_identity && result.change_block_identity,
                    "Unbound actor definition",fact.identity);
            result.initialization=fact.initialization;
        } else require(fact.initialization==actor_no_index && !result.factory_identity &&
                       !result.change_block_identity && !result.remove_identity,
                       "Malformed actor definition terminator",fact.identity);
        out.definitions_.push_back(result);
    }
    void table(const detail::DefinitionTableSpec& fact) const {
        require(fact.first_definition<=out.definitions_.size() &&
                fact.live_rows<=out.definitions_.size()-fact.first_definition &&
                fact.terminator_identity==fact.identity+fact.live_rows*16,
                "Invalid closed actor definition table",fact.identity);
        for (std::size_t i=0; i<fact.live_rows; ++i) {
            const auto& row=out.definitions_[fact.first_definition+i];
            require(row.wire.guest_address==fact.identity+i*16 && row.actor_id && !row.singleton,
                    "Actor definition table row mismatch",fact.identity);
        }
        const auto terminator=at(fact.terminator_identity,16);
        require(!terminator.u32(0) && !terminator.u32(4) && !terminator.u32(8) && !terminator.u32(12),
                "Malformed actor definition table terminator",fact.terminator_identity);
    }
    void automatic_definition(const AutomaticActorDefinition& fact) const {
        require(fact.origin.function_identity && fact.origin.line &&
                fact.initialization<out.initializations_.size() &&
                out.initializations_[fact.initialization].actor_id==fact.actor_id,
                "Unbound automatic Source actor definition",fact.origin.function_identity);
        const auto found=std::find_if(out.definitions_.begin(),out.definitions_.end(),[&](const auto& value) {
            return value.initialization==fact.initialization && value.actor_id==fact.actor_id &&
                   value.factory_identity==fact.factory_identity;
        });
        // Automatic C values have no ROM record. Their immutable Source origin
        // and closed factory/initialization family are required, but their exact
        // change/remove tuple may intentionally differ from initialized rows.
        require(found!=out.definitions_.end(),"Automatic Source actor definition lacks initialized factory family",
                fact.origin.function_identity);
    }
};

Resources::Resources(const rom::Snapshot& snapshot, std::int32_t course_id, FogProfile fog)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()), catalog_(&detail::course_catalog(course_id)) {
    require(snapshot.canonical().size()==rom::us_size && snapshot.identity().size==rom::us_size &&
            snapshot.identity().sha1==admitted_sha1,"Actor resources require admitted US Snapshot");
    require(fog.min<fog.max,"Invalid explicit actor fog profile");
    const auto& expected=catalog_->expected;
    require(expected.models==catalog_->initializations.size() && expected.models<=detail::max_catalog_models &&
            expected.headers==catalog_->headers.size() && expected.definitions==catalog_->definitions.size() &&
            expected.materials>=catalog_->material_playbacks.size() &&
            expected.materials-catalog_->material_playbacks.size()==catalog_->material_extents.size() &&
            expected.paths<=detail::max_catalog_paths,
            "Invalid closed actor catalog extents");
    models_.reserve(expected.models); materials_.reserve(expected.materials);
    animations_.reserve(expected.model_animations+expected.material_animations);
    paths_.reserve(expected.paths);
    material_playbacks_.reserve(catalog_->material_playbacks.size());
    headers_.reserve(expected.headers); initializations_.reserve(expected.models);
    definitions_.reserve(expected.definitions); visibility_.reserve(expected.models);
    const model_assets::Reader reader(snapshot,catalog_->mappings);
    ActorResourceBuilder builder{*this,reader,fog};
    for (const auto& fact:catalog_->material_playbacks) {
        require(std::none_of(catalog_->material_extents.begin(),catalog_->material_extents.end(),
                    [&](const auto& extent) { return extent.material==fact.material; }) &&
                std::none_of(material_playbacks_.begin(),material_playbacks_.end(),
                    [&](const auto& playback) { return materials_[playback.material].wire.guest_address==fact.material; }),
                "Source material playback authority duplicates an allocation or binding",fact.material);
        const auto program=builder.decoder.animation(fact.program,menu::AnimationKind::texture);
        const std::array<menu::animation::TexturePlayback,1> capabilities{{{program,fact.speeds}}};
        const auto material=builder.decoder.playback_material(fact.material,capabilities);
        material_playbacks_.push_back({material,capabilities[0]});
    }
    for (const auto& fact:catalog_->initializations) builder.model(fact);
    std::size_t next_header=0;
    for (std::size_t i=0; i<catalog_->initializations.size(); ++i) {
        const auto& fact=catalog_->initializations[i];
        require(fact.header_begin==next_header && fact.header_count<=catalog_->headers.size()-next_header,
                "Actor header slices do not cover closed catalog",fact.identity);
        for (std::size_t j=0; j<fact.header_count; ++j) builder.header(i,catalog_->headers[fact.header_begin+j]);
        next_header+=fact.header_count;
        require(!initializations_[i].animations_enabled || initializations_[i].setup.initial_header!=actor_no_index,
                "Actor initial header absent from complete catalog",fact.identity);
    }
    for (const auto& fact:catalog_->definitions) builder.definition(fact);
    for (const auto& fact:catalog_->tables) builder.table(fact);
    for (const auto& fact:catalog_->automatic_definitions) builder.automatic_definition(fact);
    const auto model_programs=static_cast<std::size_t>(std::count_if(animations_.begin(),animations_.end(),
        [](const auto& value) { return value.kind==menu::AnimationKind::model; }));
    require(models_.size()==expected.models && materials_.size()==expected.materials &&
            model_programs==expected.model_animations &&
            animations_.size()-model_programs==expected.material_animations &&
            next_header==expected.headers && headers_.size()==expected.headers && definitions_.size()==expected.definitions &&
            paths_.size()==expected.paths,
            "Incomplete closed course actor descriptor/program catalog");
    std::array<model_assets::ModelPlan,detail::max_catalog_models> plans{};
    for (std::size_t i=0; i<models_.size(); ++i) plans[i]={&models_[i],visibility_[i]};
    // The shared decoder retains native controls for interpolation. Retain the
    // same bounded wire closure in the cold installation/reservation plan too;
    // no whole mapping or unrelated trailing spline controls are admitted.
    std::array<rom::GuestView,detail::max_catalog_paths*4> path_installation{};
    std::size_t path_span_count=0;
    for (const auto& path:paths_) {
        path_installation[path_span_count++]=path.wire;
        path_installation[path_span_count++]=reader.view(path.wire.u32(8),path.points.size()*12);
        path_installation[path_span_count++]=reader.view(path.wire.u32(16),path.timestamps.size()*4);
        if (!path.speed_polynomials.empty())
            path_installation[path_span_count++]=reader.view(path.wire.u32(20),path.speed_polynomials.size()*4);
    }
    closure_.emplace(reader,std::span<const model_assets::ModelPlan>(plans.data(),models_.size()),materials_,
                     std::span<const std::uint32_t>{},
                     std::span<const rom::GuestView>(path_installation.data(),path_span_count));
    closure_->require_complete();
}
std::int32_t Resources::course_id() const noexcept { return catalog_->course_id; }
void Resources::require_live() const {
    const auto bytes=snapshot_->canonical();
    require(bytes.size()==rom::us_size && bytes.data()==source_data_,"Actor resources borrow moved-from Snapshot");
}
std::span<const menu::Model> Resources::models() const { require_live(); return models_; }
std::span<const menu::Material> Resources::materials() const { require_live(); return materials_; }
std::span<const menu::Animation> Resources::animations() const { require_live(); return animations_; }
std::span<const menu::AnimationPath> Resources::paths() const { require_live(); return paths_; }
std::span<const menu::animation::MaterialPlayback> Resources::material_playbacks() const { require_live(); return material_playbacks_; }
std::span<const ActorAnimationHeader> Resources::headers() const { require_live(); return headers_; }
std::span<const ActorInitialization> Resources::initializations() const { require_live(); return initializations_; }
std::span<const ActorDefinition> Resources::definitions() const { require_live(); return definitions_; }
std::span<const AutomaticActorDefinition> Resources::automatic_definitions() const { require_live(); return catalog_->automatic_definitions; }
const ActorInitialization& Resources::initialization(std::uint32_t identity) const { require_live(); return lookup(initializations_,identity); }
const ActorDefinition& Resources::definition(std::uint32_t identity) const { require_live(); return lookup(definitions_,identity); }
std::span<const std::uint8_t> Resources::visibility_domains(std::size_t model) const {
    require_live(); if (model>=visibility_.size()) throw std::out_of_range("Actor visibility model index"); return visibility_[model];
}
std::span<const menu::DisplayList> Resources::display_lists() const { require_live(); return closure_->display_lists(); }
std::span<const menu::VertexLoad> Resources::vertex_loads() const { require_live(); return closure_->vertex_loads(); }
std::span<const menu::ImageLoad> Resources::image_loads() const { require_live(); return closure_->image_loads(); }
std::span<const menu::DynamicMaterialCall> Resources::dynamic_material_calls() const { require_live(); return closure_->dynamic_material_calls(); }
std::span<const rom::GuestView> Resources::installation_spans() const { require_live(); return closure_->installation_spans(); }
void Resources::install(std::span<std::byte> ram, std::span<const menu::RamRange> reservations) const {
    require_live(); closure_->require_complete();
    if (ram.size()!=menu::rdram_size) throw std::invalid_argument("Actor install requires word-swapped 8 MiB RDRAM");
    const auto source=snapshot_->canonical(); const std::less<const std::byte*> less;
    if (less(ram.data(),source.data()+source.size()) && less(source.data(),ram.data()+ram.size()))
        throw std::invalid_argument("Actor installation aliases Snapshot");
    for (const auto range:reservations) valid_reservation(range);
    const auto spans=closure_->installation_spans();
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        require(span.guest_address>=ram_base && offset<=ram.size() && span.bytes.size()<=ram.size()-offset &&
                span.rom_offset<=source.size() && span.bytes.size()<=source.size()-span.rom_offset &&
                span.bytes.data()==source.data()+span.rom_offset,"Invalid actor install provenance/extent",span.guest_address);
        for (const auto range:reservations) disjoint(span,range);
    }
    for (const auto& span:spans) {
        const auto offset=static_cast<std::size_t>(span.guest_address-ram_base);
        for (std::size_t i=0; i<span.bytes.size(); ++i) ram[(offset+i)^3u]=span.bytes[i];
    }
}
} // namespace isle::actor
