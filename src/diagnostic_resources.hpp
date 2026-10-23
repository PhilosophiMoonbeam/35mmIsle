#ifndef ISLE_DIAGNOSTIC_RESOURCES_HPP
#define ISLE_DIAGNOSTIC_RESOURCES_HPP
#include "post_course_resources.hpp"

namespace isle::post_course {
// Initialized resource facts only, not a SceneSetup ABI or mutable Source state.
class DiagnosticResources final {
public:
    struct Screen {
        std::array<std::uint32_t,3> framebuffers;
        std::uint32_t depth_buffer;
        std::uint32_t width, height, flags;
    };
    static std::shared_ptr<const DiagnosticResources> load(std::shared_ptr<const Resources>);
    const std::shared_ptr<const Resources>& owner() const noexcept { return owner_; }
    const std::array<std::uint8_t,3>& links() const noexcept { return links_; }
    std::int32_t pending() const noexcept { return pending_; }
    const std::array<std::uint32_t,16>& selections() const noexcept { return selections_; }
    std::uint32_t postprocess_initial() const noexcept { return postprocess_initial_; }
    const Screen& screen() const noexcept { return screen_; }
    const std::array<std::uint32_t,35>& setup_words() const noexcept { return setup_words_; }
    float aspect() const noexcept { return aspect_; }
    const IsleSprite& sprite() const noexcept { return *sprite_; }
private:
    DiagnosticResources() = default;
    std::shared_ptr<const Resources> owner_;
    std::array<std::uint8_t,3> links_{};
    std::int32_t pending_{};
    std::array<std::uint32_t,16> selections_{};
    std::uint32_t postprocess_initial_{};
    Screen screen_{};
    std::array<std::uint32_t,35> setup_words_{};
    float aspect_{};
    const IsleSprite* sprite_{};
};
} // namespace isle::post_course
#endif
