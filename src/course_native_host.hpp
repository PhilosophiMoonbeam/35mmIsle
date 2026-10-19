#ifndef ISLE_COURSE_NATIVE_HOST_HPP
#define ISLE_COURSE_NATIVE_HOST_HPP
#include "application_manager.hpp"
#include "application_photos.hpp"
#include "actor_owner.hpp"
#include "actor_behavior_bindings.hpp"
#include "effect_bank_resources.hpp"
#include "player_root.hpp"
#include "scene_graphics.hpp"
#include "scene_resources.hpp"
#include "rom_snapshot.hpp"
#include "source_vertex_publication.hpp"
#include "host_clock.hpp"
#include "35mmisle/continuation.h"
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace isle::player { class Renderer; class Ui; class Photo; class Controller; class CartResources; }
namespace isle::player::cart { class Owner; }
namespace isle::player::camera_animation { class NativeDriver; }
namespace isle::items { class Owner; }
namespace isle::environment_sound { class Environment; }
namespace isle::actor { class InteractionServices; class CollisionPass; }
namespace isle::host { class GtlClock; }
namespace isle::course {
enum class EntryNativeProfile { unspecified, hosted_canonical_image };
struct TunnelActorNativeState;
enum class LifecycleStatus { ready, setup_pending, running, retiring, completed, cancelled, failed, retired };

// Native physical envelopes, NOT guest SceneSetup constants. The adapter retains
// and validates the Original numeric recipe separately. All spans are immutable
// cold inputs; actual canonical Source field addresses remain entry-owned.
struct NativeCapacity {
    std::size_t arena_bytes = 0;
    scene::PoolCounts pools{};
    // Bounds cover all simultaneously held native incarnations, including
    // retired slots: Actor/Item/Cart cohorts; decoded World block/road/static/
    // sky publications; Root/UI/Bump; actual scheduling/FX/Photo/input/helper
    // objects and their Source-created processes. Matrices/sprites derive from
    // that complete selected closure, not guest byte strides or logical limits.
    std::size_t command_nodes = 0;
    // Peak queued messages across that closure; reserve/freeze before activation.
    // Actual native model shapes/peak retained instances, including World,
    // Root/Cart/Actor/Item/helper cohorts. Blocks are cold, then best-fit reused.
    std::span<const IsleGameModelCapacity> model_blocks{};
    std::size_t actors = 0, items = 0, carts = 0;
    std::size_t helpers = 0, render_order = 0, sprite_draws = 0;
    menu::animation::ExecutionCapacity world_execution{}, actor_execution{}, item_execution{}, cart_execution{}, camera_execution{};
};
struct NativeHelper {
    std::uint32_t id = 0, priority = 0;
    std::uint8_t link = 0;
    IsleGameSourceCallback callback = nullptr;
};
struct NativeInput {
    std::uint32_t id = 0, priority = 0, process_priority = 0;
    std::uint8_t link = 0;
    IsleGameSourceCallback callback = nullptr;
};
struct NativeDescription {
    std::int32_t course_id = -1;
    menu::EffectGroup effects = menu::EffectGroup::common;
    NativeCapacity capacity;
    player::Root::Config player{};
    world::Callbacks world{};
    NativeInput input;
    // Exact initial Original omAddGObj recipe, independent of Actor callbacks.
    // Admission consumes it once during setup; it never authorizes live helpers.
    NativeHelper initial_helper;
    std::span<const actor::BehaviorCallbackBinding> callbacks;
    std::span<const std::uint32_t> graphs, weighted_states, roots;
    actor::RetainedFieldOptions retained_fields{};
    // Borrowed by Services until semantic-owner release, never copied/rebuilt.
    std::span<IsleGameObject** const> reference_fields;
    // Actual entry-owned Source buffers/revisions and initialized course sprites.
    // Borrowed through native detach and independent GPU/Photo joins.
    std::span<const source_geometry::VertexPublication> source_vertices;
    std::span<const scene::SpriteResource> extra_sprites;
    std::span<const IsleSprite* const> extra_sprite_roots;
    std::span<const sprite::BankMapping> extra_sprite_banks;
};

// Independent completion evidence. Only NativeHost can publish it, after actual
// native owners AND Storage have been destroyed. Never consult a dead Player.
class SceneJoinReceipt final {
public:
    SceneJoinReceipt() noexcept = default;
    SceneJoinReceipt(const SceneJoinReceipt&) = delete;
    SceneJoinReceipt& operator=(const SceneJoinReceipt&) = delete;
    SceneJoinReceipt(SceneJoinReceipt&&) = delete;
    SceneJoinReceipt& operator=(SceneJoinReceipt&&) = delete;
    bool joined() const noexcept { return joined_; }
private:
    friend class NativeHost;
    bool joined_ = false;
};
struct RetirementHooks {
    void* owner = nullptr;
    IsleGameResult (*release_references)(void*, class NativeHost&) noexcept = nullptr;
    IsleGameResult (*deactivate)(void*, class NativeHost&) noexcept = nullptr;
    void (*clear_scene)(void*) noexcept = nullptr;
};

// One non-template mechanical owner. No Original course policy or Source ABI is
// represented here. Root is canonical before Actor construction; callbacks may
// only enter compiled C fences after their returning C++ leaf has returned.
class NativeHost final {
public:
    NativeHost(const rom::Snapshot&, application::Manager&, game::State&,
        game_input::Service&, game_audio::Session&, host::GtlClock&,
        application::Photos&, world::ModuleState&, NativeDescription,
        bool require_cpu_renderer);
    ~NativeHost();
    NativeHost(const NativeHost&) = delete;
    NativeHost& operator=(const NativeHost&) = delete;
    // Nonexecuting PREPARED closure. Adapter binds scene borrows afterwards,
    // validates proposed services and publishes ACTIVE last, then invokes init.
    IsleGameResult prepare(actor::CourseContext) noexcept;
    IsleGameResult finish_initialization() noexcept;
    IsleGameResult update(std::uint32_t raw_game_time) noexcept;
    bool draw(scene::graphics::FrameContext = {}) noexcept;
    bool graphics_busy() const noexcept;
    scene::graphics::Poll poll() noexcept;
    scene::graphics::Completed completed_frame() const noexcept;
    IsleGameResult release_completed() noexcept;
    bool retire_scene(bool cancelling, RetirementHooks, SceneJoinReceipt&) noexcept;
    // Genuine actor-detach authorization, while Game/Root association is alive.
    bool detach_ready() const noexcept;
    bool closure_prepared() const noexcept;
    bool execution_open() const noexcept;
    bool exit_requested() const noexcept;
    bool initialized() const noexcept;
    bool retired() const noexcept;
    IsleGameResult error() const noexcept;
    const char* message() const noexcept;
    IsleGameResult fail(IsleGameResult, const char*) noexcept;

    // Returning mechanics for EXACT typed adapter thunks. These operations do
    // not select Source callbacks, infer identities, run Original policy or wait.
    IsleGameResult intervals(std::uint16_t update, std::uint16_t draw) noexcept;
    IsleGameResult near_clipping(bool disabled) noexcept;
    IsleGameResult exit_scene() noexcept;
    IsleGameResult destroy_world() noexcept;
    IsleGameResult publish_world() noexcept;
    IsleGameResult sky_follow() noexcept;
    IsleGameResult allocate_depth(std::uint32_t bytes, std::uint32_t alignment, void**) noexcept;
    IsleGameResult apply_screen(const scene::graphics::SourceScreen&, const void* depth) noexcept;
    IsleGameResult create_helper(std::uint32_t id, IsleGameSourceCallback,
        std::uint8_t link, std::uint32_t priority, IsleGameObject**) noexcept;
    IsleGameResult initialize_collisions(IsleGameObject* helper) noexcept;
    IsleGameResult create_cameras(std::uint32_t background) noexcept;
    IsleGameResult initialize_detector(IsleGameObject* helper) noexcept;
    IsleGameResult detector_enabled(bool) noexcept;
    IsleGameResult reset_photo_roll() noexcept;
    IsleGameResult environment_initialize() noexcept;
    IsleGameResult environment_cleanup() noexcept;
    IsleGameResult reset_effect_bank(std::size_t) noexcept;
    IsleGameResult install_effect_bank(menu::EffectGroup) noexcept;
    IsleGameResult publish_particle_camera(IsleGameObject**) noexcept;
    IsleGameResult publish_effect_emitters(IsleGameObject**) noexcept;
    IsleGameResult bind_item_collision(IsleItemCollisionProvider, void*) noexcept;
    IsleGameResult reference(IsleGameObject** field, IsleGameObject*) noexcept;
    bool valid_block(const world::Block*) const noexcept;
    bool owns_input(IsleGameObject*) const noexcept;
    IsleGameResult invalidate_input(IsleGameObject*) noexcept;
    IsleGameResult flute_command(std::int32_t*) noexcept;

    IsleGame* game() const noexcept;
    scene::Services* services() const noexcept;
    player::Root* root() const noexcept;
    player::Ui* ui() const noexcept;
    std::size_t extra_sprite_base() const noexcept;
    player::Renderer* renderer() const noexcept;
    world::World* world_owner() const noexcept;
    actor::Owner* actors() const noexcept;
    actor::Producer* actor_packet_producer() const noexcept;
    items::Owner* items_owner() const noexcept;
    player::cart::Owner* cart() const noexcept;
    player::camera_animation::NativeDriver* camera() const noexcept;
    const player::CartResources& cart_resources() const noexcept;
    const world::Resources& world_resources() const noexcept;
    const actor::Resources& actor_resources() const noexcept;
    const actor::BehaviorBindings* bindings() const noexcept;
    environment_sound::Environment* environment() const noexcept;
    game_audio::Session& audio() const noexcept;
    application::Photos& photos() const noexcept;
    application::Manager& manager() const noexcept;
    game::State& game_state() const noexcept;
    IsleGameObject* reference_owner() const noexcept;
    TunnelActorNativeState* tunnel_actor_state() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

template<class Adapter> class Lifecycle;
template<class Adapter> struct LifecycleCreateResult {
    IsleGameResult error = ISLE_GAME_OK;
    std::unique_ptr<Lifecycle<Adapter>> lifecycle;
    std::array<char,512> message{};
    explicit operator bool() const noexcept { return error == ISLE_GAME_OK && bool(lifecycle); }
};

// Compile-time Adapter contract (all methods noexcept except description/context):
// aliases ActorSource, LifecycleSource, SceneSetup; exact public actors/source;
// prepare(owner, domain, canonical_rom, entry_argument); run_entry is the actual
// C entry function pointer (a static constexpr value), never a C++ waiting thunk;
// description()->NativeDescription; context()->actor::CourseContext;
// bind_scene(host); activate(host); invoke_init(); source_status();
// release_references(host); deactivate(host); clear_scene();
// requested(), completed(), completion_ready(); setup()->const SceneSetup&;
// output()->int32; complete_scene(const SceneJoinReceipt&, IsleGameResult).
// prepare constructs the exact typed CourseState and stable field arrays once.
// NativeDescription callbacks point at this adapter/entry's fixed-address data.
// deactivate must tolerate never-ACTIVE partial construction without inventing a
// DETACHED transition. complete_scene must check receipt and call real typed C.
// No method invokes checked()/wait from C++; all Source invocations use a C fence.
template<class Adapter> class Lifecycle final {
public:
    using SceneSetup = typename Adapter::SceneSetup;
    static LifecycleCreateResult<Adapter> create(const rom::Snapshot& snapshot,
        application::Manager& manager, game::State& state, game_input::Service& input,
        game_audio::Session& audio, host::GtlClock& clock, application::Photos& photos,
        world::ModuleState& module, std::int32_t argument, bool cpu = false,
        EntryNativeProfile profile = EntryNativeProfile::unspecified) noexcept {
        LifecycleCreateResult<Adapter> result;
        try {
            auto entry = std::unique_ptr<Lifecycle>(new Lifecycle(snapshot,manager,state,input,audio,clock,photos,module,cpu));
            entry->native_profile_ = profile;
            if (isle_continuation_domain_create(&entry->domain_) != ISLE_CONT_OK) {
                result.error = ISLE_GAME_INVALID_STATE;
                std::snprintf(result.message.data(),result.message.size(),"independent course domain creation failed");
                return result;
            }
            auto error = entry->adapter_.prepare(entry.get(),entry->domain_,snapshot.canonical(),argument);
            if (error == ISLE_GAME_OK && isle_continuation_create(entry->domain_,Adapter::run_entry,
                &entry->adapter_.source,&entry->continuation_) != ISLE_CONT_OK) error = ISLE_GAME_INVALID_STATE;
            if (error != ISLE_GAME_OK) {
                result.error = error;
                std::snprintf(result.message.data(),result.message.size(),"independent typed course preparation failed");
                return result;
            }
            result.lifecycle = std::move(entry);
        } catch (const std::exception& e) {
            result.error = ISLE_GAME_INVALID_STATE;
            std::snprintf(result.message.data(),result.message.size(),"%s",e.what());
        } catch (...) {
            result.error = ISLE_GAME_INVALID_STATE;
            std::snprintf(result.message.data(),result.message.size(),"typed course creation threw");
        }
        return result;
    }
    ~Lifecycle() { if (retire() != ISLE_GAME_OK) std::terminate(); }
    Lifecycle(const Lifecycle&) = delete;
    Lifecycle& operator=(const Lifecycle&) = delete;
    Lifecycle(Lifecycle&&) = delete;
    Lifecycle& operator=(Lifecycle&&) = delete;
    // Original entry sets these before a Game/NativeHost exists.
    IsleGameResult intervals(std::uint16_t update, std::uint16_t draw) noexcept {
        return clock_.set_intervals(update,draw) == host::ClockError::none ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
    }
    IsleGameResult near_clipping(bool disabled) noexcept {
        if (host_) {
            const auto result = host_->near_clipping(disabled);
            if (result != ISLE_GAME_OK) return result;
        }
        near_disabled_ = disabled;
        return ISLE_GAME_OK;
    }
    IsleGameResult resume() noexcept {
        if (error_ != ISLE_GAME_OK) return error_;
        if (cancelling_) return ISLE_GAME_STOPPED;
        if (!continuation_ || initializing_ || (host_ && host_->game() &&
            isle_game_require_idle(host_->game()) != ISLE_GAME_OK)) return ISLE_GAME_INVALID_STATE;
        if (isle_continuation_resume(continuation_) != ISLE_CONT_OK)
            return fail(adapter_.source_status() == ISLE_GAME_OK ? ISLE_GAME_INVALID_STATE : adapter_.source_status(),
                isle_continuation_error(domain_));
        if (adapter_.source_status() != ISLE_GAME_OK) return fail(adapter_.source_status(),"typed Source entry failed");
        if (adapter_.completed()) {
            if (!receipt_.joined()) return fail(ISLE_GAME_INVALID_STATE,"Source completed before native scene joined");
            status_ = LifecycleStatus::completed;
        } else if (adapter_.requested() && !host_) status_ = LifecycleStatus::setup_pending;
        return ISLE_GAME_OK;
    }
    const SceneSetup* pending_setup() const noexcept {
        return status_ == LifecycleStatus::setup_pending && !cancelling_ && !host_ ? &adapter_.setup() : nullptr;
    }
    IsleGameResult initialize_scene() noexcept {
        if (!pending_setup() || initializing_) return ISLE_GAME_INVALID_STATE;
        initializing_ = true;
        try {
            host_ = std::make_unique<NativeHost>(snapshot_,manager_,state_,input_,audio_,clock_,photos_,module_,adapter_.description(),cpu_);
            const char* stage = "typed native near-clipping initialization failed";
            auto error = host_->near_clipping(near_disabled_);
            if (error == ISLE_GAME_OK) { stage = "typed native closure preparation failed"; error = host_->prepare(adapter_.context()); }
            if (error == ISLE_GAME_OK) { stage = "typed Source scene binding failed"; error = adapter_.bind_scene(*host_); }
            if (error == ISLE_GAME_OK) { stage = "typed Source activation failed"; error = adapter_.activate(*host_); }
            if (error == ISLE_GAME_OK) { stage = "typed Original initialization invocation failed"; error = adapter_.invoke_init(); }
            if (error == ISLE_GAME_OK) { stage = "typed Original initialization Source status failed"; error = adapter_.source_status(); }
            if (error == ISLE_GAME_OK) { stage = "typed native producer initialization failed"; error = host_->finish_initialization(); }
            initializing_ = false;
            if (error != ISLE_GAME_OK) return fail(error,host_->error() == ISLE_GAME_OK ? stage : host_->message());
            status_ = host_->exit_requested() ? LifecycleStatus::retiring : LifecycleStatus::running;
            return ISLE_GAME_OK;
        } catch (const std::exception& e) { initializing_ = false; return fail(ISLE_GAME_INVALID_STATE,e.what()); }
        catch (...) { initializing_ = false; return fail(ISLE_GAME_INVALID_STATE,"native scene composition threw"); }
    }
    IsleGameResult update(std::uint32_t raw_game_time) noexcept {
        if (status_ != LifecycleStatus::running || !host_ || cancelling_) return ISLE_GAME_INVALID_STATE;
        auto result = host_->update(raw_game_time);
        if (result == ISLE_GAME_OK) result = adapter_.source_status();
        if (result != ISLE_GAME_OK) return fail(result,host_->error() == ISLE_GAME_OK ? "typed course update failed" : host_->message());
        if (host_->exit_requested()) status_ = LifecycleStatus::retiring;
        return ISLE_GAME_OK;
    }
    bool draw(scene::graphics::FrameContext context = {}) noexcept {
        if (status_ != LifecycleStatus::running || !host_ || cancelling_) return false;
        const bool result = host_->draw(context);
        if (!result && host_->error() != ISLE_GAME_OK) fail(host_->error(),host_->message());
        return result;
    }
    bool graphics_busy() const noexcept { return host_ && host_->graphics_busy(); }
    scene::graphics::Poll poll() noexcept {
        if (!host_) return scene::graphics::Poll::idle;
        const auto result = host_->poll();
        if (result == scene::graphics::Poll::failed) fail(host_->error(),host_->message());
        return result;
    }
    scene::graphics::Completed completed_frame() const noexcept { return host_ ? host_->completed_frame() : scene::graphics::Completed{}; }
    IsleGameResult release_completed() noexcept {
        if (!host_) return ISLE_GAME_INVALID_STATE;
        const auto result = host_->release_completed();
        return result == ISLE_GAME_OK ? result : fail(result,host_->message());
    }
    bool retire_scene() noexcept {
        if (receipt_.joined()) return true;
        if (!host_) return false;
        if (adapter_.source_status() != ISLE_GAME_OK)
            fail(adapter_.source_status(),"typed Source failed while draining scene");
        if (host_->error() != ISLE_GAME_OK) fail(host_->error(),host_->message());
        if (!cancelling_ && error_ == ISLE_GAME_OK && !host_->exit_requested()) return false;
        if (error_ != ISLE_GAME_OK) cancelling_ = true;
        const bool joined = host_->retire_scene(cancelling_,hooks(),receipt_);
        if (host_->error() != ISLE_GAME_OK) fail(host_->error(),host_->message());
        if (!joined) return false;
        if (!cancelling_ && error_ == ISLE_GAME_OK && adapter_.requested() && !adapter_.completion_ready()) {
            const auto result = adapter_.complete_scene(receipt_,ISLE_GAME_OK);
            if (result != ISLE_GAME_OK) { fail(result,"typed completion rejected actual scene receipt"); return false; }
        }
        return true;
    }
    IsleGameResult cancel() noexcept {
        cancelling_ = true;
        if (continuation_ && isle_continuation_cancel(continuation_) != ISLE_CONT_OK)
            return fail(ISLE_GAME_INVALID_STATE,"independent course continuation cancellation did not join");
        if (host_ && !host_->retire_scene(true,hooks(),receipt_)) return ISLE_GAME_INVALID_STATE;
        if (error_ == ISLE_GAME_OK) status_ = LifecycleStatus::cancelled;
        return ISLE_GAME_OK;
    }
    IsleGameResult retire() noexcept {
        if (status_ == LifecycleStatus::retired) return ISLE_GAME_OK;
        if ((!receipt_.joined() || !adapter_.completed()) && cancel() != ISLE_GAME_OK) return ISLE_GAME_INVALID_STATE;
        if (continuation_) {
            if (isle_continuation_destroy(continuation_) != ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
            continuation_ = nullptr;
        }
        if (domain_) {
            if (isle_continuation_domain_destroy(domain_) != ISLE_CONT_OK) return ISLE_GAME_INVALID_STATE;
            domain_ = nullptr;
            adapter_.source.domain = nullptr;
        }
        host_.reset();
        status_ = LifecycleStatus::retired;
        return ISLE_GAME_OK;
    }
    LifecycleStatus status() const noexcept { return status_; }
    IsleGameResult error() const noexcept { return error_; }
    const char* message() const noexcept { return message_.data(); }
    std::optional<std::int32_t> result() const noexcept {
        if (cancelling_ || error_ != ISLE_GAME_OK || !receipt_.joined() || !adapter_.completed() ||
            adapter_.source_status() != ISLE_GAME_OK) return {};
        return adapter_.output();
    }
    const typename Adapter::LifecycleSource& source() const noexcept { return adapter_.source; }
    const typename Adapter::ActorSource& actors_source() const noexcept { return adapter_.actors; }
    Adapter& adapter() noexcept { return adapter_; }
    const Adapter& adapter() const noexcept { return adapter_; }
    NativeHost* native_host() const noexcept { return host_.get(); }
    const SceneJoinReceipt& scene_join_receipt() const noexcept { return receipt_; }
    // Independent application borrows, valid before Game and after Storage join.
    const rom::Snapshot& snapshot() const noexcept { return snapshot_; }
    application::Manager& manager() const noexcept { return manager_; }
    application::Photos& photos() const noexcept { return photos_; }
    game::State& game_state() const noexcept { return state_; }
    game_input::Service& input() const noexcept { return input_; }
    game_audio::Session& audio() const noexcept { return audio_; }
    host::GtlClock& clock() const noexcept { return clock_; }
    EntryNativeProfile native_profile() const noexcept { return native_profile_; }
    bool entry_joined() const noexcept {
        if (!continuation_) return true;
        IsleContinuationState state;
        if (isle_continuation_state(continuation_,&state)!=ISLE_CONT_OK) return false;
        return state==ISLE_CONT_COMPLETED || state==ISLE_CONT_CANCELLED || state==ISLE_CONT_FAILED;
    }
    IsleGameResult fail(IsleGameResult result, const char* text) noexcept {
        if (result == ISLE_GAME_OK) return result;
        if (error_ == ISLE_GAME_OK) { error_ = result; std::snprintf(message_.data(),message_.size(),"%s",text ? text : "course failure"); }
        status_ = LifecycleStatus::failed;
        return error_;
    }
private:
    Lifecycle(const rom::Snapshot& s, application::Manager& m, game::State& st,
        game_input::Service& i, game_audio::Session& a, host::GtlClock& c,
        application::Photos& p, world::ModuleState& w, bool cpu)
        : snapshot_(s), manager_(m), state_(st), input_(i), audio_(a), clock_(c), photos_(p), module_(w), cpu_(cpu) {}
    RetirementHooks hooks() noexcept {
        return {this,
            [](void* owner, NativeHost& host) noexcept { return static_cast<Lifecycle*>(owner)->adapter_.release_references(host); },
            [](void* owner, NativeHost& host) noexcept { return static_cast<Lifecycle*>(owner)->adapter_.deactivate(host); },
            [](void* owner) noexcept { static_cast<Lifecycle*>(owner)->adapter_.clear_scene(); }};
    }
    const rom::Snapshot& snapshot_;
    application::Manager& manager_;
    game::State& state_;
    game_input::Service& input_;
    game_audio::Session& audio_;
    host::GtlClock& clock_;
    application::Photos& photos_;
    world::ModuleState& module_;
    bool cpu_ = false, initializing_ = false, cancelling_ = false, near_disabled_ = false;
    Adapter adapter_{};
    EntryNativeProfile native_profile_ = EntryNativeProfile::unspecified;
    IsleContinuationDomain* domain_ = nullptr;
    IsleContinuation* continuation_ = nullptr;
    LifecycleStatus status_ = LifecycleStatus::ready;
    IsleGameResult error_ = ISLE_GAME_OK;
    std::array<char,512> message_{};
    SceneJoinReceipt receipt_;
    std::unique_ptr<NativeHost> host_;
};
} // namespace isle::course
#endif
