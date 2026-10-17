#include "player_commands.hpp"
#include "scene_services.hpp"
#include "menu_scene_bridge.h"
#include <exception>
#include <stdexcept>

namespace isle::player::commands {
struct SourceAccess {
    static Binding* binding(IslePlayerCommandSource* source) noexcept {
        if (!source || !source->owner || !source->player) return nullptr;
        auto* self=static_cast<Binding*>(source->owner);
        const auto* player=source->player;
        return &self->source_==source && player->commands==source &&
            player->game==self->services_.game && player->owner && player->rng ? self : nullptr;
    }
    static IsleGameResult ready(Binding& self) noexcept {
        IslePlayerSource* canonical=nullptr;
        const auto result=isle_player_source_context(self.services_.game,&canonical);
        if (result!=ISLE_GAME_OK) return result;
        if (canonical!=self.source_.player || self.services_.failed()) return ISLE_GAME_INVALID_STATE;
        return ISLE_GAME_OK;
    }
    static scene::Services& services(Binding& self) noexcept { return self.services_; }
};
Binding::Binding(scene::Services& services,IslePlayerSource& player):services_(services) {
    IslePlayerSource* canonical=nullptr;
    if (player.commands || player.game!=services.game || services.failed() ||
        isle_player_source_context(services.game,&canonical)!=ISLE_GAME_OK || canonical!=&player)
        throw std::invalid_argument("Player commands require actual Services and its active unbound canonical Source");
    source_={&player,this};
    player.commands=&source_;
}
Binding::~Binding() {
    // Clearing the link here would conceal callbacks that still borrow it.
    if (source_.owner || source_.player) std::terminate();
}
IsleGameResult Binding::detach() noexcept {
    if (SourceAccess::binding(&source_)!=this || source_.player->nonwaiting_call_frame ||
        isle_menu_current_object(services_.game)) return ISLE_GAME_INVALID_STATE;
    IsleGameCounts counts{};
    const auto result=isle_game_counts(services_.game,&counts);
    if (result!=ISLE_GAME_OK) return result;
    // Native joins are a caller precondition: this binding deliberately has no
    // duplicate receiver registry or guessed outstanding-callback counter.
    source_.player->commands=nullptr;
    source_={};
    return ISLE_GAME_OK;
}
}

extern "C" {
IsleGameResult isle_player_commands_backend_begin(IslePlayerCommandSource* source,
    IsleGameObject* object,IsleSourceCommandScope* scope) {
    if (!scope) return ISLE_GAME_INVALID_ARGUMENT;
    *scope={};
    auto* self=isle::player::commands::SourceAccess::binding(source);
    if (!self) return ISLE_GAME_INVALID_STATE;
    const auto ready=isle::player::commands::SourceAccess::ready(*self);
    if (ready!=ISLE_GAME_OK) return ready;
    return isle::player::commands::SourceAccess::services(*self).source_commands_begin(object,*scope)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_commands_backend_end(IslePlayerCommandSource* source,
    IsleSourceCommandScope* scope,IsleGameResult first,int delete_requested) {
    if (!scope) return first!=ISLE_GAME_OK?first:ISLE_GAME_INVALID_ARGUMENT;
    auto* self=isle::player::commands::SourceAccess::binding(source);
    if (!self) return first!=ISLE_GAME_OK?first:ISLE_GAME_INVALID_STATE;
    auto& services=isle::player::commands::SourceAccess::services(*self);
    IsleGameCounts counts{};
    const auto access=isle_game_counts(services.game,&counts);
    if (access!=ISLE_GAME_OK) return first!=ISLE_GAME_OK?first:access;
    // Do NOT use ready(): Source.checked has already latched its failure. The
    // real owned scope must close before that failure transfers out of C.
    const bool closed=services.source_commands_end(*scope,first,delete_requested);
    if (first!=ISLE_GAME_OK) return first;
    if (source->player->status!=ISLE_GAME_OK) return source->player->status;
    const auto native=isle_game_status(services.game);
    if (native!=ISLE_GAME_OK) return native;
    return closed?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_commands_backend_send(IslePlayerCommandSource* source,
    IsleGameObject* destination,int32_t command,IsleGameObject* sender) {
    auto* self=isle::player::commands::SourceAccess::binding(source);
    if (!self) return ISLE_GAME_INVALID_STATE;
    const auto ready=isle::player::commands::SourceAccess::ready(*self);
    if (ready!=ISLE_GAME_OK) return ready;
    return isle::player::commands::SourceAccess::services(*self).send_command(destination,command,sender)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
IsleGameResult isle_player_commands_backend_send_to_link(IslePlayerCommandSource* source,
    uint8_t link,int32_t command,IsleGameObject* sender) {
    auto* self=isle::player::commands::SourceAccess::binding(source);
    if (!self) return ISLE_GAME_INVALID_STATE;
    const auto ready=isle::player::commands::SourceAccess::ready(*self);
    if (ready!=ISLE_GAME_OK) return ready;
    return isle::player::commands::SourceAccess::services(*self).send_command_to_link(link,command,sender)?
        ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
}
}
