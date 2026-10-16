#ifndef ISLE_EFFECT_BANK_RESOURCES_HPP
#define ISLE_EFFECT_BANK_RESOURCES_HPP

#include "menu_resources.hpp"

#include <array>
#include <stdexcept>

namespace isle::menu {
enum class EffectGroup { common, beach, tunnel, cave, river, volcano, valley, rainbow, intro };

class EffectResourceError final : public std::runtime_error {
public:
    explicit EffectResourceError(Status status);
    Status status() const noexcept { return status_; }
private:
    Status status_;
};

// Relative offsets retain section-base provenance and borrowed payload identity.
// Owners must outlive the result. No mutation, relocation, installation or GPU
// admission occurs. Throws EffectResourceError for malformed/escaping input;
// allocation failures propagate. bank_id is a source resource slot (0..7), not
// an encoded particle-list selector. Control validation proves finite individual
// interpreter ticks from the source initial registers, including across waits;
// it does not certify a runtime's opcode implementation or child allocation.
EffectBank decode_effect_bank(rom::GuestView script_section,
                              rom::GuestView image_section, std::uint32_t bank_id);

// Immutable nine-group inventory, not nine simultaneously active bank slots.
// Scene callers select common(slot3) and one course(slot0), or intro(slot0).
// Snapshot must outlive this owner and every borrowed result. Moving Snapshot
// invalidates subsequent access (throws EffectResourceError invalid_snapshot).
class EffectResources final {
public:
    explicit EffectResources(const rom::Snapshot& snapshot);
    EffectResources(rom::Snapshot&&) = delete;
    EffectResources(const rom::Snapshot&&) = delete;
    EffectResources(const EffectResources&) = delete;
    EffectResources& operator=(const EffectResources&) = delete;
    EffectResources(EffectResources&&) = delete;
    EffectResources& operator=(EffectResources&&) = delete;
    const EffectBank& bank(EffectGroup group) const;
    // Borrowed unique/merged frame+palette extents only; no code, descriptors,
    // forced GPU addresses or duplicate image storage. Scene packs these and
    // retains the Snapshot through all CPU/GPU users and submission completion.
    std::span<const rom::GuestView> installation_spans(EffectGroup group) const;
private:
    void require_live() const;
    const rom::Snapshot* snapshot_;
    const std::byte* source_data_;
    std::array<EffectBank, 9> banks_;
    std::array<std::vector<rom::GuestView>, 9> installation_;
};
} // namespace isle::menu
#endif
