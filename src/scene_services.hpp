#ifndef ISLE_SCENE_SERVICES_HPP
#define ISLE_SCENE_SERVICES_HPP

#include "35mmisle/engine.h"
#include "35mmisle/game_input.hpp"
#include "35mmisle/game_save.hpp"
#include "35mmisle/rng.h"
#include "game_audio.hpp"
#include "game_state.hpp"
#include "scene_resources.hpp"
#include "sprite_producer.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace isle::menu::model { struct CameraBasisState; }
namespace isle::scene {
struct SpriteHandle { IsleGameObject* object = nullptr; IsleGameSprite* sprite = nullptr; };
struct FadeView {
    bool visible = false, running = false;
    std::array<std::uint8_t, 3> rgb{};
    std::uint8_t alpha = 0;
};
struct RenderMetadata {
    std::uint8_t link = 0;
    std::uint32_t priority = 0x80000000u, camera_tag = 0;
};
struct SpriteDraw {
    sprite::Draw draw;
    IsleGameObject* object = nullptr;
    RenderMetadata render;
};
// Observers compose with Services-owned native hooks; they never replace queue/
// record ownership. Retiring sees intact payload/processes; joined sees actual
// joins and payload removal, not graphics completion. Neither may throw, wait or
// mutate OM. Userdata outlives both callbacks and enclosing command dispatch.
struct ObjectLifecycle {
    IsleGameCallback retiring = nullptr, joined = nullptr;
    void* user = nullptr;
};
// Concrete scene-owned native records. ResourceView, State, input/audio, game
// and arena are borrowed stable owners. Destruction is outside OM traversal and
// after actual graphics borrowers retire; all native processes are joined before
// callback userdata/queues are recycled. Pending external/manual dispatch must
// finish before destruction; an unproven join/caller boundary terminates rather
// than free live callback storage. No per-scene audio drain or scheduler.
class Services final {
public:
    Services(IsleGame*, isle_arena&, ResourceView, game_input::Service&,
             game_audio::Session&, ::isle::game::State&);
    ~Services();
    Services(const Services&) = delete;
    Services& operator=(const Services&) = delete;
    IsleGame* const game;
    ::isle::game::State& state;
    game_input::Service& input;
    game_audio::Session& audio;
    save::GameSession& save;
    isle_rng& rng;
    int& controller;
    std::uint8_t& start_flags;
    std::uint32_t raw_game_time = 0;

    // App-lifetime source camera globals; scene/resource retirement never resets
    // retained matrices. Source render reset changes only the selector.
    ::isle::menu::model::CameraBasisState& camera_basis() noexcept;
    bool failed() const noexcept;
    const char* error() const noexcept;
    void fail(const char*) noexcept;
    bool check(IsleGameResult, const char*) noexcept;
    bool check(::isle::audio::Error, const char*) noexcept;
    // Active native stackful process only; no owning C++ locals across wait.
    bool wait(std::uint32_t updates = 1) noexcept;
    IsleGameObject* object(std::uint32_t id, std::uint8_t update_link = 0,
                          std::uint32_t priority = 0x80000000u) noexcept;
    bool set_update(IsleGameObject*, IsleGameCallback, void*) noexcept;
    // Registered live owners only, before Services erase/actual retirement.
    // All-null clears observers even after sticky scene failure, after validating
    // the live owner; failure remains set and new observers still reject.
    // Rejection leaves old observers intact.
    bool set_lifecycle(IsleGameObject*, ObjectLifecycle) noexcept;
    bool set_command_handler(IsleGameObject*, IsleGameCommandHandler, void*) noexcept;
    // Borrow accounting only, after a successful FIFO enqueue. Observer must
    // not throw/wait/mutate OM or enqueue recursively. Retiring lifecycle closes
    // remaining queue borrows when native deletion discards the receiver FIFO.
    bool set_command_queued_observer(IsleGameObject*, IsleGameCommandHandler, void*) noexcept;
    bool process_commands(IsleGameObject* = nullptr) noexcept;
    bool send_command(IsleGameObject*, std::int32_t, IsleGameObject* source = nullptr) noexcept;
    bool send_command_to_link(std::uint8_t, std::int32_t, IsleGameObject* source = nullptr) noexcept;
    IsleGameProcess* process(IsleGameObject*, IsleGameCallback, void*,
                            IsleGameProcessKind kind = ISLE_GAME_STACKFUL,
                            std::uint32_t priority = 1) noexcept;
    void end_process(IsleGameProcess*) noexcept;
    void end_processes(IsleGameObject*) noexcept;
    void erase(IsleGameObject*) noexcept;
    bool set_render(IsleGameObject*, RenderMetadata) noexcept;
    bool render_metadata(IsleGameObject*, RenderMetadata&) const noexcept;
    // Head inserts before equal DL priorities; tail inserts after equal priorities.
    bool move_to_head(IsleGameObject*) noexcept;
    bool move_to_tail(IsleGameObject*) noexcept;

    SpriteHandle sprite(std::uint32_t identity, std::uint8_t render_link = 1,
                        std::uint32_t render_priority = 0x80000000u,
                        std::uint32_t source_object_id = 31) noexcept;
    IsleGameSprite* append_sprite(IsleGameObject*, std::uint32_t identity) noexcept;
    IsleGameSprite* append_sprite(IsleGameObject*, std::size_t producer_resource,
                                 const IsleSprite&, std::uint64_t generation = 0) noexcept;
    // Full source descriptor replacement. Explicit producer resource index also
    // admits a registered dynamic Raster view without mutating immutable banks.
    bool rebind_sprite(IsleGameSprite*, std::uint32_t identity) noexcept;
    bool replace_sprite(IsleGameSprite*, std::size_t producer_resource, const IsleSprite&, std::uint64_t generation = 0) noexcept;
    bool remove_sprites(IsleGameObject*) noexcept;
    IsleSprite* view(IsleGameSprite*) noexcept;
    std::size_t sprite_count() const noexcept;
    bool sprite_draws(std::span<::isle::sprite::Draw>, std::size_t& written) noexcept;
    bool sprite_draws(std::span<SpriteDraw>, std::size_t& written) noexcept;
    std::span<const SpriteResource> sprite_resources() const noexcept;
    ResourceView resources() const noexcept;

    bool begin_fade(bool reveal, float seconds, std::array<std::uint8_t,3> rgb,
                    bool delayed_delete = false) noexcept;
    FadeView fade() const noexcept;
    const IsleGameModelTransform* fade_transform() const noexcept;
    int play_sound(std::uint32_t sound, game_audio::Parameters = {}) noexcept;
    bool ramp_sound_volume(int slot, std::uint32_t from, std::uint32_t to, std::int32_t updates) noexcept;
    bool ramp_sound_pan(int slot, std::uint32_t from, std::uint32_t to, std::int32_t updates) noexcept;
    void stop_sound_ramps() noexcept;
    bool fade_all_sounds(std::int32_t updates) noexcept;
    void request_exit(std::int32_t source_result) noexcept;
    bool exit_requested() const noexcept;
    std::int32_t exit_result() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::scene
#endif
