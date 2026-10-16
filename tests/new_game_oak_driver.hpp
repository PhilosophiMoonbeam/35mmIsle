#ifndef ISLE_NEW_GAME_OAK_DRIVER_HPP
#define ISLE_NEW_GAME_OAK_DRIVER_HPP

#include "native_scene_driver.hpp"
#include "new_game_scene_private_smoke.hpp"
#include "oaks_lab_scene_private_smoke.hpp"
#include "oaks_lab_resources.hpp"
#include "scene_storage.hpp"
#include <vector>

namespace isle::private_scene {
struct NameOakMetrics {
    std::uint64_t name_updates = 0, name_frames = 0, name_presentations = 0;
    std::uint64_t oak_updates = 0, oak_frames = 0, oak_presentations = 0;
};
inline void demand(bool value,const char* reason) {
    if (!value) throw std::runtime_error(reason);
}
// Enter only after the actual MAIN_MENU result0/notice1 and joined startup
// retirement. Source result0 here is Name entry; only Oak's later course0 is
// the Beach request. This consumer stops at that real request, not fake gameplay.
inline NameOakMetrics exercise_name_to_oak(
        const rom::Snapshot& snapshot, game::State& state, scene::Storage& storage,
        game_input::Service& input, game_host::Host& platform,
        scene::graphics::Frame& graphics, host::GtlClock& clock,
        host::RetraceClock& retraces, std::uint64_t& ordinal,
        Driver<new_game::Scene>::Time epoch, Driver<new_game::Scene>::Time deadline,
        const std::filesystem::path& root, Driver<new_game::Scene>::Capture capture) {
    demand(state.initialized() && state.save() && (state.scene_notices()&1u) &&
           !storage.game() && graphics.quiescent() && clock.stopped(),
           "actual source title-to-Name entry prerequisites are missing");
    auto* const persistent_save = state.save();
    const auto* const initialization = state.save_initialization();
    const auto ui_resources = window::Resources::load(snapshot);
    NameOakMetrics metrics;
    IsleGame* backing = nullptr;
    {
        const auto common = ui_resources->sprites(window::ResourceBank::Group::common);
        const auto names = ui_resources->sprites(window::ResourceBank::Group::name);
        const auto all = ui_resources->sprites();
        demand(common.data() == all.data() && names.data() == all.data()+common.size(),
               "source common/name descriptor order is not a contiguous residency domain");
        const auto sprites = all.first(common.size()+names.size());
        std::vector<const IsleSprite*> roots;
        roots.reserve(sprites.size());
        for (const auto& resource : sprites) roots.push_back(resource.sprite.view());
        const auto banks = ui_resources->banks();
        demand(banks.size() == 3,"source window bank domain is incomplete");
        // Native sprite addresses are derived from retained pixel owners. Name's
        // original overlay range collides with common task commands; relocate
        // the selected immutable bank explicitly, never overwrite a reservation.
        const std::array<sprite::BankMapping,2> mapping{{
            {banks[0].owner,banks[0].guest},{banks[1].owner,0x700000}}};
        sprite::Producer producer(roots,mapping);
        demand(storage.reset() == ISLE_GAME_OK && storage.create() == ISLE_GAME_OK,
               "actual Name native arena entry failed");
        backing = storage.game();
        scene::Services services(storage.game(),storage.arena(),{{64,64,128,8,1024},sprites},input,*platform.audio(),state);
        new_game::Scene name(services,ui_resources,producer);
        using NativeDriver = Driver<new_game::Scene>;
        NativeDriver driver(name,services,input,platform,graphics,clock,retraces,ordinal,epoch,deadline,
            [](const new_game::Scene& value) noexcept { return value.exit_requested(); },
            [](const new_game::Scene& value) noexcept { return value.observation().controls_ready; },
            root/"proof"/"actual-name-window.ppm",capture);
        new_game::private_smoke::Hooks hooks{
            &driver,
            [](void* owner,const game_input::HostFrame& sample,new_game::private_smoke::Frame& frame) noexcept {
                return static_cast<NativeDriver*>(owner)->advance(sample,frame);
            },
            [](void* owner) noexcept { return static_cast<NativeDriver*>(owner)->fence(); }};
        std::string error;
        const bool exercised = new_game::private_smoke::exercise(name,state,hooks,
            new_game::private_smoke::Scenario::empty_default,2400,error);
        demand(exercised,driver.error()[0]?driver.error():error.c_str());
        demand(driver.fence(),driver.error());
        demand(driver.captured(),"actual completed Name window was not observed");
        demand(state.save() == persistent_save && state.save_initialization() == initialization &&
               (state.scene_notices()&1u),"Name replaced the profile or consumed Oak's first-meet notice");
        metrics.name_updates = driver.updates();
        metrics.name_frames = driver.frames();
        metrics.name_presentations = driver.presentations();
    }
    demand(graphics.quiescent() && storage.destroy() == ISLE_GAME_OK,
           "Name actual native/resource retirement failed");
    demand(platform.cancel_present(),platform.message());
    {
        const auto lab_resources = oaks_lab::Resources::load(snapshot,ui_resources);
        const auto banks = lab_resources->banks();
        demand(banks.size() == 2,"source common/Oak bank domain is incomplete");
        const std::array<sprite::BankMapping,2> mapping{{
            {banks[0].owner,banks[0].guest_begin},{banks[1].owner,0x700000}}};
        sprite::Producer producer(lab_resources->roots(),mapping);
        demand(storage.reset() == ISLE_GAME_OK && storage.create() == ISLE_GAME_OK && storage.game() == backing,
               "Oak failed to reuse the actual application-owned native backing");
        scene::Services services(storage.game(),storage.arena(),lab_resources->view(),input,*platform.audio(),state);
        oaks_lab::Config configuration;
        configuration.level_id = 0;
        configuration.resource_identity = 0x4f414bu;
        oaks_lab::Scene lab(services,lab_resources,producer,configuration);
        using NativeDriver = Driver<oaks_lab::Scene>;
        NativeDriver driver(lab,services,input,platform,graphics,clock,retraces,ordinal,epoch,deadline,
            [](const oaks_lab::Scene& value) noexcept { return value.view().exit_requested; },
            [](const oaks_lab::Scene& value) noexcept {
                const auto view = value.view();
                return (view.phase == oaks_lab::Phase::dialogue && view.printed_characters >= 4) ||
                       view.phase == oaks_lab::Phase::lab || view.phase == oaks_lab::Phase::courses;
            },root/"proof"/"actual-oak-window.ppm",capture);
        oaks_lab::private_smoke::Hooks hooks{
            &driver,
            [](void* owner,const game_input::HostFrame& sample,oaks_lab::private_smoke::Frame& frame) noexcept {
                return static_cast<NativeDriver*>(owner)->advance(sample,frame);
            },
            [](void* owner) noexcept { return static_cast<NativeDriver*>(owner)->fence(); }};
        std::string error;
        const bool exercised = oaks_lab::private_smoke::exercise(lab,state,hooks,
            oaks_lab::private_smoke::Scenario::first_meet_course,0,8000,error);
        demand(exercised,driver.error()[0]?driver.error():error.c_str());
        demand(driver.fence(),driver.error());
        demand(driver.captured(),"actual completed Oak portrait/dialogue window was not observed");
        demand(state.save() == persistent_save && state.save_initialization() == initialization &&
               !(state.scene_notices()&1u),"Oak lost the persistent profile or failed source notice consumption");
        metrics.oak_updates = driver.updates();
        metrics.oak_frames = driver.frames();
        metrics.oak_presentations = driver.presentations();
    }
    demand(graphics.quiescent() && clock.stopped() && storage.destroy() == ISLE_GAME_OK,
           "Oak actual native/context/resource retirement failed");
    return metrics;
}
} // namespace isle::private_scene
#endif
