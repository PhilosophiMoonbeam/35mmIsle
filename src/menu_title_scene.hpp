#ifndef ISLE_MENU_TITLE_SCENE_HPP
#define ISLE_MENU_TITLE_SCENE_HPP

#include "scene_services.hpp"
#include "menu_model_producer.hpp"
#include "menu_effects.hpp"

namespace isle::menu::scene {
// Actual A0BE70 title processes, not a fixed-duration replacement. play() runs
// inside a native stackful process; its concurrent model/camera/effect/audio
// processes retain source OM ordering and finish on actual callbacks/input.
class Title final {
public:
    explicit Title(::isle::scene::Services&, const Catalog&);
    ~Title();
    Title(const Title&) = delete;
    Title& operator=(const Title&) = delete;
    bool play() noexcept;
    bool active() const noexcept;
    std::int32_t source_state() const noexcept;
    float source_time() const noexcept;
    IsleGameCamera* camera() const noexcept;
    animation::Runtime* animation() noexcept;
    model::Producer* models() noexcept;
    MenuEffects* effects() noexcept;
    // Parent must stop callbacks and retire every view before scene destruction.
    void retire() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::menu::scene
#endif
