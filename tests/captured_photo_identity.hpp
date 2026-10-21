#ifndef ISLE_PRIVATE_CAPTURED_PHOTO_IDENTITY_HPP
#define ISLE_PRIVATE_CAPTURED_PHOTO_IDENTITY_HPP

#include "player_photo.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>

namespace isle::private_course {
// Original scorer-owned flags may revise the same canonical captured record.
// Everything else in the complete wire image remains part of its identity.
inline std::uint64_t photo_identity(const IslePlayerPhotoData& photo,
        std::array<std::byte,save::game_photo_bytes>& bytes) noexcept {
    player::encode_photo(photo,bytes);
    bytes[1]=std::byte{0};
    std::uint64_t hash=14695981039346656037ull;
    for(const auto byte:bytes) {hash^=std::to_integer<std::uint8_t>(byte);hash*=1099511628211ull;}
    return hash;
}
// Original capture moves the chosen subject to slot0 and copies every other
// context field. This is checked before scorer-owned flags can revise the roll.
inline bool photo_context_copied(const IslePlayerPhotoData& image,
        const IslePlayerPhotoContext& context,std::size_t primary) noexcept {
    const auto same_position=[](const IsleGameVec3& a,const IsleGameVec3& b) {
        return a.x==b.x && a.y==b.y && a.z==b.z;
    };
    const auto same_subject=[&same_position](const IslePlayerPokemonPhoto& a,const IslePlayerPokemonPhoto& b) {
        return a.pokemonID==b.pokemonID && a.unk_00_13==b.unk_00_13 &&
            a.specialID==b.specialID && a.poseID==b.poseID && a.animationTime==b.animationTime &&
            same_position(a.position,b.position) && a.yaw==b.yaw;
    };
    const auto& source=context.main;
    if(primary>=std::size(source.pokemons) || image.levelID!=source.levelID ||
        image.unk_00_24!=source.unk_00_24 || image.unk_00_16!=(source.unk_00_16|0x20u) ||
        image.unk_04.f32!=source.unk_04.f32 || !same_position(image.unk_08,source.unk_08) ||
        !same_position(image.unk_14,source.unk_14) ||
        !same_subject(image.pokemons[0],source.pokemons[primary]))return false;
    std::size_t output=1;
    for(std::size_t i=0;i<std::size(source.pokemons);++i)if(i!=primary)
        if(!same_subject(image.pokemons[output++],source.pokemons[i]))return false;
    for(std::size_t i=0;i<std::size(source.items);++i) {
        const auto& a=image.items[i];const auto& b=source.items[i];
        if(a.itemType!=b.itemType || a.animationTime!=b.animationTime || !same_position(a.pos,b.pos))return false;
    }
    for(std::size_t i=0;i<std::size(source.effects);++i) {
        const auto& a=image.effects[i];const auto& b=source.effects[i];
        if(a.textureID!=b.textureID || a.dataID!=b.dataID || a.bankID!=b.bankID || a.unk_03!=b.unk_03 ||
            a.posX!=b.posX || a.posY!=b.posY || a.posZ!=b.posZ || a.size!=b.size)return false;
        for(std::size_t c=0;c<std::size(a.primColor);++c)if(a.primColor[c]!=b.primColor[c])return false;
    }
    return true;
}
} // namespace isle::private_course
#endif
