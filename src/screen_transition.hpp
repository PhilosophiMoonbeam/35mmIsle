#ifndef ISLE_SCREEN_TRANSITION_HPP
#define ISLE_SCREEN_TRANSITION_HPP
#include "screen_transition_source.h"
#include <cstddef>
namespace isle::scene { class Services; }
namespace isle::screen_transition {
class Owner final {
public:
    explicit Owner(scene::Services&) noexcept;
    IsleScreenTransitionSource source{};
    IsleScreenTransitionObject* objects = nullptr;
    scene::Services& services;
    ~Owner();
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    const IsleScreenTransitionObject* solid() const noexcept;
    // Preparation and GPU/readback owners retain this exact module, not Player.
    bool borrow() noexcept;
    void release() noexcept;
    bool retirement_ready() const noexcept;
private:
    std::size_t borrows_ = 0;
};
}
#endif
