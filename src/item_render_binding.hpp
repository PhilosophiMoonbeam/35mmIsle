#ifndef ISLE_ITEM_RENDER_BINDING_HPP
#define ISLE_ITEM_RENDER_BINDING_HPP

#include "borrowed_model_kernel.hpp"
#include "item_resources.hpp"
#include "scene_services.hpp"

namespace isle::items {
// One actual native GObj/model/Runtime incarnation, including independent fumes.
// Hidden/pass-filtered active bindings remain supplied. Owner native mutations
// publish both canonical root and Runtime before any graphics borrower starts.
// Relative Item order is native DL/priority/insertion order; course composition
// must merge it with World through Services' real global DL order, not append.
struct RenderBinding {
    menu::model::BorrowedBinding model;
    Family family = Family::pester_ball;
    scene::RenderMetadata render{3,0x80000000u,UINT32_MAX};
};
} // namespace isle::items
#endif
