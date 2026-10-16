#include "scene_services.hpp"
#include "menu_scene_bridge.h"
#include "borrowed_model_kernel.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdexcept>

namespace isle::scene {
namespace {
save::GameSession& require_save(::isle::game::State& state) {
    auto* session = state.save();
    if (session == nullptr) throw std::invalid_argument("scene Services requires initialized persistent State");
    return *session;
}
}
struct Services::Impl {
    struct SpriteRecord {
        SpriteRecord* next = nullptr;
        IsleGameSprite* sprite = nullptr;
        std::size_t resource = 0;
        std::uint64_t generation = 0;
    };
    struct ObjectRecord {
        ObjectRecord* next = nullptr;
        ObjectRecord* draw_next = nullptr;
        Impl* owner = nullptr;
        IsleGameObject* object = nullptr;
        SpriteRecord* sprites = nullptr;
        SpriteRecord* sprite_tail = nullptr;
        IsleGameCommandQueue commands{};
        IsleGameCommandHandler handler = nullptr;
        void* handler_user = nullptr;
        RenderMetadata render{};
        bool drawing = false, deleting = false, dispatching = false, retired = false;
    };
    struct Ramp {
        Impl* owner = nullptr;
        IsleGameObject* object = nullptr;
        int slot = 0;
        std::uint32_t from = 0, to = 0, tick = 0, duration = 0;
        bool pan = false;
    };
    Services& s;
    isle_arena& arena;
    ResourceView resources;
    ::isle::menu::model::CameraBasisState camera_basis{};
    ObjectRecord *objects = nullptr, *free_objects = nullptr, *draws = nullptr;
    SpriteRecord* free_sprites = nullptr;
    IsleGameCommandPool commands{};
    std::array<Ramp, 5> volumes{}, pans{};
    IsleGameObject* fade_object = nullptr;
    FadeView fade{};
    const IsleGameModelTransform fade_transform{{0,0,0},{0,0,0},{1,3.2f,2.4f}};
    float fade_seconds = 0;
    bool reveal = false, delayed = false;
    bool exit = false;
    std::int32_t result = 0;
    char error[512]{};

    explicit Impl(Services& services, isle_arena& storage, ResourceView view)
        : s(services), arena(storage), resources(view) {
        commands.arena = &arena;
        for (auto& ramp : volumes) ramp.owner = this;
        for (auto& ramp : pans) { ramp.owner = this; ramp.pan = true; }
    }
    void initialize() {
        const auto& setup = resources.pools;
        for (const auto& resource : resources.sprites)
            if (!resource.sprite) { s.fail("scene resource view contains an uninitialized sprite"); return; }
        if (!s.check(isle_menu_reserve_pools(s.game, &arena, setup.objects, setup.processes,
                setup.sprites, setup.cameras, setup.matrices), "scene initial native pools")) return;
        for (std::size_t i = 0; i < setup.objects; ++i) {
            auto* entry = allocate<ObjectRecord>();
            if (entry == nullptr) return;
            entry->next = free_objects;
            free_objects = entry;
        }
        for (std::size_t i = 0; i < setup.sprites; ++i) {
            auto* entry = allocate<SpriteRecord>();
            if (entry == nullptr) return;
            entry->next = free_sprites;
            free_sprites = entry;
        }
    }
    template<class T> T* allocate() noexcept {
        isle_span storage{};
        if (isle_arena_allocate(&arena, sizeof(T), alignof(T), &storage) != ISLE_OK) {
            s.fail("menu native bookkeeping arena exhausted");
            return nullptr;
        }
        return ::new (static_cast<void*>(storage.data)) T{};
    }
    ObjectRecord* find(IsleGameObject* object) noexcept {
        if (object == nullptr) object = isle_menu_current_object(s.game);
        for (auto* entry = objects; entry != nullptr; entry = entry->next)
            if (entry->object == object) return entry;
        return nullptr;
    }
    SpriteRecord* find_sprite(IsleGameSprite* sprite) noexcept {
        for (auto* object = objects; object != nullptr; object = object->next)
            for (auto* entry = object->sprites; entry != nullptr; entry = entry->next)
                if (entry->sprite == sprite) return entry;
        return nullptr;
    }
    static void default_update(IsleGameObject*, void* user) noexcept {
        auto* entry = static_cast<ObjectRecord*>(user);
        entry->owner->s.process_commands(entry->object);
    }
    static void retired(IsleGameObject*, void* user) noexcept {
        auto* entry = static_cast<ObjectRecord*>(user);
        entry->retired = true;
        if (entry->dispatching) isle_game_commands_release(&entry->owner->commands, &entry->commands);
        if (!entry->dispatching) entry->owner->unlink(entry);
    }
    void unlink(ObjectRecord* entry) noexcept {
        auto** at = &objects;
        while (*at != nullptr && *at != entry) at = &(*at)->next;
        if (*at != nullptr) *at = entry->next;
        at = &draws;
        while (*at != nullptr && *at != entry) at = &(*at)->draw_next;
        if (*at != nullptr) *at = entry->draw_next;
        isle_game_commands_release(&commands, &entry->commands);
        while (entry->sprites != nullptr) {
            auto* sprite = entry->sprites;
            entry->sprites = sprite->next;
            sprite->next = free_sprites;
            free_sprites = sprite;
        }
        *entry = ObjectRecord{};
        entry->next = free_objects;
        free_objects = entry;
    }
    void unlink_draw(ObjectRecord* entry) noexcept {
        auto** at = &draws;
        while (*at != nullptr && *at != entry) at = &(*at)->draw_next;
        if (*at != nullptr) *at = entry->draw_next;
        entry->draw_next = nullptr;
    }
    void link_draw(ObjectRecord* entry, RenderMetadata render, bool head = false) noexcept {
        unlink_draw(entry);
        entry->render = render;
        entry->drawing = true;
        auto** at = &draws;
        while (*at != nullptr && ((*at)->render.link < render.link ||
            ((*at)->render.link == render.link && ((*at)->render.priority > render.priority ||
             (!head && (*at)->render.priority == render.priority))))) at = &(*at)->draw_next;
        entry->draw_next = *at;
        *at = entry;
    }
    static void fade_process(IsleGameObject* object, void* user) noexcept {
        auto& self = *static_cast<Impl*>(user);
        const float step = static_cast<float>(255.0 / (static_cast<double>(self.fade_seconds) * 60.0));
        float value = self.reveal ? 255.0f : 0.0f;
        self.fade.running = true;
        while (!self.s.failed()) {
            value = self.reveal ? value - step : value + step;
            if (self.reveal ? value < 0.0f : value > 255.0f) break;
            self.fade.alpha = static_cast<std::uint8_t>(value);
            if (!self.s.wait()) return;
        }
        if (self.s.failed()) return;
        self.fade.alpha = self.reveal ? 0 : 255;
        self.fade.running = false;
        if (self.delayed && !self.s.wait(3)) return;
        self.fade.visible = false;
        self.fade_object = nullptr;
        self.s.erase(object);
    }
    static void ramp_process(IsleGameObject* object, void* user) noexcept {
        auto& ramp = *static_cast<Ramp*>(user);
        auto& self = *ramp.owner;
        while (!self.s.failed()) {
            const float from = static_cast<float>(ramp.from);
            const float to = static_cast<float>(ramp.to);
            const float value = ((static_cast<float>(ramp.tick) * (to - from)) /
                                 static_cast<float>(ramp.duration)) + from;
            auto output = static_cast<std::uint32_t>(value);
            if (!ramp.pan && output > 0x7fff) output = 0x7fff;
            if (!self.s.check(ramp.pan ? self.s.audio.sound_pan(static_cast<unsigned>(ramp.slot), output) :
                    self.s.audio.sound_volume(static_cast<unsigned>(ramp.slot), output), "menu sound ramp control")) return;
            if (ramp.tick >= ramp.duration) {
                ramp.object = nullptr;
                ramp.slot = 0;
                ramp.from = ramp.to = ramp.tick = ramp.duration = 0;
                self.s.erase(object);
                return;
            }
            ++ramp.tick;
            if (!self.s.wait()) return;
        }
    }
    bool ramp(bool pan, int slot, std::uint32_t from, std::uint32_t to, std::int32_t updates) noexcept {
        if (s.failed()) return false;
        if (slot < 0 || static_cast<std::size_t>(slot) >= game_audio::sound_slots || updates <= 0) {
            s.fail("invalid menu sound ramp slot or duration");
            return false;
        }
        auto& calendar = pan ? pans : volumes;
        for (auto& ramp : calendar) {
            if (ramp.object != nullptr) continue;
            ramp.slot = slot;
            ramp.from = std::min(from, pan ? 127u : 0x7fffu);
            ramp.to = std::min(to, pan ? 127u : 0x7fffu);
            ramp.tick = 0;
            ramp.duration = static_cast<std::uint32_t>(updates);
            ramp.object = s.object(14);
            if (ramp.object == nullptr) return false;
            if (!s.check(isle_menu_set_update(s.game, ramp.object, nullptr, nullptr), "menu ramp object update")) return false;
            if (s.process(ramp.object, ramp_process, &ramp) == nullptr) return false;
            return true;
        }
        // Source AB41C/AB700 return -1 on a full five-entry calendar.
        return false;
    }
};
::isle::menu::model::CameraBasisState& Services::camera_basis() noexcept { return impl_->camera_basis; }

Services::Services(IsleGame* owner, isle_arena& arena, ResourceView resources,
    game_input::Service& controls, game_audio::Session& sounds, ::isle::game::State& persistent)
    : game(owner), state(persistent), input(controls), audio(sounds), save(require_save(persistent)),
      rng(persistent.rng), controller(persistent.controller), start_flags(persistent.title_start_flags),
      impl_(std::make_unique<Impl>(*this, arena, resources)) {
    impl_->initialize();
}
Services::~Services() {
    while (impl_->objects != nullptr) erase(impl_->objects->object);
}
bool Services::failed() const noexcept { return impl_->error[0] != '\0'; }
const char* Services::error() const noexcept { return impl_->error; }
void Services::fail(const char* text) noexcept {
    if (impl_->error[0] != '\0') return;
    if (text == nullptr || *text == '\0') text = "menu scene service failure";
    std::size_t i = 0;
    while (i + 1 < sizeof(impl_->error) && text[i] != '\0') { impl_->error[i] = text[i]; ++i; }
    impl_->error[i] = '\0';
}
bool Services::check(IsleGameResult result, const char* operation) noexcept {
    if (result != ISLE_GAME_OK) fail(operation);
    return result == ISLE_GAME_OK && !failed();
}
bool Services::check(::isle::audio::Error result, const char* operation) noexcept {
    if (result != ::isle::audio::Error::none) fail(operation);
    return result == ::isle::audio::Error::none && !failed();
}
bool Services::wait(std::uint32_t updates) noexcept {
    if (failed()) return false;
    return check(isle_game_wait(game, updates), "menu native process wait");
}
IsleGameObject* Services::object(std::uint32_t id, std::uint8_t link, std::uint32_t priority) noexcept {
    if (failed()) return nullptr;
    auto* record = impl_->free_objects;
    if (record != nullptr) impl_->free_objects = record->next;
    else record = impl_->allocate<Impl::ObjectRecord>();
    if (record == nullptr) return nullptr;
    *record = Impl::ObjectRecord{};
    record->owner = impl_.get();
    if (!check(isle_game_add_object(game, id, link, priority, Impl::default_update, record,
            &record->object), "menu native object creation")) {
        record->next = impl_->free_objects;
        impl_->free_objects = record;
        return nullptr;
    }
    record->next = impl_->objects;
    impl_->objects = record;
    if (!check(isle_game_object_set_retire_callback(game, record->object, Impl::retired, record),
               "scene native retirement callback")) {
        erase(record->object);
        return nullptr;
    }
    return record->object;
}
bool Services::set_update(IsleGameObject* object, IsleGameCallback callback, void* user) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || record->deleting) { fail("scene update owner is unregistered or retiring"); return false; }
    return check(isle_menu_set_update(game, record->object, callback, user), "scene source update replacement");
}
bool Services::set_command_handler(IsleGameObject* object, IsleGameCommandHandler callback, void* user) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || record->deleting) { fail("scene command owner is unregistered or retiring"); return false; }
    record->handler = callback;
    record->handler_user = user;
    return !failed();
}
bool Services::process_commands(IsleGameObject* object) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr) { fail("scene command processing owner is unregistered"); return false; }
    if (record->deleting) return !failed();
    if (record->dispatching) { fail("recursive scene command dispatch"); return false; }
    int erase_requested = 0;
    record->dispatching = true;
    const auto status = isle_game_commands(game, record->object, &impl_->commands, &record->commands,
                                           record->handler, record->handler_user, &erase_requested);
    record->dispatching = false;
    if (record->retired) impl_->unlink(record);
    else if (erase_requested) erase(record->object);
    return check(status, "scene source command processing");
}
bool Services::send_command(IsleGameObject* object, std::int32_t command, IsleGameObject* source) noexcept {
    if (failed()) return false;
    auto* record = impl_->find(object);
    if (record == nullptr || record->deleting) { fail("scene command destination is unregistered or retiring"); return false; }
    if (source == nullptr) source = isle_menu_current_object(game);
    return check(isle_game_command_send(&impl_->commands, &record->commands, command, source), "scene command enqueue");
}
bool Services::send_command_to_link(std::uint8_t link, std::int32_t command, IsleGameObject* source) noexcept {
    if (link >= 32) { fail("scene command update link is invalid"); return false; }
    for (auto* object = isle_menu_first_object(game, link); object != nullptr; object = isle_menu_next_object(game, object)) {
        auto* record = impl_->find(object);
        if (record != nullptr && !record->deleting && !send_command(object, command, source)) return false;
    }
    return !failed();
}
bool Services::set_render(IsleGameObject* object, RenderMetadata render) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || record->deleting || render.link >= 32) { fail("scene render metadata owner/link is invalid"); return false; }
    impl_->link_draw(record, render);
    return !failed();
}
bool Services::render_metadata(IsleGameObject* object, RenderMetadata& output) const noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || !record->drawing || record->deleting) return false;
    output = record->render;
    return true;
}
bool Services::move_to_head(IsleGameObject* object) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || !record->drawing || record->deleting) { fail("scene DL head owner is invalid"); return false; }
    impl_->link_draw(record, record->render, true);
    return !failed();
}
bool Services::move_to_tail(IsleGameObject* object) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr || !record->drawing || record->deleting) { fail("scene DL tail owner is invalid"); return false; }
    impl_->link_draw(record, record->render);
    return !failed();
}
IsleGameProcess* Services::process(IsleGameObject* object, IsleGameCallback entry, void* user,
    IsleGameProcessKind kind, std::uint32_t priority) noexcept {
    IsleGameProcess* result = nullptr;
    if (failed()) return nullptr;
    if (!check(isle_game_add_process(game, object, entry, user, kind, priority, &result), "menu native process creation")) return nullptr;
    return result;
}
void Services::end_process(IsleGameProcess* process) noexcept {
    check(isle_game_end_process(game, process), "menu native process end");
}
void Services::end_processes(IsleGameObject* object) noexcept {
    check(isle_menu_end_processes(game, object), "menu native object process end");
}
void Services::erase(IsleGameObject* object) noexcept {
    auto* record = impl_->find(object);
    if (record == nullptr) { fail("menu erasing an unregistered native object"); return; }
    object = record->object;
    if (record->deleting && isle_menu_current_object(game) == object) return;
    record->deleting = true;
    check(isle_game_delete_object(game, object), "scene native whole-object deletion");
}
SpriteHandle Services::sprite(std::uint32_t identity, std::uint8_t link, std::uint32_t priority,
                              std::uint32_t object_id) noexcept {
    if (link >= 32) { fail("invalid menu sprite render link"); return {}; }
    auto* owner = object(object_id);
    if (owner == nullptr) return {};
    auto* child = append_sprite(owner, identity);
    if (child == nullptr) { erase(owner); return {}; }
    impl_->link_draw(impl_->find(owner), {link,priority,0xffffffffu});
    return {owner, child};
}
IsleGameSprite* Services::append_sprite(IsleGameObject* object, std::uint32_t identity) noexcept {
    if (failed()) return nullptr;
    const auto resources = impl_->resources.sprites;
    std::size_t index = 0;
    while (index < resources.size() && resources[index].identity != identity) ++index;
    if (index == resources.size()) { fail("unknown menu sprite resource identity"); return nullptr; }
    const auto* initial = resources[index].sprite.view();
    if (initial == nullptr) { fail("scene sprite resource is uninitialized"); return nullptr; }
    return append_sprite(object,index,*initial);
}
IsleGameSprite* Services::append_sprite(IsleGameObject* object, std::size_t index,
                                      const IsleSprite& initial, std::uint64_t generation) noexcept {
    if (failed()) return nullptr;
    auto* owner = impl_->find(object);
    if (owner == nullptr || owner->deleting) { fail("scene sprite owner is unregistered or retiring"); return nullptr; }
    auto* record = impl_->free_sprites;
    if (record != nullptr) impl_->free_sprites = record->next;
    else record = impl_->allocate<Impl::SpriteRecord>();
    if (record == nullptr) return nullptr;
    *record = Impl::SpriteRecord{};
    record->resource = index;
    record->generation = generation;
    if (!check(isle_game_add_sprite(game, owner->object, &initial, &record->sprite), "scene sprite attachment")) {
        record->next = impl_->free_sprites;
        impl_->free_sprites = record;
        return nullptr;
    }
    if (owner->sprite_tail != nullptr) owner->sprite_tail->next = record;
    else owner->sprites = record;
    owner->sprite_tail = record;
    return record->sprite;
}
bool Services::rebind_sprite(IsleGameSprite* sprite, std::uint32_t identity) noexcept {
    const auto resources = impl_->resources.sprites;
    for (std::size_t i = 0; i < resources.size(); ++i) {
        if (resources[i].identity != identity) continue;
        const auto* replacement = resources[i].sprite.view();
        if (replacement == nullptr) { fail("scene replacement resource is uninitialized"); return false; }
        return replace_sprite(sprite, i, *replacement);
    }
    fail("scene replacement sprite identity is unknown");
    return false;
}
bool Services::replace_sprite(IsleGameSprite* sprite, std::size_t resource, const IsleSprite& replacement,
                              std::uint64_t generation) noexcept {
    if (failed()) return false;
    auto* record = impl_->find_sprite(sprite);
    if (record == nullptr) { fail("scene replacement sprite is unregistered"); return false; }
    auto* destination = view(sprite);
    if (destination == nullptr) return false;
    *destination = replacement;
    record->resource = resource;
    record->generation = generation;
    return true;
}
bool Services::remove_sprites(IsleGameObject* object) noexcept {
    auto* owner = impl_->find(object);
    if (owner == nullptr) { fail("menu sprite removal owner is unregistered"); return false; }
    while (owner->sprites != nullptr) {
        auto* record = owner->sprites;
        if (!check(isle_game_remove_sprite(game, record->sprite), "menu native sprite removal")) return false;
        owner->sprites = record->next;
        record->next = impl_->free_sprites;
        impl_->free_sprites = record;
    }
    owner->sprite_tail = nullptr;
    return !failed();
}
IsleSprite* Services::view(IsleGameSprite* sprite) noexcept {
    IsleSprite* result = nullptr;
    if (!check(isle_game_sprite_view(game, sprite, &result), "menu sprite mutable view")) return nullptr;
    return result;
}
std::size_t Services::sprite_count() const noexcept {
    std::size_t result = 0;
    for (auto* owner = impl_->draws; owner != nullptr; owner = owner->draw_next) {
        if (owner->deleting || isle_menu_object_hidden(game, owner->object)) continue;
        for (auto* sprite = owner->sprites; sprite != nullptr; sprite = sprite->next) ++result;
    }
    return result;
}
bool Services::sprite_draws(std::span<::isle::sprite::Draw> output, std::size_t& written) noexcept {
    written = 0;
    if (failed()) return false;
    if (output.size() < sprite_count()) { fail("menu sprite draw destination too small"); return false; }
    // Validate every native borrow before publishing the first draw.
    for (auto* owner = impl_->draws; owner != nullptr; owner = owner->draw_next) {
        if (owner->deleting || isle_menu_object_hidden(game, owner->object)) continue;
        for (auto* sprite = owner->sprites; sprite != nullptr; sprite = sprite->next)
            if (view(sprite->sprite) == nullptr) return false;
    }
    for (auto* owner = impl_->draws; owner != nullptr; owner = owner->draw_next) {
        if (owner->deleting || isle_menu_object_hidden(game, owner->object)) continue;
        for (auto* sprite = owner->sprites; sprite != nullptr; sprite = sprite->next)
            output[written++] = {sprite->resource, view(sprite->sprite), sprite->generation};
    }
    return true;
}
bool Services::sprite_draws(std::span<SpriteDraw> output, std::size_t& written) noexcept {
    written = 0;
    if (failed()) return false;
    if (output.size() < sprite_count()) { fail("scene metadata sprite draw destination too small"); return false; }
    for (auto* owner = impl_->draws; owner != nullptr; owner = owner->draw_next) {
        if (owner->deleting || isle_menu_object_hidden(game, owner->object)) continue;
        for (auto* sprite = owner->sprites; sprite != nullptr; sprite = sprite->next)
            if (view(sprite->sprite) == nullptr) return false;
    }
    for (auto* owner = impl_->draws; owner != nullptr; owner = owner->draw_next) {
        if (owner->deleting || isle_menu_object_hidden(game, owner->object)) continue;
        for (auto* sprite = owner->sprites; sprite != nullptr; sprite = sprite->next)
            output[written++] = {{sprite->resource, view(sprite->sprite), sprite->generation},owner->object,owner->render};
    }
    return true;
}
std::span<const SpriteResource> Services::sprite_resources() const noexcept { return impl_->resources.sprites; }
ResourceView Services::resources() const noexcept { return impl_->resources; }
bool Services::begin_fade(bool reveal, float seconds, std::array<std::uint8_t,3> rgb, bool delayed) noexcept {
    if (failed()) return false;
    if (!std::isfinite(seconds) || seconds <= 0 || !std::isfinite(static_cast<float>(255.0 / (static_cast<double>(seconds) * 60.0)))) {
        fail("invalid source screen fade duration"); return false;
    }
    if (impl_->fade_object != nullptr) { erase(impl_->fade_object); impl_->fade_object = nullptr; }
    impl_->reveal = reveal;
    impl_->delayed = delayed;
    impl_->fade_seconds = seconds;
    impl_->fade = {true, true, rgb, static_cast<std::uint8_t>(reveal ? 255 : 0)};
    impl_->fade_object = object(2, 1, 1);
    if (impl_->fade_object == nullptr) return false;
    if (!check(isle_menu_set_update(game, impl_->fade_object, nullptr, nullptr), "menu fade object update")) return false;
    return process(impl_->fade_object, Impl::fade_process, impl_.get(), ISLE_GAME_STACKFUL, 0) != nullptr;
}
FadeView Services::fade() const noexcept { return impl_->fade; }
const IsleGameModelTransform* Services::fade_transform() const noexcept {
    return impl_->fade.visible ? &impl_->fade_transform : nullptr;
}
int Services::play_sound(std::uint32_t sound, game_audio::Parameters parameters) noexcept {
    int slot = -1;
    if (!check(audio.play_sound(sound, parameters, slot), "menu play sound")) return -1;
    return slot;
}
bool Services::ramp_sound_volume(int slot, std::uint32_t from, std::uint32_t to, std::int32_t updates) noexcept {
    return impl_->ramp(false, slot, from, to, updates);
}
bool Services::ramp_sound_pan(int slot, std::uint32_t from, std::uint32_t to, std::int32_t updates) noexcept {
    return impl_->ramp(true, slot, from, to, updates);
}
void Services::stop_sound_ramps() noexcept {
    for (auto& ramp : impl_->volumes) {
        if (ramp.object != nullptr) erase(ramp.object);
        ramp = Impl::Ramp{impl_.get()};
    }
    for (auto& ramp : impl_->pans) {
        if (ramp.object != nullptr) erase(ramp.object);
        ramp = Impl::Ramp{impl_.get()};
        ramp.pan = true;
    }
}
bool Services::fade_all_sounds(std::int32_t updates) noexcept {
    if (updates <= 0) { fail("invalid source sound fade duration"); return false; }
    // AB918 intentionally writes volume.to twice; the pan target is retained.
    for (auto& ramp : impl_->volumes) { ramp.tick = 0; ramp.duration = static_cast<std::uint32_t>(updates); ramp.to = 0; }
    for (auto& ramp : impl_->pans) { ramp.tick = 0; ramp.duration = static_cast<std::uint32_t>(updates); }
    return !failed();
}
void Services::request_exit(std::int32_t result) noexcept {
    if (!impl_->exit) { impl_->exit = true; impl_->result = result; }
}
bool Services::exit_requested() const noexcept { return impl_->exit; }
std::int32_t Services::exit_result() const noexcept { return impl_->result; }
} // namespace isle::scene
