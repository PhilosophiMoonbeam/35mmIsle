#include "actor_interaction_services.hpp"
#include "actor_behavior_bindings.hpp"
#include "actor_owner.hpp"
#include "actor_spatial.h"
#include "item_owner.hpp"
#include "beach_initial_species_source.h"
#include "beach_initial_species.hpp"
#include "beach_ending_source.h"
#include "beach_snorlax_kangaskhan.hpp"
#include "beach_meowth.hpp"
#include "beach_eevee_chansey.hpp"
#include "beach_scyther_special.hpp"
#include "35mmisle/focus_detector.hpp"
#include "rainbow_actor.hpp"
#include "player_root.hpp"

#include <limits>
#include <stdexcept>

#include <algorithm>
namespace isle::actor {
namespace {
enum class Position { root, auxiliary, preferred };

IsleGameResult position_ref(IsleGame* game, IsleGameObject* object, Position mode,
                            IsleActorPositionRef& output) noexcept {
    if (!object) return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameModel* model = nullptr;
    auto result = isle_actor_spatial_object_model(game, object, &model);
    if (result != ISLE_GAME_OK) return result;
    if (!model) return ISLE_GAME_INVALID_STATE;
    IsleActorPositionRef reference{};
    result = isle_game_model_reference(game, object, model, &reference.model);
    if (result != ISLE_GAME_OK) return result;
    reference.space = ISLE_ACTOR_POSITION_ROOT;
    if (mode != Position::root) {
        const IsleGameModelPlacement* placement = nullptr;
        result = isle_game_model_placement(game, model, &placement);
        if (result != ISLE_GAME_OK) return result;
        if (!placement) return ISLE_GAME_INVALID_STATE;
        switch (placement->mode) {
        case ISLE_GAME_MODEL_PLACEMENT_RT:
        case ISLE_GAME_MODEL_PLACEMENT_RTS:
            reference.space = ISLE_ACTOR_POSITION_AUX;
            break;
        case ISLE_GAME_MODEL_PLACEMENT_DISABLED:
            if (mode == Position::auxiliary) return ISLE_GAME_INVALID_STATE;
            break;
        default:
            return ISLE_GAME_INVALID_STATE;
        }
    }
    output = reference;
    return ISLE_GAME_OK;
}

IsleGameResult position_read(IsleGame* game, const IsleActorPositionRef& reference,
                             const IsleGameVec3*& output) noexcept {
    IsleGameModel* model = nullptr;
    auto result = isle_game_model_resolve(game, &reference.model, &model);
    if (result != ISLE_GAME_OK) return result;
    if (reference.space == ISLE_ACTOR_POSITION_AUX) {
        const IsleGameModelPlacement* placement = nullptr;
        result = isle_game_model_placement(game, model, &placement);
        if (result != ISLE_GAME_OK) return result;
        if (!placement || (placement->mode != ISLE_GAME_MODEL_PLACEMENT_RT &&
            placement->mode != ISLE_GAME_MODEL_PLACEMENT_RTS)) return ISLE_GAME_INVALID_STATE;
        output = &placement->transform.position;
        return ISLE_GAME_OK;
    }
    if (reference.space != ISLE_ACTOR_POSITION_ROOT) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameModelNode* nodes = nullptr;
    std::size_t count = 0;
    result = isle_game_model_peek_nodes(game, model, &nodes, &count);
    if (result != ISLE_GAME_OK) return result;
    if (!nodes || !count || nodes[0].parent != UINT32_MAX) return ISLE_GAME_INVALID_STATE;
    output = &nodes[0].transform.position;
    return ISLE_GAME_OK;
}

IsleGameResult position(IsleGame* game, IsleGameObject* object, Position mode,
                        IsleGameVec3& output) noexcept {
    IsleActorPositionRef reference{};
    auto result = position_ref(game, object, mode, reference);
    if (result != ISLE_GAME_OK) return result;
    const IsleGameVec3* view = nullptr;
    result = position_read(game, reference, view);
    if (result == ISLE_GAME_OK) output = *view;
    return result;
}


struct WeightedAdmission {
    std::uint32_t identity;
    std::span<const std::uint32_t> callbacks;
    std::span<const std::int32_t> weights;
};
constexpr std::uint32_t snorlax_headers[] = {0x802cc10c, 0x802cc120, 0x802cc134,
    0x802cc148, 0x802cc15c, 0x802cc170, 0x802cc184, 0x802cc198, 0x802cc1ac, 0x802cc1c0};
constexpr std::uint32_t meowth_headers[] = {0x802cc344, 0x802cc358, 0x802cc36c,
    0x802cc380, 0x802cc394, 0x802cc3a8, 0x802cc3bc, 0x802cc3d0, 0x802cc3e4, 0x802cc3f8};
constexpr std::uint32_t eevee_headers[] = {0x802cc9ac, 0x802cc9c0, 0x802cc9d4, 0x802cc9e8, 0x802cc9fc};
constexpr std::uint32_t scyther_headers[] = {0x802cd168, 0x802cd17c, 0x802cd190,
    0x802cd1a4, 0x802cd1b8, 0x802cd1cc};
constexpr std::uint32_t chansey_headers[] = {0x802cd2a8, 0x802cd2bc, 0x802cd2d0,
    0x802cd2e4, 0x802cd2f8, 0x802cd30c, 0x802cd320, 0x802cd334, 0x802cd348};
constexpr std::uint32_t kangaskhan_headers[] = {0x802cd5a8, 0x802cd5bc, 0x802cd5d0,
    0x802cd5e4, 0x802cd5f8, 0x802cd60c, 0x802cd620};
constexpr std::uint32_t helper_headers[] = {0x802ce070};
constexpr std::uint32_t snorlax_graphs[] = {0x802cc1d4, 0x802cc234, 0x802cc264, 0x802cc294};
constexpr std::uint32_t eevee_graphs[] = {0x802cca10, 0x802ccab8, 0x802ccae8,
    0x802ccb28, 0x802ccba8, 0x802ccbf8, 0x802ccc28, 0x802ccc68, 0x802ccca8};
constexpr std::uint32_t scyther_graphs[] = {0x802cd1e4, 0x802cd214};
constexpr std::uint32_t chansey_graphs[] = {0x802cd35c, 0x802cd3f8, 0x802cd438,
    0x802cd478, 0x802cd4a8, 0x802cd4e8, 0x802cd518};
constexpr std::uint32_t kangaskhan_graphs[] = {0x802cd634, 0x802cd694, 0x802cd6c4, 0x802cd6f4};
constexpr std::uint32_t special_graphs[] = {0x802cd858};
constexpr std::uint32_t snorlax_choices[] = {0x802c5700, 0x802c5828};
constexpr std::uint32_t meowth_choices[] = {0x802c5e88, 0x802c5ef8};
constexpr std::uint32_t eevee_choices[] = {0x802c737c, 0x802c73ec};
constexpr std::uint32_t chansey_choices[] = {0x802c9580, 0x802c95f0};
constexpr std::uint32_t kangaskhan_choices[] = {0x802c9cf8};
constexpr std::uint32_t kangaskhan_idle_choices[] = {0x802c9ae4, 0x802c9b40, 0x802c9b9c};
constexpr std::uint32_t special_choices[] = {0x802ca1bc, 0x802ca23c};
constexpr std::int32_t equal_pair[] = {100, 100}, equal_single[] = {100},
    equal_triple[] = {100, 100, 100}, special_weights[] = {70, 30};
const WeightedAdmission snorlax_weighted[] = {{0x802cc2c4, snorlax_choices, equal_pair}};
const WeightedAdmission meowth_weighted[] = {{0x802cc4ac, meowth_choices, equal_pair}};
const WeightedAdmission eevee_weighted[] = {{0x802ccaa0, eevee_choices, equal_pair}};
const WeightedAdmission chansey_weighted[] = {{0x802cd3dc, chansey_choices, equal_pair}};
const WeightedAdmission kangaskhan_weighted[] = {
    {0x802cd764, kangaskhan_choices, equal_single},
    {0x802cd774, kangaskhan_idle_choices, equal_triple}};
const WeightedAdmission special_weighted[] = {{0x802cd8b8, special_choices, special_weights}};

struct FamilyAdmission {
    std::uint32_t id, factory;
    unsigned group;
    std::span<const BehaviorCallbackBinding> callbacks;
    // These bounds partition a shared module's registry, never admit guest
    // addresses as callable pointers. Membership still requires pointer equality.
    std::uint32_t first_callback, last_callback;
    std::span<const std::uint32_t> headers, graphs;
    std::span<const WeightedAdmission> weighted;
};
const FamilyAdmission remaining_families[] = {
    {143, 0x802c5c28,
        1024, beach::snorlax_kangaskhan_callbacks(), 0x802c5620, 0x802c5bd4,
        snorlax_headers, snorlax_graphs, snorlax_weighted},
    {52, 0x802c7270,
        2048, beach::meowth_callbacks(), 0x802c5c60, 0x802c7130,
        meowth_headers, beach::meowth_graphs(), meowth_weighted},
    {133, 0x802c7ad0,
        4096, beach::eevee_chansey_callbacks(), 0x802c72e0, 0x802c7a60,
        eevee_headers, eevee_graphs, eevee_weighted},
    {123, 0x802c91cc,
        8192, beach::scyther_special_callbacks(), 0x802c8db0, 0x802c914c,
        scyther_headers, scyther_graphs, {}},
    {113, 0x802c9a7c,
        16384, beach::eevee_chansey_callbacks(), 0x802c9240, 0x802c9a10,
        chansey_headers, chansey_graphs, chansey_weighted},
    {115, 0x802ca05c,
        32768, beach::snorlax_kangaskhan_callbacks(), 0x802c9ac0, 0x802ca010,
        kangaskhan_headers, kangaskhan_graphs, kangaskhan_weighted},
    {1005, 0x802ca2bc,
        65536, beach::scyther_special_callbacks(), 0x802ca0a0, 0x802ca23c,
        {}, special_graphs, special_weighted},
    {1006, 0x802cbda0,
        131072, beach::scyther_special_callbacks(), 0x802cbce0, 0x802cbd04,
        helper_headers, {}, {}},
};
unsigned remaining_species_group(IsleGameSourceCallback function) noexcept {
    for (const auto& family : remaining_families)
        for (const auto& binding : family.callbacks)
            if (binding.identity >= family.first_callback && binding.identity <= family.last_callback &&
                binding.function == function) return family.group;
    return 0;
}
bool remaining_species_callback(IsleGameSourceCallback function, void* context) noexcept {
    return remaining_species_group(function) == *static_cast<const unsigned*>(context);
}

unsigned initial_species_group(IsleGameSourceCallback function) noexcept {
    if (function == func_beach_802C7B10 || function == func_beach_802C7BB8 ||
        function == func_beach_802C7C7C || function == func_beach_802C7CF0 ||
        function == func_beach_802C7DA0 || function == func_beach_802C7DEC) return 8;
    if (function == func_beach_802C82B0 || function == func_beach_802C8354 ||
        function == func_beach_802C83D0 || function == func_beach_802C8438 ||
        function == func_beach_802C84A4) return 16;
    if (function == func_beach_802C7E70 ||
        function == func_beach_802C7F1C ||
        function == func_beach_802C7F74 ||
        function == func_beach_802C8028 ||
        function == func_beach_802C80BC ||
        function == func_beach_802C80F0 ||
        function == func_beach_802C8124 ||
        function == func_beach_802C816C ||
        function == func_beach_802C81C4 ||
        function == func_beach_802C821C) return 32;
    if (function == func_beach_802C85B0 ||
        function == func_beach_802C85E0 ||
        function == func_beach_802C86A8 ||
        function == func_beach_802C87BC ||
        function == func_beach_802C8828 ||
        function == func_beach_802C8894 ||
        function == func_beach_802C8900 ||
        function == func_beach_802C8980 ||
        function == func_beach_802C89A4 ||
        function == func_beach_802C8AAC ||
        function == func_beach_802C8B60 ||
        function == func_beach_802C8BC4 ||
        function == func_beach_802C8C30 ||
        function == func_beach_802C8C58 ||
        function == func_beach_802C8C7C ||
        function == func_beach_802C8CC0 ||
        function == func_beach_802C8CFC) return 64;
    if (function == func_beach_802CAC80 ||
        function == func_beach_802CACD4 ||
        function == func_beach_802CAD58 ||
        function == func_beach_802CAD7C ||
        function == func_beach_802CADCC ||
        function == func_beach_802CAE14 ||
        function == func_beach_802CAEB0 ||
        function == func_beach_802CAF1C ||
        function == func_beach_802CAF90 ||
        function == func_beach_802CAFEC ||
        function == func_beach_802CB054 ||
        function == func_beach_802CB08C ||
        function == func_beach_802CB128 ||
        function == func_beach_802CB2D8 ||
        function == func_beach_802CB36C ||
        function == func_beach_802CB3DC ||
        function == func_beach_802CB44C ||
        function == func_beach_802CB520 ||
        function == func_beach_802CB594 ||
        function == func_beach_802CB6B4 ||
        function == func_beach_802CB710 ||
        function == func_beach_802CB784 ||
        function == func_beach_802CB814 ||
        function == func_beach_802CB874 ||
        function == func_beach_802CB9E0 ||
        function == func_beach_802CBA48 ||
        function == func_beach_802CBB34 ||
        function == func_beach_802CBB90 ||
        function == func_beach_802CBC4C) return 128;
    if (function == func_beach_802CA300) return 256;
    if (function == func_beach_802CA450 || function == func_beach_802CA474 ||
        function == func_beach_802CA4CC || function == func_beach_802CA5DC ||
        function == func_beach_802CA8C4 || function == func_beach_802CA950) return 512;
    return 0;
}
bool initial_species_callback(IsleGameSourceCallback function, void* context) noexcept {
    return initial_species_group(function) == *static_cast<const unsigned*>(context);
}

bool header_assets(const Resources& resources, Owner& actors,
    const ActorInitialization* initialization, std::uint32_t identity) {
    if (!initialization) return false;
    const auto initializations = resources.initializations();
    const auto models = resources.models();
    const auto animations = resources.animations();
    const auto init_index = static_cast<std::size_t>(initialization - initializations.data());
    if (init_index >= initializations.size() || initialization->model >= models.size()) return false;
    const auto& model = models[initialization->model];
    for (const auto& header : resources.headers()) {
        if (header.wire.guest_address != identity || header.initialization != init_index) continue;
        if (header.nodes.size() != model.nodes.size() || header.model_bindings.size() != model.nodes.size())
            return false;
        auto program = [&](std::size_t index, std::size_t node) {
            if (index == actor_no_index) return true;
            if (index >= animations.size()) return false;
            for (const auto& command : animations[index].commands) {
                const auto category = command.mask >> 8;
                const auto value = command.mask & 255u;
                if (command.opcode != 16 || category > 3 || !value) continue;
                if (model.nodes[node].parent == actor_no_index) {
                    if (header.sound_ids.empty()) continue; // Genuine source no-op.
                    if (value > header.sound_ids.size() ||
                        !actors.environment().admits_sound(header.sound_ids[value - 1])) return false;
                } else if (!actors.effects().has_effect(static_cast<std::uint16_t>(category), value - 1)) {
                    return false;
                }
            }
            return true;
        };
        for (std::size_t node = 0; node < header.nodes.size(); ++node) {
            const auto& binding = header.nodes[node];
            if (!program(binding.model_animation, node)) return false;
            for (const auto index : binding.material_animations) if (!program(index, node)) return false;
        }
        return true;
    }
    return false;
}

IsleGameResult item_status(items::OwnerStatus status) noexcept {
    if (status) return ISLE_GAME_OK;
    if (status.error == items::OwnerError::capacity) return ISLE_GAME_EXHAUSTED;
    if (status.error == items::OwnerError::admission) return ISLE_GAME_INVALID_ARGUMENT;
    if (status.error == items::OwnerError::lifetime) return ISLE_GAME_INVALID_STATE;
    return ISLE_GAME_EXECUTION_FAILURE;
}

bool rainbow_callback(IsleGameSourceCallback function, void*) noexcept {
    for (const auto& binding : rainbow::actor_callbacks())
        if (binding.function == function) return true;
    return false;
}

bool rainbow_services(const IsleRainbowActorServices* s) noexcept {
    // Check the actual retained complete table, not a declared capability mask.
    return s && s->reference_assign && s->current_block && s->add_one &&
        s->sound_play && s->sound_active && s->sound_stop && s->sound_stop_all &&
        s->bgm_play && s->bgm_stop && s->bgm_volume && s->bgm_channel_volume &&
        s->remove_flying_items && s->reset_camera && s->reset_messages &&
        s->remove_pokemon && s->cutscene_start && s->resume_processes &&
        s->main_camera && s->pause_processes && s->camera_speed &&
        s->camera_animation && s->camera_animation_process && s->sky_speed &&
        s->player_model_init && s->player_model_animation &&
        s->animation_callback && s->collision_initialize && s->collision_query &&
        s->collision_callback;
}
} // namespace

InteractionServices::InteractionServices(scene::Services& services, Owner& actors,
    items::Owner& items, world::World& world, focus::Detector& detector,
    const BehaviorBindings& bindings, const Resources& resources)
    : services_(services), actors_(actors), items_(items), world_(world), detector_(detector),
      bindings_(bindings), resources_(resources) {
    resources.require_live();
    bindings.require_live();
    const auto& course = actors.course_context();
    course.require_live();
    const auto setup = course.course_id()==0 ? 0x8011b914u : 0x800f5da0u;
    IslePlayerSource* player = nullptr;
    if (resources.course_id()!=course.course_id() || bindings.course_id()!=course.course_id() ||
        !world.resources().setup || world.resources().setup->wire.guest_address!=setup ||
        services.failed() || world.retired() || !world.error().empty() ||
        !items.matches(services, world, actors) || actors.player_source().game!=services.game ||
        isle_player_source_context(services.game, &player)!=ISLE_GAME_OK ||
        player!=&actors.player_source())
        throw std::invalid_argument("Actor interactions require matching course and canonical native owners");
    if (course.course_id()==6) {
        auto& source = course.rainbow().actors;
        IsleRainbowActorSource* canonical = nullptr;
        if (source.phase!=ISLE_RAINBOW_ACTOR_ACTIVE || source.nativeGame!=services.game ||
            isle_rainbow_actor_native_context(services.game, &canonical)!=ISLE_GAME_OK ||
            canonical!=&source)
            throw std::invalid_argument("Rainbow interactions require activated canonical Source");
        rainbow_source_ = &source;
        rainbow_services_ = source.services;
        if (!prove_rainbow_closure())
            throw std::invalid_argument("Rainbow interactions require the complete native Source closure");
        rainbow_closed_ = true;
        return;
    }
    common_ = actors.admitted_factory(1003, 0x8035e0d4u);
    magikarp_ = actors.admitted_factory(129, 0x802cac48u);
    butterfree_ = actors.admitted_factory(12, 0x802c7e38u);
    doduo_ = actors.admitted_factory(84, 0x802c8570u);
    lapras_ = actors.admitted_factory(131, 0x802c8274u);
    pidgey_ = actors.admitted_factory(16, 0x802c8d3cu);
    pikachu_ = actors.admitted_factory(25, 0x802cbca8u);
    sign_ = actors.admitted_factory(1004, 0x802ca418u);
    common_assets_ = header_assets(resources, actors, common_, 0x80388310u);
    magikarp_assets_ = header_assets(resources, actors, magikarp_, 0x802cda8cu) &&
        header_assets(resources, actors, magikarp_, 0x802cdab4u);
    butterfree_assets_ = header_assets(resources, actors, butterfree_, 0x802ccd24u) &&
        header_assets(resources, actors, butterfree_, 0x802ccd38u) &&
        header_assets(resources, actors, butterfree_, 0x802ccd4cu);
    doduo_assets_ = header_assets(resources, actors, doduo_, 0x802cceb0u) &&
        header_assets(resources, actors, doduo_, 0x802ccec4u) &&
        header_assets(resources, actors, doduo_, 0x802cced8u) &&
        header_assets(resources, actors, doduo_, 0x802cceecu) &&
        header_assets(resources, actors, doduo_, 0x802ccf00u);
    lapras_assets_ = header_assets(resources, actors, lapras_, 0x802ccde8u) &&
        header_assets(resources, actors, lapras_, 0x802cce08u);
    pidgey_assets_ = true;
    for (const auto header : {0x802ccfc0u, 0x802ccfd4u, 0x802ccfe8u, 0x802ccffcu})
        pidgey_assets_ &= header_assets(resources, actors, pidgey_, header);
    pikachu_assets_ = true;
    for (const auto header : {0x802cdb5cu, 0x802cdb70u, 0x802cdb84u, 0x802cdbacu,
                             0x802cdbc0u, 0x802cdbd4u, 0x802cdbe8u, 0x802cdbfcu, 0x802cdc10u})
        pikachu_assets_ &= header_assets(resources, actors, pikachu_, header);
    sign_assets_ = header_assets(resources, actors, sign_, 0x802cd920u);
    for (std::size_t i = 0; i < remaining_.size(); ++i) {
        const auto& family = remaining_families[i];
        remaining_[i] = actors.admitted_factory(family.id, family.factory);
        remaining_assets_[i] = remaining_[i] != nullptr;
        for (const auto header : family.headers)
            remaining_assets_[i] &= header_assets(resources, actors, remaining_[i], header);
        if (family.id == 1006)
            remaining_assets_[i] &= actors.environment().admits_sound(0x34);
    }
}

bool InteractionServices::matches(Owner& actors, scene::Services& services,
    const Resources& resources, world::World& world, focus::Detector& detector) const noexcept {
    return &actors_ == &actors && &services_ == &services && &resources_ == &resources &&
        &world_ == &world && &detector_ == &detector &&
        resources.course_id()==actors.course_context().course_id() &&
        bindings_.course_id()==resources.course_id() && items_.matches(services, world, actors);
}

bool InteractionServices::admits_source() const noexcept {
    try {
        const auto& course = actors_.course_context();
        if (!rainbow_closed_ || course.course_id()!=6 || services_.failed() ||
            !world_.error().empty() ||
            !items_.matches(services_, world_, actors_)) return false;
        resources_.require_live();
        bindings_.require_live();
        course.require_live();
        auto& state = course.rainbow();
        auto& source = state.actors;
        const auto& player = actors_.player_source();
        if (&source!=rainbow_source_ || source.services!=rainbow_services_ ||
            source.phase!=ISLE_RAINBOW_ACTOR_ACTIVE || source.status!=ISLE_GAME_OK ||
            source.nativeGame!=services_.game || player.game!=services_.game ||
            player.status!=ISLE_GAME_OK ||
            (state.source.player && state.source.player!=&player)) return false;
        if (!player.native_root) return false;
        auto& root = *static_cast<isle::player::Root*>(player.native_root);
        if (&root.source()!=&player || root.bound_actors()!=&actors_) return false;
        // The canonical lookup also validates actual Game/Root/Player/Owner
        // association. PREPARED activation lookup is not execution admission.
        IsleRainbowActorSource* canonical = nullptr;
        return isle_rainbow_actor_native_context(services_.game, &canonical)==ISLE_GAME_OK &&
            canonical==rainbow_source_;
    } catch (...) {
        return false;
    }
}

bool InteractionServices::admits_rainbow(const void* path) const {
    if (world_.retired() || !admits_source()) return false;
    bool current = false;
    for (const auto& block : world_.blocks())
        if (&block==world_.current() && block.resource) current = true;
    if (!current) return false;
    if (path) {
        bool found = false;
        for (const auto& candidate : world_.resources().paths)
            if (&candidate==path) found = true;
        if (!found) return false;
    }
    return true;
}

bool InteractionServices::prove_rainbow_closure() {
    if (!rainbow_source_ || !rainbow_services(rainbow_services_)) return false;
    const auto callbacks = rainbow::actor_callbacks();
    const auto providers = rainbow::actor_dependencies();
    const auto factories = rainbow::actor_factories();
    if (callbacks.size()!=67 || providers.size()!=77 || factories.size()!=5 ||
        rainbow::actor_graphs().size()!=20 || rainbow::actor_weighted_states().size()!=1 ||
        rainbow::actor_child_factories().size()!=5) return false;
    for (const auto& binding : callbacks)
        if (!binding.function || bindings_.callback(binding.identity)!=binding.function) return false;
    auto provider = [&](std::uint32_t identity) {
        return std::any_of(providers.begin(), providers.end(),
            [&](const auto& dependency) { return dependency.identity==identity; });
    };
    auto factory = [&](std::uint32_t identity) -> const ActorInitialization* {
        for (const auto& binding : factories)
            if (binding.factory==identity && binding.function)
                return actors_.admitted_factory(binding.actor_id, binding.factory);
        return nullptr;
    };
    // Prove every member, not merely a DFS prefix that may stop at a cycle.
    // This finite universal closure includes all five child factories and the
    // typed utility/animation/collision providers absent from the GObj registry.
    std::uint64_t services = 0;
    for (const auto& dependency : providers) {
        bool typed = false;
        for (const auto& binding : callbacks)
            typed |= binding.identity==dependency.identity && binding.function;
        typed |= factory(dependency.identity)!=nullptr;
        typed |= dependency.identity==0x803466c0u || dependency.identity==0x80346898u ||
            dependency.identity==0x803484f0u;
        for (const auto& binding : rainbow::actor_animation_callbacks())
            typed |= binding.identity==dependency.identity &&
                binding.function==func_80348BDC_82834C;
        for (const auto& binding : rainbow::actor_collision_callbacks())
            typed |= binding.identity==dependency.identity &&
                binding.function==func_80348FF0_828760;
        if (!typed) return false;
        services |= dependency.services;
        for (const auto identity : dependency.callbacks) if (!provider(identity)) return false;
        for (const auto identity : dependency.graphs)
            if (!bindings_.admits_native_graph(bindings_.graph(identity), rainbow_callback, nullptr)) return false;
        for (const auto identity : dependency.weighted_states) {
            const auto& weighted = bindings_.weighted_states(identity);
            if (weighted.rows.empty() || weighted.rows.back().func ||
                weighted.rows.back().weight || weighted.total_weight<=0) return false;
            std::int64_t total = 0;
            for (std::size_t i=0; i+1<weighted.rows.size(); ++i) {
                const auto& row = weighted.rows[i];
                if (row.weight<=0 || !rainbow_callback(row.func, nullptr)) return false;
                total += row.weight;
            }
            if (total!=weighted.total_weight) return false;
        }
        for (const auto identity : dependency.headers) {
            bool found = false;
            for (const auto& header : resources_.headers()) {
                if (header.wire.guest_address!=identity ||
                    header.initialization>=resources_.initializations().size()) continue;
                const auto* initialization = &resources_.initializations()[header.initialization];
                for (const auto& binding : factories)
                    if (factory(binding.factory)==initialization)
                        found |= header_assets(resources_, actors_, initialization, identity);
            }
            if (!found) return false;
        }
        for (const auto identity : dependency.child_factories)
            if (!factory(identity) || !provider(identity)) return false;
    }
    if (services!=rainbow::actor_required_services()) return false;
    std::size_t factory_index = 0;
    for (const auto& binding : factories) {
        const auto* initialization = factory(binding.factory);
        if (!initialization || !provider(binding.initial_state)) return false;
        rainbow_initializations_[factory_index++] = initialization;
        if (initialization->setup.initial_header!=actor_no_index) {
            if (initialization->setup.initial_header>=resources_.headers().size() ||
                !header_assets(resources_, actors_, initialization,
                    resources_.headers()[initialization->setup.initial_header].wire.guest_address)) return false;
        }
    }
    for (const auto& child : rainbow::actor_child_factories()) {
        const auto& definition = child.definition;
        const auto* initialization = factory(definition.factory);
        if (!provider(child.helper_identity) || !initialization ||
            child.written_fields!=ISLE_RAINBOW_SPAWN_ID_TRANSFORMS) return false;
        if (!definition.identity) continue; // Actual automatic Source origin, not a GuestView.
        bool found = false;
        for (const auto& candidate : resources_.definitions())
            if (candidate.wire.guest_address==definition.identity &&
                candidate.actor_id==definition.actor_id && candidate.factory_identity==definition.factory &&
                candidate.change_block_identity==definition.change_block &&
                candidate.remove_identity==definition.remove &&
                candidate.initialization<resources_.initializations().size() &&
                &resources_.initializations()[candidate.initialization]==initialization) found = true;
        if (!found) return false;
    }
    for (const auto identity : rainbow::actor_graphs())
        if (!bindings_.admits_native_graph(bindings_.graph(identity), rainbow_callback, nullptr)) return false;
    return true;
}

struct InteractionSourceAccess {
    template<class Operation>
    static IsleGameResult context(IsleActorSource* source, Operation operation) noexcept {
        try {
            void* value = nullptr;
            const auto result = isle_actor_owner_interaction_context(source, &value);
            if (result != ISLE_GAME_OK) return result;
            if (!value) return ISLE_GAME_INVALID_STATE;
            auto& self = *static_cast<InteractionServices*>(value);
            const auto live = validate(self, source);
            return live == ISLE_GAME_OK ? operation(self) : live;
        } catch (...) {
            return ISLE_GAME_INVALID_STATE;
        }
    }

    static IsleGameResult validate(InteractionServices& self, IsleActorSource* source) {
        if (!source || source->game != self.services_.game || self.services_.failed() ||
            self.world_.retired() || !self.world_.error().empty()) return ISLE_GAME_INVALID_STATE;
        if (self.actors_.course_context().course_id()==6)
            return self.admits_source() ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
        IslePlayerSource* active = nullptr;
        if (isle_player_source_context(self.services_.game, &active) != ISLE_GAME_OK ||
            active != &self.actors_.player_source()) return ISLE_GAME_INVALID_STATE;
        self.resources_.require_live();
        self.bindings_.require_live();
        const auto& course = self.actors_.course_context();
        course.require_live();
        if (self.resources_.course_id()!=course.course_id() ||
            self.bindings_.course_id()!=course.course_id() ||
            !self.items_.matches(self.services_, self.world_, self.actors_)) return ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_OK;
    }
    static IsleGameResult scene_path(IsleActorSource* source,std::uint32_t identity,const void*** output) noexcept {
        if (output) *output=nullptr;
        if (!output || identity!=0x802cd3f4u || !source || (source->id!=133 && source->id!=113))
            return ISLE_GAME_INVALID_ARGUMENT;
        return context(source,[&](InteractionServices& self) {
            Admission admission{self, *source};
            if (!callback_proof(admission, source->id == 133 ? 4096u : 16384u))
                return ISLE_GAME_INVALID_STATE;
            auto& state = self.actors_.course_context().beach();
            if (state.eevee_chansey_path) {
                bool found=false;
                for (const auto& path:self.world_.resources().paths)
                    if (&path==state.eevee_chansey_path) { found=true; break; }
                if (!found) return ISLE_GAME_INVALID_STATE;
            }
            *output=&state.eevee_chansey_path;
            return ISLE_GAME_OK;
        });
    }

    static IsleGameResult player(InteractionServices& self, IsleGameObject*& output) noexcept {
        const auto& player = self.actors_.player_source();
        IslePlayerSource* active = nullptr;
        if (isle_player_source_context(self.services_.game, &active) != ISLE_GAME_OK ||
            active != &player || !player.object || !player.model || !player.camera ||
            !self.services_.source_owner_registered(player.object) ||
            self.services_.source_owner_joined(player.object)) return ISLE_GAME_INVALID_STATE;
        auto result = isle_game_model_validate_binding(self.services_.game, player.object, player.model);
        if (result != ISLE_GAME_OK) return result;
        const IsleGameCameraParameters* camera = nullptr;
        result = isle_game_camera_parameters(self.services_.game, player.camera, &camera);
        if (result != ISLE_GAME_OK) return result;
        if (!camera) return ISLE_GAME_INVALID_STATE;
        output = player.object;
        return ISLE_GAME_OK;
    }

    struct Admission {
        InteractionServices& self;
        IsleActorSource& source;
        unsigned checked = 0, admitted = 0;
    };
    static bool callback_proof(Admission& admission, unsigned group) {
        auto& self = admission.self;
        auto& source = admission.source;
        if (self.actors_.course_context().course_id()!=0) return false;
        if (group == 512) {
            for (const auto& init : self.resources_.initializations())
                if (source.initData == &init && source.animSetup == &init.setup &&
                    source.id == 1001) return self.admits_ending(init);
            return false;
        }
        if (group == 8 || group == 16 || group == 32 || group == 64 || group == 128 || group == 256) {
            const auto* init = group == 8 ? self.butterfree_ : group == 16 ? self.doduo_ :
                group == 32 ? self.lapras_ : group == 64 ? self.pidgey_ :
                group == 128 ? self.pikachu_ : self.sign_;
            IsleGameVec3 auxiliary;
            return init && source.id == static_cast<std::int32_t>(init->actor_id) &&
                source.initData == init && source.animSetup == &init->setup &&
                self.admits_initial_species(*init, source.path) &&
                (group == 256 || position(source.game, source.object, Position::auxiliary, auxiliary) == ISLE_GAME_OK);
        }
        for (std::size_t i = 0; i < self.remaining_.size(); ++i) {
            const auto& family = remaining_families[i];
            if (family.group != group) continue;
            const auto* init = self.remaining_[i];
            IsleGameVec3 auxiliary;
            return init && source.id == static_cast<std::int32_t>(family.id) &&
                source.initData == init && source.animSetup == &init->setup &&
                self.admits_initial_species(*init, source.path) &&
                (family.id >= 1005 ||
                    position(source.game, source.object, Position::auxiliary, auxiliary) == ISLE_GAME_OK);
        }
        const bool common = self.common_ && source.id == 1003 && source.initData == self.common_ &&
            source.animSetup == &self.common_->setup;
        const bool magikarp = self.magikarp_ && source.id == 129 && source.initData == self.magikarp_ &&
            source.animSetup == &self.magikarp_->setup;
        if (group == 1) return common || magikarp;
        if (group == 2) return common && self.common_assets_;
        if (!magikarp || !self.magikarp_assets_ || !self.common_assets_) return false;
        // The real Magikarp path calls E298 -> E174 -> AddAtGeo -> the common
        // factory, including its later animation/state cleanup. Prove that
        // complete closure before any random yaw or process/spawn/field effect.
        std::size_t ground_node = 0;
        if (isle_actor_owner_ground_node(&source, &ground_node) != ISLE_GAME_OK) return false;
        auto* current = self.world_.current();
        bool block = false;
        for (const auto& candidate : self.world_.blocks())
            if (&candidate == current && candidate.resource) { block = true; break; }
        if (!block) return false;
        if (source.path) {
            bool path = false;
            for (const auto& candidate : self.world_.resources().paths)
                if (&candidate == source.path) { path = true; break; }
            if (!path) return false;
        }
        IsleGameVec3 auxiliary;
        return position(source.game, source.object, Position::auxiliary, auxiliary) == ISLE_GAME_OK;
    }

    static bool callback(IsleGameSourceCallback function, void* context) {
        auto& admission = *static_cast<Admission*>(context);
        if (admission.self.actors_.course_context().course_id()==6) {
            if (!rainbow_callback(function, nullptr)) return false;
            if (!(admission.checked & 1u)) {
                admission.checked |= 1u;
                for (const auto& init : admission.self.resources_.initializations())
                    if (admission.source.initData==&init && admission.source.animSetup==&init.setup &&
                        admission.source.id==static_cast<std::int32_t>(init.actor_id) &&
                        admission.self.admits_initial_species(init, admission.source.path))
                        admission.admitted |= 1u;
            }
            return (admission.admitted & 1u)!=0;
        }
        // Actual compiled source identities, never guest addresses or generic
        // callable registrations. Memoize only this one serialized admission.
        unsigned group;
        if (function == func_8035DFB0_4FE3C0) group = 1;
        else if (function == func_8035DF20_4FE330 || function == func_8035E034_4FE444) group = 2;
        else if (function == func_beach_802CAB50 || function == func_beach_802CAC08 ||
                 function == func_8035E298_4FE6A8) group = 4;
        else if ((group = initial_species_group(function)) == 0 &&
                 (group = remaining_species_group(function)) == 0) return false;
        if (!(admission.checked & group)) {
            admission.checked |= group;
            if (callback_proof(admission, group)) admission.admitted |= group;
        }
        return (admission.admitted & group) != 0;
    }

    static IsleGameResult graph(IsleActorSource* source, std::uint32_t identity,
        const IsleActorInteractionGraph** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](InteractionServices& self) {
            if (self.actors_.course_context().course_id()==6) {
                const auto graphs = rainbow::actor_graphs();
                if (std::find(graphs.begin(), graphs.end(), identity)==graphs.end())
                    return ISLE_GAME_INVALID_ARGUMENT;
                bool admitted = false;
                for (const auto& init : self.resources_.initializations())
                    if (source->initData==&init && source->animSetup==&init.setup &&
                        source->id==static_cast<std::int32_t>(init.actor_id))
                        admitted = self.admits_initial_species(init, source->path);
                if (!admitted) return ISLE_GAME_INVALID_STATE;
                *output = &self.bindings_.graph(identity);
                return ISLE_GAME_OK;
            }
            unsigned group = identity == 0x802ccd60u ? 8u :
                (identity == 0x802ccf14u || identity == 0x802ccf44u) ? 16u :
                identity == 0x802cce1cu ? 32u :
                (identity == 0x802cd010u || identity == 0x802cd040u || identity == 0x802cd064u ||
                 identity == 0x802cd0b4u || identity == 0x802cd0e4u) ? 64u : 0u;
            if (!group) for (const auto candidate : {0x802cdc60u, 0x802cdc80u, 0x802cdca0u,
                    0x802cdcd0u, 0x802cdd40u, 0x802cdd80u, 0x802cde48u, 0x802cde78u,
                    0x802cdef8u, 0x802cdf98u, 0x802cdfd8u})
                if (identity == candidate) { group = 128u; break; }
            if (identity == 0x802cd9d8u) group = 512u;
            if (!group) for (const auto& family : remaining_families)
                for (const auto candidate : family.graphs)
                    if (identity == candidate) { group = family.group; break; }
            if (!group) return ISLE_GAME_INVALID_ARGUMENT;
            Admission admission{self, *source};
            if (!callback_proof(admission, group)) return ISLE_GAME_INVALID_STATE;
            *output = &self.bindings_.graph(identity);
            return ISLE_GAME_OK;
        });
    }

    static IsleGameResult rows(IsleActorSource* source, const IsleActorInteraction** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        try {
            void* value = nullptr;
            auto result = isle_actor_owner_interaction_context(source, &value);
            if (result != ISLE_GAME_OK) return result;
            // Preserve Beach's context-free NULL graph path. A bound context,
            // including Rainbow, must still prove its live canonical course.
            if (!value) return source->transitionGraph ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
            auto& self = *static_cast<InteractionServices*>(value);
            result = validate(self, source);
            if (result != ISLE_GAME_OK) return result;
            if (!source->transitionGraph) return ISLE_GAME_OK;
            if (!self.services_.source_owner_registered(source->object)) return ISLE_GAME_INVALID_STATE;
            IsleGameVec3 root;
            result = position(source->game, source->object, Position::root, root);
            if (result != ISLE_GAME_OK) return result;
            const auto& graph = *source->transitionGraph;
            Admission admission{self, *source};
            if (!self.bindings_.admits_native_graph(graph, callback, &admission)) return ISLE_GAME_INVALID_STATE;
            // Admission covers every executable row, not just the first match
            // or a row whose current distance/timer/flute predicate can win.
            bool needs_player = false, needs_auxiliary = false;
            for (std::size_t i = 0; i < graph.count && graph.rows[i].cmd != 58; ++i) {
                const auto& row = graph.rows[i];
                if (!row.state && !row.aux) continue;
                needs_player |= row.cmd == 5 || row.cmd == 6 || row.cmd == 7 || row.cmd == 16;
                needs_auxiliary |= row.cmd == 15 || row.cmd == 17;
            }
            if (needs_player) {
                IsleGameObject* object = nullptr;
                result = player(self, object);
                if (result != ISLE_GAME_OK) return result;
            }
            if (needs_auxiliary) {
                IsleGameVec3 auxiliary;
                result = position(source->game, source->object, Position::auxiliary, auxiliary);
                if (result != ISLE_GAME_OK) return result;
            }
            *output = graph.rows;
            return ISLE_GAME_OK;
        } catch (...) {
            return ISLE_GAME_INVALID_STATE;
        }
    }

    static IsleGameResult item(InteractionServices& self, IsleGameObject* object,
                               IsleItemSource*& output) noexcept {
        if (!object) return ISLE_GAME_INVALID_ARGUMENT;
        IsleItemSource* source = nullptr;
        const auto result = isle_item_source_get(self.services_.game, object, &source);
        if (result != ISLE_GAME_OK) return result;
        if (!source || self.items_.source(object) != source) return ISLE_GAME_INVALID_STATE;
        output = source;
        return ISLE_GAME_OK;
    }

    static IsleGameResult item_iterator(IsleActorSource* source) noexcept {
        return context(source, [](auto& self) {
            self.items_.init_iterator();
            return ISLE_GAME_OK; // Source cursor initialization is infallible.
        });
    }
    static IsleGameResult item_next(IsleActorSource* source, IsleGameObject** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            auto* object = self.items_.next_valid();
            auto result = item_status(self.items_.query_status());
            if (result != ISLE_GAME_OK) return result;
            if (object) {
                IsleItemSource* item_source = nullptr;
                result = item(self, object, item_source);
                if (result != ISLE_GAME_OK) return result;
            }
            *output = object;
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult item_source(IsleActorSource* source, IsleGameObject* object,
                                     IsleItemSource** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            IsleItemSource* value = nullptr;
            const auto result = item(self, object, value);
            if (result == ISLE_GAME_OK) *output = value;
            return result;
        });
    }
    static IsleGameResult native_position(IsleActorSource* source, IsleGameObject* object,
                                         int auxiliary, IsleGameVec3* output) noexcept {
        if (!output || (auxiliary != 0 && auxiliary != 1)) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            IsleGameVec3 value;
            const auto result = position(self.services_.game, object,
                auxiliary ? Position::auxiliary : Position::root, value);
            if (result == ISLE_GAME_OK) *output = value;
            return result;
        });
    }
    static IsleGameResult eat_apple(IsleActorSource* source) noexcept {
        return context(source, [&](auto& self) { return item_status(self.items_.eat(source->object)); });
    }
    static IsleGameResult send_command(IsleActorSource* source, std::int32_t command,
        IsleGameObject* sender) noexcept {
        return context(source, [&](auto& self) {
            return self.services_.send_command(source->object, command, sender)
                ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
        });
    }
    static IsleGameResult send_addressed(IsleActorSource* source, IsleGameObject* destination,
        std::int32_t command, IsleGameObject* sender) noexcept {
        return context(source, [&](auto& self) {
            return self.services_.send_command(destination, command, sender)
                ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
        });
    }
    static IsleGameResult send_to_link(IsleActorSource* source, std::uint8_t link,
        std::int32_t command, IsleGameObject* sender) noexcept {
        return context(source, [&](auto& self) {
            return self.services_.send_command_to_link(link, command, sender)
                ? ISLE_GAME_OK : ISLE_GAME_EXECUTION_FAILURE;
        });
    }
    static IsleGameResult scene_counter(IsleActorSource* source, std::uint32_t identity,
        std::int32_t** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            Admission admission{self, *source};
            if (identity == 0x802cd060u) {
                if (!callback_proof(admission, 64)) return ISLE_GAME_INVALID_STATE;
                *output = &self.actors_.course_context().beach().pidgey_counter;
            } else {
                if (!callback_proof(admission, 32)) return ISLE_GAME_INVALID_STATE;
                switch (identity) {
                case 0x802ccdfcu: *output = &self.actors_.course_context().beach().lapras_counters[0]; break;
                case 0x802cce00u: *output = &self.actors_.course_context().beach().lapras_counters[1]; break;
                case 0x802cce04u: *output = &self.actors_.course_context().beach().lapras_counters[2]; break;
                default: return ISLE_GAME_INVALID_ARGUMENT;
                }
            }
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult pikachu_temporary(IsleActorSource* source, IsleGameVec3* output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            Admission admission{self, *source};
            if (!callback_proof(admission, 128)) return ISLE_GAME_INVALID_STATE;
            const auto& value = self.actors_.course_context().beach().resources.initial_scalars().pikachu_temporary;
            *output = {value.x, value.y, value.z};
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult weighted(IsleActorSource* source, std::uint32_t identity,
        const IsleActorRandomState** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            if (self.actors_.course_context().course_id()==6) {
                if (identity!=0x8034ae30u) return ISLE_GAME_INVALID_ARGUMENT;
                if (!self.admits_rainbow(source->path)) return ISLE_GAME_INVALID_STATE;
                *output = self.bindings_.weighted_states(identity).rows.data();
                return ISLE_GAME_OK;
            }
            unsigned group = identity == 0x802cde30u ? 128u : 0u;
            if (!group) for (const auto& family : remaining_families)
                for (const auto& list : family.weighted)
                    if (list.identity == identity) { group = family.group; break; }
            if (!group) return ISLE_GAME_INVALID_ARGUMENT;
            Admission admission{self, *source};
            if (!callback_proof(admission, group)) return ISLE_GAME_INVALID_STATE;
            *output = self.bindings_.weighted_states(identity).rows.data();
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult flute(IsleActorSource* source, std::int32_t* output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            *output = self.items_.flute().command();
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult distance(IsleActorSource* source, IsleGameObject* second, float* output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            if (second && self.services_.source_owner_registered(second) && self.services_.source_owner_joined(second)) {
                // Joined registration is actual identity-hold proof, not an
                // error rescue for retiring/foreign/stale/modelless objects.
                // The source consumes the first position before model2==NULL.
                IsleGameVec3 first;
                const auto result = position(source->game, source->object, Position::preferred, first);
                if (result != ISLE_GAME_OK) return result;
                *output = std::numeric_limits<float>::max();
                return ISLE_GAME_OK;
            }
            return isle_actor_spatial_distance(source->game, source->object, second, output);
        });
    }
    static IsleGameResult neighbors(IsleActorSource* source, IsleGameVec3* output, float* magnitude) noexcept {
        if (!output || !magnitude) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto&) {
            IsleGameVec3 direction;
            float length;
            const auto result = isle_actor_spatial_neighbors(source, &direction, &length);
            if (result == ISLE_GAME_OK) { *output = direction; *magnitude = length; }
            return result;
        });
    }
    static IsleGameResult player(IsleActorSource* source, IsleGameObject** output) noexcept {
        if (output) *output = nullptr;
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            IsleGameObject* object = nullptr;
            const auto result = player(self, object);
            if (result == ISLE_GAME_OK) *output = object;
            return result;
        });
    }
    static IsleGameResult focus(IsleActorSource* source, std::uint32_t* timer, IsleGameObject** output) noexcept {
        if (output) *output = nullptr;
        if (!timer || !output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            *timer = self.actors_.player_source().sTimerInFocus;
            // Source23 compares identity only, including a published pointer
            // whose present flag was cleared. Never substitute previous focus.
            *output = reinterpret_cast<IsleGameObject*>(self.detector_.published().object);
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult direction(IsleActorSource* source, std::int32_t* output) noexcept {
        if (!output) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            *output = self.actors_.player_source().gDirectionIndex;
            return ISLE_GAME_OK;
        });
    }
    static IsleGameResult level_progress(IsleActorSource* source, std::int32_t* block_count, float* block_part) noexcept {
        if (!block_count || !block_part) return ISLE_GAME_INVALID_ARGUMENT;
        return context(source, [&](auto& self) {
            const auto& movement = self.actors_.player_source().movement;
            for (const auto& block : self.world_.blocks()) {
                if (&block != movement.block) continue;
                if (block.index > std::size_t(std::numeric_limits<std::int32_t>::max()))
                    return ISLE_GAME_INVALID_STATE;
                *block_count = static_cast<std::int32_t>(block.index);
                *block_part = movement.move_time;
                return ISLE_GAME_OK;
            }
            return ISLE_GAME_INVALID_STATE;
        });
    }

    static IsleGameResult native_weighted(IsleRainbowActorSource* source, std::uint32_t identity,
        const IsleActorRandomState** output) noexcept {
        if (output) *output = nullptr;
        if (!source || !output) return ISLE_GAME_INVALID_ARGUMENT;
        if (identity!=0x8034ae30u) return ISLE_GAME_INVALID_ARGUMENT;
        try {
            if (source->phase!=ISLE_RAINBOW_ACTOR_ACTIVE || !source->nativeGame ||
                source->status!=ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
            IslePlayerSource* player = nullptr;
            if (isle_player_source_context(source->nativeGame, &player)!=ISLE_GAME_OK ||
                !player || player->game!=source->nativeGame || player->status!=ISLE_GAME_OK ||
                !player->native_root) return ISLE_GAME_INVALID_STATE;
            auto& root = *static_cast<isle::player::Root*>(player->native_root);
            if (&root.source()!=player) return ISLE_GAME_INVALID_STATE;
            auto* owner = root.bound_actors();
            if (!owner || &owner->player_source()!=player ||
                owner->course_context().course_id()!=6 ||
                &owner->course_context().rainbow().actors!=source) return ISLE_GAME_INVALID_STATE;
            auto* self = owner->bound_interactions();
            if (!self || &self->actors_!=owner || !self->admits_source())
                return ISLE_GAME_INVALID_STATE;
            // Return the immutable translated Source list. The existing C
            // weighted chooser alone consumes Services' shared RNG; lookup
            // never draws, rewrites weights, or substitutes a native decision.
            *output = self->bindings_.weighted_states(identity).rows.data();
            return ISLE_GAME_OK;
        } catch (...) {
            return ISLE_GAME_INVALID_STATE;
        }
    }
};

bool InteractionServices::admits_initial_species(const ActorInitialization& initialization,
    const void* path) noexcept {
    try {
        if (services_.failed() || world_.retired() || !world_.error().empty()) return false;
        resources_.require_live();
        bindings_.require_live();
        if (actors_.course_context().course_id()==6) {
            bool admitted = false;
            for (const auto* admitted_initialization : rainbow_initializations_)
                admitted |= admitted_initialization==&initialization;
            return admitted && admits_rainbow(path);
        }
        const bool butterfree = &initialization == butterfree_ && butterfree_assets_;
        const bool doduo = &initialization == doduo_ && doduo_assets_;
        const bool lapras = &initialization == lapras_ && lapras_assets_;
        const bool pidgey = &initialization == pidgey_ && pidgey_assets_;
        const bool pikachu = &initialization == pikachu_ && pikachu_assets_;
        const bool sign = &initialization == sign_ && sign_assets_;
        const FamilyAdmission* family = nullptr;
        for (std::size_t i = 0; i < remaining_.size(); ++i)
            if (&initialization == remaining_[i] && remaining_assets_[i]) {
                family = &remaining_families[i];
                break;
            }
        if (!butterfree && !doduo && !lapras && !pidgey && !pikachu && !sign && !family) return false;
        bool current = false;
        for (const auto& block : world_.blocks())
            if (&block == world_.current() && block.resource) { current = true; break; }
        if (!current) return false;
        if (path) {
            bool found = false;
            for (const auto& candidate : world_.resources().paths)
                if (&candidate == path) { found = true; break; }
            if (!found) return false;
        }
        // Original Root Movement_Init creates actors before publishing its
        // Player model and shared FX helpers. Prove the actual prepared Source
        // provider now; scheduled distance/interaction consumers validate the
        // published native model/camera when they run, not during allocation.
        const auto& player = actors_.player_source();
        IslePlayerSource* active = nullptr;
        if (player.game != services_.game ||
            isle_player_source_context(services_.game, &active) != ISLE_GAME_OK ||
            active != &player) return false;
        if (family) {
            unsigned group = family->group;
            for (const auto& binding : family->callbacks)
                if (binding.identity >= family->first_callback && binding.identity <= family->last_callback &&
                    bindings_.callback(binding.identity) != binding.function) return false;
            for (const auto identity : family->graphs)
                if (!bindings_.admits_native_graph(bindings_.graph(identity),
                        remaining_species_callback, &group)) return false;
            for (const auto& list : family->weighted) {
                const auto& weighted = bindings_.weighted_states(list.identity);
                if (weighted.rows.size() != list.callbacks.size() + 1) return false;
                std::int32_t total = 0;
                for (std::size_t i = 0; i < list.callbacks.size(); ++i) {
                    if (weighted.rows[i].weight != list.weights[i] ||
                        weighted.rows[i].func != bindings_.callback(list.callbacks[i]) ||
                        remaining_species_group(weighted.rows[i].func) != group) return false;
                    total += list.weights[i];
                }
                if (weighted.total_weight != total || weighted.rows.back().weight != 0 ||
                    weighted.rows.back().func) return false;
            }
            if (family->id == 1005) {
                // Either weighted outcome spawns through a real original
                // singleton definition. Prove both native family closures.
                const ActorInitialization* scyther = nullptr;
                for (std::size_t i = 0; i < remaining_.size(); ++i)
                    if (remaining_families[i].id == 123) scyther = remaining_[i];
                if (!pidgey_ || !scyther || !admits_initial_species(*pidgey_, path) ||
                    !admits_initial_species(*scyther, path)) return false;
                bool pidgey_definition = false, scyther_definition = false;
                const auto initializations = resources_.initializations();
                for (const auto& definition : resources_.definitions()) {
                    if (!definition.singleton || definition.change_block_identity != 0x80363dbcu ||
                        definition.remove_identity != 0x80364280u ||
                        definition.initialization >= initializations.size()) continue;
                    const auto* init = &initializations[definition.initialization];
                    pidgey_definition |= definition.wire.guest_address == 0x802cd838u &&
                        definition.actor_id == 16 && definition.factory_identity == 0x802c8d74u && init == pidgey_;
                    scyther_definition |= definition.wire.guest_address == 0x802cd848u &&
                        definition.actor_id == 123 && definition.factory_identity == 0x802c9204u &&
                        init == scyther;
                }
                if (!pidgey_definition || !scyther_definition) return false;
            }
            return true;
        }
        // Include initial/path callbacks even though no interaction row points
        // to them. Graph-only admission would miss their executable closure.
        unsigned group = butterfree ? 8u : doduo ? 16u : lapras ? 32u : pidgey ? 64u : pikachu ? 128u : 256u;
        // The registry includes initial/path/aux callbacks, all branch callbacks,
        // and every graph/weighted dependency, not merely today's winning row.
        for (const auto& binding : beach::initial_species_callbacks())
            if (initial_species_group(binding.function) == group &&
                bindings_.callback(binding.identity) != binding.function) return false;
        for (const auto identity : beach::initial_species_graphs()) {
            const bool family_graph = group == 8 ? identity == 0x802ccd60u :
                group == 16 ? (identity == 0x802ccf14u || identity == 0x802ccf44u) :
                group == 32 ? identity == 0x802cce1cu :
                group == 64 ? (identity == 0x802cd010u || identity == 0x802cd040u ||
                    identity == 0x802cd064u || identity == 0x802cd0b4u || identity == 0x802cd0e4u) :
                group == 128 && (identity == 0x802cdc60u || identity == 0x802cdc80u || identity == 0x802cdca0u ||
                    identity == 0x802cdcd0u || identity == 0x802cdd40u || identity == 0x802cdd80u ||
                    identity == 0x802cde48u || identity == 0x802cde78u || identity == 0x802cdef8u ||
                    identity == 0x802cdf98u || identity == 0x802cdfd8u);
            if (family_graph && !bindings_.admits_native_graph(bindings_.graph(identity),
                    initial_species_callback, &group)) return false;
        }
        if (pikachu) {
            const auto& weighted = bindings_.weighted_states(0x802cde30u);
            if (weighted.rows.size() != 3 || weighted.total_weight != 2 ||
                weighted.rows[0].weight != 1 || weighted.rows[0].func != func_beach_802CB36C ||
                weighted.rows[1].weight != 1 || weighted.rows[1].func != func_beach_802CB3DC ||
                weighted.rows[2].func != nullptr) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}
bool InteractionServices::admits_ending(const ActorInitialization& initialization) noexcept {
    try {
        if (services_.failed() || world_.retired() || !world_.error().empty()) return false;
        resources_.require_live();
        bindings_.require_live();
        if (actors_.course_context().course_id()!=0) return false;
        bool member = false;
        for (const auto& init : resources_.initializations()) member |= &init == &initialization;
        if (!member || actors_.admitted_factory(1001, 0x802cab14u)!=&initialization) return false;
        for (const auto identity : {0x802cd980u, 0x802cd994u, 0x802cd9a8u, 0x802cd9bcu}) {
            bool found = false;
            for (const auto& header : resources_.headers())
                if (header.wire.guest_address == identity &&
                    &resources_.initializations()[header.initialization] == &initialization) found = true;
            if (!found) return false;
        }
        unsigned group = 512;
        for (const auto& binding : beach::initial_species_callbacks())
            if (initial_species_group(binding.function) == group &&
                bindings_.callback(binding.identity) != binding.function) return false;
        return bindings_.admits_native_graph(bindings_.graph(0x802cd9d8u),
            initial_species_callback, &group);
    } catch (...) {
        return false;
    }
}
} // namespace isle::actor

extern "C" {
IsleGameResult isle_rainbow_actor_native_weighted(IsleRainbowActorSource* source, uint32_t identity,
    const IsleActorRandomState** output) {
    return isle::actor::InteractionSourceAccess::native_weighted(source, identity, output);
}
IsleGameResult isle_actor_interaction_scene_path(IsleActorSource* source,uint32_t identity,const void*** output) {
    return isle::actor::InteractionSourceAccess::scene_path(source,identity,output);
}
IsleGameResult isle_actor_interaction_eat_apple(IsleActorSource* source) {
    return isle::actor::InteractionSourceAccess::eat_apple(source);
}
IsleGameResult isle_actor_interaction_send_command(IsleActorSource* source, int32_t command, IsleGameObject* sender) {
    return isle::actor::InteractionSourceAccess::send_command(source, command, sender);
}
IsleGameResult isle_actor_interaction_send_addressed(IsleActorSource* source, IsleGameObject* destination,
    int32_t command, IsleGameObject* sender) {
    return isle::actor::InteractionSourceAccess::send_addressed(source,destination,command,sender);
}
IsleGameResult isle_actor_interaction_send_to_link(IsleActorSource* source, uint8_t link, int32_t command, IsleGameObject* sender) {
    return isle::actor::InteractionSourceAccess::send_to_link(source, link, command, sender);
}
IsleGameResult isle_actor_interaction_scene_counter(IsleActorSource* source, uint32_t identity, int32_t** output) {
    return isle::actor::InteractionSourceAccess::scene_counter(source, identity, output);
}
IsleGameResult isle_actor_interaction_pikachu_temporary(IsleActorSource* source, IsleGameVec3* output) {
    return isle::actor::InteractionSourceAccess::pikachu_temporary(source, output);
}
IsleGameResult isle_actor_interaction_weighted(IsleActorSource* source, uint32_t identity, const IsleActorRandomState** output) {
    return isle::actor::InteractionSourceAccess::weighted(source, identity, output);
}
IsleGameResult isle_actor_owner_graph(IsleActorSource* source, uint32_t identity,
    const IsleActorInteractionGraph** output) {
    return isle::actor::InteractionSourceAccess::graph(source, identity, output);
}
IsleGameResult isle_actor_owner_target_position_ref(IsleActorSource* source, IsleGameObject* target,
    IsleActorPositionRef* output) {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = {};
    void* context = nullptr;
    const auto result = isle_actor_owner_interaction_context(source, &context);
    if (result != ISLE_GAME_OK) return result;
    return isle::actor::position_ref(source->game, target, isle::actor::Position::preferred, *output);
}
IsleGameResult isle_actor_owner_position_read(IsleActorSource* source, const IsleActorPositionRef* reference,
    const IsleGameVec3** output) {
    if (output) *output = nullptr;
    if (!reference || !output) return ISLE_GAME_INVALID_ARGUMENT;
    void* context = nullptr;
    const auto result = isle_actor_owner_interaction_context(source, &context);
    if (result != ISLE_GAME_OK) return result;
    return isle::actor::position_read(source->game, *reference, *output);
}
IsleGameResult isle_actor_interaction_rows(IsleActorSource* source, const IsleActorInteraction** output) {
    return isle::actor::InteractionSourceAccess::rows(source, output);
}
IsleGameResult isle_actor_interaction_position(IsleActorSource* source, IsleGameObject* object, int auxiliary, IsleGameVec3* output) {
    return isle::actor::InteractionSourceAccess::native_position(source, object, auxiliary, output);
}
IsleGameResult isle_actor_interaction_item_iterator(IsleActorSource* source) {
    return isle::actor::InteractionSourceAccess::item_iterator(source);
}
IsleGameResult isle_actor_interaction_item_next(IsleActorSource* source, IsleGameObject** output) {
    return isle::actor::InteractionSourceAccess::item_next(source, output);
}
IsleGameResult isle_actor_interaction_item_source(IsleActorSource* source, IsleGameObject* object, IsleItemSource** output) {
    return isle::actor::InteractionSourceAccess::item_source(source, object, output);
}
IsleGameResult isle_actor_interaction_flute(IsleActorSource* source, int32_t* output) {
    return isle::actor::InteractionSourceAccess::flute(source, output);
}
IsleGameResult isle_actor_interaction_distance(IsleActorSource* source, IsleGameObject* second, float* output) {
    return isle::actor::InteractionSourceAccess::distance(source, second, output);
}
IsleGameResult isle_actor_interaction_neighbors(IsleActorSource* source, IsleGameVec3* output, float* magnitude) {
    return isle::actor::InteractionSourceAccess::neighbors(source, output, magnitude);
}
IsleGameResult isle_actor_interaction_player(IsleActorSource* source, IsleGameObject** output) {
    return isle::actor::InteractionSourceAccess::player(source, output);
}
IsleGameResult isle_actor_interaction_focus(IsleActorSource* source, uint32_t* timer, IsleGameObject** output) {
    return isle::actor::InteractionSourceAccess::focus(source, timer, output);
}
IsleGameResult isle_actor_interaction_direction(IsleActorSource* source, int32_t* output) {
    return isle::actor::InteractionSourceAccess::direction(source, output);
}
IsleGameResult isle_actor_interaction_level_progress(IsleActorSource* source, int32_t* block_count, float* block_part) {
    return isle::actor::InteractionSourceAccess::level_progress(source, block_count, block_part);
}
}
