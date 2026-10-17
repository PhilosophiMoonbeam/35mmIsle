#include "player_bump.hpp"
#include <exception>
#include <stdexcept>

namespace isle::player {
BumpOwner::BumpOwner(scene::Services& services, IslePlayerSource& player)
    : services_(services) {
    if (services.game != player.game ||
        isle_player_bump_source_bind(&player, &source_, this) != ISLE_GAME_OK)
        throw std::invalid_argument("BumpOwner requires the sole prepared canonical Player context");
}

BumpOwner::~BumpOwner() {
    if (retire() != ISLE_GAME_OK) std::terminate();
    if (source_.player->bump != &source_) std::terminate();
    source_.player->bump = nullptr;
}

void BumpOwner::joined(IsleGameObject* object, void* user) noexcept {
    auto& self = *static_cast<BumpOwner*>(user);
    if (self.source_.BumpDetector_Object == object)
        self.source_.BumpDetector_Object = nullptr;
    // A registered sender retains its Services record/native identity until
    // actual command/field borrowers close and outside-OM collection succeeds.
    if (!self.registered_ && self.identity_ == object) self.identity_ = nullptr;
}

IsleGameResult BumpOwner::object(std::uint32_t id, std::uint8_t link,
    std::uint32_t priority, IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output = nullptr;
    if (identity_ || source_.BumpDetector_Object) return ISLE_GAME_INVALID_STATE;
    auto* object = services_.object(id, link, priority);
    if (!object) return ISLE_GAME_EXHAUSTED;
    // Leave the actual Services default command dispatcher intact. This
    // observer composes with its retirement/queue hooks rather than replacing.
    if (!services_.set_lifecycle(object, {nullptr, joined, this})) {
        services_.erase(object); // No processes or borrowed sender were published.
        return ISLE_GAME_INVALID_STATE;
    }
    identity_ = object;
    if (!services_.source_owner_register(object, {})) {
        services_.erase(object);
        return ISLE_GAME_INVALID_STATE;
    }
    registered_ = true;
    *output = object;
    return ISLE_GAME_OK;
}

IsleGameResult BumpOwner::command(IsleGameObject* receiver, std::int32_t command,
    IsleGameObject* sender) noexcept {
    IslePlayerSource* bound = nullptr;
    if (receiver != source_.player->object || sender != source_.BumpDetector_Object ||
        sender != identity_ || !registered_ ||
        isle_player_source_get(services_.game, receiver, &bound) != ISLE_GAME_OK ||
        bound != source_.player || !services_.source_owner_registered(receiver))
        return ISLE_GAME_INVALID_STATE;
    return services_.send_command(receiver, command, sender)
        ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}

IsleGameResult BumpOwner::erase(IsleGameObject* object) noexcept {
    if (!object || object != identity_ || object != source_.BumpDetector_Object)
        return ISLE_GAME_INVALID_ARGUMENT;
    services_.erase(object);
    // Self-delete is only requested here. Its source pointer is deliberately
    // still present until Services invokes the actual joined observer.
    return services_.failed() ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
}

IsleGameResult BumpOwner::retire() noexcept {
    if (!identity_) return source_.BumpDetector_Object ? ISLE_GAME_INVALID_STATE : ISLE_GAME_OK;
    if (!services_.source_owner_joined(identity_)) services_.erase(identity_);
    if (source_.BumpDetector_Object) return ISLE_GAME_INVALID_STATE;
    if (registered_) {
        if (!services_.source_owner_collectable(identity_) ||
            !services_.source_owner_release(identity_))
            return ISLE_GAME_INVALID_STATE;
        registered_ = false;
    } else if (identity_) {
        // Only the actual joined callback may drop an unregistered identity.
        return ISLE_GAME_INVALID_STATE;
    }
    identity_ = nullptr;
    return ISLE_GAME_OK;
}
}

namespace {
isle::player::BumpOwner* owner(IslePlayerBumpSource* source) noexcept {
    if (!source || !source->owner) return nullptr;
    auto* owner = static_cast<isle::player::BumpOwner*>(source->owner);
    return &owner->source() == source ? owner : nullptr;
}
}
extern "C" {
IsleGameResult isle_player_bump_native_object(IslePlayerBumpSource* source, uint32_t id,
    uint8_t link, uint32_t priority, IsleGameObject** output) {
    auto* native = owner(source);
    if (!native) {
        if (output) *output = nullptr;
        return ISLE_GAME_INVALID_ARGUMENT;
    }
    return native->object(id, link, priority, output);
}
IsleGameResult isle_player_bump_native_command(IslePlayerBumpSource* source,
    IsleGameObject* receiver, int32_t command, IsleGameObject* sender) {
    auto* native = owner(source);
    return native ? native->command(receiver, command, sender) : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult isle_player_bump_native_delete(IslePlayerBumpSource* source, IsleGameObject* object) {
    auto* native = owner(source);
    return native ? native->erase(object) : ISLE_GAME_INVALID_ARGUMENT;
}
}
