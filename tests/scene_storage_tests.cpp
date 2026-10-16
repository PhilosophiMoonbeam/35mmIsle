#include "scene_storage.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace sc = isle::scene;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
struct Process {
    sc::Storage* storage = nullptr;
    unsigned entered = 0, resumed = 0;
    IsleGameResult destroy = ISLE_GAME_OK, reset = ISLE_GAME_OK, reenter = ISLE_GAME_OK;
};
void waiting(IsleGameObject*, void* opaque) {
    auto& process = *static_cast<Process*>(opaque);
    ++process.entered;
    process.destroy = process.storage->destroy();
    process.reset = process.storage->reset();
    process.reenter = process.storage->reenter();
    while (isle_game_wait(process.storage->game(),1) == ISLE_GAME_OK) ++process.resumed;
}
void joined_reentry_reuses_the_backing_allocation() {
    sc::Storage storage(256*1024);
    auto* base = storage.arena().storage.data;
    check(storage.game() == nullptr && storage.capacity() == 256*1024 && storage.arena().cursor == 0,
          "bare Storage allocates one arena without prematurely creating native scene or profile");
    check(storage.create() == ISLE_GAME_OK && storage.game() != nullptr,"create publishes actual native owner");
    const auto created_cursor = storage.arena().cursor;
    check(storage.create() == ISLE_GAME_INVALID_STATE && storage.reset() == ISLE_GAME_INVALID_STATE &&
          storage.arena().cursor == created_cursor,"live world creation/reset reject without rewinding active storage");
    Process process{&storage};
    IsleGameObject* object = nullptr;
    IsleGameProcess* native_process = nullptr;
    check(isle_game_add_object(storage.game(),14,0,0x80000000,nullptr,nullptr,&object) == ISLE_GAME_OK &&
          isle_game_add_process(storage.game(),object,waiting,&process,ISLE_GAME_STACKFUL,1,&native_process) == ISLE_GAME_OK,
          "attach real waiting continuation to bare Storage");
    check(isle_game_tick(storage.game()) == ISLE_GAME_OK && process.entered == 1 && process.resumed == 0 &&
          process.destroy == ISLE_GAME_INVALID_STATE && process.reset == ISLE_GAME_INVALID_STATE &&
          process.reenter == ISLE_GAME_INVALID_STATE,"destroy/reset/reentry reject active native traversal without fake join");
    check(storage.reenter() == ISLE_GAME_OK && storage.game() != nullptr && process.resumed == 0 &&
          storage.arena().storage.data == base && storage.arena().cursor == created_cursor,
          "outside-traversal reentry joins suspended process, resets arena and creates native world on same backing");
    IsleGameCounts counts{};
    check(isle_game_counts(storage.game(),&counts) == ISLE_GAME_OK && counts.objects == 0 && counts.processes == 0,
          "new native owner contains no retired object/process borrowers");
    check(storage.destroy() == ISLE_GAME_OK && storage.game() == nullptr && storage.destroy() == ISLE_GAME_OK &&
          storage.reset() == ISLE_GAME_OK && storage.arena().cursor == 0 && storage.arena().storage.data == base,
          "explicit idempotent joined destroy and closed reset preserve the one backing allocation");
}
void failed_admission_never_publishes_native_owner() {
    sc::Storage storage(1);
    auto* base = storage.arena().storage.data;
    check(storage.create() == ISLE_GAME_EXHAUSTED && storage.game() == nullptr && storage.arena().cursor == 0 &&
          storage.destroy() == ISLE_GAME_OK && storage.reset() == ISLE_GAME_OK && storage.arena().storage.data == base,
          "real native allocation rejection publishes neither game nor partial arena cursor");
}
} // namespace
static_assert(!std::is_copy_constructible_v<sc::Storage> && !std::is_move_constructible_v<sc::Storage>);
int main() {
    joined_reentry_reuses_the_backing_allocation();
    failed_admission_never_publishes_native_owner();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
