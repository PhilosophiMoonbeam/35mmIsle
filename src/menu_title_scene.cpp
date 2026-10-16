#include "menu_title_scene.hpp"
#include "menu_scene_bridge.h"
#include "source_effect_world.hpp"

#include <cmath>
#include <exception>
#include <initializer_list>
#include <utility>

namespace isle::menu::scene {
struct Title::Impl {
    struct ModelCallback { Impl* owner = nullptr; animation::InstanceHandle instance; };
    struct LabelCallback { Impl* owner = nullptr; IsleGameSprite* sprite = nullptr; };
    ::isle::scene::Services& s;
    const Catalog& catalog;
    std::array<IsleGameObject*, 5> objects{};
    std::array<IsleGameModel*, 5> native_models{};
    std::array<animation::InstanceHandle, 5> instances{};
    std::array<ModelCallback, 5> model_callbacks{};
    std::array<LabelCallback, 3> label_callbacks{};
    IsleGameObject *camera_object = nullptr, *particle_object = nullptr, *effect_object = nullptr;
    IsleGameObject *labels = nullptr, *audio_object = nullptr, *input_object = nullptr, *setup_object = nullptr;
    IsleGameCamera* camera = nullptr;
    IsleGameCamera* particle_camera = nullptr;
    std::unique_ptr<animation::Runtime> animation;
    std::unique_ptr<model::Producer> models;
    std::array<const EffectBank*, 1> active_effect_banks;
    std::unique_ptr<EffectRuntime> effects;
    std::int32_t state = 0;
    std::uint32_t counter = 0;
    float last_time = 0;
    int ambient = -1, voice = -1, wing = -1;
    bool active = false, animations_started = false;

    explicit Impl(::isle::scene::Services& services, const Catalog& resources)
        : s(services), catalog(resources), active_effect_banks{&resources.effects()} {
        for (auto& callback : model_callbacks) callback.owner = this;
        for (auto& callback : label_callbacks) callback.owner = this;
    }
    IsleGameModel* native_target(EffectAttachment attachment) noexcept {
        if (!models || !animation || !animation->model(attachment.instance)) return nullptr;
        return models->native_model(attachment.instance);
    }
    static bool resolve(void* context, EffectAttachment attachment, EffectWorld& output) noexcept {
        auto& self = *static_cast<Impl*>(context);
        auto* native = self.native_target(attachment);
        return native && bool(model::source_effect_world(self.s.game, native, attachment.node, output));
    }
    static bool read_target(void* context, EffectAttachment attachment, Vec3& output) noexcept {
        auto& self = *static_cast<Impl*>(context);
        auto* native = self.native_target(attachment);
        const IsleGameModelNode* nodes = nullptr;
        std::size_t count = 0;
        if (!native || isle_game_model_peek_nodes(self.s.game, native, &nodes, &count) != ISLE_GAME_OK ||
            attachment.node >= count) return false;
        const auto& position = nodes[attachment.node].transform.position;
        output = {position.x, position.y, position.z};
        return true;
    }
    static bool write_target(void* context, EffectAttachment attachment, Vec3 position) noexcept {
        auto& self = *static_cast<Impl*>(context);
        auto* native = self.native_target(attachment);
        const IsleGameModelNode* nodes = nullptr;
        std::size_t count = 0;
        if (!native || isle_game_model_peek_nodes(self.s.game, native, &nodes, &count) != ISLE_GAME_OK ||
            attachment.node >= count) return false;
        auto transform = nodes[attachment.node].transform;
        transform.position = {position.x, position.y, position.z};
        if (isle_game_model_validate_transform(self.s.game, native, attachment.node, &transform) != ISLE_GAME_OK)
            return false;
        const auto state = self.animation->model(attachment.instance);
        if (attachment.node >= state.nodes.size()) return false;
        auto pose = state.nodes[attachment.node].transform;
        pose.position = position;
        // Native is the source target, but the animation pose is authoritative
        // at the next producer synchronization. Publish the same local write to
        // both under the native update exclusion, preserving rotation/scale.
        if (!self.animation->set_node_transform(attachment.instance, attachment.node, pose)) return false;
        return isle_game_model_set_transform(self.s.game, native, attachment.node, &transform) == ISLE_GAME_OK;
    }
    bool animation_event(const animation::CallbackEvent& event) noexcept {
        if (event.event_id == -1 || event.event_id == -2) {
            state = 3;
            return true;
        }
        // A0BE70 subtracts in binary32 before signed truncation: 0.5 -> 0.
        const float selection = event.argument - 1.0f;
        if (!std::isfinite(selection) || selection < -2147483648.0f || selection >= 2147483648.0f)
            return false;
        const auto script = static_cast<std::int32_t>(selection);
        if (script < 0) return true;
        if (event.event_id < 0 || event.event_id >= 128) return false;
        const auto result = effects->create_effect(static_cast<std::uint16_t>(event.event_id),
            static_cast<std::size_t>(script), {event.instance, event.node});
        return bool(result.status);
    }
    static void particles_update(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        if (!self.s.failed() && self.effects && !self.effects->update_particles())
            self.s.fail("title particle update");
    }
    static void effects_update(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        if (!self.s.failed() && self.effects && !self.effects->update_effects(resolve, &self))
            self.s.fail("title effect update or attachment transform");
    }
    static void model_update(IsleGameObject*, void* context) noexcept {
        auto& binding = *static_cast<ModelCallback*>(context);
        auto& self = *binding.owner;
        if (self.s.failed() || !self.active || !self.animations_started) return;
        if (!self.animation->step_model(binding.instance)) { self.s.fail("title model animation update"); return; }
        if (!self.models->synchronize()) { self.s.fail("title model native synchronization"); return; }
        self.last_time = self.animation->model(self.instances[0]).animation_time;
        // Events borrow Runtime storage only until the next step. Deliver them
        // here, with this exact native world pose, before the next model runs.
        for (const auto& event : self.animation->events()) {
            if (!self.animation_event(event)) { self.s.fail("title animation effect callback"); return; }
        }
    }
    static void camera_update(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        if (self.s.failed() || !self.active || !self.animations_started) return;
        if (!self.animation->step_camera()) { self.s.fail("title camera animation update"); return; }
        const auto& value = self.animation->camera();
        const IsleGameCameraParameters* old = nullptr;
        if (!self.s.check(isle_game_camera_parameters(self.s.game, self.camera, &old), "title camera parameter view")) return;
        if (value.view_dirty) {
            IsleGameLookAt look = old->look_at;
            look.eye[0] = value.eye.x; look.eye[1] = value.eye.y; look.eye[2] = value.eye.z;
            look.at[0] = value.at.x; look.at[1] = value.at.y; look.at[2] = value.at.z;
            if (!self.s.check(isle_game_camera_set_look_at(self.s.game, self.camera, &look), "title animated camera view") ||
                !self.s.check(isle_game_camera_set_roll(self.s.game, self.camera, value.roll), "title animated camera roll")) return;
        }
        if (value.projection_dirty) {
            IsleGamePerspective perspective = old->perspective;
            perspective.fovy = value.fov_y;
            self.s.check(isle_game_camera_set_perspective(self.s.game, self.camera, &perspective), "title animated camera perspective");
        }
    }
    static void initialize_animations(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        // A0C0DC adds the camera process only here; existing model processes
        // remain unanimated until this last-created initializer first executes.
        const IsleGameCameraParameters* parameters = nullptr;
        if (!self.s.check(isle_game_camera_parameters(self.s.game, self.camera, &parameters), "title initializer camera")) return;
        IsleGamePerspective perspective = parameters->perspective;
        perspective.near_plane = 47.0f;
        if (!self.s.check(isle_game_camera_set_perspective(self.s.game, self.camera, &perspective), "title near plane47")) return;
        if (self.s.process(self.camera_object, camera_update, &self, ISLE_GAME_SYNCHRONOUS) == nullptr) return;
        self.animations_started = true;
        while (self.s.wait()) {}
    }
    static void input_process(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        while (!self.s.failed()) {
            if (self.state == 5) {
                if (self.counter >= 3600u) self.state = 1;
                else if (self.s.controller == 0 && self.counter > 1u &&
                    (self.s.input.snapshot().primary().pressed & (0x8000u | 0x1000u))) self.state = 4;
            }
            if (self.state == 5) ++self.counter;
            else if (self.state == 1 || self.state == 4) break;
            if (!self.s.wait()) return;
        }
        while (self.s.wait()) {}
    }
    static void label_fade(IsleGameObject*, void* context) noexcept {
        auto& binding = *static_cast<LabelCallback*>(context);
        auto& self = *binding.owner;
        IsleSprite* sprite = self.s.view(binding.sprite);
        if (sprite == nullptr) return;
        sprite->red = sprite->green = sprite->blue = sprite->alpha = 0;
        sprite->attr &= ~ISLE_SP_HIDDEN;
        for (float color = 0.0f; color < 255.0f; color += 8.5f) {
            sprite->red = sprite->green = sprite->blue = sprite->alpha = static_cast<std::uint8_t>(color);
            if (!self.s.wait()) return;
        }
        if (!self.s.wait(180)) return;
        for (float color = 255.0f; color >= 8.5f; color -= 8.5f) {
            sprite->red = sprite->green = sprite->blue = sprite->alpha = static_cast<std::uint8_t>(color);
            if (!self.s.wait()) return;
        }
        sprite->attr |= ISLE_SP_HIDDEN;
        self.s.end_process(nullptr);
    }
    static void label_process(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        for (auto& binding : self.label_callbacks) {
            IsleSprite* sprite = self.s.view(binding.sprite);
            if (sprite == nullptr) return;
            sprite->attr |= ISLE_SP_HIDDEN;
        }
        while (!self.s.failed()) {
            switch (self.counter) {
            case 210: self.s.process(self.labels, label_fade, &self.label_callbacks[1]); break;
            case 480: self.s.process(self.labels, label_fade, &self.label_callbacks[0]); break;
            case 750: self.s.process(self.labels, label_fade, &self.label_callbacks[2]); break;
            default: break;
            }
            if (!self.s.wait()) return;
        }
    }
    int sound(std::uint32_t id, std::uint16_t volume, std::uint8_t pan, float pitch, std::uint8_t reverb) noexcept {
        return s.play_sound(id, {volume, pan, reverb, pitch});
    }
    void audio_tick() noexcept {
        switch (counter) {
        case 1:
            s.check(s.audio.play_song(0, 0x19), "title main song");
            s.check(s.audio.bgm_volume(1, 0x1000), "title auxiliary initial volume");
            s.check(s.audio.play_song(1, 0x14), "title auxiliary song");
            ambient = sound(0x2c, 0x2000, 0x40, 1.0f, 30);
            break;
        case 2:
            s.check(s.audio.bgm_volume(1, 0x6400, 60), "title auxiliary swell");
            s.ramp_sound_volume(ambient, 0x2000, 0x4800, 60);
            break;
        case 70:
            s.ramp_sound_volume(ambient, 0x4800, 0x2000, 600);
            s.ramp_sound_pan(ambient, 0x40, 0, 400);
            break;
        case 750:
            sound(0x18, 0x3800, 0, 1.0f, 0);
            s.ramp_sound_volume(ambient, 0x2000, 0x400, 90);
            break;
        case 792: sound(0x18, 0x6800, 0x40, 1.0f, 0); break;
        case 840:
            sound(0x18, 0x3800, 0x7f, 1.0f, 0);
            s.check(s.audio.stop_sound(static_cast<unsigned>(ambient)), "title ambient stop");
            break;
        case 880: case 920: sound(0x18, 0x2000, 0x40, 1.0f, 0); break;
        case 960: case 1040: sound(0x18, 0x3800, 0x40, 1.0f, 0); break;
        case 1000: case 1080: sound(0x18, 0x4000, 0x40, 1.0f, 0); break;
        case 1110: sound(0x18, 0x3000, 0x40, 1.0f, 0); break;
        case 1690:
            voice = sound(0x12, 0x6800, 0x14, 1.0f, 10);
            s.check(s.audio.bgm_volume(1, 0, 30), "title auxiliary duck");
            break;
        case 1700: s.ramp_sound_pan(voice, 0x18, 0x5e, 30); break;
        case 1740: wing = sound(0x75, 0x3800, 40, 1.0f, 30); break;
        case 1750: s.ramp_sound_pan(wing, 0x2c, 0x7c, 20); break;
        case 1830: sound(0x12, 0x4000, 0x40, 0.9f, 20); break;
        case 1900: case 1960: case 2020: case 2140: case 2200: case 2260:
            sound(0, 0x7fff, 0x40, 1.0f, 10); break;
        case 2320: voice = sound(0x9b, 0x6000, 0x36, 0.7f, 40); break;
        case 2330: s.ramp_sound_pan(voice, 0x36, 0x0e, 40); break;
        case 2370: sound(0x9b, 0x4800, 0x18, 0.7f, 20); break;
        case 2410: sound(0x9b, 0x3c00, 0x5e, 1.0f, 30); break;
        case 2450: sound(0x9b, 0x2c00, 0x0e, 0.7f, 30); break;
        case 2480: sound(0x9b, 0x2000, 0x54, 0.68f, 30); break;
        case 2500: sound(0x9b, 0x1800, 0x2c, 0.65f, 40); break;
        case 2730: s.check(s.audio.bgm_volume(1, 0x6400, 120), "title auxiliary return"); break;
        case 3414: sound(0x12, 0x1c00, 0x40, 0.9f, 30); break;
        default: break;
        }
    }
    static void audio_process(IsleGameObject*, void* context) noexcept {
        auto& self = *static_cast<Impl*>(context);
        self.s.stop_sound_ramps();
        if (!self.s.check(self.s.audio.bgm_volume(0, 0x7f00), "title initial main volume") ||
            !self.s.check(self.s.audio.bgm_volume(1, 0x7f00), "title initial auxiliary volume")) return;
        while (!self.s.failed()) {
            self.audio_tick();
            if (self.s.failed()) return;
            if (self.state == 1 || self.state == 4) {
                const std::uint32_t duration = self.state == 1 ? 120 : 30;
                if (!self.s.check(self.s.audio.bgm_volume(1, 0, duration), "title auxiliary exit fade") ||
                    !self.s.check(self.s.audio.bgm_volume(0, 0, duration), "title main exit fade")) return;
                break;
            }
            if (!self.s.wait()) return;
        }
        if (!self.s.wait()) return;
        while (self.s.wait()) {}
    }
    bool prefix() noexcept {
        try {
            if (camera_object == nullptr) {
                camera_object = s.object(1);
                if (camera_object == nullptr || !s.check(isle_game_attach_camera(s.game, camera_object, &camera), "title native camera")) return false;
                if (!s.check(isle_game_camera_add_matrix(s.game, camera, ISLE_GAME_CAMERA_PERSPECTIVE_FAST), "title camera kind3") ||
                    !s.check(isle_game_camera_add_matrix(s.game, camera, ISLE_GAME_CAMERA_LOOK_AT_ROLL), "title camera kind8")) return false;
                if (particle_object != nullptr && !s.check(isle_menu_place_before(s.game, camera_object, particle_object), "title reentry source camera order")) return false;
            }
            if (!effects) {
                effects = std::make_unique<EffectRuntime>(active_effect_banks, s.rng,
                    EffectTargetServices{this, read_target, write_target});
                if (!effects->status()) { s.fail("title effect construction"); return false; }
            }
            if (particle_object == nullptr) {
                particle_object = s.object(static_cast<std::uint32_t>(-6));
                if (particle_object == nullptr ||
                    !s.check(isle_game_attach_camera(s.game, particle_object, &particle_camera), "title particle dispatch camera") ||
                    !s.check(isle_menu_set_update(s.game, particle_object, particles_update, this), "title particle OM callback")) return false;
            }
            if (effect_object == nullptr) {
                effect_object = s.object(static_cast<std::uint32_t>(-7));
                if (effect_object == nullptr ||
                    !s.check(isle_menu_set_update(s.game, effect_object, effects_update, this), "title effect OM callback")) return false;
            }
            return !s.failed();
        } catch (const std::exception& error) {
            s.fail(error.what());
        } catch (...) {
            s.fail("title source prefix initialization exception");
        }
        return false;
    }
    bool initialize() noexcept {
        try {
            active = true;
            animations_started = false;
            state = 0;
            counter = 0;
            last_time = 0;
            ambient = voice = wing = -1;
            if (!prefix()) return false;
            const IsleGameCameraParameters* parameters = nullptr;
            if (!s.check(isle_game_camera_parameters(s.game, camera, &parameters), "title initial camera")) return false;
            IsleGamePerspective perspective = parameters->perspective;
            effects->reset();
            animation::CameraState initial;
            initial.eye = {parameters->look_at.eye[0], parameters->look_at.eye[1], parameters->look_at.eye[2]};
            initial.at = {parameters->look_at.at[0], parameters->look_at.at[1], parameters->look_at.at[2]};
            initial.fov_y = perspective.fovy;
            std::array<animation::SlotCapacity,5> capacities;
            if (catalog.models().size() != capacities.size()) { s.fail("title model count"); return false; }
            for (std::size_t i = 0; i < capacities.size(); ++i) capacities[i] = animation::slot_capacity(catalog.models()[i]);
            auto runtime = animation::Runtime::create(catalog, capacities, initial, {});
            if (!runtime) { s.fail("title animation construction"); return false; }
            animation = std::move(runtime.runtime);
            // Prefix camera/FX objects already precede the source UI timer and
            // control objects, so actual OM animation process order is retained.
            std::array<model::Binding,5> bindings;
            std::size_t max_nodes = 0;
            for (const auto capacity : capacities) if (capacity.nodes > max_nodes) max_nodes = capacity.nodes;
            std::vector<IsleGameModelNodeInit> attachment_scratch(max_nodes);
            for (std::size_t i = 0; i < objects.size(); ++i) {
                const auto acquired = animation->acquire(i,i,catalog.models()[i].receives_effect_callback);
                if (!acquired) { s.fail("title instance acquisition"); return false; }
                instances[i] = acquired.handle;
                model_callbacks[i].instance = acquired.handle;
                objects[i] = s.object(14);
                if (objects[i] == nullptr || s.process(objects[i], model_update, &model_callbacks[i], ISLE_GAME_SYNCHRONOUS) == nullptr) return false;
                if (!model::attach_title_model(s.game,objects[i],catalog.models()[i],
                        animation->model(instances[i]),attachment_scratch,&native_models[i])) {
                    s.fail("title native model attachment"); return false;
                }
                bindings[i] = {instances[i],objects[i],native_models[i]};
            }
            auto producer = model::Producer::create(s.game, camera, s.camera_basis(), catalog, *animation, bindings);
            if (!producer) { s.fail("title native model producer construction"); return false; }
            models = std::move(producer.producer);
            if (!models->synchronize()) { s.fail("title native initial model poses"); return false; }
            const auto first = s.sprite(0x803051f0);
            labels = first.object;
            label_callbacks[0].sprite = first.sprite;
            if (labels == nullptr || s.process(labels, label_process, this) == nullptr) return false;
            label_callbacks[1].sprite = s.append_sprite(labels, 0x803072a0);
            label_callbacks[2].sprite = s.append_sprite(labels, 0x80308a00);
            constexpr std::array<std::array<std::int16_t, 2>, 3> positions{{{98,68}, {96,103}, {71,112}}};
            for (std::size_t i = 0; i < label_callbacks.size(); ++i) {
                auto* view = s.view(label_callbacks[i].sprite);
                if (view == nullptr) return false;
                view->x = positions[i][0]; view->y = positions[i][1];
                view->attr = ISLE_SP_TEXSHUF | ISLE_SP_TRANSPARENT;
            }
            audio_object = s.object(14);
            if (audio_object == nullptr || s.process(audio_object, audio_process, this) == nullptr) return false;
            input_object = s.object(14);
            if (input_object == nullptr || s.process(input_object, input_process, this) == nullptr) return false;
            setup_object = s.object(14);
            if (setup_object == nullptr || s.process(setup_object, initialize_animations, this) == nullptr) return false;
            state = 5;
            return !s.failed();
        } catch (const std::exception& error) {
            s.fail(error.what());
        } catch (...) {
            s.fail("title initialization exception");
        }
        return false;
    }
    void cleanup(bool final = false) noexcept {
        // Caller precondition: the runner retired the actual Frame completion
        // and completed-memory lease before offering this native update (or
        // fenced final Runtime retirement). play runs in Ui's separate control
        // process, never a Title-owned object, so every deletion below joins
        // immediately rather than requesting self-deletion.
        animations_started = false;
        active = false;
        if (animation) for (const auto instance : instances)
            if (instance && !animation->begin_retire(instance)) s.fail("title instance retirement");
        for (auto* object : objects) if (object != nullptr) s.end_processes(object);
        for (auto* object : {camera_object, labels, audio_object, input_object, setup_object})
            if (object != nullptr) s.end_processes(object);
        if (effects) {
            for (const auto instance : instances) if (instance) {
                effects->clear_targets(instance);
                effects->detach_instance(instance);
            }
            effects->reset();
        }
        if (models) models->retire();
        // Keep producer, animator and every callback context alive until whole
        // object deletion joins ALL processes. end_processes can return early
        // after ending one process when a preexisting native fault is latched;
        // erase still performs complete noncurrent-object teardown and its
        // retirement callback before recycling the object, even on that path.
        for (auto*& object : objects) { if (object != nullptr) s.erase(object); object = nullptr; }
        for (auto* object : {camera_object, labels, audio_object, input_object, setup_object})
            if (object != nullptr) s.erase(object);
        camera_object = labels = audio_object = input_object = setup_object = nullptr;
        camera = nullptr;
        native_models.fill(nullptr);
        models.reset();
        if (animation) {
            for (auto& instance : instances) {
                if (instance && !animation->release_retired(instance)) s.fail("title instance release after native/graphics/effect retirement");
                instance = {};
            }
            animation->retire(); animation.reset();
        }
        if (final) {
            if (particle_object != nullptr) s.erase(particle_object);
            if (effect_object != nullptr) s.erase(effect_object);
            particle_object = effect_object = nullptr;
            particle_camera = nullptr;
            effects.reset();
        }
        for (auto& binding : label_callbacks) binding.sprite = nullptr;
    }
};

Title::Title(::isle::scene::Services& services, const Catalog& catalog)
    : impl_(std::make_unique<Impl>(services,catalog)) { impl_->prefix(); }
Title::~Title() { retire(); }
bool Title::play() noexcept {
    if (impl_->s.failed()) return false;
    if (impl_->active) { impl_->s.fail("title play while already active"); return false; }
    if (!impl_->initialize()) { impl_->cleanup(); return false; }
    if (!impl_->s.begin_fade(true, 1.0f, {0, 0, 0}) || !impl_->s.wait(60)) { impl_->cleanup(); return false; }
    while (impl_->state != 1 && impl_->state != 4) {
        if (!impl_->s.wait()) { impl_->cleanup(); return false; }
    }
    if (!impl_->s.fade_all_sounds(impl_->state == 1 ? 90 : 30) ||
        !impl_->s.begin_fade(false, impl_->state == 1 ? 1.5f : 0.5f, {0, 0, 0}) ||
        !impl_->s.wait(impl_->state == 1 ? 90 : 30)) { impl_->cleanup(); return false; }
    impl_->s.check(impl_->s.audio.stop_sounds(), "title stop all sounds");
    impl_->s.check(impl_->s.audio.stop_bgm(), "title stop BGM");
    impl_->cleanup();
    return !impl_->s.failed();
}
bool Title::active() const noexcept { return impl_->active; }
std::int32_t Title::source_state() const noexcept { return impl_->state; }
float Title::source_time() const noexcept { return impl_->last_time; }
IsleGameCamera* Title::camera() const noexcept { return impl_->camera; }
animation::Runtime* Title::animation() noexcept { return impl_->animation.get(); }
model::Producer* Title::models() noexcept { return impl_->models.get(); }
EffectRuntime* Title::effects() noexcept { return impl_->effects.get(); }
void Title::retire() noexcept { impl_->cleanup(true); }
} // namespace isle::menu::scene
