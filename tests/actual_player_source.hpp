#ifndef ISLE_TEST_ACTUAL_PLAYER_SOURCE_HPP
#define ISLE_TEST_ACTUAL_PLAYER_SOURCE_HPP
#include "player_source.h"
#include "scene_services.hpp"
#include <exception>
#include <stdexcept>

namespace isle::private_player {
// One real execution context per Services lifetime. Construct before every
// Actor/Item borrower; publish only at the fixture's explicit native init stage.
class Source final {
public:
    explicit Source(scene::Services& services) : services_(services) {
        checked(isle_player_source_prepare(services.game,&value,&services.rng,this,10));
        checked(isle_player_source_activate(&value));
    }
    ~Source() {
        if (owned_root_) services_.erase(owned_root_);
        const auto result=isle_player_source_deactivate(&value);
        if (result!=ISLE_GAME_OK && result!=isle_game_status(services_.game)) std::terminate();
    }
    Source(const Source&)=delete;
    Source& operator=(const Source&)=delete;
    void bind(IsleGameObject* root,IsleGameModel* model,IsleGameCamera* camera) {
        checked(isle_game_model_validate_binding(services_.game,root,model));
        checked(isle_player_source_bind(services_.game,root,&value));
        value.model=model;
        value.camera=camera;
    }
    // Fixtures without an authored Player still use a real native PLAYER Root,
    // native model and AUX. This does not implement or mirror Player physics.
    void create(IsleGameCamera* camera,std::uint32_t id=0x1001) {
        owned_root_=services_.object(id,9);
        if (!owned_root_) throw std::runtime_error("canonical Player Root allocation failed");
        IsleGameModelNodeInit node{};
        node.parent=UINT32_MAX;
        node.transform.scale={1,1,1};
        node.matrix_kind=ISLE_GAME_MODEL_CONTEXTUAL_RPY;
        IsleGameModel* model=nullptr;
        checked(isle_game_attach_model(services_.game,owned_root_,&node,1,&model));
        checked(isle_game_model_set_placement(services_.game,model,ISLE_GAME_MODEL_PLACEMENT_RTS,&node.transform));
        bind(owned_root_,model,camera);
    }
    IslePlayerSource value{};
private:
    static void checked(IsleGameResult result) {
        if (result!=ISLE_GAME_OK) throw std::runtime_error("canonical Player fixture initialization failed");
    }
    scene::Services& services_;
    IsleGameObject* owned_root_=nullptr;
};
} // namespace isle::private_player
#endif
