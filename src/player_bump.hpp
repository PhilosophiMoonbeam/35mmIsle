#ifndef ISLE_PLAYER_BUMP_HPP
#define ISLE_PLAYER_BUMP_HPP

#include "player_bump_source.h"
#include "scene_services.hpp"

namespace isle::player {
/* Stable native ownership only: the complete admitted C Update owns all bump
 * arithmetic and continuation state. Construct after Player prepare and before
 * original initialization/OM. Services, Player and every actual ActorSource on
 * link3 must outlive all detector callbacks; link3 is Actor-only as in GAME.
 *
 * Parent integration: compile the private prepare_player_bump output together
 * with player_bump_source.c, player_bump.cpp, ActorSource and Services. Original
 * Init creates ID8/link0/priority0x80000000, with Services' default command drain,
 * then installs the actual kind0/priority9 C Update. The canonical Player root
 * must be bound and registered with the SAME Services before the first tick.
 * Its real command handler consumes PLAYER_CMD_BUMP (4) with detector sender.
 *
 * Retirement order: stop/join the original detector (Delete or retire), finish
 * Player/other queue and canonical-field sender borrowers, then retire() outside
 * OM to collect the retained identity. Do not deactivate/destroy Player, Actors,
 * this owner, or Services beforehand. Real joined is not any external fence. */
class BumpOwner final {
public:
    BumpOwner(scene::Services&, IslePlayerSource&);
    ~BumpOwner();
    BumpOwner(const BumpOwner&) = delete;
    BumpOwner& operator=(const BumpOwner&) = delete;
    IslePlayerBumpSource& source() noexcept { return source_; }

    IsleGameResult object(std::uint32_t id, std::uint8_t link, std::uint32_t priority,
                          IsleGameObject**) noexcept;
    IsleGameResult command(IsleGameObject* receiver, std::int32_t command,
                           IsleGameObject* sender) noexcept;
    IsleGameResult erase(IsleGameObject*) noexcept;
    /* May delete/join now, but never invents queue/field completion. Failure
     * retains this owner and identity; collect after real borrowers close.
     * Successful collection permits a later original Init on this same record.
     * Original Delete/retirement do not reset IsEnabled or proximity. */
    IsleGameResult retire() noexcept;
private:
    static void joined(IsleGameObject*, void*) noexcept;
    scene::Services& services_;
    IslePlayerBumpSource source_{};
    IsleGameObject* identity_ = nullptr; /* Retained sender, even after joined. */
    bool registered_ = false;
};
}
#endif
