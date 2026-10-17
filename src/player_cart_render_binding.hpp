#ifndef ISLE_PLAYER_CART_RENDER_BINDING_HPP
#define ISLE_PLAYER_CART_RENDER_BINDING_HPP
#include "borrowed_model_kernel.hpp"
#include "player_cart_resources.hpp"
#include "scene_services.hpp"

namespace isle::player::cart {
// Actual Services DL order, not physical-slot order. Every active binding must
// remain supplied, including hidden/pass-filtered objects. Native local pose is
// authoritative; Runtime supplies only this incarnation's animation/materials.
struct RenderBinding {
    menu::model::BorrowedBinding model;
    scene::RenderMetadata render{3,0x80000000u,UINT32_MAX};
};
} // namespace isle::player::cart
#endif
