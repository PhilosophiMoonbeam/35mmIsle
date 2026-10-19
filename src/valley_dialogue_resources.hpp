#ifndef ISLE_VALLEY_DIALOGUE_RESOURCES_HPP
#define ISLE_VALLEY_DIALOGUE_RESOURCES_HPP
#include "course_resources.hpp"
#include "valley_actor_source.h"
#include "35mmisle/sprite.hpp"
#include "scene_resources.hpp"
#include "sprite_producer.hpp"
#include <array>
#include <memory>
namespace isle::valley {
// Source must outlive this immutable owner; only its canonical name buffer is borrowed.
class DialogueResources final {
public:
    static std::shared_ptr<const DialogueResources> load(const rom::Snapshot&, IsleValleyActorSource&);
    DialogueResources(const DialogueResources&) = delete;
    DialogueResources& operator=(const DialogueResources&) = delete;
    IsleGameResult dialogue(std::uint32_t, std::size_t, const IsleValleyDialogue**) const noexcept;
    IsleGameResult sprite(std::uint32_t, const IsleSprite**) const noexcept;
    IsleGameResult sprite_list(std::uint32_t, const IsleSprite* const**) const noexcept;
    static IsleGameResult dialogue_format(char*, std::size_t, const IsleValleyDialogue*, std::int32_t*) noexcept;
    std::span<const scene::SpriteResource> resources() const noexcept { return resources_; }
    std::span<const IsleSprite* const> roots() const noexcept { return roots_; }
    std::span<const sprite::BankMapping> banks() const noexcept { return banks_; }
private:
    DialogueResources() = default;
    sprite::BankOwner bank_;
    std::array<IsleValleyDialogue,15> main_{};
    std::array<IsleValleyDialogue,2> empty_{};
    std::array<sprite::Sprite,8> sprites_{};
    std::array<const IsleSprite*,6> first_{}, second_{};
    std::array<scene::SpriteResource,8> resources_{};
    std::array<const IsleSprite*,8> roots_{};
    std::array<sprite::BankMapping,1> banks_{};
};
}
#endif
