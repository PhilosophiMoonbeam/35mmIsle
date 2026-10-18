#include "post_course_replay.hpp"
#include "35mmisle/continuation.h"
#include <array>
#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>

/* Authorities: pinned app_render/score.c230-327,488-674;
 * app_render/47380.c recorded reconstruction; window/847B60.c59-96.
 * Source control/waits remain exclusively in post_course_replay_source.c. */
struct IslePostCourseReplay {
    const std::thread::id creator = std::this_thread::get_id();
    IsleGame* game;
    isle::application::Photos& photos;
    isle::photo::Replay& replay;
    isle::host::Progress& progress;
    isle::game_audio::Session& audio;
    isle::post_course::ReplayNativeProgress native;
    IsleGameResult status = ISLE_GAME_OK;
    const char* diagnostic = "";
    IslePhotoRecord record{};
    const std::uint64_t* revision = nullptr;
    std::array<std::byte, sizeof(IslePlayerPhotoData)> unchanged{};
    std::uint64_t serial = 0, sound_played = 0;
    isle::photo::ReplayKind kind = isle::photo::ReplayKind::score;
    bool active = false, absent = false, consumed = false, leased = false;
    bool preview_exposed = false, terminal = false, stopping = false, retired = false;

    IslePostCourseReplay(IsleGame* g, isle::application::Photos& p, isle::photo::Replay& r,
        isle::host::Progress& h, isle::game_audio::Session& a,
        isle::post_course::ReplayNativeProgress n)
        : game(g), photos(p), replay(r), progress(h), audio(a), native(n) {
        if (!game || !native.owner || !native.service || replay.busy() || replay.gpu_busy() || replay.error()[0])
            throw std::invalid_argument("post-course Replay requires actual exclusive native services");
    }
    bool is_creator() const noexcept { return creator == std::this_thread::get_id(); }
    template<class F>
    auto on_creator(F&& function, decltype(function()) failure) noexcept -> decltype(function()) {
        using Result = decltype(function());
        using Function = std::remove_reference_t<F>;
        struct Request {
            IslePostCourseReplay* owner;
            Function* function;
            Result result;
        } request{this,&function,failure};
        // Existing synchronous continuation service: no Source wait, OM/GTL
        // update, retrace or extra resume. Executor/SDL access stays on its
        // actual constructing OS thread while the Source worker is parked.
        const auto result = isle_continuation_on_owner([](void* user) {
            auto& call = *static_cast<Request*>(user);
            if (call.owner->is_creator()) call.result = (*call.function)();
        },&request);
        return result == ISLE_CONT_OK ? request.result : failure;
    }
    IsleGameResult fail(IsleGameResult value, const char* message) noexcept {
        if (value == ISLE_GAME_OK) value = ISLE_GAME_INVALID_STATE;
        if (status == ISLE_GAME_OK || (status == ISLE_GAME_STOPPED && value != ISLE_GAME_STOPPED)) {
            status = value; diagnostic = message;
        }
        stopping = true;
        replay.cancel();
        return status;
    }
    void observe_host() noexcept {
        using S = isle::host::ProgressStatus;
        if (progress.status() == S::failed || progress.deadline_expired())
            fail(ISLE_GAME_EXECUTION_FAILURE, "actual Host progress failed or deadline expired");
        else if (progress.status() == S::stopped)
            fail(ISLE_GAME_STOPPED, "actual Host requested stop");
        const auto state = isle_game_status(game);
        if (state != ISLE_GAME_OK) fail(state, "actual Game execution stopped or failed");
    }
    bool handle(IslePostCourseReplayOperation operation) noexcept {
        if (active && operation.owner == this && operation.serial && operation.serial == serial) return true;
        fail(ISLE_GAME_INVALID_STATE, "stale or foreign replay operation");
        return false;
    }
    bool current() noexcept {
        if (!leased || !revision || *revision != record.revision) {
            fail(ISLE_GAME_INVALID_STATE, "canonical replay revision changed"); return false;
        }
        const auto actual = photos.record(record.storage, record.index);
        const auto identity = photos.identify(record.photo);
        if (!actual || !identity || actual.value.owner != record.owner ||
            actual.value.photo != record.photo || actual.value.storage != record.storage ||
            actual.value.index != record.index || actual.value.revision != record.revision ||
            identity.value.owner != record.owner || identity.value.storage != record.storage ||
            identity.value.index != record.index || identity.value.photo != record.photo ||
            identity.value.revision != record.revision || photos.source().owner ||
            std::memcmp(unchanged.data(), record.photo, unchanged.size()) != 0) {
            fail(ISLE_GAME_INVALID_STATE, "canonical replay identity or complete record changed"); return false;
        }
        return true;
    }
    bool release_lease() noexcept {
        if (!leased) return true;
        if (photos.release_replay(this)) {
            fail(ISLE_GAME_INVALID_STATE, "canonical replay lease release rejected"); return false;
        }
        leased = false; revision = nullptr;
        return true;
    }
    IslePostCourseReplayStep poll() noexcept {
        using S = isle::photo::ReplaySignal;
        observe_host();
        if (!active && !stopping) fail(ISLE_GAME_INVALID_STATE, "no active replay operation");
        if (terminal) return {status == ISLE_GAME_STOPPED ? ISLE_POST_REPLAY_CANCELLED : ISLE_POST_REPLAY_FAILED,0};
        if (absent && !stopping) return {ISLE_POST_REPLAY_ABSENT,0};
        if (consumed && !stopping) { fail(ISLE_GAME_INVALID_STATE, "replay result already consumed"); }
        if (leased) (void)current();
        const auto step = replay.poll();
        switch (step.signal) {
        case S::pending: return {ISLE_POST_REPLAY_PENDING,0};
        case S::play_sound85: return {ISLE_POST_REPLAY_SOUND85,step.obligation};
        case S::wait60: return {ISLE_POST_REPLAY_WAIT60,step.obligation};
        case S::wait1: return {ISLE_POST_REPLAY_WAIT1,step.obligation};
        case S::result: return {ISLE_POST_REPLAY_RESULT,0};
        case S::cancelled:
        case S::failed:
            if (step.signal == S::failed) fail(ISLE_GAME_EXECUTION_FAILURE, replay.error());
            else if (status == ISLE_GAME_OK) fail(ISLE_GAME_STOPPED, "replay cancelled");
            terminal = release_lease();
            return {status == ISLE_GAME_STOPPED ? ISLE_POST_REPLAY_CANCELLED : ISLE_POST_REPLAY_FAILED,0};
        case S::idle:
            fail(ISLE_GAME_INVALID_STATE, "Replay unexpectedly idle before consumption");
            return {ISLE_POST_REPLAY_PENDING,0};
        }
        fail(ISLE_GAME_INVALID_STATE, "unknown Replay signal");
        return {ISLE_POST_REPLAY_PENDING,0};
    }
    IslePostCourseReplayStep begin(const IslePlayerPhotoData* photo,
        isle::photo::ReplayKind requested, int32_t width, int32_t height, float scale,
        IslePostCourseReplayOperation* output) noexcept {
        if (output) *output = {};
        observe_host();
        if (!output || active || stopping || retired || status != ISLE_GAME_OK ||
            serial == std::numeric_limits<std::uint64_t>::max()) {
            fail(ISLE_GAME_INVALID_STATE, "replay admission rejected");
            return {ISLE_POST_REPLAY_FAILED,0};
        }
        active = true; absent = !photo; consumed = false; terminal = false;
        preview_exposed = false; sound_played = 0; kind = requested; record = {};
        *output = {this,++serial};
        const auto identity = photos.identify(photo);
        if (!identity) {
            fail(ISLE_GAME_INVALID_ARGUMENT, "Photo pointer has no canonical owner identity");
            return poll();
        }
        record = identity.value;
        if (absent) return {ISLE_POST_REPLAY_ABSENT,0};
        if (photos.acquire_replay(record,this,&revision)) {
            fail(ISLE_GAME_INVALID_STATE, "canonical replay read lease unavailable"); return poll();
        }
        leased = true;
        std::memcpy(unchanged.data(),record.photo,unchanged.size());
        if (!current()) return poll();
        bool dash = false;
        if (kind == isle::photo::ReplayKind::score) {
            // Pinned enums.h PFID_HAS_DASH_ENGINE=5, same Source save service.
            const auto flag = isle_player_photo_checkPlayerFlag(&photos.source(),5);
            if (flag < 0 || photos.source().status != ISLE_GAME_OK) {
                fail(ISLE_GAME_INVALID_STATE, "canonical dash-engine query failed"); return poll();
            }
            dash = flag != 0;
        }
        const isle::photo::ReplayRequest request{kind,{record.photo,revision,record.revision},dash,width,height,scale};
        if (!replay.begin(request)) fail(ISLE_GAME_EXECUTION_FAILURE, "actual Replay begin failed");
        return poll();
    }
    IsleGameResult retire(bool& done) noexcept {
        stopping = true;
        replay.cancel();
        done = false;
        // A terminal, including a failed one, is the only native lifetime fence.
        if (leased) (void)current();
        if (!replay.retire()) return status;
        if (replay.error()[0]) fail(ISLE_GAME_EXECUTION_FAILURE, replay.error());
        if (!release_lease()) return status;
        terminal = true; retired = true; active = false; done = true;
        return status;
    }
};

namespace isle::post_course {
struct Replay::Impl : IslePostCourseReplay { using IslePostCourseReplay::IslePostCourseReplay; };
Replay::Replay(IsleGame* game, application::Photos& photos, photo::Replay& replay,
    host::Progress& progress, game_audio::Session& audio, ReplayNativeProgress native)
    : p_(std::make_unique<Impl>(game,photos,replay,progress,audio,native)) {}
Replay::~Replay() noexcept { if (!p_->retired || p_->leased || p_->active) std::terminate(); }
IslePostCourseReplay* Replay::source() noexcept { return p_.get(); }
IsleGameResult Replay::retire(bool& done) noexcept { return p_->retire(done); }
}

extern "C" {
IsleGame* isle_post_replay_game(IslePostCourseReplay* p) { return p ? p->game : nullptr; }
IsleGameResult isle_post_replay_status(IslePostCourseReplay* p) { return p ? p->status : ISLE_GAME_INVALID_ARGUMENT; }
const char* isle_post_replay_error(IslePostCourseReplay* p) { return p ? p->diagnostic : "NULL replay context"; }
IsleGameResult isle_post_replay_fail(IslePostCourseReplay* p, IsleGameResult result) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_fail(p,result);
    },ISLE_GAME_WRONG_OWNER);
    return p ? p->fail(result,"Source replay operation failed") : ISLE_GAME_INVALID_ARGUMENT;
}
IslePostCourseReplayStep isle_post_replay_score_begin(IslePostCourseReplay* p,
    const IslePlayerPhotoData* photo, IslePostCourseReplayOperation* output) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_score_begin(p,photo,output);
    },IslePostCourseReplayStep{ISLE_POST_REPLAY_FAILED,0});
    if (!p) { if (output) *output = {}; return {ISLE_POST_REPLAY_FAILED,0}; }
    return p->begin(photo,isle::photo::ReplayKind::score,320,210,1,output);
}
IslePostCourseReplayStep isle_post_replay_preview_begin(IslePostCourseReplay* p,
    const IslePlayerPhotoData* photo, int32_t width, int32_t height, float scale,
    IslePostCourseReplayOperation* output) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_preview_begin(p,photo,width,height,scale,output);
    },IslePostCourseReplayStep{ISLE_POST_REPLAY_FAILED,0});
    if (!p) { if (output) *output = {}; return {ISLE_POST_REPLAY_FAILED,0}; }
    return p->begin(photo,isle::photo::ReplayKind::preview,width,height,scale,output);
}
IslePostCourseReplayStep isle_post_replay_poll(IslePostCourseReplay* p, IslePostCourseReplayOperation operation) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_poll(p,operation);
    },IslePostCourseReplayStep{ISLE_POST_REPLAY_FAILED,0});
    if (!p) return {ISLE_POST_REPLAY_FAILED,0};
    (void)p->handle(operation);
    return p->poll();
}
IsleGameResult isle_post_replay_sound85(IslePostCourseReplay* p,
    IslePostCourseReplayOperation operation, uint64_t obligation) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_sound85(p,operation,obligation);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!p->handle(operation)) return p->status;
    const auto step = p->poll();
    if (p->status != ISLE_GAME_OK) return p->status;
    if (step.signal != ISLE_POST_REPLAY_SOUND85 || !obligation || step.obligation != obligation ||
        p->sound_played == obligation) return p->fail(ISLE_GAME_INVALID_STATE,"invalid sound85 obligation");
    int slot = -1;
    if (p->audio.play_sound(85,{},slot) != isle::audio::Error::none)
        return p->fail(ISLE_GAME_EXECUTION_FAILURE,"actual Source sound85 failed");
    p->sound_played = obligation;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_replay_acknowledge(IslePostCourseReplay* p,
    IslePostCourseReplayOperation operation, uint64_t obligation) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_acknowledge(p,operation,obligation);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!p->handle(operation)) return p->status;
    const auto step = p->poll();
    if (p->status != ISLE_GAME_OK) return p->status;
    if (!obligation || obligation != step.obligation ||
        (step.signal != ISLE_POST_REPLAY_SOUND85 && step.signal != ISLE_POST_REPLAY_WAIT60 && step.signal != ISLE_POST_REPLAY_WAIT1) ||
        (step.signal == ISLE_POST_REPLAY_SOUND85 && p->sound_played != obligation) || !p->current() ||
        !p->replay.acknowledge(obligation)) return p->fail(ISLE_GAME_INVALID_STATE,"Replay acknowledgment rejected");
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_replay_take_score(IslePostCourseReplay* p,
    IslePostCourseReplayOperation operation, IslePostCourseScoredPhoto* output) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_take_score(p,operation,output);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!output || !p->handle(operation)) return p->fail(ISLE_GAME_INVALID_ARGUMENT,"invalid score result destination");
    const auto step = p->poll();
    if (p->status != ISLE_GAME_OK) return p->status;
    if (step.signal != ISLE_POST_REPLAY_RESULT || p->kind != isle::photo::ReplayKind::score || !p->current())
        return p->fail(ISLE_GAME_INVALID_STATE,"score result unavailable");
    const auto result = p->replay.result();
    if (!result.score || result.kind != isle::photo::ReplayKind::score)
        return p->fail(ISLE_GAME_INVALID_STATE,"actual Replay score missing");
    const auto& s = *result.score;
    const auto flags = s.photo_flags;
    output->photo = *p->record.photo;
    output->score = {s.total,s.same_species_bonus,s.same_species_number,
        static_cast<uint8_t>(s.well_framed),s.pose_id,s.pokemon_in_focus,
        s.proximity,s.completeness,s.pose_points,s.special_bonus,s.special_id};
    // Copy above, acknowledgment below, canonical mutation LAST. No result
    // pointer is read after acknowledge_result invalidates its borrow.
    if (!p->current() || !p->replay.acknowledge_result() || !p->release_lease()) {
        *output = {}; return p->fail(ISLE_GAME_INVALID_STATE,"score result acknowledgment failed");
    }
    p->consumed = true;
    try {
        if (p->photos.apply_photo_flags(p->record,flags)) {
            *output = {}; return p->fail(ISLE_GAME_INVALID_STATE,"canonical score flags apply failed");
        }
    } catch (...) { *output = {}; return p->fail(ISLE_GAME_INVALID_STATE,"canonical score flags apply threw"); }
    output->photo.unk_00_16 = flags;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_replay_preview_result(IslePostCourseReplay* p,
    IslePostCourseReplayOperation operation, IslePostCoursePreview* output) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_preview_result(p,operation,output);
    },ISLE_GAME_WRONG_OWNER);
    if (output) *output = {};
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!output || !p->handle(operation)) return p->fail(ISLE_GAME_INVALID_ARGUMENT,"invalid preview result destination");
    const auto step = p->poll();
    if (p->status != ISLE_GAME_OK) return p->status;
    if (step.signal != ISLE_POST_REPLAY_RESULT || p->kind != isle::photo::ReplayKind::preview || !p->current())
        return p->fail(ISLE_GAME_INVALID_STATE,"preview result unavailable");
    const auto result = p->replay.result();
    if (result.kind != isle::photo::ReplayKind::preview || result.rgba5551.empty() ||
        !result.width || !result.height || result.stride_bytes < result.width * 2 ||
        result.rgba5551.size() < static_cast<size_t>(result.height-1)*result.stride_bytes + result.width*2)
        return p->fail(ISLE_GAME_INVALID_STATE,"actual completed preview extent invalid");
    *output = {reinterpret_cast<const uint8_t*>(result.rgba5551.data()),result.rgba5551.size(),
        result.stride_bytes,result.width,result.height};
    p->preview_exposed = true;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_replay_acknowledge_preview(IslePostCourseReplay* p,
    IslePostCourseReplayOperation operation) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_acknowledge_preview(p,operation);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!p->handle(operation)) return p->status;
    const auto step = p->poll();
    if (p->status != ISLE_GAME_OK) return p->status;
    if (step.signal != ISLE_POST_REPLAY_RESULT || p->kind != isle::photo::ReplayKind::preview ||
        !p->preview_exposed || !p->current() || !p->replay.acknowledge_result() || !p->release_lease())
        return p->fail(ISLE_GAME_INVALID_STATE,"preview consumption acknowledgment rejected");
    p->consumed = true;
    return ISLE_GAME_OK;
}
IsleGameResult isle_post_replay_operation_release(IslePostCourseReplay* p, IslePostCourseReplayOperation operation) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_operation_release(p,operation);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    if (!p->handle(operation)) return p->status;
    if (p->leased || (!p->consumed && !p->terminal && !(p->absent && !p->stopping)))
        return p->fail(ISLE_GAME_INVALID_STATE,"replay release before actual consumption or retirement");
    p->active = false; p->record = {}; p->preview_exposed = false;
    return p->status;
}
IsleGameResult isle_post_replay_cancel(IslePostCourseReplay* p) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_cancel(p);
    },ISLE_GAME_WRONG_OWNER);
    return p ? p->fail(ISLE_GAME_STOPPED,"Source replay cancelled") : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult isle_post_replay_progress(IslePostCourseReplay* p) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_progress(p);
    },ISLE_GAME_WRONG_OWNER);
    if (!p) return ISLE_GAME_INVALID_ARGUMENT;
    (void)p->progress.poll();
    p->observe_host();
    const auto result = p->native.service(p->native.owner);
    if (result != ISLE_GAME_OK) p->fail(result,"actual native replay progress failed");
    (void)p->progress.idle();
    p->observe_host();
    return p->status;
}
IsleGameResult isle_post_replay_retire(IslePostCourseReplay* p, int* retired) {
    if (p && !p->is_creator()) return p->on_creator([=] {
        return isle_post_replay_retire(p,retired);
    },ISLE_GAME_WRONG_OWNER);
    if (retired) *retired = 0;
    if (!p || !retired) return ISLE_GAME_INVALID_ARGUMENT;
    bool done = false;
    const auto result = p->retire(done);
    *retired = done ? 1 : 0;
    return result;
}
}
