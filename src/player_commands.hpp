#ifndef ISLE_PLAYER_COMMANDS_HPP
#define ISLE_PLAYER_COMMANDS_HPP
#include "player_commands_source.h"

namespace isle::scene { class Services; }
namespace isle::player::commands {
/* A nonowning domain link, not an interpreter or a receiver registry. Services,
 * its native game and the active canonical Player Source outlive this binding.
 * Callers register receivers with Services and retain every original C callback
 * and its dependencies until actual native retirement/join. Before detach,
 * close every command dispatch and join ALL callbacks that can use this domain;
 * neither a Source failure nor an empty FIFO is evidence of such a join. */
class Binding final {
    friend struct SourceAccess;
public:
    Binding(scene::Services&, IslePlayerSource&);
    ~Binding();
    Binding(const Binding&) = delete;
    Binding& operator=(const Binding&) = delete;
    // Outside native traversal, after the real joins described above. Remains
    // available after Source/Services failure. Destruction requires detach.
    IsleGameResult detach() noexcept;
    IslePlayerCommandSource* source() noexcept { return &source_; }
    const IslePlayerCommandSource* source() const noexcept { return &source_; }
private:
    scene::Services& services_;
    IslePlayerCommandSource source_{};
};
}
#endif
