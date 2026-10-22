// Opt-in PRIVATE_ROM NEW_OWNED_EMPTY_ROOT; NEVER a ROM-free CTest.
// These canonical records are explicitly AUTHORED, NOT captured gameplay or an
// Original-machine oracle. Real Replay/feedback/RSP/RDP, Host wall clock, SDL
// dummy Audio, application Photos/GameSession, typed C kind0 and Window raster.
// OM updates below are directly authored native updates: no GTL/retrace/update
// credits, Runtime scene completion, hardware playback or Windows parity claim.
// Initial display publishes admitted common pre-render/light assets and real
// Window residency. Constructor failures retire actual owners before unwinding.
#include "post_course_replay.hpp"
#include "post_course_window.hpp"
#include "post_course_replay_probe.h"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
namespace fs = std::filesystem;
namespace pc = isle::post_course;
namespace pg = isle::photo;
namespace sg = isle::scene::graphics;
namespace sc = isle::scene;
namespace ui = isle::window;
using Time = std::chrono::steady_clock;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void ok(IsleGameResult result, const char* message) { require(result == ISLE_GAME_OK, message); }

struct Fixture {
    const isle::rom::Snapshot& rom;
    isle::game_host::Host& host;
    isle::host::Progress& progress;
    isle::application::Photos& photos;
    isle::game_input::Service& input;
    IsleReplayProbe probe{};
    sc::Storage storage;
    isle::world::ModuleState module;
    std::unique_ptr<sg::Frame> gpu;
    std::unique_ptr<pg::Replay> pipeline;
    std::unique_ptr<pc::Replay> replay;
    std::shared_ptr<const ui::Resources> common;
    std::shared_ptr<const pc::Resources> resources;
    std::unique_ptr<sc::Services> services;
    std::shared_ptr<ui::Heap> heap = std::make_shared<ui::Heap>(2 * 1024 * 1024);
    std::unique_ptr<isle::sprite::Producer> producer;
    std::vector<isle::sprite::AssetSpan> immutable;
    std::unique_ptr<ui::Text> text;
    std::unique_ptr<ui::Frame> frame;
    std::unique_ptr<ui::Focus> focus;
    std::unique_ptr<ui::Layout> layout;
    std::unique_ptr<ui::Buttons> buttons;
    std::unique_ptr<pc::Window> window;
    IsleGameSprite* thumbnail_sprite = nullptr;
    IsleGameObject* witness = nullptr;
    std::vector<IsleGameObject*> source_owners;
    std::array<pc::WindowRasterSlot,1> thumbnail_slots{};
    std::array<pc::WindowRasterSlot,1> element_slots{};
    std::array<std::byte,isle::save::game_photo_bytes> wire{};
    std::vector<std::uint8_t> preview = std::vector<std::uint8_t>(0x20d00);
    std::thread::id creator = std::this_thread::get_id();
    unsigned acquisitions = 0, releases = 0, services_count = 0, display_completed = 0, display_presented = 0;
    unsigned ticks = 0, source_joined = 0, sources_created = 0;
    std::uint64_t display_ordinal = 0;
    bool wire_watch = false, witness_requested = false;

    Fixture(const isle::rom::Snapshot& r, isle::game_host::Host& h, isle::host::Progress& p,
            isle::application::Photos& ph, isle::game_input::Service& in, isle::game::State& state)
        : rom(r),host(h),progress(p),photos(ph),input(in) {
        try { initialize(state); }
        catch (const std::exception& e) {
            // A constructor-body handler runs BEFORE member destruction. In
            // particular, Replay must reach its real retirement fence here.
            std::fprintf(stderr,"Replay consumer construction: %.512s\n",e.what());
            cleanup();
            throw;
        } catch (...) {
            std::fputs("Replay consumer construction: unknown exception\n",stderr);
            cleanup();
            throw;
        }
    }
    void initialize(isle::game::State& state) {
        ok(storage.create(),"actual Replay NativeGame creation");
        sg::Config config; config.require_cpu = true;
        config.memory.commands = {0x6b0000,0x10000};
        config.memory.workspace = {0x6c0000,0x10000};
        config.memory.output = {0x6d0000,0x40000};
        auto made = sg::Frame::create(rom,config);
        require(bool(made),made.message.data()); gpu = std::move(made.frame);
        sg::SourceScreen screen; screen.flags = 0x16a99;
        require(gpu->configure_source_screen(screen,nullptr),gpu->message());
        common = ui::Resources::load(rom);
        resources = pc::Resources::load(rom,common,pc::Resources::Kind::camera_check);
        const auto bytes = pc::Window::thumbnail_plane_bytes(*resources,0x80201520);
        require(bytes && bytes <= 0x10000,"bounded real thumbnail plane");
        thumbnail_slots[0] = {{0x300000,0x310000},bytes};
        element_slots[0] = {{0x320000,0x321000},0x1000};
        std::vector<isle::menu::RamRange> external;
        external.reserve(gpu->reservations().size()+4);
        for (const auto range:gpu->reservations()) {
            if (range.address == 0 && range.size == isle::graphics::bss_end) continue;
            external.push_back({range.address,range.size});
        }
        for (const auto& slots:{std::span<const pc::WindowRasterSlot>(thumbnail_slots),
                               std::span<const pc::WindowRasterSlot>(element_slots)})
            for (const auto& slot:slots)
                for (const auto address:slot.physical) external.push_back({address,slot.plane_capacity});
        pg::ReplayConfig rc; auto& m = rc.memory;
        m.world = {{0x400000,0x20000},{0x420000,0x10000},{0x430000,0x10000}};
        m.subjects = {{0x440000,0x60000},{0x4a0000,0x20000},{0x4c0000,0x20000},{}};
        m.items = {{0x4e0000,0x10000},{0x4f0000,0x10000},{0x500000,0x10000},{}};
        m.commands = {0x510000,0x20000}; m.workspace = {0x530000,0x10000}; m.output = {0x540000,0x40000};
        m.effects = {0x580000,0x10000}; m.effect_assets = {0x630000,0x80000};
        m.feedback = {0x7be600,0x27000}; m.preview_color = {0x7be600,0x20d00}; m.preview_depth = {0x7df300,0x20d00};
        rc.external_reservations = external;
        pipeline = std::make_unique<pg::Replay>(storage.game(),rom,module,pg::ReplayGpuAccess{this,acquire,release},rc);
        std::vector<isle::sprite::RamRange> protected_ranges;
        for (const auto range:pipeline->reservations()) protected_ranges.push_back({std::uint32_t(range.offset),std::uint32_t(range.size)});
        require(gpu->reserve_source_gpu_storage(protected_ranges),gpu->message());
        replay = std::make_unique<pc::Replay>(storage.game(),photos,*pipeline,progress,*host.audio(),pc::ReplayNativeProgress{this,service});
        producer = std::make_unique<isle::sprite::Producer>(resources->roots(),resources->banks());
        const auto sprite_assets=producer->assets();
        const auto common_assets=common->common_render_assets();
        immutable.reserve(sprite_assets.size()+common_assets.size());
        immutable.insert(immutable.end(),sprite_assets.begin(),sprite_assets.end());
        immutable.insert(immutable.end(),common_assets.begin(),common_assets.end());
        auto view = resources->view(); view.pools = {128,128,256,8,1024};
        services = std::make_unique<sc::Services>(storage.game(),storage.arena(),view,input,*host.audio(),state);
        require(!services->failed(),services->error());
        text = std::make_unique<ui::Text>(common->font());
        frame = std::make_unique<ui::Frame>(*services,*common);
        focus = std::make_unique<ui::Focus>(*services,*common);
        layout = std::make_unique<ui::Layout>(*services,*common);
        buttons = std::make_unique<ui::Buttons>(common,heap,*services);
        pc::WindowCapacity capacity; capacity.rasters = element_slots; capacity.thumbnails = thumbnail_slots;
        window = std::make_unique<pc::Window>(*services,*text,*frame,*focus,*layout,*buttons,*resources,heap,*producer,capacity);
        probe.game = storage.game(); probe.replay = replay->source(); probe.photos = photos.native_source(); probe.window = window->source();
        probe.preview_copy = preview.data(); probe.preview_capacity = preview.size();
        IsleGameObject* object = nullptr; IsleGameSprite* parent = nullptr;
        ok(isle_post_window_sprite_object_create(probe.window,6,0x801fe318,&object,&parent),"actual ROM thumbnail parent");
        ok(isle_post_window_sprite_child(probe.window,parent,0x80201520,&thumbnail_sprite),"actual ROM thumbnail child");
        ok(isle_post_window_thumbnail_create(probe.window,thumbnail_sprite,0x80201520,&probe.thumbnail),"actual Producer-registered thumbnail");
        ok(isle_game_add_object(storage.game(),0x719,0,0,nullptr,nullptr,&witness),"native service retirement witness");
        ok(isle_game_object_set_retire_callback(storage.game(),witness,isle_replay_probe_joined,&probe),"actual pure-C joined observer");
        // A genuine display submission initially denies Replay RAM admission.
        // The Source worker's returning Progress leaf must complete/map/release
        // it and hand its actual copied image to the real Host, on creator OS thread.
        sc::render::Pass clear; clear.kind = sc::render::Kind::clear; clear.flags = 2; clear.background_rgba = 0x182840ff;
        sc::render::Input display; display.game = storage.game();
        display.resources.identity = 1;
        display.resources.sprites = producer.get();
        display.resources.immutable = immutable;
        display.resources.pre_render = common->common_pre_render();
        display.resources.framebuffers = {screen.framebuffer_identity[0] & 0x1fffffffu,
                                          screen.framebuffer_identity[1] & 0x1fffffffu};
        display.passes = {&clear,1};
        require(gpu->begin(display),gpu->message());
        require(gpu->submission() != 0,"real initial display has no submission token");
    }
    ~Fixture() { cleanup(); }
    [[noreturn]] static void cleanup_failure(const char* message) noexcept {
        std::fprintf(stderr,"Replay consumer cleanup: %.512s\n",message);
        std::fflush(stderr);
        // Retain callback/GPU backing rather than unwinding unsafe members.
        std::_Exit(EXIT_FAILURE);
    }
    void cleanup() noexcept {
        const auto deadline = Time::now() + std::chrono::seconds(120);
        // Join every actual Source invocation before releasing the pure-C
        // callback/probe backing. No Source update or invented terminal.
        for (auto* owner:source_owners)
            if (isle_game_delete_object(storage.game(),owner) != ISLE_GAME_OK)
                cleanup_failure("actual Source owner failed to join");
        source_owners.clear();
        if (replay) (void)isle_post_replay_cancel(replay->source());
        else if (pipeline) pipeline->cancel();
        bool retired = !pipeline;
        bool reported_service_failure = false;
        while (!retired) {
            if (replay) (void)replay->retire(retired);
            else retired = pipeline->retire();
            if (!retired) {
                (void)progress.poll();
                const auto result = service(this);
                if (result != ISLE_GAME_OK && !reported_service_failure) {
                    std::fprintf(stderr,"Replay consumer cleanup service failed (%d): %.256s\n",
                                 int(result),gpu ? gpu->message() : "no Frame");
                    reported_service_failure = true;
                }
                if (Time::now() >= deadline)
                    cleanup_failure("actual Replay/native/GPU retirement deadline");
                std::this_thread::yield();
            }
        }
        replay.reset(); pipeline.reset();
        // A normal display can still borrow the real Window Producer/raster,
        // including after a partially completed initial submission.
        while (gpu && !gpu->quiescent()) {
            (void)gpu->poll();
            if (Time::now() >= deadline)
                cleanup_failure("actual display GPU/readback retirement deadline");
            std::this_thread::yield();
        }
        if (gpu && !gpu->retire()) cleanup_failure("actual Frame retirement rejected");
        gpu.reset();
        if (witness && !witness_requested) {
            if (isle_game_delete_object(storage.game(),witness) != ISLE_GAME_OK)
                cleanup_failure("actual witness owner failed to join");
            witness_requested = true;
        }
        if (window && window->retire() != ISLE_GAME_OK)
            cleanup_failure("actual Window/raster retirement rejected");
        window.reset(); buttons.reset(); layout.reset(); focus.reset(); frame.reset();
        if (text) text->detach();
        text.reset(); services.reset(); producer.reset();
        if (storage.destroy() != ISLE_GAME_OK)
            cleanup_failure("actual native Game retirement rejected");
    }
    static pg::ReplayAcquire acquire(void* context, pg::ReplayGpuBinding& out) noexcept {
        auto& f = *static_cast<Fixture*>(context);
        if (std::this_thread::get_id() != f.creator) return pg::ReplayAcquire::failed;
        sg::SourceGpuBinding binding;
        switch (f.gpu->acquire_source_gpu(binding)) {
        case sg::SourceGpuAcquire::pending: return pg::ReplayAcquire::pending;
        case sg::SourceGpuAcquire::failed: return pg::ReplayAcquire::failed;
        case sg::SourceGpuAcquire::admitted:
            ++f.acquisitions;out={binding.executor,binding.programs,binding.epoch,binding.admission,f.gpu.get()};
            return pg::ReplayAcquire::admitted;
        }
        return pg::ReplayAcquire::failed;
    }
    static bool release(void* context, const pg::ReplayGpuBinding& binding) noexcept {
        auto& f = *static_cast<Fixture*>(context);
        if (std::this_thread::get_id() != f.creator) return false;
        if(binding.frame!=f.gpu.get()||
           !f.gpu->release_source_gpu({binding.executor,binding.programs,binding.epoch,binding.admission}))return false;
        ++f.releases; return true;
    }
    static IsleGameResult service(void* context) noexcept {
        auto& f = *static_cast<Fixture*>(context);
        if (std::this_thread::get_id() != f.creator) return ISLE_GAME_WRONG_OWNER;
        ++f.services_count;
        const auto polled = f.gpu ? f.gpu->poll() : sg::Poll::idle;
        if (polled == sg::Poll::failed) return ISLE_GAME_EXECUTION_FAILURE;
        if (polled == sg::Poll::completed) {
            const auto done = f.gpu->completed();
            if (done && done.ordinal != f.display_ordinal) {
                if (!f.host.queue_present(done.rgba,done.screen_offsets)) return ISLE_GAME_EXECUTION_FAILURE;
                f.display_ordinal = done.ordinal; ++f.display_completed;
            }
        }
        if (f.host.presentation_pending()) {
            const auto presented = f.host.try_present();
            if (presented == isle::game_host::Present::failed) return ISLE_GAME_EXECUTION_FAILURE;
            if (presented == isle::game_host::Present::completed) ++f.display_presented;
        }
        if (f.wire_watch && !f.probe.returned) {
            const auto current = f.photos.save_session().photo(isle::save::PhotoSet::temporary,0);
            if (!current || current.value.size() != f.wire.size() || !std::equal(current.value.begin(),current.value.end(),f.wire.begin()))
                return ISLE_GAME_INVALID_STATE;
        }
        if (f.probe.stage == -1 && f.pipeline && f.pipeline->gpu_busy()) {
            f.probe.cancelled_submitted = 1;
            f.progress.request_stop();
        }
        // This is a real, distinct non-current native object. Deletion invokes
        // its actual C joined observer; counts are not used as fake join proof.
        if (f.witness && f.progress.status() == isle::host::ProgressStatus::stopped && !f.witness_requested) {
            const auto result = isle_game_delete_object(f.storage.game(),f.witness);
            if (result != ISLE_GAME_OK) return result;
            f.witness_requested = true;
        }
        return ISLE_GAME_OK;
    }
    static void joined(IsleGameObject*, void* user) noexcept { ++static_cast<Fixture*>(user)->source_joined; }
    void launch(int mode) {
        // Join the previous C invocation before reusing its shared probe.
        while (source_owners.size() > 1) {
            services->erase(source_owners.back());
            source_owners.pop_back();
        }
        probe.mode = mode; probe.entered = probe.returned = 0; probe.result = ISLE_GAME_INVALID_STATE;
        std::memcpy(&probe.unchanged,probe.photo,sizeof(probe.unchanged));
        const auto current = photos.save_session().photo(isle::save::PhotoSet::temporary,0);
        require(bool(current) && current.value.size() == wire.size(),"canonical saved wire projection");
        std::copy(current.value.begin(),current.value.end(),wire.begin()); wire_watch = true;
        if (source_owners.empty()) source(isle_replay_probe_heartbeat,ISLE_GAME_SYNCHRONOUS,0xffffffffu);
        source(isle_replay_probe_main,ISLE_GAME_STACKFUL,1);
    }
    void source(IsleGameSourceCallback callback, IsleGameProcessKind kind, std::uint32_t priority) {
        auto* owner = services->object(0x710,0,priority); require(owner,"actual typed C process owner");
        source_owners.push_back(owner); isle_replay_probe_bind(owner,&probe);
        ++sources_created;
        require(services->observe_lifecycle(owner,{nullptr,joined,this}),"source owner joined observer");
        IsleGameProcess* process = nullptr;
        ok(isle_game_add_source_process(storage.game(),owner,callback,kind,0,&process),"install actual typed C kind0 process");
        require(process != nullptr,"typed C process missing");
    }
    IsleGameResult tick() {
        (void)progress.poll();
        require(progress.status() != isle::host::ProgressStatus::failed,progress.message());
        require(input.begin_update() == isle::game_input::Error::none,"native authored OM input boundary");
        ++ticks;
        const auto result = isle_game_tick(storage.game());
        if (result == ISLE_GAME_OK || result == ISLE_GAME_STOPPED)
            ok(service(this),"actual Frame/presentation/native service");
        return result;
    }
    void finish() {
        for (unsigned n = 0; n < 100 && !probe.returned; ++n) {
            ok(tick(),"actual C Replay native update");
            require(!services->failed(),services->error());
            (void)progress.idle();
        }
        require(probe.returned,"C Replay failed bounded authored native updates");
        ok(probe.result,isle_post_replay_error(probe.replay)); wire_watch = false;
    }
    void canonical(const IslePlayerPhotoData& record) {
        ok(isle_application_photos_set_temporary(probe.photos,0,&record),"post-ack canonical mutation must be writable");
        IslePhotoRecord retained{};
        ok(isle_application_photos_temporary(probe.photos,0,&retained),"canonical retained temporary record");
        require(retained.photo != nullptr,"authored canonical record absent"); probe.photo = retained.photo;
    }
    void verify_thumbnail() {
        IsleSprite* view = nullptr;
        ok(isle_post_window_sprite_view(probe.window,thumbnail_sprite,&view),"actual native thumbnail view");
        const auto& prototype = *resources->roots()[resources->sprite_index(0x80201520)];
        require(view && view->bitmap != prototype.bitmap && view->width > 0 && view->height > 0,"real independently published thumbnail backing");
        std::size_t piece = 0, row = 0;
        const auto stride = std::size_t(prototype.bitmap[0].width_img), tile = std::size_t(prototype.bitmap[0].actualHeight);
        for (int y = 0; y < view->height; ++y) {
            for (int x = 0; x < view->width; ++x) {
                unsigned r = 0, g = 0, b = 0;
                for (unsigned dy = 0; dy < 2; ++dy) for (unsigned dx = 0; dx < 2; ++dx) {
                    const auto at = (std::size_t(y)*2+dy)*probe.preview_stride+(std::size_t(x)*2+dx)*2;
                    require(at+1 < probe.preview_size,"canonical reduction read extent");
                    const unsigned pixel = (unsigned(preview[at])<<8)|preview[at+1];
                    r += (pixel>>11)&31; g += (pixel>>6)&31; b += (pixel>>1)&31;
                }
                const auto expected = ((r/4)<<11)|((g/4)<<6)|((b/4)<<1)|1;
                auto at = (row*stride+std::size_t(x))*2; if (row&1) at ^= 4;
                const auto* bytes = view->bitmap[piece].buf+at;
                require(((unsigned(bytes[0])<<8)|bytes[1]) == expected,"real Window thumbnail differs from canonical BE5551 four-pixel reduction");
            }
            if (++row == tile) { row = 0; ++piece; }
        }
    }
    void join_all() {
        for (auto* owner:source_owners) services->erase(owner);
        const auto expected = sources_created; source_owners.clear();
        require(source_joined == expected,"actual C workers/heartbeat not joined");
        bool retired = false;
        const auto deadline = Time::now()+std::chrono::seconds(120);
        while (!retired) {
            (void)replay->retire(retired);
            if (!retired) { (void)progress.poll(); ok(service(this),"real native cancellation drain"); }
            require(Time::now()<deadline,"actual GPU/native retirement deadline");
        }
        require(!pipeline->busy() && !pipeline->gpu_busy() && gpu->quiescent() && acquisitions == releases,
                "submitted native Replay GPU/readback/admission retirement not balanced");
        require(probe.joined == 1 && witness_requested,"service never joined its actual native witness");
        ok(window->retire(),"Window thumbnail producer/raster retirement");
        window.reset(); buttons.reset(); layout.reset(); focus.reset(); frame.reset(); text->detach(); text.reset();
        services.reset();
        IsleGameCounts counts{}; ok(isle_game_counts(storage.game(),&counts),"actual joined final counts");
        require(!counts.objects && !counts.processes && !counts.models && !counts.cameras && !counts.sprites,
                "Replay/Window/Source native owners survived joined retirement");
    }
};
IslePlayerPhotoData authored(bool subject) {
    IslePlayerPhotoData photo{}; photo.levelID = 0;
    photo.unk_08 = {0,60,300}; photo.unk_14 = {0,60,-250};
    for (auto& p:photo.pokemons) p.pokemonID = -1;
    for (auto& p:photo.items) p.itemType = -1;
    for (auto& p:photo.effects) p.textureID = -1;
    if (subject) {
        photo.unk_00_16 = 0x20;
        auto& p = photo.pokemons[0]; p.pokemonID = 16; p.position = {0,30,-250};
        p.animationTime = -1; // Real Source sound85 -> wait60 branch, not ack injection.
    }
    return photo;
}
void preview_artifacts(const fs::path& root,const Fixture& f) {
    std::ofstream raw(root/"authored-preview.rgba5551",std::ios::binary);
    raw.write(reinterpret_cast<const char*>(f.preview.data()),static_cast<std::streamsize>(f.probe.preview_size));
    std::ofstream ppm(root/"authored-preview.ppm",std::ios::binary); ppm << "P6\n103 77\n255\n";
    for (unsigned y=0;y<77;++y) for (unsigned x=0;x<103;++x) {
        const auto at=y*f.probe.preview_stride+x*2; const unsigned pixel=(unsigned(f.preview[at])<<8)|f.preview[at+1];
        const char rgb[]{char(((pixel>>11)&31)*255/31),char(((pixel>>6)&31)*255/31),char(((pixel>>1)&31)*255/31)};
        ppm.write(rgb,3);
    }
    raw.flush(); ppm.flush(); require(bool(raw) && bool(ppm),"private bounded canonical preview evidence write");
}
int run(const fs::path& rom_path,const fs::path& root) {
    require(root.is_absolute() && fs::is_directory(fs::symlink_status(root)) && !fs::is_symlink(fs::symlink_status(root)) &&
            fs::is_empty(root) && fs::canonical(root)==root.lexically_normal(),"NEW_OWNED_EMPTY_ROOT must already exist, absolute, empty, canonical and nonsymlink");
    for (const char* name:{"SDL_AUDIO_DRIVER","SDL_AUDIODRIVER"}) {
        const auto* value=std::getenv(name); require(value && std::string_view(value)=="dummy","both explicit SDL dummy Audio variables required");
    }
    require(fs::create_directory(root/"host") && fs::create_directory(root/"profile") && fs::create_directory(root/"artifacts"),"new private owned child directories");
    const auto rom=isle::rom::read_snapshot(rom_path);
    isle::game_host::Config config; config.data_directory=root/"host"; config.input=isle::game_host::InputMode::diagnostic_disabled;
    auto platform=isle::game_host::Host::create(rom,config); require(bool(platform),platform.message.data());
    auto& host=*platform.host;
    require(std::string_view(host.audio_info().driver.data())=="dummy" && host.audio_info().source.freq==32006 &&
            host.audio_info().source.channels==2 && host.audio_info().source.format==SDL_AUDIO_S16,"actual checked SDL dummy source format");
    isle::game_input::Service input; require(input.start({})==isle::game_input::Error::none,"actual absent-port Input Service");
    isle::host::Progress progress(host,input); progress.set_policy({false,false,false,false,true});
    progress.set_deadline(Time::now()+std::chrono::minutes(10),"private real Source Replay Host/GPU deadline");
    isle::game::State state(root/"profile"); require(state.initialize(1),state.error()); require(state.save()!=nullptr,"canonical application GameSession");
    isle::application::Photos photos(rom,*state.save());
    std::ofstream log(root/"artifacts"/"chronology.tsv"); require(bool(log),"fresh bounded private chronology");
    log << "scope\tauthored_canonical_records_NOT_captured_gameplay;direct_native_OM_updates_NOT_GTL;real_Host_wallclock_SDL_dummy\n";
    {
        Fixture f(rom,host,progress,photos,input,state);
        const auto subject=authored(true); f.canonical(subject); f.launch(ISLE_REPLAY_PROBE_SCORE);
        ok(f.tick(),"first real Source score update");
        require(f.probe.entered && !f.probe.returned && f.probe.stage==60 && f.probe.sounds==1 &&
                f.acquisitions==2 && f.releases==2 && f.gpu->quiescent(),"two genuine score GPU stages/sound85/wait60 suspension were bypassed");
        require(f.display_completed==1 && f.display_presented==1,"worker progress failed actual display GPU/readback/Host presentation");
        for (unsigned n=0;n<59;++n) { ok(f.tick(),"real suspended wait60 update"); require(!f.probe.returned && f.probe.stage==60,"Source wait60 resumed early"); }
        f.finish();
        const auto& s=f.probe.score.score;
        require(f.probe.present && f.probe.sounds==1 && f.probe.waits60==1 && f.probe.waits1==1 &&
                f.probe.wait60_end-f.probe.wait60_begin==60 && f.probe.wait1_end-f.probe.wait1_begin==1 && f.probe.lease_checks>61,
                "real pure-C sound/wait obligations/unchanged record leases missing");
        require(s.pokemonInFocus==16 && s.proximityScore>0 && s.completenessScore>0 && s.completenessScore<=10000 &&
                s.proximityScore<=1000 && s.samePkmnNumber==0 && s.samePkmnBonus==0,"actual authored Pidgey focus/feedback semantics");
        const auto size_score=int((double(s.proximityScore)*s.completenessScore/10000.0+5)/10)*10;
        const bool pose=s.proximityScore>=250 && s.completenessScore>=6000;
        const auto total=(size_score+(pose?s.posePts:0)+s.specialBonus)*(s.isWellFramed?2:1)+s.samePkmnBonus;
        require(s.totalScore==total && (!s.isWellFramed || (pose && s.posePts>=200)),"Source size/pose/framing total semantics");
        auto scored_wire=photos.save_session().photo(isle::save::PhotoSet::temporary,0);
        require(bool(scored_wire) && scored_wire.value.size()==f.wire.size(),"post-ack canonical wire exists");
        auto expected=f.wire;
        // Derive the single allowed flag mutation using the canonical encoder,
        // not a second saved-photo representation or native struct serialization.
        std::array<std::byte,isle::save::game_photo_bytes> encoded{};
        isle::player::encode_photo(f.probe.score.photo,encoded);
        require(std::equal(encoded.begin(),encoded.end(),scored_wire.value.begin()),"canonical score flags differ from consumed result");
        auto without_flags=f.probe.score.photo; without_flags.unk_00_16=subject.unk_00_16;
        isle::player::encode_photo(without_flags,encoded);
        require(encoded==expected,"scoring changed canonical non-flag fields");
        log << "subject_score\t" << s.pokemonInFocus << '\t' << s.totalScore << '\t' << f.probe.wait60_end-f.probe.wait60_begin << '\t' << f.probe.wait1_end-f.probe.wait1_begin << '\n';
        f.canonical(subject); f.launch(ISLE_REPLAY_PROBE_PREVIEW); f.finish();
        require(f.probe.preview_calls==1 && f.probe.preview_width==103 && f.probe.preview_height==77 &&
                f.probe.preview_stride==640 && f.probe.preview_size==0x20d00 && f.probe.callback_end-f.probe.callback_begin==1,
                "actual C preview callback retained canonical readback/Source wait contract");
        f.verify_thumbnail(); preview_artifacts(root/"artifacts",f);
        require(f.acquisitions==3 && f.releases==3,"preview did not close actual independent admission");
        log << "preview_callback_thumbnail\t" << f.probe.preview_size << '\t' << f.probe.callback_end-f.probe.callback_begin << '\n';
        const auto terrain=authored(false); f.canonical(terrain); f.launch(ISLE_REPLAY_PROBE_DRIVER_SCORE); f.finish();
        require(f.probe.score.score.pokemonInFocus==0 && f.probe.score.score.totalScore==0 && f.probe.sounds==1 &&
                f.probe.waits60==1 && f.probe.waits1==2 && f.acquisitions==5 && f.releases==5,"actual terrain two-stage score/final wait1 semantics");
        log << "terrain_score\t0\t0\tactual_two_stages_wait1\n";
        f.canonical(subject); f.launch(ISLE_REPLAY_PROBE_STOP);
        require(f.tick()==ISLE_GAME_STOPPED && f.probe.cancelled_submitted && !f.probe.returned &&
                isle_post_replay_status(f.probe.replay)==ISLE_GAME_STOPPED && isle_game_status(f.storage.game())==ISLE_GAME_OK,
                "Source-submitted Host stop did not use exact nonreturning STOPPED fence");
        f.wire_watch=false;
        require(!f.pipeline->gpu_busy() && f.gpu->quiescent() && f.acquisitions==6 && f.releases==6,
                "STOPPED published before actual submitted GPU/readback ownership drained");
        // Cancellation must release the canonical lease without reporting a scene result.
        f.canonical(subject); f.join_all();
        log << "submitted_host_stop\tSTOPPED_NOT_SCENE_COMPLETE\t" << f.probe.joined << '\t' << f.source_joined << '\t' << f.acquisitions << '\t' << f.releases << '\n';
        log << "native_progress\t" << f.services_count << '\t' << f.display_completed << '\t' << f.display_presented << '\t' << f.ticks << '\n';
    }
    require(input.stop().error==isle::game_input::Error::none,"actual Input Service stop");
    require(host.stop_audio(),host.message());
    progress.set_deadline(Time::now()+std::chrono::seconds(90),"actual SDL dummy native Audio drain deadline");
    bool drained=false;
    while (!progress.deadline_expired()) {
        (void)progress.poll(); require(host.error()==isle::game_host::Error::none,host.message());
        const auto state=host.poll_audio_drain();
        require(state!=isle::game_host::Drain::failed,host.message());
        if (state==isle::game_host::Drain::drained) { drained=true; break; }
        (void)progress.idle();
    }
    require(drained && host.audio()->accepted_frames()>0,"actual sound85 RSP/SDL dummy PCM did not drain");
    log << "actual_audio_accepted_frames\t" << host.audio()->accepted_frames() << '\n';
    const auto closed=host.shutdown();
    require(!closed.failed && closed.owners_released && closed.audio==isle::audio::SdlAudioOutput::CloseDisposition::stream_drained,
            "real Host native/SDL owners not released after actual drain");
    log.flush(); require(bool(log),"private chronology flush");
    std::cout << "post-course Replay actual native-C/score/preview/Host-stop consumer passed; authored records only, no full-game/GTL/hardware parity claim\n";
    return 0;
}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try { require(argc==3,"usage: post-course-replay-rom-smoke PRIVATE_ROM NEW_OWNED_EMPTY_ROOT"); return run(fs::path(argv[1]),fs::path(argv[2])); }
    catch (const fs::filesystem_error&) { std::cerr << "Replay consumer: private filesystem operation failed\n"; }
    catch (const std::exception& e) { std::fprintf(stderr,"Replay consumer: %.512s\n",e.what()); }
    return 1;
}
