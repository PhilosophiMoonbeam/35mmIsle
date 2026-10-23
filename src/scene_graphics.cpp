#include "scene_graphics.hpp"
#include "graphics_task.hpp"
#include "effect_graphics_resources.hpp"
#include "source_effect_camera.hpp"
#include "menu_scene_bridge.h"
#include "screen_transition_resources.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>


namespace isle::scene::graphics {
using startup::View;
namespace {
namespace sp = ::isle::sprite;
namespace wire = ::isle::graphics;
constexpr std::uint32_t startup_target_bytes = 320 * 240 * 2;
constexpr std::uint32_t transition_workspace = screen_transition::Resources::workspace.address;
constexpr sp::RamRange sprite_commands{0x150000, 0x50000};
constexpr std::uint32_t effect_commands = 0x1a0000, effect_assets = 0x1b0000, effect_end = 0x1f0000;
static_assert((menu::EffectRuntime::max_render_words + 2) * sizeof(std::uint32_t) <=
              effect_assets - effect_commands, "FX packet plus caller END exceeds command arena");
constexpr std::uint32_t transition_vertices = 0xaec70;
constexpr std::array<sp::RamRange,6> startup_producer_ranges{{
    {0x110000,0x10000}, {0x120000,0x10000}, {0x130000,0x10000},
    sprite_commands, {effect_commands,effect_assets-effect_commands},
    {transition_workspace,0x10000}}};
constexpr std::array<sp::RamRange,3> startup_fixed_targets{{
    {0x3b5000,startup_target_bytes}, {0x3da800,startup_target_bytes},
    {transition_vertices,0x3e0}}};
struct Failure { Error error; const char* message; };
bool intersects(sp::RamRange a, sp::RamRange b) noexcept {
    return a.size && b.size && a.address < std::uint64_t(b.address) + b.size &&
           b.address < std::uint64_t(a.address) + a.size;
}
void require(bool ok, const char* message, Error error = Error::invalid) {
    if (!ok) throw Failure{error, message};
}
std::array<sp::RamRange, 5> validate_layout(Layout layout, bool startup_adapter,
                                          std::uint32_t target_bytes) {
    require(!(layout.commands.address & 63u) && !(layout.commands.size & 63u) &&
            layout.commands.size >= 0x540, "scene command storage lacks aligned reset/list/viewport capacity");
    require(!(layout.workspace.address & 63u) && !(layout.workspace.size & 63u) &&
            layout.workspace.size >= 0x8000, "scene task workspace lacks stack/yield/task/boot capacity");
    require(!(layout.output.address & 63u) && !(layout.output.size & 7u) &&
            layout.output.size >= 8, "scene RSP output storage is empty or unaligned");
    require(!(layout.depth & 63u), "scene depth storage is unaligned");
    const std::array<sp::RamRange, 5> ranges{{
        {0, wire::bss_end}, layout.commands, layout.workspace, layout.output,
        {layout.depth, target_bytes}}};
    for (std::size_t i = 1; i < ranges.size(); ++i) {
        const auto range = ranges[i];
        require(range.address <= menu::rdram_size && range.size <= menu::rdram_size - range.address,
                "scene core storage exceeds physical RAM");
        for (std::size_t j = 0; j < i; ++j)
            require(!intersects(range, ranges[j]), "scene core storage ranges overlap");
        if (startup_adapter) {
            for (const auto owned : startup_producer_ranges)
                require(!intersects(range, owned), "scene core storage overlaps startup producer storage");
            for (const auto owned : startup_fixed_targets)
                require(!intersects(range, owned), "scene core storage overlaps fixed startup targets");
        }
    }
    return ranges;
}
std::uint32_t validate_extent(std::uint32_t width, std::uint32_t height, bool startup_adapter) {
    require((width == 320 && height == 240) ||
            (!startup_adapter && width == 640 && height == 480),
            "scene raster extent is unsupported by the cold renderer");
    return width * height * 2;
}
struct CoreStorage {
    const std::uint32_t width, height, target_bytes;
    const std::array<sp::RamRange, 5> ranges;
    const std::uint32_t top, top_end, identity, reset_viewport;
    const std::uint32_t stack, yield, task, boot, output, output_end, depth;
    CoreStorage(Config config, bool startup_adapter)
        : width(config.raster_width), height(config.raster_height),
          target_bytes(validate_extent(width,height,startup_adapter)),
          ranges(validate_layout(config.memory, startup_adapter, target_bytes)), top(ranges[1].address),
          top_end(top + ranges[1].size - 0x400), identity(top_end), reset_viewport(identity + 64),
          stack(ranges[2].address), yield(stack + 0x4000), task(stack + 0x6000), boot(stack + 0x7000),
          output(ranges[3].address), output_end(output + ranges[3].size), depth(ranges[4].address) {}
};
void native(IsleGameResult value) {
    require(value == ISLE_GAME_OK, "startup graphics native view rejected", Error::native);
}
std::uint32_t physical(std::uint32_t guest) {
    require(guest < menu::rdram_size || (guest & 0xff800000u) == 0x80000000u,
            "startup graphics nonphysical source identity", Error::resource);
    return guest & 0x1fffffffu;
}
std::span<const std::uint8_t> bytes(std::span<const std::byte> value) {
    return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}
void diagnostic(std::span<char> output, const char* message) noexcept {
    if (message == nullptr) message = "startup graphics failure";
    std::size_t n = 0;
    while (n + 1 < output.size() && message[n]) { output[n] = message[n]; ++n; }
    output[n] = '\0';
}
std::uint32_t pair(unsigned x, unsigned y) { return (x << 12) | y; }
std::uint32_t packed_color(std::uint32_t rgba) {
    const auto value = ((rgba >> 16) & 0xf800u) | ((rgba >> 13) & 0x7c0u) |
                       ((rgba >> 10) & 0x3eu) | ((rgba >> 7) & 1u);
    return (value << 16) | value;
}
IsleGameViewport full_viewport(const CoreStorage& core) {
    IsleGameViewport vp{};
    vp.scale[0] = vp.translate[0] = static_cast<std::int16_t>(core.width * 2);
    vp.scale[1] = vp.translate[1] = static_cast<std::int16_t>(core.height * 2);
    vp.scale[2] = vp.translate[2] = 511;
    return vp;
}
sp::Bounds bounds(const IsleGameViewport& vp, const CoreStorage& core) {
    // Match Source integer rounding; target clipping affects scissor, not the viewport.
    sp::Bounds result{vp.translate[0] / 4 - vp.scale[0] / 4,
        vp.translate[1] / 4 - vp.scale[1] / 4,
        vp.translate[0] / 4 + vp.scale[0] / 4,
        vp.translate[1] / 4 + vp.scale[1] / 4};
    result.left=std::max(result.left,0); result.top=std::max(result.top,0);
    result.right=std::min(result.right,int(core.width)); result.bottom=std::min(result.bottom,int(core.height));
    require(result.left >= 0 && result.top >= 0 && result.right <= int(core.width) &&
            result.bottom <= int(core.height) && result.left < result.right && result.top < result.bottom,
            "startup source viewport exceeds source GPU target", Error::preparation);
    return result;
}
void viewport(wire::Memory& memory, std::uint32_t at, const IsleGameViewport& vp, const CoreStorage& core) {
    static_cast<void>(bounds(vp,core));
    for (unsigned i = 0; i < 4; ++i) {
        memory.half(at + i * 2, static_cast<std::uint16_t>(vp.scale[i]));
        memory.half(at + 8 + i * 2, static_cast<std::uint16_t>(vp.translate[i]));
    }
}
void validate_depthless_sprite(const sp::Draw& sprite) {
    const auto* view = sprite.view;
    require(view,"depthless Source sprite lacks its native view",Error::resource);
    if (!(view->attr & ISLE_SP_HIDDEN) && view->width && view->height && view->nbitmaps)
        require((view->attr & ISLE_SP_FASTCOPY) || !(view->attr & ISLE_SP_Z),
                "depthless Source sprite requested depth compare/update",Error::resource);
}
struct Commands {
    // Bound only inside a returning mapped-writer scope.
    wire::Memory* memory;
    const CoreStorage& core;
    std::uint32_t cursor, end, depth, depth_bytes;
    // Physical cursor state survives mappings; no RAM span is retained.
    Commands(wire::Memory& ram, const CoreStorage& layout, std::uint32_t begin, std::uint32_t limit,
             std::uint32_t source_depth, std::uint32_t source_depth_bytes)
        : memory(&ram), core(layout), cursor(begin), end(limit), depth(source_depth), depth_bytes(source_depth_bytes) {}
    Commands(const CoreStorage& layout, std::uint32_t source_depth, std::uint32_t source_depth_bytes)
        : memory(nullptr), core(layout), cursor(layout.top), end(layout.top_end),
          depth(source_depth), depth_bytes(source_depth_bytes) {}
    void command(std::uint32_t a, std::uint32_t b) {
        require(cursor<=end&&end-cursor>=8,"scene command list exhausted",Error::preparation);
        require(memory != nullptr, "scene command write has no scoped RAM authority", Error::preparation);
        memory->word(cursor, a); memory->word(cursor + 4, b); cursor += 8;
    }
    void call(std::uint32_t address) { command(0xde000000, address); }
    template<class Function>
    std::uint32_t into(std::uint32_t begin,std::uint32_t limit,Function&& function) {
        require(!(begin&7u)&&!(limit&7u)&&begin<=limit&&limit<=menu::rdram_size,
                "source command cursor exceeds actual RAM",Error::preparation);
        struct Restore {
            Commands& commands;std::uint32_t cursor,end;
            ~Restore() {commands.cursor=cursor;commands.end=end;}
        } restore{*this,cursor,end};
        cursor=begin;end=limit;
        function();
        return cursor;
    }
    void mode(bool high, unsigned shift, unsigned length, std::uint32_t value) {
        command((high ? 0xe3000000u : 0xe2000000u) |
                ((32 - shift - length) << 8) | (length - 1), value);
    }
    void scissor(sp::Bounds b) {
        command(0xed000000u | pair(unsigned(b.left) * 4, unsigned(b.top) * 4),
                pair(unsigned(b.right) * 4, unsigned(b.bottom) * 4));
    }
    void fill(sp::Bounds b) {
        command(0xf6000000u | pair(unsigned(b.right - 1) * 4, unsigned(b.bottom - 1) * 4),
                pair(unsigned(b.left) * 4, unsigned(b.top) * 4));
    }
    void camera(std::uint32_t vp_address, const IsleGameViewport& vp,
                unsigned flags, std::uint32_t rgba, unsigned lane, std::uint32_t color,
                bool restoring = false) {
        require(restoring || !(flags & 1u) || depth_bytes,
                "depthless Source camera requested an actual depth clear",Error::resource);
        const auto clipped = bounds(vp,core); // Scissor never changes the uploaded viewport.
        command(0xdc080008, vp_address);
        scissor(clipped);
        if (!restoring && (flags & 1u)) {
            command(0xe7000000, 0); mode(true, 20, 2, 0x300000);
            mode(false, 3, 29, 0); command(0xff100000u | (core.width - 1), depth);
            command(0xf7000000, 0xfffcfffc); fill(clipped);
        }
        command(0xe7000000, 0); command(0xff100000u | (core.width - 1), color);
        if (!restoring && (flags & 2u)) {
            mode(true, 20, 2, 0x300000); mode(false, 3, 29, 0);
            command(0xf7000000, packed_color(rgba)); fill(clipped);
        }
        if (!restoring) command(0xe7000000, 0);
        mode(true, 20, 2, 0);
        mode(false, 3, 29, (lane & 1u) ? 0x005049d8u : 0x00552078u);
    }
    void reset(std::uint32_t color, std::uint32_t pre_render) {
        const auto words = wire::reset_commands(
            {color, depth, core.reset_viewport, core.identity, pre_render,
             core.target_bytes, depth_bytes, core.width, core.height});
        for (std::size_t i = 0; i < words.size(); i += 2) command(words[i], words[i + 1]);
    }
};
struct SceneAssets {
    std::unique_ptr<sp::Producer> sprites;
    std::vector<sp::AssetSpan> installation;
    std::vector<sp::RamRange> reservations;
};
// A source alias is accepted only when it names the identical immutable byte,
// not merely a permissive reservation or coincidentally equal content. Split
// overlapping extents into a disjoint union, so every physical byte copies once.
void disjoint(std::vector<sp::AssetSpan>& spans) {
    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.address < b.address || (a.address == b.address && a.bytes.size() > b.bytes.size());
    });
    std::size_t count = 0;
    for (auto item : spans) {
        require(item.address <= menu::rdram_size && item.bytes.size() <= menu::rdram_size - item.address,
                "startup immutable span exceeds RAM", Error::resource);
        if (item.bytes.empty()) continue;
        if (count) {
            const auto& previous = spans[count - 1];
            const auto end = previous.address + previous.bytes.size();
            if (item.address < end) {
                const auto overlap = std::min(end - item.address, item.bytes.size());
                const auto offset = item.address - previous.address;
                require(previous.bytes.data() + offset == item.bytes.data() &&
                        std::equal(item.bytes.begin(), item.bytes.begin() + overlap,
                                   previous.bytes.begin() + offset),
                        "startup immutable physical aliases have different backing identities", Error::resource);
                item.address += static_cast<std::uint32_t>(overlap);
                item.bytes = item.bytes.subspan(overlap);
            }
        }
        if (!item.bytes.empty()) spans[count++] = item;
    }
    spans.resize(count);
}
const IsleGameCameraParameters& camera_parameters(IsleGame* game, IsleGameCamera* camera) {
    const IsleGameCameraParameters* parameters = nullptr;
    require(camera != nullptr, "startup native source camera is missing", Error::native);
    native(isle_game_camera_parameters(game, camera, &parameters));
    require(parameters != nullptr, "startup camera parameter borrow is empty", Error::native);
    return *parameters;
}
} // namespace

struct SourceControl::State {
    Commands& commands;
    void* mapping_context;
    void (*mapping)(void*,void*,void (*)(void*,std::span<std::uint32_t>));
    std::uint64_t epoch;
    std::uint32_t framebuffer, depth, flags;
    void* initialize_context;
    void (*initialize)(void*);
    void* prepare_context;
    const render::Prepared& (*prepare)(void*);
    void* draw_context;
    void (*draw)(void*);
    void* call_context;
    void (*call)(void*, render::List);
    wire::ResetInputs reset;
    void* restore_context;
    std::uint32_t (*restore)(void*,unsigned,std::uint32_t,std::uint32_t);
    void* finish_context;
    void (*finish)(void*,render::List,render::List,wire::SourceProgram);
    void* drain_context;
    void (*drain)(void*);
    void* reset_context;
    void (*reset_slots)(void*);
    void* process_context;
    void (*process)(void*,bool);
    void* capacity_context;
    void (*capacity)(void*);
    bool initialized = false, prepared = false, drawn = false;
};
struct SourceControlAccess {
    template<class Mapping, class Initialize, class Prepare, class Draw, class Call, class Restore, class Finish, class Drain, class Reset, class Process, class Capacity>
    static bool dispatch(render::Control callback, void* user, Commands& commands,
                         Mapping& mapping, std::uint64_t epoch,
                         std::uint32_t framebuffer, std::uint32_t pre_render, std::uint32_t flags,
                         Initialize& initialize, Prepare& prepare, Draw& draw, Call& call,
                         Restore& restore, Finish& finish, Drain& drain, Reset& reset, Process& process, Capacity& capacity) {
        SourceControl::State state{commands,
            &mapping,[](void* context,void* user,void (*function)(void*,std::span<std::uint32_t>)) {
                (*static_cast<Mapping*>(context))(user,function);
            },epoch,framebuffer,commands.depth,flags,
            &initialize, [](void* context) { (*static_cast<Initialize*>(context))(); },
            &prepare, [](void* context) -> const render::Prepared& {
                return (*static_cast<Prepare*>(context))();
            },
            &draw, [](void* context) { (*static_cast<Draw*>(context))(); },
            &call, [](void* context, render::List list) { (*static_cast<Call*>(context))(list); },
            {framebuffer,commands.depth,commands.core.reset_viewport,commands.core.identity,pre_render,
             commands.core.target_bytes,commands.depth_bytes,commands.core.width,commands.core.height},
            &restore,[](void* context,unsigned lane,std::uint32_t cursor,std::uint32_t end) {
                return (*static_cast<Restore*>(context))(lane,cursor,end);
            },&finish,[](void* context,render::List root,render::List terminal,wire::SourceProgram program) {
                (*static_cast<Finish*>(context))(root,terminal,program);
            },&drain,[](void* context) { (*static_cast<Drain*>(context))(); },
            &reset,[](void* context) { (*static_cast<Reset*>(context))(); },
            &process,[](void* context,bool reset) { (*static_cast<Process*>(context))(reset); },
            &capacity,[](void* context) { (*static_cast<Capacity*>(context))(); }};
        SourceControl control(state);
        return callback(user, control);
    }
};
void SourceControl::with_ram(void* context,void (*function)(void*,std::span<std::uint32_t>)) {
    require(function != nullptr,"source scoped writer is missing",Error::preparation);
    state_->mapping(state_->mapping_context,context,function);
}
std::uint64_t SourceControl::epoch() const noexcept { return state_->epoch; }
std::uint32_t SourceControl::framebuffer() const noexcept { return state_->framebuffer; }
std::uint32_t SourceControl::depth() const noexcept { return state_->depth; }
std::uint32_t SourceControl::camera_flags() const noexcept { return state_->flags; }
void SourceControl::command(std::uint32_t first, std::uint32_t second) {
    struct Write { Commands& commands;std::uint32_t first,second; } write{state_->commands,first,second};
    with_ram(&write,[](void* context,std::span<std::uint32_t>) {
        auto& write=*static_cast<Write*>(context);write.commands.command(write.first,write.second);
    });
}
void SourceControl::call(render::List list) { state_->call(state_->call_context, list); }
void SourceControl::reset_program_state(void* context,
    void (*emit)(void*, std::uint32_t, std::uint32_t)) const {
    require(emit != nullptr, "source reset requires a bounded native cursor", Error::preparation);
    const auto words = wire::reset_commands(state_->reset);
    for (std::size_t i = 0; i < words.size(); i += 2) emit(context, words[i], words[i + 1]);
}
std::uint32_t SourceControl::restore_camera(unsigned lane,std::uint32_t cursor,std::uint32_t end) {
    require(state_->initialized&&!state_->drawn&&lane>0&&lane<4,
            "source restore requires its initialized actual secondary lane",Error::preparation);
    return state_->restore(state_->restore_context,lane,cursor,end);
}
std::uint32_t SourceControl::initialize_camera_into(std::uint32_t cursor,std::uint32_t end) {
    return state_->commands.into(cursor,end,[&] {initialize_camera();});
}
std::uint32_t SourceControl::draw_into(std::uint32_t cursor,std::uint32_t end) {
    return state_->commands.into(cursor,end,[&] {draw();});
}
void SourceControl::reserve_task() {state_->capacity(state_->capacity_context);}
void SourceControl::submit_task(render::List root,render::List terminal,wire::SourceProgram program) {
    state_->finish(state_->finish_context,root,terminal,program);
}
void SourceControl::drain_tasks() { state_->drain(state_->drain_context); }
void SourceControl::reset_task_slots() { state_->reset_slots(state_->reset_context); }
void SourceControl::process_commands(bool reset) { state_->process(state_->process_context,reset); }
void SourceControl::initialize_camera() {
    require(!state_->drawn && !state_->initialized,
            "source camera initialization cannot replay or mix with draw", Error::preparation);
    state_->initialized = true;
    state_->initialize(state_->initialize_context);
}
const render::Prepared& SourceControl::prepare() {
    require(!state_->drawn && !state_->prepared,
            "source camera preparation cannot replay or mix with draw", Error::preparation);
    state_->prepared = true;
    return state_->prepare(state_->prepare_context);
}
void SourceControl::draw() {
    require(!state_->drawn && !state_->initialized && !state_->prepared,
            "source camera draw cannot replay or mix with explicit phases", Error::preparation);
    state_->drawn = true;
    state_->draw(state_->draw_context);
}

struct Frame::Impl {
    const CoreStorage storage;
    const DepthReservation source_depth;
    std::optional<SourceScreen> source_screen;
    std::array<sp::RamRange, 7> source_reservations{};
    std::array<std::uint32_t, 2> source_targets{};
    bool source_depth_reserved = false;
    const menu::Catalog* catalog;
    const rom::Snapshot& snapshot;
    const std::byte* source;
    wire::SourcePrograms source_programs{snapshot};
    std::vector<sp::RamRange> source_gpu_storage;
    std::uint64_t source_gpu_admission = 0;
    bool source_gpu_active = false;
    const void* source_display_owner = nullptr;
    std::vector<sp::RamRange> source_display_storage, source_display_protected;
    bool source_display_active = false;
    std::array<SceneAssets, 2> scenes;
    std::optional<menu::EffectGraphicsResources> fx_graphics;
    screen_transition::Resources transitions;
    std::vector<sp::Draw> draws;
    std::vector<scene::SpriteDraw> startup_sprites;
    std::vector<std::byte> rgba;
    sp::Producer empty_sprites{std::span<const IsleSprite* const>{}, std::span<const sp::BankMapping>{}};
    std::vector<sp::RamRange> composition_reservations, command_reservations;
    std::optional<RenderCapacity> render_capacity;
    bool dispatch_started = false;
    std::uint64_t installed_identity = 0;
    sp::Producer* installed_producer = nullptr;
    std::optional<sp::Producer::Borrow> graphics_borrow;
    struct TransitionBorrow {
        screen_transition::Owner* owner = nullptr;
        ~TransitionBorrow() { reset(); }
        void reset() noexcept { if (owner) owner->release(); owner=nullptr; }
        bool retain(screen_transition::Owner* value) noexcept {
            if (owner) return owner==value;
            if (!value || !value->borrow()) return false;
            owner=value; return true;
        }
    } transition_borrow;
    // The executor dies before immutable source/resource storage is released.
    rsp::CanonicalExecutor executor;
    rsp::CanonicalExecutor::Token token = 0;
    bool presentation_eligible = true;
    // Logical ownership is independent of executor quiescence between tasks.
    bool logical_active = false, dispatching = false;
    unsigned writer_depth = 0;
    rsp::CanonicalExecutor::PreparationLease preparation;
    rsp::CanonicalExecutor::MemoryLease final_mapping;
    std::array<TaskReceipt,33> receipts{};
    std::size_t receipt_count = 0, source_task_count = 0;
    rsp::CanonicalExecutor::Token pending_token = 0, last_submission = 0;
    unsigned submission_context = 0;
    std::uint64_t epoch = 0, ordinal = 0;
    Error error = Error::none;
    std::array<char, 512> message{};
    save::ScreenOffsets offsets{};
    std::uint32_t color = 0;
    wire::SourceProgram program = wire::SourceProgram::f3dex2_non_fifo;
    CompletedWork completion_work{};
    bool cold = true, resident = false, ready = false, retired = false;
    bool boot_resident = false;
    // Every returning backend operation latches its own first failure before a
    // renderer's plain-C fence translates it. Never replace that evidence with
    // the outer "control rejected" consequence.
    template<class Function>
    decltype(auto) attempt(Function&& function) {
        try {return function();}
        catch(const Failure& e) {fail(e.error,e.message);throw;}
        catch(const std::exception& e) {fail(Error::execution,e.what());throw;}
        catch(...) {fail(Error::execution,"source graphics boundary exception");throw;}
    }
    void drain_task() {
        attempt([&] {
        require(writer_depth == 0,"source task boundary crossed an active RAM writer",Error::preparation);
        if (!pending_token) return;
        const auto joined = pending_token;
        try {
            auto completed = executor.wait_completed(joined);
            receipts[receipt_count-1].completed = true;
            require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs == 1,
                    "source actual task did not finish its unique FullSync",Error::execution);
            completed.release();
            pending_token = 0;
        } catch (...) {
            receipts[receipt_count-1].failed = true;
            if (executor.quiescent()) pending_token = 0;
            throw;
        }
        });
    }
    void ensure_preparation() {
        if (preparation) return;
        require(writer_depth == 0,"source writer cannot acquire RAM through an active writer",Error::preparation);
        drain_task();
        preparation = executor.begin_preparation();
    }
    template<class Function>
    decltype(auto) mapped(Function&& function) {
        return attempt([&]() -> decltype(auto) {
        require(error == Error::none && logical_active && dispatching,
                "source writer escaped its live dispatch",Error::preparation);
        ensure_preparation();
        struct Writer {
            Impl& owner;
            explicit Writer(Impl& value):owner(value) {++owner.writer_depth;}
            ~Writer() {--owner.writer_depth;}
        } writer{*this};
        return function(preparation.words());
        });
    }
    template<class Function>
    decltype(auto) write_commands(Commands& commands,Function&& function) {
        return mapped([&](std::span<std::uint32_t> words) -> decltype(auto) {
            wire::Memory memory(words);
            struct Binding {
                Commands& commands;wire::Memory* previous;
                ~Binding() {commands.memory=previous;}
            } binding{commands,commands.memory};
            commands.memory=&memory;
            return function(words,memory);
        });
    }
    void reserve_task() {
        attempt([&] {
        require(logical_active && dispatching && error==Error::none && writer_depth == 0,
                "source task boundary requires its live dispatch and no writer",Error::preparation);
        const auto capacity = render_capacity.value_or(RenderCapacity{});
        require(receipt_count < capacity.task_receipts && receipt_count < receipts.size(),
                "source logical task receipt cold capacity exhausted",Error::preparation);
        require(source_task_count < capacity.source_task_slots,
                "source task slots exhausted before Source reset",Error::preparation);
        });
    }
    void submit(render::List root,render::List terminal,wire::SourceProgram selected) {
        attempt([&] {
        reserve_task();
        static_cast<void>(source_programs.descriptor(selected));
        ensure_preparation();
        const auto task = wire::finish_task(preparation.words(),snapshot,storage.boot,storage.stack,
            storage.yield,storage.task,storage.output,storage.output_end,root.address,selected);
        // Reserve the bounded receipt before entering execution: failures must
        // never erase evidence of earlier actual submissions.
        auto& receipt = receipts[receipt_count++];
        receipt = {0,selected,root.address,terminal.address,false,false};
        receipt.task=task.task;
        ++source_task_count;
        try {
            pending_token = executor.begin(std::move(preparation),task.task,task.boot);
            receipt.token = token = last_submission = pending_token;
            const auto* execution=executor.attempted_result();
            require(execution,"successful task admission lost its actual RSP result",Error::execution);
            receipt.execution=*execution;
            receipt.has_execution=true;
        } catch (...) {
            if(const auto* execution=executor.attempted_result()) {
                receipt.execution=*execution;
                receipt.has_execution=true;
            }
            receipt.failed=true;
            throw;
        }
        });
    }
    void reset_slots() {
        attempt([&] {
        require(writer_depth == 0 && !pending_token,
                "Source task reset preceded actual completion",Error::preparation);
        source_task_count = 0;
        });
    }

    Impl(const menu::Catalog* c, const rom::Snapshot& s, Config config)
        : storage(config, c != nullptr), source_depth{storage.ranges[4], 0x40},
          catalog(c), snapshot(s), source(s.canonical().data()),
          transitions(s,config.transition_memory), rgba(storage.target_bytes * 2),
          executor(config.require_cpu, config.limits) {
        require(source != nullptr && snapshot.canonical().size() == rom::us_size,
                "startup graphics requires an admitted stable Snapshot", Error::resource);
        require(!catalog || (config.transition_memory.address==screen_transition::Resources::workspace.address &&
                config.transition_memory.size==screen_transition::Resources::workspace.size),
                "Catalog transition storage must retain its admitted fixed plan",Error::resource);
        if (!catalog) return;
        // Startup and course composition share the same alias-preserving packer.
        const std::array<const menu::EffectBank*, 1> selected{&catalog->effects()};
        fx_graphics.emplace(snapshot, selected, menu::RamRange{effect_assets, effect_end - effect_assets});
        const auto effect_spans = fx_graphics->installation_spans();
        std::array<menu::RamRange,11> runtime_ranges{};
        std::size_t cursor = 0;
        for (const auto range : storage.ranges)
            runtime_ranges[cursor++] = {range.address,range.size};
        for (const auto range : startup_producer_ranges)
            runtime_ranges[cursor++] = {range.address,range.size};
        for (std::size_t index = 0; index < scenes.size(); ++index) {
            const auto scene = index == 0 ? menu::Scene::logo : menu::Scene::title_menu;
            auto& result = scenes[index];
            const auto& screen = catalog->screen(scene);
            require(screen.width == storage.width && screen.height == storage.height &&
                    screen.allocated_depth_bytes == storage.target_bytes &&
                    physical(screen.framebuffer_guests[0]) == 0x3b5000 &&
                    physical(screen.framebuffer_guests[1]) == 0x3da800 && screen.framebuffer_guests[2] == 0,
                    "startup source framebuffer layout differs", Error::resource);
            const auto roots = catalog->sprites(scene);
            std::vector<const IsleSprite*> resources;
            resources.reserve(roots.size());
            for (const auto& root : roots) resources.push_back(root.sprite.view());
            const auto& bank = catalog->bank(scene);
            const std::array<sp::BankMapping, 1> banks{{{bank.owner, bank.guest_begin}}};
            result.sprites = std::make_unique<sp::Producer>(resources, banks);
            const auto plan = catalog->plan_install(scene, runtime_ranges);
            require(bool(plan.status), "startup selected Catalog closure collides with runtime RAM", Error::resource);
            for (const auto& span : plan.spans)
                result.installation.push_back({physical(span.guest_address), bytes(span.bytes)});
            for (const auto& span : result.sprites->assets()) result.installation.push_back(span);
            if (scene == menu::Scene::title_menu) {
                result.installation.insert(result.installation.end(), effect_spans.begin(), effect_spans.end());
                const auto spans=transitions.immutable();
                result.installation.insert(result.installation.end(),spans.begin(),spans.end());
            }
            disjoint(result.installation);
            for (const auto range : runtime_ranges) {
                const sp::RamRange converted{static_cast<std::uint32_t>(range.offset), static_cast<std::uint32_t>(range.size)};
                result.reservations.push_back(converted);
            }
            for (unsigned i = 0; i < 2; ++i) {
                const sp::RamRange framebuffer{physical(screen.framebuffer_guests[i]), storage.target_bytes};
                result.reservations.push_back(framebuffer);
            }
        }
        draws.reserve(std::max(catalog->setup(menu::Scene::logo).sprites,
                               catalog->setup(menu::Scene::title_menu).sprites));
        startup_sprites.reserve(draws.capacity());
    }
    void reserve_source_gpu_storage(std::span<const sp::RamRange> ranges) {
        require(!dispatch_started && !retired && !source_gpu_active && executor.quiescent() &&
                source_gpu_storage.empty() && !ranges.empty(),
                "Source GPU storage must be reserved once before first dispatch", Error::preparation);
        std::vector<sp::RamRange> candidate(ranges.begin(),ranges.end());
        for (const auto range : candidate) {
            require(range.size && !(range.address & 7u) && range.address <= menu::rdram_size &&
                    range.size <= menu::rdram_size-range.address,
                    "Source GPU reservation exceeds aligned physical RAM", Error::resource);
            for (const auto core : storage.ranges)
                require(!intersects(range,core),"Source GPU storage aliases display core storage",Error::resource);
            if (source_screen) for (const auto framebuffer : source_targets)
                require(!intersects(range,{framebuffer,storage.target_bytes}),
                        "Source GPU storage aliases display framebuffer",Error::resource);
        }
        std::sort(candidate.begin(),candidate.end(),[](const auto& a,const auto& b) { return a.address<b.address; });
        for (std::size_t i=1;i<candidate.size();++i)
            require(!intersects(candidate[i-1],candidate[i]),
                    "Source GPU storage reservations overlap",Error::resource);
        source_gpu_storage=std::move(candidate);
        if (render_capacity) reserve_render_capacity(*render_capacity);
    }
    void register_source_display_storage(const void* owner,std::span<const sp::RamRange> ranges) {
        require(owner && !ranges.empty() && !source_display_owner && !dispatch_started &&
                !retired && !source_gpu_active && executor.quiescent(),
                "Source display storage needs one cold retained owner",Error::preparation);
        std::vector<sp::RamRange> loan(ranges.begin(),ranges.end()), protected_ranges;
        protected_ranges.reserve(source_gpu_storage.size());
        for(std::size_t i=0;i<loan.size();++i) {
            const auto range=loan[i];
            bool found=false;
            for(const auto retained:source_gpu_storage)
                if(range.address==retained.address && range.size==retained.size)found=true;
            require(found,"Source display loan is not an exact retained Replay range",Error::resource);
            for(std::size_t j=0;j<i;++j)
                require(!intersects(range,loan[j]),"Source display loan ranges overlap",Error::resource);
        }
        for(const auto retained:source_gpu_storage) {
            bool lent=false;
            for(const auto range:loan)
                if(range.address==retained.address && range.size==retained.size)lent=true;
            if(!lent)protected_ranges.push_back(retained);
        }
        source_display_storage=std::move(loan);
        source_display_protected=std::move(protected_ranges);
        source_display_owner=owner;
    }
    void source_configuration_ready() {
        require(!dispatch_started && !render_capacity && !retired && !source_gpu_active && executor.quiescent(),
                "source screen/depth setup must precede cold render reservation and first dispatch");
    }
    void configure_source_screen(const SourceScreen& screen, const DepthReservation* depth) {
        source_configuration_ready();
        require(!source_screen && (depth ? source_depth_reserved && depth == &source_depth : !source_depth_reserved),
                "source screen depth must be this Frame's actual capability or explicit NULL",Error::resource);
        // Exact admitted Source modes; native readback is not VI filtering.
        require(screen.width == storage.width && screen.height == storage.height &&
                ((storage.width == 320 && (screen.flags == 0xa99 || screen.flags == 0x16a99)) ||
                 (storage.width == 640 && screen.flags == 0x695)) &&
                screen.framebuffer_identity[2] == 0,
                "source screen mode is not supported by the actual RGBA16 renderer", Error::resource);
        std::array<std::uint32_t, 2> targets{};
        for (std::size_t i = 0; i < targets.size(); ++i) {
            require(screen.framebuffer_identity[i] != 0,
                    "source screen is missing a source framebuffer identity", Error::resource);
            targets[i] = physical(screen.framebuffer_identity[i]);
            const sp::RamRange target{targets[i], storage.target_bytes};
            require(!(target.address & 63u) && target.address <= menu::rdram_size &&
                    target.size <= menu::rdram_size - target.address,
                    "source framebuffer lacks aligned physical RDRAM storage", Error::resource);
            for (const auto core : storage.ranges)
                require(!intersects(target, core),
                        "source screen framebuffer aliases actual task/depth storage", Error::resource);
            for (const auto range : source_gpu_storage)
                require(!intersects(target,range),
                        "source screen framebuffer aliases retained replay storage",Error::resource);
        }
        require(!intersects({targets[0], storage.target_bytes}, {targets[1], storage.target_bytes}),
                "source screen framebuffer contexts overlap", Error::resource);
        std::copy(storage.ranges.begin(), storage.ranges.end(), source_reservations.begin());
        for (std::size_t i = 0; i < targets.size(); ++i)
            source_reservations[storage.ranges.size() + i] = {targets[i], storage.target_bytes};
        source_targets = targets;
        source_screen = screen;
    }
    void reserve_render_capacity(RenderCapacity capacity) {
        require(!dispatch_started && !source_gpu_active && executor.quiescent(),
                "scene render capacity must be reserved before first dispatch", Error::preparation);
        // Both framebuffer contexts plus every possible sprite pass are owned
        // here; source resource reservations are supplied by the cold caller.
        require(capacity.task_receipts && capacity.task_receipts <= receipts.size() &&
                capacity.source_task_slots && capacity.source_task_slots <= receipts.size(),
                "scene task capacity exceeds bounded logical storage",Error::resource);
        const auto overhead = storage.ranges.size() + 2u + 32u + source_gpu_storage.size();
        require(capacity.resource_reservations <= composition_reservations.max_size() - overhead,
                "scene render reservation capacity exceeds storage", Error::resource);
        const auto count = overhead + capacity.resource_reservations;
        // Each covering reservation can split into two around a sprite arena.
        require(count <= command_reservations.max_size() / 2,
                "scene sprite reservation capacity exceeds storage", Error::resource);
        draws.reserve(capacity.sprite_draws);
        startup_sprites.reserve(capacity.sprite_draws);
        composition_reservations.reserve(count);
        command_reservations.reserve(count * 2);
        render_capacity = capacity;
    }
    void fail(Error value, const char* reason) noexcept {
        for (auto& draw : draws) draw = {};
        draws.clear();
        for (auto& sprite : startup_sprites) sprite = {};
        startup_sprites.clear();
        ready = false;
        if (error == Error::none) { error = value; diagnostic(message, reason); }
        // A failed begin can have already submitted AND drained Source10. Keep
        // that logical admission and its owners live until poll acknowledges
        // failure, so GTL cannot misclassify real work as an unsubmitted draw.
        if ((!logical_active || !last_submission) && !dispatching &&
            !preparation && !final_mapping && executor.quiescent()) {
            token = 0; pending_token = 0; logical_active = false;
            completion_work = {};
            graphics_borrow.reset();
            transition_borrow.reset();
        }
    }
    struct StartupPreparation { Impl* owner; View view; };
    static bool title_pass(void* userdata, std::span<std::uint32_t> ram, std::uint64_t epoch,
                           const render::Pass&, render::Prepared& result) {
        const auto& context = *static_cast<StartupPreparation*>(userdata);
        auto prepared = context.view.title->models()->prepare(ram, epoch);
        require(bool(prepared), "startup native model/material preparation rejected", Error::preparation);
        result.camera = {prepared.frame.camera.address, prepared.frame.camera.commands * 2u};
        for (unsigned i = 0; i < 4; ++i)
            result.lanes[i] = {prepared.frame.lanes[i].address, prepared.frame.lanes[i].commands * 2u};
        return true;
    }
    static bool effect_pass(void* userdata, std::span<std::uint32_t> ram, std::uint64_t,
                            const render::Pass&, render::Prepared& result) {
        const auto& context = *static_cast<StartupPreparation*>(userdata);
        auto& owner = *context.owner;
        menu::EffectCamera camera;
        require(menu::source_effect_camera(context.view.game, context.view.title->camera(), camera),
                "startup source FX camera projection rejected", Error::native);
        require(owner.fx_graphics.has_value(), "startup FX resources are not installed", Error::resource);
        require(effect_commands / 4 <= ram.size() &&
                menu::EffectRuntime::max_render_words + 2 <= ram.size() - effect_commands / 4,
                "startup FX packet and caller END exceed RAM", Error::preparation);
        const auto rendered = context.view.title->effects()->render(camera,
            {ram.subspan(effect_commands / 4, menu::EffectRuntime::max_render_words),
             owner.fx_graphics->bindings(), owner.fx_graphics->reservations()});
        require(bool(rendered.status) && rendered.words <= menu::EffectRuntime::max_render_words &&
                !(rendered.words & 1u), "startup actual native FX render rejected", Error::preparation);
        wire::Memory memory(ram);
        memory.word(effect_commands + static_cast<std::uint32_t>(rendered.words * 4), 0xdf000000);
        memory.word(effect_commands + static_cast<std::uint32_t>(rendered.words * 4 + 4), 0);
        result.lanes[0] = {effect_commands, rendered.words + 2};
        return true;
    }
    void prepare(View view, FrameContext context, rsp::CanonicalExecutor::PreparationLease& preparation) {
        require(catalog && view.game && view.sprite_camera && context.buffer_index < 2,
                "startup adapter requires actual source resources/camera/context");
        const auto index = view.resources == menu::Scene::logo ? 0u : 1u;
        auto& assets = scenes[index];
        const auto& screen = catalog->screen(view.resources);
        std::size_t count = 0;
        if (view.resources == menu::Scene::logo) {
            require(!render_capacity || !view.logo_sprite || render_capacity->sprite_draws >= 1,
                    "startup sprites exceed cold render capacity", Error::preparation);
            startup_sprites.resize(view.logo_sprite ? 1 : 0);
            if (view.logo_sprite) {
                IsleSprite* scalar = nullptr;
                native(isle_game_sprite_view(view.game, view.logo_sprite, &scalar));
                const auto roots = catalog->sprites(view.resources);
                const auto root = std::find_if(roots.begin(), roots.end(), [](const auto& value) {
                    return value.identity == 0x802bc0c0;
                });
                require(root != roots.end(), "startup logo resource missing", Error::resource);
                startup_sprites[0] = {{static_cast<std::size_t>(root - roots.begin()), scalar},
                                     nullptr, {1,0x80000000u,UINT32_MAX}};
                count = 1;
            }
        } else {
            require(view.services != nullptr, "startup menu services missing");
            require(!render_capacity || view.services->sprite_count() <= render_capacity->sprite_draws,
                    "startup sprites exceed cold render capacity", Error::preparation);
            startup_sprites.resize(view.services->sprite_count());
            require(view.services->sprite_draws(startup_sprites, count),
                    "startup ordered native sprite draw borrow failed", Error::native);
        }
        StartupPreparation startup{this, view};
        std::array<render::Pass, 5> passes{};
        std::size_t used = 0;
        auto& clear = passes[used++];
        clear.kind = render::Kind::clear; clear.priority = 100;
        clear.flags = 3; clear.background_rgba = view.background_rgba;
        if (view.title && view.title->models()) {
            auto& pass = passes[used++];
            pass.camera = view.title->camera(); pass.priority = 5; pass.flags = 7;
            pass.background_rgba = 0xff; pass.prepare = title_pass; pass.userdata = &startup;
        }
        if (view.title && view.title->effects() && view.title->effects()->particle_count()) {
            auto& pass = passes[used++];
            pass.camera = view.title->camera(); pass.priority = 4;
            pass.prepare = effect_pass; pass.userdata = &startup;
        }
        auto& sprites = passes[used++];
        sprites.kind = render::Kind::sprites; sprites.camera = view.sprite_camera;
        sprites.priority = 3; sprites.flags = 8; sprites.link_mask = 2;
        sprites.camera_tag = UINT32_MAX; sprites.tag_match = render::TagMatch::equality;
        sprites.sprites = std::span<const scene::SpriteDraw>(startup_sprites).first(count);
        sprites.sprite_commands = sprite_commands;
        if (view.services && view.services->screen_transition_source()->D_800BE2C8) {
            passes[used++]=transitions.pass(view.services->screen_transition());
        }
        render::Input input{view.game,
            {index + 1, assets.sprites.get(), assets.installation, assets.reservations,
             {physical(screen.framebuffer_guests[0]), physical(screen.framebuffer_guests[1])},
             physical(catalog->pre_render(view.resources).display_list.guest_address)},
            std::span<const render::Pass>(passes).first(used), view.screen_offsets};
        prepare(input, context, preparation);
        for (auto& sprite : startup_sprites) sprite = {};
        startup_sprites.clear();
    }
    void prepare(render::Input input, FrameContext context,
                 rsp::CanonicalExecutor::PreparationLease& preparation) {
        require(input.game && context.buffer_index < 2 && input.passes.size() <= 32,
                "scene graphics requires a native game and bounded source passes");
        // Reject an unincluded task before mutable producer/RAM publication.
        static_cast<void>(source_programs.descriptor(input.program));
        program = input.program;
        const auto targets = source_screen ? source_targets : input.resources.framebuffers;
        const bool depthless = source_screen && !source_depth_reserved;
        const std::uint32_t depth = depthless ? 0 : storage.depth;
        const std::uint32_t depth_bytes = depthless ? 0 : storage.target_bytes;
        require(!source_depth_reserved || source_screen.has_value(),
                "reserved source depth was never applied to an actual screen", Error::resource);
        require(!source_screen || input.resources.framebuffers == source_targets,
                "source producer framebuffer identities differ from the applied screen", Error::resource);
        require(!render_capacity ||
                input.resources.reservations.size() <= render_capacity->resource_reservations,
                "scene resource reservations exceed cold render capacity", Error::preparation);
        for (const auto& pass : input.passes) if (pass.transition_owner)
            require(transition_borrow.retain(pass.transition_owner),
                    "scene transition preparation owner borrow rejected",Error::native);
        require(snapshot.canonical().data() == source, "scene graphics Snapshot moved", Error::resource);
        auto* producer = input.resources.sprites ? input.resources.sprites : &empty_sprites;
        auto ram = preparation.words();
        wire::validate_cold_inputs(ram, snapshot, source, storage.boot);
        composition_reservations.clear();
        composition_reservations.insert(composition_reservations.end(), storage.ranges.begin(), storage.ranges.end());
        if(source_display_active) {
            bool owner_present=false;
            for(const auto& pass:input.passes)
                if(pass.userdata==source_display_owner && pass.kind==render::Kind::compiled)
                    owner_present=true;
            require(owner_present,"Source display loan lacks its actual compiled owner",Error::resource);
        }
        const auto& retained=source_display_active?source_display_protected:source_gpu_storage;
        composition_reservations.insert(composition_reservations.end(),retained.begin(),retained.end());
        for (const auto framebuffer : targets) {
            require(!(framebuffer & 7u) && framebuffer <= menu::rdram_size &&
                    storage.target_bytes <= menu::rdram_size - framebuffer,
                    "scene framebuffer exceeds physical RAM", Error::resource);
            for (const auto core : storage.ranges)
                require(!intersects({framebuffer, storage.target_bytes}, core),
                        "scene framebuffer overlaps core task/depth/command storage");
            for (const auto range : source_gpu_storage)
                require(!intersects({framebuffer,storage.target_bytes},range),
                        "scene framebuffer overlaps retained Source GPU storage",Error::resource);
            composition_reservations.push_back({framebuffer, storage.target_bytes});
        }
        require(targets[0] + storage.target_bytes <= targets[1] ||
                targets[1] + storage.target_bytes <= targets[0],
                "scene source framebuffer contexts overlap", Error::resource);
        const auto protected_count = composition_reservations.size();
        require(input.resources.pre_render && input.resources.pre_render < menu::rdram_size &&
                !(input.resources.pre_render & 7u), "scene pre-render list has invalid physical identity");
        composition_reservations.insert(composition_reservations.end(), input.resources.reservations.begin(),
                                        input.resources.reservations.end());
        std::array<std::size_t, 32> order{};
        struct SpritePool { sprite::RamRange arena; std::uint32_t used = 0; };
        std::array<SpritePool, 32> sprite_pools{};
        std::array<std::size_t, 32> pass_pool{};
        std::size_t pool_count = 0;
        for (std::size_t i = 0; i < input.passes.size(); ++i) {
            const auto& pass = input.passes[i];
            require((pass.camera || pass.kind == render::Kind::clear) &&
                    (pass.kind != render::Kind::compiled || pass.prepare),
                    "scene pass is missing its actual camera/compiled preparation");
            if (pass.camera) static_cast<void>(bounds(camera_parameters(input.game, pass.camera).viewport,storage));
            require(!depthless || pass.control || !(pass.flags & 1u),
                    "depthless Source pass requested an actual depth clear",Error::resource);
            require(!depthless || pass.control || pass.kind != render::Kind::compiled || !pass.requires_depth,
                    "depthless Source compiled pass requires an actual depth target",Error::resource);
            if (pass.kind == render::Kind::sprites) {
                if (depthless) for (const auto& sprite : pass.sprites) if (pass.includes(sprite.render))
                    validate_depthless_sprite(sprite.draw);
                const auto arena = pass.sprite_commands;
                require(!(arena.address & 7u) && !(arena.size & 7u) &&
                        arena.address <= ram.size_bytes() &&
                        arena.size <= ram.size_bytes() - arena.address,
                        "sprite command arena has invalid physical bounds/alignment", Error::resource);
                for (std::size_t r = 0; r < protected_count; ++r)
                    require(!intersects(arena, composition_reservations[r]),
                            "sprite commands overlap core task/framebuffer storage");
                std::size_t pool = 0;
                for (; pool < pool_count; ++pool) {
                    const auto other = sprite_pools[pool].arena;
                    if (arena.address == other.address && arena.size == other.size) break;
                    require(!intersects(arena, other),
                            "sprite command arenas partially overlap", Error::resource);
                }
                if (pool == pool_count) {
                    sprite_pools[pool_count++] = {arena, 0};
                    composition_reservations.push_back(arena);
                }
                pass_pool[i] = pool;
            }
            require(pass.tag_match == render::TagMatch::intersection ||
                    pass.tag_match == render::TagMatch::equality, "invalid scene camera tag rule");
            order[i] = i;
        }
        // Stable source priority order, without a frame-time sorting allocation.
        for (std::size_t i = 1; i < input.passes.size(); ++i) {
            const auto item = order[i]; auto j = i;
            while (j && input.passes[order[j - 1]].priority < input.passes[item].priority) {
                order[j] = order[j - 1]; --j;
            }
            order[j] = item;
        }
        const bool changed = !resident || installed_identity != input.resources.identity ||
                             installed_producer != producer;
        require(!changed || epoch != std::numeric_limits<std::uint64_t>::max(), "scene RAM epoch exhausted");
        const auto next_epoch = epoch + (changed ? 1 : 0);
        graphics_borrow.emplace(producer->borrow());
        // Validate the complete ROM/mutable/producer/task/framebuffer union
        // before any RAM or residency write; only changed publications upload.
        producer->install_composed(ram, next_epoch, input.resources.immutable, composition_reservations);
        if (cold) {wire::install_cold_inputs(ram,snapshot,storage.boot);boot_resident=true;}
        cold = false; resident = true; epoch = next_epoch;
        installed_identity = input.resources.identity; installed_producer = producer;
        color = targets[context.buffer_index];
        Commands commands{storage,depth,depth_bytes};
        // One stable callable reset body per logical dispatch. Each generic
        // task gets its own post-END reset-call/branch entry. Saved/current
        // cursors advance on process40 and rewind only on actual reset10.
        const auto outer_begin=storage.top+0x100u;
        auto outer_saved=outer_begin;
        {
            wire::Memory memory(ram);
            const IsleGameFixedMatrix one{{{0x10000, 0, 1, 0}, {0, 0x10000, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}};
            memory.matrix(storage.identity,one);
            viewport(memory,storage.reset_viewport,full_viewport(storage),storage);
            commands.memory=&memory;
            commands.reset(color,input.resources.pre_render);
            require(commands.cursor<=outer_begin-8,"scene reset prefix exceeds its cold reservation",Error::preparation);
            commands.command(0xdf000000u,0);
            commands.cursor=outer_begin;
            commands.memory=nullptr;
        }
        ram={};
        // No retained mapping or Memory survives a Source callback. Commands
        // stores physical cursors; its writer is rebound only for this scope.
        auto mapped_commands = [&](auto&& function) -> decltype(auto) {
            return write_commands(commands,std::forward<decltype(function)>(function));
        };
        auto mapping = [&](void* user,void (*function)(void*,std::span<std::uint32_t>)) {
            mapped_commands([&](auto words,auto&) {function(user,words);});
        };
        constexpr std::array<unsigned, 4> lanes{0, 2, 1, 3};
        auto validate_list = [&](render::List list) {
            if (!list.words) return;
            require(!(list.address & 7u) && list.words >= 2 && !(list.words & 1u) &&
                    list.address <= menu::rdram_size &&
                    list.words <= (menu::rdram_size - list.address) / 4,
                    "scene compiled sublist exceeds physical RAM", Error::preparation);
            const sp::RamRange extent{list.address, static_cast<std::uint32_t>(list.words * 4)};
            for (std::size_t r = 0; r < protected_count; ++r)
                require(!intersects(extent, composition_reservations[r]),
                        "compiled sublist overlaps core task/framebuffer storage");
            bool covered = false;
            for (const auto range : input.resources.reservations)
                covered = covered || (range.address <= extent.address &&
                    std::uint64_t(extent.address) + extent.size <= std::uint64_t(range.address) + range.size);
            require(covered, "compiled sublist has no source producer reservation");
        };
        auto call = [&](render::List list) {
            attempt([&] {
            if(!list.words) return;
            validate_list(list);
            mapped_commands([&](auto,auto&) {commands.call(list.address);});
            });
        };
        auto finish = [&](render::List root,render::List terminal,wire::SourceProgram selected) {
            attempt([&] {
            require(writer_depth==0&&root.words&&terminal.words==4,
                    "source task boundary requires a unique actual terminal and no writer",Error::preparation);
            validate_list(root);validate_list(terminal);
            mapped_commands([&](auto words,auto&) {
                const auto at=terminal.address/4;
                require(words[at]==0xe9000000u&&words[at+1]==0&&words[at+2]==0xdf000000u&&words[at+3]==0,
                        "source task terminal is not actual FullSync/EndDL packets",Error::preparation);
            });
            submit(root,terminal,selected);
            commands.cursor=outer_saved=outer_begin;
            });
        };
        auto drain = [&] {drain_task();};
        auto reset = [&] {reset_slots();};
        auto capacity = [&] {reserve_task();};
        auto process = [&](bool resetting) {
            attempt([&] {
            require(writer_depth==0,"source process crossed an active RAM writer",Error::preparation);
            if(commands.cursor!=outer_saved) {
                reserve_task();
                std::array<wire::SourceLaneCursor,4> cursors{};
                cursors[0]={outer_saved,commands.cursor,commands.end,true};
                wire::SourceProgramSelection selection;
                selection.primary=selection.alternate=program;
                wire::SourceJoinPlan plan;
                wire::plan_source_join(source_programs,selection,cursors,
                    {storage.top,wire::reset_command_words/2+1},true,plan);
                mapped_commands([&](auto,auto&) {
                    for(std::size_t i=0;i<plan.word_counts[0];i+=2)
                        commands.command(plan.words[0][i],plan.words[0][i+1]);
                });
                submit({plan.root.address,plan.root.commands*2},
                    {plan.terminal.address,plan.terminal.commands*2},plan.program);
                outer_saved=commands.cursor;
            }
            if(resetting) {
                drain_task();reset_slots();
                commands.cursor=outer_saved=outer_begin;
            }
            });
        };
        for (std::size_t n = 0; n < input.passes.size(); ++n) {
            const auto& pass = input.passes[order[n]];
            IsleGameViewport vp{};
            sp::Bounds vp_bounds{}, clipped{};
            const auto vp_address = storage.reset_viewport + 16u * static_cast<std::uint32_t>(n + 1);
            render::Prepared prepared;
            auto initialize = [&] {
                mapped_commands([&](auto,auto& memory) {
                vp = pass.camera ? camera_parameters(input.game, pass.camera).viewport : full_viewport(storage);
                viewport(memory, vp_address, vp, storage);
                commands.camera(vp_address, vp, pass.flags, pass.background_rgba, 0, color);
                vp_bounds = bounds(vp,storage);
                clipped = {std::max(vp_bounds.left, pass.scissor.left),
                    std::max(vp_bounds.top, pass.scissor.top), std::min(vp_bounds.right, pass.scissor.right),
                    std::min(vp_bounds.bottom, pass.scissor.bottom)};
                require(clipped.left <= clipped.right && clipped.top <= clipped.bottom,
                        "scene pass scissor is disjoint or reversed");
                commands.scissor(clipped);
                });
            };
            auto restore = [&](unsigned lane,std::uint32_t cursor,std::uint32_t end) {
                return mapped_commands([&](auto ram,auto& memory) {
                require(pass.camera&&!(cursor&7u)&&!(end&7u)&&cursor<=end&&end<=ram.size_bytes(),
                        "source secondary camera cursor exceeds actual RAM",Error::preparation);
                const auto& current=camera_parameters(input.game,pass.camera).viewport;
                viewport(memory,vp_address,current,storage);
                Commands secondary{memory,storage,cursor,end,depth,depth_bytes};
                secondary.camera(vp_address,current,pass.flags&~3u,pass.background_rgba,lane,color,true);
                const auto current_bounds=bounds(current,storage);
                const sp::Bounds restored{std::max(current_bounds.left,pass.scissor.left),
                    std::max(current_bounds.top,pass.scissor.top),std::min(current_bounds.right,pass.scissor.right),
                    std::min(current_bounds.bottom,pass.scissor.bottom)};
                require(restored.left<=restored.right&&restored.top<=restored.bottom,
                        "scene pass scissor is disjoint or reversed");
                if(restored.left!=current_bounds.left||restored.top!=current_bounds.top||
                   restored.right!=current_bounds.right||restored.bottom!=current_bounds.bottom)
                    secondary.scissor(restored);
                secondary.command(0xda380003,storage.identity);
                return secondary.cursor;
                });
            };
            auto prepare_compiled = [&]() -> const render::Prepared& {
                require(!depthless || !pass.requires_depth,
                        "depthless Source compiled pass requires an actual depth target",Error::resource);
                require(pass.kind == render::Kind::compiled && pass.prepare,
                        "source explicit preparation requires a compiled pass", Error::preparation);
                mapped_commands([&](auto ram,auto&) {
                    require(pass.prepare(pass.userdata, ram, epoch, pass, prepared),
                            "scene compiled producer rejected preparation", Error::preparation);
                });
                require(!depthless || !prepared.requires_depth,
                        "depthless Source compiled producer prepared depth-dependent drawing",Error::resource);
                return prepared;
            };
            auto draw = [&] {
                mapped_commands([&](auto ram,auto&) {
                initialize();
                if (pass.kind == render::Kind::clear) return;
                if (pass.kind == render::Kind::sprites) {
                    draws.clear();
                    for (const auto& sprite : pass.sprites) if (pass.includes(sprite.render)) {
                        if (depthless) validate_depthless_sprite(sprite.draw);
                        require(!render_capacity || draws.size() < render_capacity->sprite_draws,
                                "scene sprites exceed cold render capacity", Error::preparation);
                        draws.push_back(sprite.draw);
                    }
                    command_reservations.clear();
                    auto& pool = sprite_pools[pass_pool[order[n]]];
                    const sprite::RamRange remaining{pool.arena.address + pool.used,
                                                      pool.arena.size - pool.used};
                    const auto command_end = std::uint64_t(remaining.address) + remaining.size;
                    bool covered = false;
                    for (const auto range : composition_reservations) {
                        const auto end = std::uint64_t(range.address) + range.size;
                        if (range.address <= remaining.address && command_end <= end) {
                            covered = true;
                            if (range.address < remaining.address)
                                command_reservations.push_back({range.address, remaining.address - range.address});
                            if (command_end < end)
                                command_reservations.push_back({static_cast<std::uint32_t>(command_end),
                                                                static_cast<std::uint32_t>(end - command_end)});
                        } else command_reservations.push_back(range);
                    }
                    require(covered, "scene sprite command arena is not reserved");
                    const auto list = producer->prepare(ram, epoch, draws, {vp_bounds, pass.scissor},
                                                        remaining, command_reservations);
                    pool.used += static_cast<std::uint32_t>(list.words * sizeof(std::uint32_t));
                    commands.call(list.address);
                    commands.mode(true, 19, 1, 0x80000);
                    draws.clear();
                    return;
                }
                prepare_compiled();
                for (const auto lane : lanes) {
                    if (lane && prepared.lanes[lane].words <= 2) continue;
                    if (lane) {
                        commands.camera(vp_address, vp, 0, 0, lane, color);
                        commands.scissor(clipped);
                    }
                    commands.command(0xda380003, storage.identity);
                    call(prepared.camera); call(prepared.lanes[lane]);
                }
                });
            };
            if (pass.control) {
                require(SourceControlAccess::dispatch(pass.control, pass.userdata, commands,
                            mapping, epoch, color, input.resources.pre_render,pass.flags,
                            initialize, prepare_compiled, draw, call, restore, finish, drain, reset, process, capacity),
                        "source camera control rejected dispatch", Error::preparation);
            } else draw();
        }
        if(input.finalize) {
            auto no_initialize=[] {throw Failure{Error::preparation,"final wire phase has no camera initialization"};};
            auto no_prepare=[]() -> const render::Prepared& {
                throw Failure{Error::preparation,"final wire phase cannot prepare another producer"};
            };
            auto no_restore=[](unsigned,std::uint32_t,std::uint32_t) -> std::uint32_t {
                throw Failure{Error::preparation,"final wire phase cannot restore another camera"};
            };
            require(SourceControlAccess::dispatch(input.finalize,input.finalize_user,commands,
                mapping,epoch,color,input.resources.pre_render,0,no_initialize,no_prepare,
                no_initialize,call,no_restore,finish,drain,reset,process,capacity),
                "source final task group rejected dispatch",Error::preparation);
        }
        // Only genuine pending commands form a final task. A Source10 reset
        // followed by an empty final process is CPU-only closure, not a task.
        if(!input.finalize) process(false);
        offsets = input.screen_offsets;
    }
};

Frame::Frame(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Frame::~Frame() = default;
CreateResult Frame::create(const menu::Catalog& catalog, const rom::Snapshot& snapshot, Config config) noexcept {
    CreateResult result;
    try { result.frame.reset(new Frame(std::make_unique<Impl>(&catalog, snapshot, config))); }
    catch (const Failure& e) { result.error = e.error; diagnostic(result.message, e.message); }
    catch (const std::exception& e) { result.error = Error::resource; diagnostic(result.message, e.what()); }
    catch (...) { result.error = Error::resource; diagnostic(result.message, "startup graphics construction exception"); }
    return result;
}
CreateResult Frame::create(const rom::Snapshot& snapshot, Config config) noexcept {
    CreateResult result;
    try { result.frame.reset(new Frame(std::make_unique<Impl>(nullptr, snapshot, config))); }
    catch (const Failure& e) { result.error = e.error; diagnostic(result.message, e.message); }
    catch (const std::exception& e) { result.error = Error::resource; diagnostic(result.message, e.what()); }
    catch (...) { result.error = Error::resource; diagnostic(result.message, "scene graphics construction exception"); }
    return result;
}
namespace {
template<class State, class Input>
bool begin_frame(State& state, Input view, FrameContext context, CompletedWork work = {}) noexcept {
    if (state.error != Error::none || state.retired || state.logical_active || state.source_gpu_active ||
        !state.executor.quiescent()) return false;
    state.ready = false;
    state.dispatch_started = true;
    state.logical_active = state.dispatching = true;
    state.presentation_eligible = true;
    state.receipt_count = state.source_task_count = 0;
    state.token = state.pending_token = state.last_submission = 0;
    state.submission_context = context.buffer_index;
    state.completion_work = work;
    try {
        state.preparation = state.executor.begin_preparation();
        state.prepare(view, context, state.preparation);
        state.dispatching = false;
        if (!state.last_submission) {
            // All Source cameras may be skipped. Close CPU ownership without
            // inventing a task, receipt, Photo callback or previous-token replay.
            state.executor.release_preparation(std::move(state.preparation));
            state.completion_work = {};
            state.presentation_eligible = false;
            return true;
        }
        if (!state.pending_token) {
            // A final Source10 already joined the real task. Flush any CPU-only
            // reset writes and read that task's persistent result without
            // submitting a fabricated N64 task just to obtain a mapping.
            state.ensure_preparation();
            state.final_mapping = state.executor.finish_preparation(
                std::move(state.preparation),state.last_submission);
        }
        return true;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::execution, e.what()); }
      catch (...) { state.fail(Error::execution, "startup graphics submission exception"); }
    state.dispatching = false;
    if(!state.last_submission && state.preparation) state.preparation.cancel();
    // Preserve first error and live owners. poll() owns cleanup of an admitted
    // earlier task, including a later CPU preparation that must be relinquished.
    state.fail(state.error,state.message.data());
    return false;
}
} // namespace
bool Frame::reserve_render_capacity(RenderCapacity capacity) noexcept {
    auto& state = *impl_;
    if (state.error != Error::none || state.retired) return false;
    try {
        state.reserve_render_capacity(capacity);
        return true;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::resource, e.what()); }
      catch (...) { state.fail(Error::resource, "scene render capacity reservation exception"); }
    return false;
}
const DepthReservation* Frame::reserve_source_depth(std::uint32_t bytes,
                                                   std::uint32_t alignment) noexcept {
    auto& state = *impl_;
    if (state.error != Error::none || state.retired) return nullptr;
    try {
        state.source_configuration_ready();
        require(!state.source_screen && !state.source_depth_reserved && bytes == state.source_depth.memory.size &&
                alignment == state.source_depth.alignment,
                "source depth allocation does not match actual Frame RDRAM storage", Error::resource);
        state.source_depth_reserved = true;
        return &state.source_depth;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::resource, e.what()); }
      catch (...) { state.fail(Error::resource, "source depth reservation exception"); }
    return nullptr;
}
bool Frame::configure_source_screen(const SourceScreen& screen, const DepthReservation* depth) noexcept {
    auto& state = *impl_;
    if (state.error != Error::none || state.retired) return false;
    try {
        state.configure_source_screen(screen, depth);
        return true;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::resource, e.what()); }
      catch (...) { state.fail(Error::resource, "source screen configuration exception"); }
    return false;
}
bool Frame::reserve_source_gpu_storage(std::span<const sprite::RamRange> ranges) noexcept {
    auto& state=*impl_;
    if (state.error!=Error::none || state.retired) return false;
    try {
        state.reserve_source_gpu_storage(ranges);
        return true;
    } catch (const Failure& e) { state.fail(e.error,e.message); }
      catch (const std::exception& e) { state.fail(Error::resource,e.what()); }
      catch (...) { state.fail(Error::resource,"Source GPU storage reservation exception"); }
    return false;
}
bool Frame::register_source_display_storage(const void* owner,
    std::span<const sprite::RamRange> ranges) noexcept {
    auto& state=*impl_;
    if(state.error!=Error::none || state.retired)return false;
    try {state.register_source_display_storage(owner,ranges);return true;}
    catch(const Failure& e) {state.fail(e.error,e.message);}
    catch(const std::exception& e) {state.fail(Error::resource,e.what());}
    catch(...) {state.fail(Error::resource,"Source display storage registration exception");}
    return false;
}
bool Frame::begin_source_display_storage(const void* owner) noexcept {
    auto& state=*impl_;
    if(!owner || owner!=state.source_display_owner || state.source_display_active ||
       state.error!=Error::none || state.retired || !quiescent() ||
       state.graphics_borrow || state.transition_borrow.owner)return false;
    state.source_display_active=true;
    state.resident=false;
    return true;
}
bool Frame::end_source_display_storage(const void* owner) noexcept {
    auto& state=*impl_;
    // Sticky preparation/execution failure does not revoke cleanup authority.
    if(!owner || owner!=state.source_display_owner || !state.source_display_active ||
       !quiescent() || state.graphics_borrow || state.transition_borrow.owner)return false;
    state.source_display_active=false;
    state.resident=false;
    return true;
}
render::Pass Frame::transition_pass(::isle::screen_transition::Owner& owner) noexcept {
    return impl_->transitions.pass(owner);
}
std::span<const sprite::AssetSpan> Frame::transition_immutable() const noexcept {
    return impl_->transitions.immutable();
}
sprite::RamRange Frame::transition_workspace() const noexcept {
    return impl_->transitions.memory();
}
SourceGpuAcquire Frame::acquire_source_gpu(SourceGpuBinding& output) noexcept {
    output={};
    auto& state=*impl_;
    if (state.error!=Error::none || state.retired || state.source_gpu_storage.empty())
        return SourceGpuAcquire::failed;
    if (state.source_display_active || state.source_gpu_active || state.logical_active ||
        !state.executor.quiescent() || state.graphics_borrow ||
        state.transition_borrow.owner) return SourceGpuAcquire::pending;
    if (state.epoch==std::numeric_limits<std::uint64_t>::max() ||
        state.source_gpu_admission==std::numeric_limits<std::uint64_t>::max()) {
        state.fail(Error::preparation,"Source GPU admission/epoch exhausted");
        return SourceGpuAcquire::failed;
    }
    ++state.epoch; ++state.source_gpu_admission;
    state.source_gpu_active=true;
    state.resident=false;
    output={&state.executor,&state.source_programs,state.epoch,state.source_gpu_admission};
    return SourceGpuAcquire::admitted;
}
bool Frame::release_source_gpu(const SourceGpuBinding& binding) noexcept {
    auto& state=*impl_;
    if (!state.source_gpu_active || state.logical_active || binding.executor!=&state.executor ||
        binding.programs!=&state.source_programs || binding.epoch!=state.epoch ||
        binding.admission!=state.source_gpu_admission || !state.executor.quiescent()) return false;
    // A normal frame must republish its own immutable and mutable raster
    // residency after replay replaced RAM. Persistent replay targets remain
    // protected, so Source score/thumbnail byte history is never restored away.
    state.resident=false;
    state.source_gpu_active=false;
    return true;
}
bool Frame::run_source_control(const SourceGpuBinding& binding,void* user,
    bool (*callback)(void*,SourceControl&),CompletedWork work) noexcept {
    auto& state=*impl_;
    if(!callback || !state.source_gpu_active || state.logical_active ||
       binding.executor!=&state.executor || binding.programs!=&state.source_programs ||
       binding.epoch!=state.epoch || binding.admission!=state.source_gpu_admission ||
       state.error!=Error::none || state.retired || !state.executor.quiescent()) return false;
    state.logical_active=state.dispatching=true;
    state.presentation_eligible=false;
    state.receipt_count=state.source_task_count=0;
    state.token=state.pending_token=state.last_submission=0;
    try {
        state.preparation=state.executor.begin_preparation();
        // Replay publishes its own real program/resources, but task transport
        // uses this persistent Frame's protected descriptor/boot workspace.
        if(!state.boot_resident) {
            wire::Memory memory(state.preparation.words());
            memory.copy(state.storage.boot,state.snapshot.canonical().subspan(wire::boot_rom,wire::boot_bytes));
            state.boot_resident=true;
        }
        Commands commands{state.storage,state.storage.depth,state.storage.target_bytes};
        auto saved_command=commands.cursor;
        auto mapping=[&](void* context,void (*function)(void*,std::span<std::uint32_t>)) {
            state.write_commands(commands,[&](auto words,auto&) {function(context,words);});
        };
        auto unavailable=[] {
            throw Failure{Error::preparation,"admitted Source task has no display camera pass"};
        };
        auto no_prepare=[]() -> const render::Prepared& {
            throw Failure{Error::preparation,"admitted Source task has no display producer pass"};
        };
        auto no_restore=[](unsigned,std::uint32_t,std::uint32_t) -> std::uint32_t {
            throw Failure{Error::preparation,"admitted Source task has no display camera restore"};
        };
        auto validate=[&](render::List list) {
            require(list.words>=2 && !(list.words&1u) && !(list.address&7u) &&
                    list.address<=menu::rdram_size && list.words<=(menu::rdram_size-list.address)/4,
                    "admitted Source list exceeds actual RAM",Error::preparation);
            const auto end=std::uint64_t(list.address)+list.words*4;
            bool covered=list.address>=state.storage.top && end<=state.storage.top_end;
            for(const auto range:state.source_gpu_storage)
                covered=covered||(range.address<=list.address&&end<=std::uint64_t(range.address)+range.size);
            require(covered,"admitted Source list escaped its retained storage",Error::preparation);
        };
        auto call=[&](render::List list) {
            state.attempt([&] {
                if(!list.words) return;
                validate(list);
                state.write_commands(commands,[&](auto,auto&) {commands.call(list.address);});
            });
        };
        auto finish=[&](render::List root,render::List terminal,wire::SourceProgram program) {
            state.attempt([&] {
                state.reserve_task();validate(root);validate(terminal);
                require(terminal.words==4,"admitted Source task has no unique terminal",Error::preparation);
                state.write_commands(commands,[&](auto words,auto&) {
                    const auto at=terminal.address/4;
                    require(words[at]==0xe9000000u&&words[at+1]==0&&words[at+2]==0xdf000000u&&words[at+3]==0,
                            "admitted Source terminal is not actual FullSync/EndDL",Error::preparation);
                });
                state.submit(root,terminal,program);
                saved_command=commands.cursor;
            });
        };
        auto drain=[&] {state.drain_task();};
        auto reset=[&] {state.reset_slots();};
        auto capacity=[&] {state.reserve_task();};
        auto process=[&](bool resetting) {
            state.attempt([&] {
                if(commands.cursor!=saved_command) {
                    state.reserve_task();
                    require(commands.cursor<=commands.end&&commands.end-commands.cursor>=16,
                            "admitted Source terminal exceeds cold capacity",Error::preparation);
                    const auto terminal=commands.cursor;
                    state.write_commands(commands,[&](auto,auto&) {
                        commands.command(0xe9000000u,0);commands.command(0xdf000000u,0);
                    });
                    state.submit({saved_command,(commands.cursor-saved_command)/4},
                        {terminal,4},wire::SourceProgram::f3dex2_non_fifo);
                    saved_command=commands.cursor;
                }
                if(resetting) {
                    state.drain_task();state.reset_slots();
                    commands.cursor=saved_command=state.storage.top;
                }
            });
        };
        require(SourceControlAccess::dispatch(callback,user,commands,mapping,state.epoch,
            state.color,0,0,unavailable,no_prepare,unavailable,call,no_restore,finish,
            drain,reset,process,capacity),"admitted Source callback rejected dispatch",Error::preparation);
        state.dispatching=false;
        require(state.last_submission!=0,"admitted Source callback submitted no actual task",Error::preparation);
        state.drain_task();
        state.ensure_preparation();
        auto completed=state.executor.finish_preparation(std::move(state.preparation),state.last_submission);
        if(work.function) work.function(work.context,completed,0);
        require(bool(completed)&&!completed.postprocess_active(),
                "admitted Source feedback retained its completed mapping",Error::execution);
        completed.release();
        require(state.error==Error::none,"admitted Source feedback retained a failure",Error::execution);
        state.token=0;state.logical_active=false;
        return true;
    } catch(const Failure& e) {state.fail(e.error,e.message);}
      catch(const std::exception& e) {state.fail(Error::execution,e.what());}
      catch(...) {state.fail(Error::execution,"admitted Source control exception");}
    state.dispatching=false;
    try {
        state.drain_task();
        if(state.preparation) {
            if(state.last_submission&&state.receipt_count&&
               state.receipts[state.receipt_count-1].completed&&!state.receipts[state.receipt_count-1].failed) {
                auto completed=state.executor.finish_preparation(std::move(state.preparation),state.last_submission);
                completed.release();
            } else state.preparation.cancel();
        }
    } catch(const std::exception& e) {state.fail(Error::execution,e.what());}
      catch(...) {state.fail(Error::execution,"admitted Source fault drain exception");}
    if(state.executor.quiescent()) {state.pending_token=state.token=0;state.logical_active=false;}
    return false;
}
bool Frame::begin(startup::View view, FrameContext context) noexcept {
    return begin_frame(*impl_, view, context);
}
bool Frame::begin(render::Input input, FrameContext context, CompletedWork work) noexcept {
    return begin_frame(*impl_, input, context, work);
}
bool Frame::fail_draw(Error error,const char* reason) noexcept {
    auto& state=*impl_;
    if(error==Error::none || state.source_gpu_active ||
       (!state.logical_active && !(state.ready&&state.last_submission))) return false;
    state.fail(error,reason);
    return true;
}
Poll Frame::poll() noexcept {
    auto& state = *impl_;
    // Only the outer logical closure may present or acknowledge failure.
    // Returning Source suffixes and mapped writers cannot retire an interim
    // task through this public polling entry point.
    if(state.dispatching) return Poll::pending;
    // Exact Replay admission owns every task and result in run_source_control;
    // even a reentrant poll must never turn it into a display completion.
    if(state.source_gpu_active) return Poll::pending;
    if (state.error != Error::none) {
        if (state.pending_token) {
            try {
                if (!state.executor.poll(state.pending_token)) return Poll::pending;
                auto completed = state.executor.completed(state.pending_token);
                state.receipts[state.receipt_count-1].completed=true;
                completed.release();
                state.pending_token=0;
            } catch (const Failure& e) {
                state.receipts[state.receipt_count-1].failed=true;state.fail(e.error,e.message);
            } catch (const std::exception& e) {
                state.receipts[state.receipt_count-1].failed=true;state.fail(Error::execution,e.what());
            } catch (...) {
                state.receipts[state.receipt_count-1].failed=true;
                state.fail(Error::execution,"startup graphics fault drain exception");
            }
            if(state.pending_token && state.executor.quiescent()) {
                state.pending_token=0;
                // A throwing GPU poll has consumed its exact failed token;
                // retain only its bounded cold receipt, never poll it again.
                state.receipts[state.receipt_count-1].failed=true;
            }
        }
        if (state.final_mapping) state.final_mapping.release();
        if (state.preparation) {
            try {
                if (state.last_submission && state.receipt_count &&
                    state.receipts[state.receipt_count-1].completed &&
                    !state.receipts[state.receipt_count-1].failed) {
                    auto completed=state.executor.finish_preparation(
                        std::move(state.preparation),state.last_submission);
                    completed.release();
                } else state.preparation.cancel();
            } catch (const std::exception& e) {state.fail(Error::execution,e.what());}
              catch (...) {state.fail(Error::execution,"source preparation fault cleanup exception");}
        }
        if (!state.executor.quiescent()) return Poll::pending;
        state.logical_active=false;
        state.fail(state.error, state.message.data());
        return Poll::failed;
    }
    if (state.token == 0) {
        if (state.logical_active) {
            // A successful no-work draw still has one logical completion, but
            // neither actual GPU work nor a completed-memory callback.
            state.graphics_borrow.reset();
            state.transition_borrow.reset();
            state.logical_active = false;
            ++state.ordinal;
            state.ready = true;
        }
        return state.ready ? Poll::completed : Poll::idle;
    }
    try {
        rsp::CanonicalExecutor::MemoryLease completed;
        if(state.final_mapping) completed=std::move(state.final_mapping);
        else {
            if (!state.executor.poll(state.pending_token)) return Poll::pending;
            completed=state.executor.completed(state.pending_token);
            state.pending_token=0;
            state.receipts[state.receipt_count-1].completed=true;
        }
        const bool valid_task=bool(completed.rsp_result())&&completed.rsp_result().full_syncs==1;
        if(!valid_task) state.receipts[state.receipt_count-1].failed=true;
        require(valid_task,
                "startup actual RSP task did not finish its source FullSync", Error::execution);
        if (state.completion_work.function) {
            const auto work = state.completion_work;
            state.completion_work = {};
            work.function(work.context, completed, state.color);
            require(bool(completed) && !completed.postprocess_active(),
                    "scene completion callback retained or abandoned the completed mapping", Error::execution);
        }
        // A nonthrowing callback may latch a public cold-configuration error
        // while this actual completed mapping is still borrowed.
        if (state.error != Error::none) {
            completed.release();
            require(state.executor.quiescent(), "startup GPU result retained after fault drain", Error::execution);
            state.logical_active=false;
            state.fail(state.error, state.message.data());
            return Poll::failed;
        }
        const auto ram = completed.rdram();
        require(ram.size() == menu::rdram_size && state.color <= ram.size() &&
                state.storage.target_bytes <= ram.size() - state.color,
                "startup completed source framebuffer exceeds mapped GPU RAM", Error::execution);
        for (std::size_t y = 0; y < state.storage.height; ++y) {
            const auto row = state.color + y * state.storage.width * 2;
            require(row <= ram.size() && state.storage.width * 2 <= ram.size() - row,
                    "startup completed source row exceeds mapped GPU RAM", Error::execution);
            for (std::size_t x = 0; x < state.storage.width; ++x) {
                const auto at = row + x * 2;
                const auto pixel = (std::to_integer<unsigned>(ram[at ^ 3u]) << 8) |
                                   std::to_integer<unsigned>(ram[(at + 1) ^ 3u]);
                const unsigned r = (pixel >> 11) & 31, g = (pixel >> 6) & 31, b = (pixel >> 1) & 31;
                const auto destination = (y * state.storage.width + x) * 4;
                state.rgba[destination] = std::byte((r << 3) | (r >> 2));
                state.rgba[destination + 1] = std::byte((g << 3) | (g >> 2));
                state.rgba[destination + 2] = std::byte((b << 3) | (b >> 2));
                state.rgba[destination + 3] = std::byte(255); // VI scanout is opaque; bit0 is coverage.
            }
        }
        completed.release();
        require(state.executor.quiescent(), "startup GPU result retained after readback", Error::execution);
        state.graphics_borrow.reset();
        state.transition_borrow.reset();
        state.token = 0; state.logical_active = false; ++state.ordinal; state.ready = true;
        return Poll::completed;
    } catch (const Failure& e) { state.fail(e.error, e.message); }
      catch (const std::exception& e) { state.fail(Error::execution, e.what()); }
      catch (...) { state.fail(Error::execution, "startup graphics completion exception"); }
    if(state.pending_token && state.executor.quiescent()) {
        state.receipts[state.receipt_count-1].failed=true;
        state.pending_token=0;
    }
    if(!state.preparation && !state.final_mapping && state.executor.quiescent()) {
        state.logical_active=false;
        state.fail(state.error,state.message.data());
    }
    return Poll::failed;
}
bool Frame::busy() const noexcept {
    return impl_->logical_active || impl_->source_gpu_active || !impl_->executor.quiescent();
}
bool Frame::quiescent() const noexcept { return !busy(); }
std::uint64_t Frame::submission() const noexcept {
    return impl_->presentation_eligible&&!impl_->source_gpu_active?impl_->token:0;
}
std::uint64_t Frame::presentation_submission() const noexcept {
    return impl_->presentation_eligible&&!impl_->source_gpu_active?impl_->last_submission:0;
}
std::span<const TaskReceipt> Frame::task_receipts() const noexcept {
    return std::span<const TaskReceipt>(impl_->receipts).first(impl_->receipt_count);
}
bool Frame::preparing() const noexcept {return impl_->dispatching;}
unsigned Frame::submission_context() const noexcept { return impl_->submission_context; }
bool Frame::failed_submission(std::uint64_t token) const noexcept {
    if(!token || impl_->error==Error::none || !quiescent()) return false;
    for(const auto& receipt:task_receipts()) if(receipt.token==token) return true;
    return false;
}
std::span<const sp::RamRange> Frame::reservations() const noexcept {
    if (impl_->source_screen) return impl_->source_reservations;
    return impl_->storage.ranges;
}
Error Frame::error() const noexcept { return impl_->error; }
const char* Frame::message() const noexcept { return impl_->message.data(); }
Completed Frame::completed() const noexcept {
    if (!impl_->ready || busy() || impl_->retired || impl_->error != Error::none) return {};
    return {impl_->rgba, impl_->storage.width, impl_->storage.height,
            impl_->storage.width * 4, impl_->offsets, impl_->ordinal};
}
bool Frame::retire() noexcept {
    if (busy() || impl_->source_display_active) return false;
    impl_->graphics_borrow.reset();
    impl_->transition_borrow.reset();
    impl_->ready = false; impl_->retired = true;
    return true;
}
} // namespace isle::scene::graphics
