#ifndef ISLE_COURSE_ACTOR_EGG_NATIVE_HPP
#define ISLE_COURSE_ACTOR_EGG_NATIVE_HPP
#include "egg_source.h"
#include "source_vertex_publication.hpp"
#include "rom_snapshot.hpp"
#include "tunnel_actor_source.h"
#include "cave_actor_source.h"
#include <array>
#include <span>
namespace isle::course {
class NativeHost;
/* Fixed Adapter-owned Source geometry, not a renderer RAM mirror. Source BSS
 * owns frame0; these two arrays own only destination and immutable endpoint. */
class EggGeometry final {
public:
    EggGeometry() noexcept = default;
    EggGeometry(const EggGeometry&) = delete;
    EggGeometry& operator=(const EggGeometry&) = delete;
    EggGeometry(EggGeometry&&) = delete;
    EggGeometry& operator=(EggGeometry&&) = delete;
    IsleGameResult prepare(const rom::Snapshot&, IsleTunnelActorSource&, void*) noexcept;
    IsleGameResult prepare(const rom::Snapshot&, IsleCaveActorSource&, void*) noexcept;
    const IsleEggPreparation* preparation() const noexcept { return prepared_ ? &preparation_ : nullptr; }
    std::span<const source_geometry::VertexPublication> publications() const noexcept { return prepared_ ? std::span<const source_geometry::VertexPublication>(publication_) : std::span<const source_geometry::VertexPublication>{}; }
    IsleGameResult attach(NativeHost&) noexcept;
    bool source_matches() const noexcept;
private:
    IsleGameResult prepare(const rom::Snapshot&, void*, void*, std::int32_t,
        IsleEgg&, IsleEggVertex*, std::size_t) noexcept;
    static IsleGameResult bind(IsleEgg*, IsleActorSource*) noexcept;
    IsleGameResult bind(IsleActorSource&) noexcept;
    std::array<IsleEggVertex,336> destination_{};
    std::array<IsleEggVertex,336> endpoint_{};
    std::array<IsleEggVertex*,2> frames_{};
    IsleEggPreparation preparation_{};
    std::array<source_geometry::VertexPublication,1> publication_{};
    IsleEgg* egg_ = nullptr;
    NativeHost* host_ = nullptr;
    const rom::Snapshot* snapshot_ = nullptr;
    std::uint32_t initialization_ = 0, factory_ = 0, actor_id_ = 0;
    bool prepared_ = false;
};
}
#endif
