#include "actor_interaction_services.hpp"
#include "actor_behavior_bindings.hpp"
#include "actor_owner.hpp"
#include "actor_spatial.h"
#include "item_owner.hpp"
#include "beach_initial_species_source.h"
#include "35mmisle/focus_detector.hpp"

#include <limits>
#include <stdexcept>

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

const beach::ActorInitialization* factory(const beach::ActorResources& resources,
    std::uint32_t id, std::uint32_t factory_identity, std::uint32_t init_identity,
    std::uint32_t render, std::uint32_t setup, std::uint32_t state, std::uint32_t header) {
    const auto initializations = resources.initializations();
    const auto headers = resources.headers();
    const auto models = resources.models();
    for (const auto& definition : resources.definitions()) {
        if (definition.actor_id != id || definition.factory_identity != factory_identity ||
            (id == 1003 && definition.wire.guest_address != 0x8038837cu) ||
            definition.initialization >= initializations.size()) continue;
        const auto& init = initializations[definition.initialization];
        if (init.actor_id == id && init.wire.guest_address == init_identity &&
            init.render_callback_identity == render && init.setup.wire.guest_address == setup &&
            init.setup.initial_state_identity == state && !init.setup.transition_graph_identity &&
            !init.setup.unknown_0c_identity && init.setup.process_kind == 0 && init.animations_enabled &&
            init.model < models.size() && init.setup.initial_header < headers.size() &&
            headers[init.setup.initial_header].initialization == definition.initialization &&
            headers[init.setup.initial_header].wire.guest_address == header) return &init;
    }
    return nullptr;
}

unsigned initial_species_group(IsleGameSourceCallback function) noexcept {
    if (function == func_beach_802C7B10 || function == func_beach_802C7BB8 ||
        function == func_beach_802C7C7C || function == func_beach_802C7CF0 ||
        function == func_beach_802C7DA0 || function == func_beach_802C7DEC) return 8;
    if (function == func_beach_802C82B0 || function == func_beach_802C8354 ||
        function == func_beach_802C83D0 || function == func_beach_802C8438 ||
        function == func_beach_802C84A4) return 16;
    return 0;
}
bool initial_species_callback(IsleGameSourceCallback function, void* context) noexcept {
    return initial_species_group(function) == *static_cast<const unsigned*>(context);
}

bool header_assets(const beach::ActorResources& resources, Owner& actors,
    const beach::ActorInitialization* initialization, std::uint32_t identity) {
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
            if (index == beach::actor_no_index) return true;
            if (index >= animations.size()) return false;
            for (const auto& command : animations[index].commands) {
                const auto category = command.mask >> 8;
                const auto value = command.mask & 255u;
                if (command.opcode != 16 || category > 3 || !value) continue;
                if (model.nodes[node].parent == beach::actor_no_index) {
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
} // namespace

InteractionServices::InteractionServices(scene::Services& services, Owner& actors,
    items::Owner& items, world::World& world, focus::Detector& detector,
    const BehaviorBindings& bindings, const beach::ActorResources& resources)
    : services_(services), actors_(actors), items_(items), world_(world), detector_(detector),
      bindings_(bindings), resources_(resources) {
    resources.require_live();
    bindings.require_live();
    if (services.failed() || world.retired() || !world.error().empty() || !items.matches(services, world, actors))
        throw std::invalid_argument("Interaction context requires matching live native service owners");
    common_ = factory(resources, 1003, 0x8035e0d4u, 0x80388348u, 0x800a1608u,
        0x80388334u, 0x8035df20u, 0x80388310u);
    magikarp_ = factory(resources, 129, 0x802cac48u, 0x802cdaecu, 0x803594dcu,
        0x802cdad8u, 0x802cab50u, 0x802cda8cu);
    butterfree_ = factory(resources, 12, 0x802c7e38u, 0x802ccda4u, 0x803594dcu,
        0x802ccd90u, 0x802c7b10u, 0x802ccd24u);
    doduo_ = factory(resources, 84, 0x802c8570u, 0x802ccf78u, 0x803594dcu,
        0x802ccf64u, 0x802c82b0u, 0x802cceb0u);
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
}

bool InteractionServices::matches(Owner& actors, scene::Services& services,
    const beach::ActorResources& resources, world::World& world, focus::Detector& detector) const noexcept {
    return &actors_ == &actors && &services_ == &services && &resources_ == &resources &&
        &world_ == &world && &detector_ == &detector;
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
        IslePlayerSource* active = nullptr;
        if (isle_player_source_context(self.services_.game, &active) != ISLE_GAME_OK ||
            active != &self.actors_.player_source()) return ISLE_GAME_INVALID_STATE;
        self.resources_.require_live();
        self.bindings_.require_live();
        return ISLE_GAME_OK;
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
        if (group == 8 || group == 16) {
            const auto* init = group == 8 ? self.butterfree_ : self.doduo_;
            IsleGameVec3 auxiliary;
            return init && source.id == (group == 8 ? 12 : 84) &&
                source.initData == init && source.animSetup == &init->setup &&
                self.admits_initial_species(*init, source.path) &&
                position(source.game, source.object, Position::auxiliary, auxiliary) == ISLE_GAME_OK;
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
        // Actual compiled source identities, never guest addresses or generic
        // callable registrations. Memoize only this one serialized admission.
        unsigned group;
        if (function == func_8035DFB0_4FE3C0) group = 1;
        else if (function == func_8035DF20_4FE330 || function == func_8035E034_4FE444) group = 2;
        else if (function == func_beach_802CAB50 || function == func_beach_802CAC08 ||
                 function == func_8035E298_4FE6A8) group = 4;
        else if ((group = initial_species_group(function)) == 0) return false;
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
            const unsigned group = identity == 0x802ccd60u ? 8u :
                (identity == 0x802ccf14u || identity == 0x802ccf44u) ? 16u : 0u;
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
            // The getter validates the canonical Source even with no bound
            // context. NULL graphs preserve the old context-free native path.
            if (!source->transitionGraph) return ISLE_GAME_OK;
            if (!value) return ISLE_GAME_INVALID_STATE;
            auto& self = *static_cast<InteractionServices*>(value);
            result = validate(self, source);
            if (result != ISLE_GAME_OK) return result;
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
};

bool InteractionServices::admits_initial_species(const beach::ActorInitialization& initialization,
    const void* path) noexcept {
    try {
        if (services_.failed() || world_.retired() || !world_.error().empty()) return false;
        resources_.require_live();
        bindings_.require_live();
        const bool butterfree = &initialization == butterfree_ && butterfree_assets_;
        const bool doduo = &initialization == doduo_ && doduo_assets_;
        if (!butterfree && !doduo) return false;
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
        // Include initial/path callbacks even though no interaction row points
        // to them. Graph-only admission would miss their executable closure.
        const auto bound = [&](std::uint32_t identity, IsleGameSourceCallback function) {
            return bindings_.callback(identity) == function;
        };
        unsigned group = butterfree ? 8u : 16u;
        if (butterfree) {
            if (!bound(0x802c7b10u, func_beach_802C7B10) ||
                !bound(0x802c7bb8u, func_beach_802C7BB8) ||
                !bound(0x802c7c7cu, func_beach_802C7C7C) ||
                !bound(0x802c7cf0u, func_beach_802C7CF0) ||
                !bound(0x802c7da0u, func_beach_802C7DA0) ||
                !bound(0x802c7decu, func_beach_802C7DEC)) return false;
            return bindings_.admits_native_graph(bindings_.graph(0x802ccd60u),
                initial_species_callback, &group);
        }
        if (!bound(0x802c82b0u, func_beach_802C82B0) ||
            !bound(0x802c8354u, func_beach_802C8354) ||
            !bound(0x802c83d0u, func_beach_802C83D0) ||
            !bound(0x802c8438u, func_beach_802C8438) ||
            !bound(0x802c84a4u, func_beach_802C84A4)) return false;
        return bindings_.admits_native_graph(bindings_.graph(0x802ccf14u),
                   initial_species_callback, &group) &&
            bindings_.admits_native_graph(bindings_.graph(0x802ccf44u),
                initial_species_callback, &group);
    } catch (...) {
        return false;
    }
}
} // namespace isle::actor

extern "C" {
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
}
