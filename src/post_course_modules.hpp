#ifndef ISLE_POST_COURSE_MODULES_HPP
#define ISLE_POST_COURSE_MODULES_HPP
#include "camera_check_source.h"
#include "world_module.hpp"
#include <cstdint>
#include <array>
#include <cstddef>
#include <string_view>
#include <thread>

namespace isle::application { class Resources; }
namespace isle::rom { class Snapshot; }
namespace isle::post_course {
/* Original mutable module data, independent of disposable native scenes and
 * course asset residency. Observe actual successful Source overlay loads before
 * binding a new scene. Neither rendering nor photo begin/clear resets this.
 * Snapshot storage also outlives these retained source-byte name views. */
struct Modules final {
    float preview_scale = 0;
    IsleCameraCheckMusicState music{};
    ::isle::world::ModuleState world;
    std::array<std::string_view,7> course_names{};
    const std::byte* rom_binding = nullptr;
    std::uint64_t window_generation = 0, world_generation = 0;
    // No WORLD load yet is ordinary before the original manager reaches Oak.
    void observe_world(const application::Resources&);
    void observe_window(const rom::Snapshot&, const application::Resources&);
};

class Component;

/* Independently initialized Window data for standalone End8/Credits execution.
 * The borrowed Snapshot must remain alive and unmoved through this owner's
 * lifetime. Serialized access remains on its creator thread. Component epochs
 * never represent Root overlay residency. */
class ComponentModules final {
public:
    class Lease final {
    public:
        ~Lease();
        Lease(Lease&&) noexcept;
        Lease& operator=(Lease&&) noexcept;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Modules& data() const;
        std::uint64_t generation() const;
        const rom::Snapshot& snapshot() const;
    private:
        friend class ComponentModules;
        explicit Lease(ComponentModules&) noexcept;
        ComponentModules& checked_owner() const;
        void release() noexcept;
        ComponentModules* owner_;
    };

    explicit ComponentModules(const rom::Snapshot&);
    ~ComponentModules();
    ComponentModules(const ComponentModules&) = delete;
    ComponentModules& operator=(const ComponentModules&) = delete;
    ComponentModules(ComponentModules&&) = delete;
    ComponentModules& operator=(ComponentModules&&) = delete;
private:
    friend class Component;
    Lease acquire();
    void validate_binding() const;
    const std::thread::id creator_ = std::this_thread::get_id();
    const rom::Snapshot& snapshot_;
    Modules modules_;
    std::uint64_t epoch_;
    std::size_t leases_ = 0;
};
}
#endif
