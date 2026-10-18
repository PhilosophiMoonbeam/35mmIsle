// Opt-in private-ROM component consumer. Never CTest/default discovery, a
// Player FSM, a photo-capture fixture, a physical input/device probe or a second
// renderer. Main owns compilation/execution and the CPU-Vulkan qualification.
// Usage: player-ui-screen-rsp-smoke PRIVATE_ROM ABSOLUTE_EMPTY_OUTPUT_DIRECTORY
#include "player_ui.hpp"
#include "player_photo.hpp"
#include "screen_transition_resources.hpp"
#include "scene_graphics.hpp"
#include "scene_storage.hpp"
#include "photo_lighting.hpp"
#include "actual_audio_driver.hpp"
#include "player_source_fixture.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace isle;
namespace g = scene::graphics;
namespace r = scene::render;
namespace sp = sprite;
constexpr std::size_t width = 320, height = 240, plane = width * height * 2;
constexpr std::uint32_t background = 0x385878ff;
constexpr sp::RamRange ordinary_commands{0x150000,0x50000};
constexpr sp::RamRange message_commands{0x1a0000,0x40000};
constexpr std::array<sp::RamRange,3> producer_ranges{{ordinary_commands,message_commands,
    screen_transition::Resources::workspace}};
constexpr std::array<std::uint32_t,2> framebuffers{0x3b5000,0x3da800};
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void native(IsleGameResult value, std::source_location where = std::source_location::current()) {
    if (value != ISLE_GAME_OK) throw std::runtime_error(std::string("native UI/screen operation at ") +
        where.file_name() + ":" + std::to_string(where.line()) + " result=" + std::to_string(int(value)));
}
void audio_ok(audio::Error value) { require(value == audio::Error::none,"actual audio Session operation rejected"); }

struct SourceLifetime {
    IslePlayerSource value{};
    explicit SourceLifetime(IsleGame* game, isle_rng& rng) {
        native(isle_player_source_prepare(game,&value,&rng,this,10));
        native(isle_player_source_activate(&value));
    }
    ~SourceLifetime() { if (isle_player_source_deactivate(&value) != ISLE_GAME_OK) std::terminate(); }
};
struct AudioClock {
    game_audio::Session& session;
    private_audio::Execution& execution;
    std::uint64_t retrace = 0;
    unsigned tasks = 0;
    void tick() {
        ++retrace;
        audio_ok(session.retrace(retrace,retrace*1000000000ull/60));
        audio_ok(session.submit());
        audio::TaskView task; audio_ok(session.build(task));
        if (task.token.serial) {
            private_audio::execute(session,execution,task); ++tasks;
            audio_ok(session.submit());
        }
    }
};
struct Capture {
    std::vector<std::byte> rgba, color, depth;
    std::size_t triangle_packets = 0, rectangle_packets = 0;
    std::array<std::uint32_t,3> shared_list_bytes{};
};
struct Readback {
    Capture& capture;
    screen_transition::Owner* transition = nullptr;
    unsigned calls = 0;
    std::uint32_t shared_capacity = 0;
    static void finish(void* opaque, rsp::CanonicalExecutor::MemoryLease& lease, std::uint32_t color) {
        auto& self = *static_cast<Readback*>(opaque);
        require(++self.calls == 1,"surface did not receive exactly one real completed mapping");
        if (self.transition)
            require(!self.transition->retirement_ready(),"transition Owner released before actual GPU/postprocess/readback");
        const auto ram = lease.rdram();
        require(ram.size() == 8*1024*1024,"completed real RDRAM mapping has wrong extent");
        self.capture.color.resize(plane); self.capture.depth.resize(plane);
        for (std::size_t i = 0; i < plane; ++i) {
            self.capture.color[i] = ram[(color+i)^3u];
            self.capture.depth[i] = ram[(0x600000+i)^3u];
        }
        if (self.shared_capacity) {
            auto cursor = ordinary_commands.address;
            const auto end = cursor + self.shared_capacity;
            for (auto& bytes : self.capture.shared_list_bytes) {
                const auto begin = cursor;
                bool terminated = false;
                while (cursor + 8 <= end) {
                    const auto opcode = ram[cursor ^ 3u];
                    cursor += 8;
                    if (opcode == std::byte{0xdf}) { terminated = true; break; }
                }
                require(terminated,"shared arena lost an independently retained EndDL list");
                bytes = cursor - begin;
            }
        }
        const auto& packets = lease.packets();
        for (const auto packet : packets.packets()) {
            const auto words = packets.words().subspan(packet.offset,packet.count);
            require(!words.empty(),"empty actual RDP packet");
            const auto op = (words[0]>>24)&63;
            self.capture.triangle_packets += op >= 8 && op <= 15;
            self.capture.rectangle_packets += op == 0x24 || op == 0x25;
        }
        require(!lease.postprocess_active(),"UI surface consumer retained postprocess ownership");
    }
};
void write_bytes(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    require(!std::filesystem::exists(path),"private output would replace an existing artifact");
    std::ofstream out(path,std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    out.flush(); require(bool(out),"private completed surface write failed");
}
std::size_t changes(const Capture& a, const Capture& b, sp::Bounds region = {}) {
    require(a.rgba.size() == width*height*4 && b.rgba.size() == a.rgba.size(),"RGBA comparison extent mismatch");
    std::size_t changed = 0;
    for (int y = region.top; y < region.bottom; ++y) for (int x = region.left; x < region.right; ++x) {
        const auto at = (std::size_t(y)*width+std::size_t(x))*4;
        changed += !std::equal(a.rgba.begin()+static_cast<std::ptrdiff_t>(at),
            a.rgba.begin()+static_cast<std::ptrdiff_t>(at+3),b.rgba.begin()+static_cast<std::ptrdiff_t>(at));
    }
    return changed;
}
void unchanged_outside(const Capture& a, const Capture& b, sp::Bounds region) {
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        if (int(x) >= region.left && int(x) < region.right && int(y) >= region.top && int(y) < region.bottom) continue;
        const auto at = (y*width+x)*4;
        require(std::equal(a.rgba.begin()+static_cast<std::ptrdiff_t>(at),
            a.rgba.begin()+static_cast<std::ptrdiff_t>(at+3),b.rgba.begin()+static_cast<std::ptrdiff_t>(at)),
            "source UI draw changed pixels outside its independently bounded placement");
    }
}
std::array<unsigned,3> center(const Capture& capture) {
    const auto at = (120*width+160)*4;
    return {unsigned(capture.rgba[at]),unsigned(capture.rgba[at+1]),unsigned(capture.rgba[at+2])};
}
unsigned distance(const Capture& a, const Capture& b) {
    const auto x = center(a), y = center(b);
    unsigned result = 0;
    for (unsigned n = 0; n < 3; ++n) result += x[n] > y[n] ? x[n]-y[n] : y[n]-x[n];
    return result;
}

// This is only the composition seam: original C parses and emits native glyph
// Draws once at its real Msg camera lane0. The SAME Producer and actual Frame
// perform all texture/GBI/RSP/RDP work. There is no copied glyph or CPU raster.
struct MessagePass {
    player::Ui& ui;
    std::vector<sp::RamRange> protected_ranges;
    unsigned calls = 0;
    static bool prepare(void* opaque, std::span<std::uint32_t> words, std::uint64_t epoch,
                        const r::Pass&, r::Prepared& output) {
        auto& self = *static_cast<MessagePass*>(opaque);
        native(self.ui.message_draw()); ++self.calls;
        const auto list = self.ui.producer().prepare(words,epoch,self.ui.message_draws(),{},
            message_commands,self.protected_ranges);
        output.lanes[0] = {list.address,list.words};
        return true;
    }
};
struct SourceDispatch {
    scene::Services& services;
    IslePlayerSource& source;
    IsleGameObject* film;
    std::span<scene::SpriteDraw> scratch{};
    r::Pass* ordinary = nullptr;
    g::SourceControl* active = nullptr;
    unsigned cleanups = 0;
    std::size_t late_count = 0;
    bool mutate = true, black = false, fail = false, prepared = false;
    bool early_film_absent = false, late_film_present = false;
    IsleGameResult result = ISLE_GAME_OK;
    struct Scope {
        SourceDispatch& owner;
        Scope(SourceDispatch& value, g::SourceControl& control) : owner(value) {
            require(!owner.active,"source control sink was already live");
            owner.active = &control;
        }
        ~Scope() { owner.active = nullptr; ++owner.cleanups; }
    };
    static bool before(void* context, g::SourceControl& control) {
        auto& self = *static_cast<SourceDispatch*>(context);
        Scope scope(self,control);
        if (self.fail) {
            self.result = isle_player_source_call(&self.source,player_source_nested_failure,nullptr);
            return self.result == ISLE_GAME_OK;
        }
        if (self.mutate) {
            IsleGameObjectMetadata metadata{};
            native(isle_game_object_metadata(self.services.game,self.film,&metadata));
            native(isle_game_object_set_flags(self.services.game,self.film,metadata.flags & ~1u));
            PlayerSpriteBorrow request{self.film,nullptr,nullptr,0,0,0};
            self.result = isle_player_source_call(&self.source,player_source_sprite_borrow,&request);
            require(self.result == ISLE_GAME_OK && request.entered == 1 && request.completed == 1,
                "real C source sprite mutation did not return before camera dispatch");
        }
        self.prepared = true;
        if (self.black) {
            // Actual inclusive 320x240 RGBA16 fill, not a CPU image edit.
            control.command(0xe7000000u,0);
            control.command(0xff10013fu,control.framebuffer());
            control.command(0xe3000a01u,0x00300000u);
            control.command(0xf7000000u,0x00010001u);
            control.command(0xf64fc3bcu,0);
            control.command(0xe7000000u,0);
            control.command(0xe3000a01u,0);
        } else control.draw();
        return true;
    }
    static bool sprites(void* context, g::SourceControl& control) {
        auto& self = *static_cast<SourceDispatch*>(context);
        Scope scope(self,control);
        require(self.prepared && self.ordinary,"UI dispatch preceded source camera control");
        require(self.services.sprite_draws(self.scratch,self.late_count),
            "dispatch-time actual native sprite projection failed");
        self.ordinary->sprites = self.scratch.first(self.late_count);
        self.late_film_present = std::any_of(self.ordinary->sprites.begin(),self.ordinary->sprites.end(),
            [&](const auto& draw) { return draw.object == self.film; });
        control.draw();
        return true;
    }
};
struct Consumer {
    scene::Services& services;
    player::Ui& ui;
    g::Frame& frame;
    screen_transition::Resources& transitions;
    AudioClock& audio;
    const std::filesystem::path& root;
    std::ofstream& report;
    IsleGameCamera* sprite_camera;
    std::span<const sp::AssetSpan> immutable;
    MessagePass message;
    unsigned frames = 0;
    std::array<scene::SpriteDraw,64> native_sprites{};
    Consumer(scene::Services& s, player::Ui& u, g::Frame& f, screen_transition::Resources& t,
             AudioClock& a, const std::filesystem::path& directory, std::ofstream& log,
             IsleGameCamera* camera, std::span<const sp::AssetSpan> assets)
      : services(s),ui(u),frame(f),transitions(t),audio(a),root(directory),report(log),
        sprite_camera(camera),immutable(assets),message{u,{}} {
        // Complete disjoint live RDRAM reservation union, EXCEPT the message
        // arena that this one compiled pass writes. No broad cold/command span
        // that accidentally aliases glyph ROM, transitions or framebuffers.
        const auto core = frame.reservations();
        message.protected_ranges.assign(core.begin(),core.end());
        message.protected_ranges.push_back(ordinary_commands);
        message.protected_ranges.push_back(screen_transition::Resources::workspace);
        for (auto color : framebuffers) message.protected_ranges.push_back({color,plane});
        for (const auto asset : immutable)
            message.protected_ranges.push_back({asset.address,static_cast<std::uint32_t>(asset.bytes.size())});
    }
    void healthy() {
        native(ui.source().status); native(ui.source().player->status);
        native(services.screen_transition_source()->status);
        require(!services.failed(),services.error());
    }
    void tick(std::uint32_t pressed = 0) {
        require(frame.quiescent(),"native OM tick attempted during real graphics/readback ownership");
        // Explicit component input boundary: all six canonical scalars publish
        // ONCE before OM. No physical/LUS controller or full Player is claimed.
        ui.source().player->input = {pressed,pressed,0,0,0,0};
        native(isle_game_tick(services.game)); healthy(); audio.tick();
    }
    void ticks(unsigned count) { while (count--) tick(); }
    Capture draw(const std::string& name = {}, bool ready_owner_probe = false,
                 SourceDispatch* source = nullptr, bool shared_arena = false,
                 std::uint32_t shared_capacity = ordinary_commands.size / 2,
                 bool expected_failure = false, unsigned overlap = 0) {
        healthy(); require(frame.quiescent(),"previous actual frame has not retired");
        std::size_t count = 0;
        require(services.sprite_draws(native_sprites,count),"actual ordered native sprite projection failed");
        std::array<r::Pass,8> passes{};
        auto& clear = passes[0]; clear.kind = r::Kind::clear; clear.priority = 100;
        clear.flags = 3; clear.background_rgba = background;
        auto& msg = passes[1]; msg.camera = ui.source().Msg_Camera; msg.priority = 3;
        msg.prepare = MessagePass::prepare; msg.userdata = &message;
        auto& ordinary = passes[2]; ordinary.kind = r::Kind::sprites;
        ordinary.camera = sprite_camera; ordinary.priority = 2; ordinary.flags = 8;
        ordinary.link_mask = 2; ordinary.camera_tag = UINT32_MAX; ordinary.tag_match = r::TagMatch::equality;
        ordinary.sprites = std::span(native_sprites).first(count); ordinary.sprite_commands = ordinary_commands;
        if (source) {
            source->scratch = native_sprites; source->ordinary = &ordinary;
            source->early_film_absent = std::none_of(ordinary.sprites.begin(),ordinary.sprites.end(),
                [&](const auto& draw) { return draw.object == source->film; });
            clear.control = SourceDispatch::before; clear.userdata = source;
            ordinary.control = SourceDispatch::sprites; ordinary.userdata = source;
        }
        std::size_t used = 3;
        if (shared_arena && source) {
            // A real earlier empty dispatch consumes the same arena before
            // Source mutates and re-gathers the later ordinary inventory.
            passes[3] = ordinary; passes[3].sprites = {};
            passes[3].control = nullptr; passes[3].priority = 3;
            used = 4;
        } else if (shared_arena) {
            require(count >= 3,"shared arena regression requires ordered native sprite inventory");
            const auto inventory = ordinary.sprites;
            const auto first = count / 3, second = count * 2 / 3;
            ordinary.sprites = inventory.first(first);
            ordinary.sprite_commands = {ordinary_commands.address,shared_capacity};
            passes[3] = ordinary;
            passes[3].sprites = inventory.subspan(first,second-first);
            passes[3].sprite_commands = {ordinary_commands.address + ordinary_commands.size / 2,
                                        ordinary_commands.size / 2};
            if (overlap) {
                const auto base = ordinary_commands.address;
                passes[3].sprite_commands = overlap == 1 ?
                    sp::RamRange{base + 8,shared_capacity} :
                    sp::RamRange{base,shared_capacity - 8};
                if (overlap == 3)
                    std::swap(ordinary.sprite_commands,passes[3].sprite_commands);
            }
            passes[4] = ordinary;
            passes[4].sprites = inventory.subspan(second);
            // Actual empty draws still append a complete list. A skipped
            // dispatch must not consume capacity or overwrite prior lists.
            passes[5] = ordinary; passes[5].sprites = {};
            passes[6] = ordinary;
            passes[6].control = +[](void*,g::SourceControl&) { return true; };
            if (overlap) for (std::size_t i = 2; i <= 6; ++i)
                passes[i].control = +[](void*,g::SourceControl&) { return true; };
            used = 7;
        }
        auto& owner = services.screen_transition();
        const bool transitioning = owner.source.D_800BE2C8 != nullptr;
        if (transitioning) passes[used++] = transitions.pass(owner);
        if (ready_owner_probe) require(owner.retirement_ready(),"joined transition Owner was not ready before submission");
        Capture capture;
        Readback readback{capture,transitioning ? &owner : nullptr};
        readback.shared_capacity = shared_arena && !source ? shared_capacity : 0;
        const auto calls = message.calls;
        const r::Input input{services.game,{1,&ui.producer(),immutable,producer_ranges,framebuffers,0xaec18},
            std::span(passes).first(used),{}};
        const auto begun = frame.begin(input,{frames&1u},{&readback,Readback::finish});
        if (source) { source->ordinary = nullptr; source->scratch = {}; }
        if (expected_failure) {
            require(!begun && !frame.submission() && !frame.busy(),
                    "failed shared arena preparation submitted a GPU task");
            require(message.calls == calls + (overlap ? 0 : 1),
                    "arena overlap was not rejected before compiled callback side effects");
            return capture;
        }
        require(begun,frame.message());
        require(frame.busy() && frame.submission() && message.calls == calls+1,
            "actual Frame failed to retain submission or drew Msg more than once");
        if (transitioning) require(!owner.retirement_ready(),"actual Frame preparation failed to retain exact transition Owner");
        require(!frame.retire(),"actual executing Frame accepted premature retirement");
        const auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes(5);
        for (;;) {
            const auto state = frame.poll();
            require(state != g::Poll::failed,frame.message());
            if (state == g::Poll::completed) break;
            require(state == g::Poll::pending && std::chrono::steady_clock::now() < deadline,
                "actual CPU RSP/RDP completion timed out");
            std::this_thread::yield(); // No second simulation clock or OM tick.
        }
        require(frame.quiescent() && readback.calls == 1,"GPU/postprocess/readback did not quiesce");
        const auto done = frame.completed();
        require(done && done.rgba.size() == width*height*4,"actual Frame RGBA readback extent mismatch");
        capture.rgba.assign(done.rgba.begin(),done.rgba.end()); ++frames;
        if (ready_owner_probe) require(owner.retirement_ready(),"Frame retained joined Owner after actual completed readback");
        if (!name.empty()) save(name,capture);
        return capture;
    }
    void save(const std::string& name, const Capture& capture) {
        const auto file = root/(name+".pam");
        require(!std::filesystem::exists(file),"private RGBA artifact already exists");
        std::ofstream out(file,std::ios::binary);
        out << "P7\nWIDTH 320\nHEIGHT 240\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
        out.write(reinterpret_cast<const char*>(capture.rgba.data()),static_cast<std::streamsize>(capture.rgba.size()));
        out.flush(); require(bool(out),"actual RGBA PAM write failed");
        write_bytes(root/(name+".rgba16be"),capture.color);
        write_bytes(root/(name+".depth16be"),capture.depth);
        report << "surface\t" << name << "\ttriangles=" << capture.triangle_packets
               << "\trectangles=" << capture.rectangle_packets << '\n'; report.flush();
    }
    void evidence(const char* text) { report << "passed\t" << text << '\n'; report.flush(); }
};

unsigned special_b = 0, special_q = 0;
extern "C" void message_special(std::uint8_t code) {
    if (code == 'B') ++special_b;
    if (code == 'Q') ++special_q;
}
const std::uint8_t* text(const char* value) { return reinterpret_cast<const std::uint8_t*>(value); }
void show(Consumer& c, const char* value, int delay = 0, std::uint8_t alpha = 255) {
    native(isle_player_ui_Msg_ShowMessage(&c.ui.source(),text(value),24,36,248,216,168,alpha,delay,0));
}
void messages(Consumer& c) {
    auto& ui = c.ui.source();
    native(isle_player_ui_Msg_SetNonInteractive(&ui));
    native(isle_player_ui_Msg_Reset(&ui));
    const auto blank = c.draw("message-empty");
    show(c,"\\#S0\\#F0\\#X10\\#Y10ABCDEFG xyz 09!?");
    const auto ascii = c.draw("message-ascii-unshadowed");
    require(ascii.rectangle_packets >= 12 && changes(blank,ascii,{24,36,180,52}) > 40,
        "actual source ASCII partial S-load produced no meaningful glyph output");
    unchanged_outside(blank,ascii,{24,36,200,52});
    require(ascii.depth == blank.depth,"Msg lane0 unexpectedly wrote the actual depth surface");
    show(c,"\\#S1\\#F0\\#X10\\#Y10ABCDEFG xyz 09!?");
    const auto shadow = c.draw("message-ascii-shadow");
    require(shadow.rectangle_packets == ascii.rectangle_packets*2 && changes(ascii,shadow) > 10,
        "original C shadow did not produce extra real rectangles and changed pixels");
    show(c,"\\#S0\\#F0\\#X10\\#Y10ABCDEFG xyz 09!?",0,96);
    const auto alpha = c.draw("message-ascii-alpha96");
    require(changes(alpha,ascii) > 30 && alpha.depth == ascii.depth,
        "source message primitive alpha did not affect actual RDP blending");
    // EUC-JP symbols from the ORIGINAL D_8039379C table, not invented kana or
    // normalized font copies. The A3 pair also takes its wide-Latin branch.
    show(c,"\\#S0\\#F0\\#X10\\#Y10\xA1\xCA\xA1\xCB\xA1\xAA\xA2\xA5\xA3\xC1\xA3\xDA");
    const auto japanese = c.draw("message-japanese-source-symbols-wide-latin");
    require(japanese.rectangle_packets == 6 && changes(blank,japanese,{24,36,120,52}) > 25 &&
            changes(ascii,japanese) > 25,"original two-byte/Japanese-source glyph paths failed actual GPU output");
    show(c,"\\#S0\\#F0\\#X10\\#Y10\\z\\r\\a\\b\\[\\]\\F");
    auto buttons = c.draw();
    require(buttons.rectangle_packets == 7 && changes(blank,buttons) > 30,
        "source RGB button glyphs did not produce actual S-load output");
    const auto first_index = ui.Msg_ImageIndex;
    c.save("message-buttons-bank-first",buttons);
    bool second_bank = false;
    for (unsigned n = 0; n < 9; ++n) {
        const auto next = c.draw();
        if (ui.Msg_ImageIndex != first_index) {
            require(changes(buttons,next) > 4,"original eight-draw button bank change did not change actual pixels");
            c.save("message-buttons-bank-other",next); second_bank = true; break;
        }
    }
    require(second_bank,"actual Msg draw cadence did not reach its other source button bank");
    native(isle_player_ui_Msg_SetSpecialHandler(&ui,message_special));
    show(c,"\\#S1\\#F1\\#X15\\#Y20A\\B\\Q\nZ\\a");
    require(special_b == 1 && special_q == 1,"actual C message preparse did not dispatch B/Q controls");
    const auto controls = c.draw("message-controls-newline-scale-shadow-blink");
    require(ui.Message_ScaleX == 1.5f && ui.Message_ScaleY == 2 && ui.Msg_ShadowEnabled &&
            ui.Msg_BlinkEnabled && controls.rectangle_packets == 6 &&
            changes(blank,controls,{24,36,100,100}) > 30,
            "actual control/newline/scaled message output disagrees with original C state");
    native(isle_player_ui_Msg_SetSpecialHandler(&ui,nullptr));
    c.evidence("ASCII I8 / EUC-JP table+wide Latin / RGBA16 buttons both banks / source shadow-alpha-controls actual color+depth");

    for (const auto [button,name] : std::array<std::pair<std::uint32_t,const char*>,3>{{
        {0x8000,"a"},{0x4000,"b"},{0x1000,"start"}}}) {
        native(isle_player_ui_Msg_SetInteractive(&ui));
        show(c,"\\#S0\\#F0\\#X10\\#Y10");
        show(c,"ABCDEFGHIJKLMN OPQRSTUVWXYZ",4);
        c.tick();
        const auto prefix = std::strlen(reinterpret_cast<const char*>(ui.Msg_PrintedText));
        require(prefix == 2 && !isle_player_ui_Msg_IsMessagePrinted(&ui),
            "real Msg stackful printer did not suspend after original two-byte progress");
        const auto partial = c.draw(std::string("message-interactive-")+name+"-prefix");
        c.tick();
        require(std::strlen(reinterpret_cast<const char*>(ui.Msg_PrintedText)) == prefix,
            "source printer skipped its genuine delay with no published button");
        c.tick(button);
        require(std::strcmp(reinterpret_cast<const char*>(ui.Msg_PrintedText),
                            reinterpret_cast<const char*>(ui.Msg_MessageText)) == 0 && !ui.Msg_IsPrinted,
            "published native input did not complete text before the original final wait");
        c.tick();
        require(isle_player_ui_Msg_IsMessagePrinted(&ui),"source printer never completed after its final native wait");
        const auto complete = c.draw(std::string("message-interactive-")+name+"-complete");
        require(changes(partial,complete) > 20 && changes(blank,complete,{211,68,225,82}) > 2,
            "real interactive completion failed full text and original NEXT glyph output");
    }
    // Independently exercise the noninteractive delay path without button skip.
    native(isle_player_ui_Msg_SetNonInteractive(&ui));
    show(c,"ABCD",2); c.tick();
    require(std::strcmp(reinterpret_cast<const char*>(ui.Msg_PrintedText),"AB") == 0 && !ui.Msg_IsPrinted,
        "noninteractive source printer did not publish the first genuine prefix");
    c.ticks(2); require(!ui.Msg_IsPrinted && std::strcmp(reinterpret_cast<const char*>(ui.Msg_PrintedText),"ABCD") == 0,
        "noninteractive delayed printer did not preserve its final wait");
    c.tick(); require(ui.Msg_IsPrinted,"noninteractive printer did not join after its original final wait");
    c.draw("message-noninteractive-complete");
    native(isle_player_ui_Msg_Reset(&ui));
    c.evidence("canonical six-scalar publication before OM; original Msg_UpdatePrinter A/B/START skip and noninteractive native wait completion");
}

void icons_and_film(Consumer& c) {
    auto& ui = c.ui.source();
    ui.player->ProgressFlags = 0x1027; // Explicit diagnostic component progress boundary: all original item/dash/zoom bits.
    native(isle_player_ui_Icons_Init(&ui));
    native(isle_player_ui_func_8035E37C_4FE78C(&ui));
    require(ui.Icons_NumItemsAvailable == 3 && ui.player->LastItemId == 163 &&
        isle_player_ui_func_8035E508_4FE918(&ui) == 60,"original Icons/film initialization did not consume canonical progress/roll");
    c.tick(); const auto normal = c.draw("icons-film-60");
    native(isle_player_ui_Icons_ProcessButtonPress(&ui,163));
    c.ticks(12); const auto shrinking = c.draw("icons-apple-native-shrink");
    auto* apple = c.services.view(ui.Icons_IconObjects[0].spriteObj);
    require(apple && apple->scalex < 0.9f && changes(normal,shrinking,{200,180,240,220}) > 5,
        "original apple stackful shrink process did not change real native sprite/GPU output");
    c.ticks(32); const auto restored = c.draw("icons-apple-native-restored");
    require(apple->scalex == 1 && apple->scaley == 1 && changes(normal,restored) == 0,
        "original item-icon process failed exact source restoration");
    native(isle_player_ui_Icons_ProcessButtonPress(&ui,161)); c.ticks(5);
    const auto flute = c.draw("icons-flute-native-pulse");
    require(changes(normal,flute,{230,180,285,235}) > 5,"original flute process did not pulse actual source icon pixels");
    native(isle_player_ui_Icons_ProcessButtonPress(&ui,0)); c.tick();
    native(isle_player_ui_Icons_ProcessZoom(&ui,1)); c.ticks(24);
    const auto zoom = c.draw("icons-native-zoom-in");
    require(ui.Icons_IsZoomedIn && (apple->attr&ISLE_SP_HIDDEN) && changes(normal,zoom) > 30,
        "source zoom process failed real outgoing/incoming icon visibility");
    native(isle_player_ui_Icons_ProcessTakePhotoPressed(&ui)); c.ticks(2);
    const auto squeezed = c.draw("icons-photo-native-squeeze");
    auto* photo = c.services.view(ui.Icons_IconObjects[6].spriteObj);
    require(photo && photo->scaley < 1 && changes(zoom,squeezed,{225,175,290,230}) > 2,
        "original photo-button icon process did not change actual pixels");
    c.ticks(10); require(photo->scaley == 1 && !(photo->attr&ISLE_SP_HIDDEN),"photo icon native process failed restoration");
    native(isle_player_ui_Icons_ProcessZoom(&ui,0)); c.ticks(24);
    require(!ui.Icons_IsZoomedIn && !(apple->attr&ISLE_SP_HIDDEN),"original zoom-out process did not restore item icon");
    native(isle_player_ui_Icons_Hide(&ui));
    const auto film60 = c.draw("film-only-60");
    // Intentionally ONLY a canonical roll-state boundary fixture. This neither
    // snapshots a scene nor calls makePhoto nor claims fifty captured photos.
    ui.player->photo->gPhotoCount = 50;
    require(isle_player_ui_func_8035E52C_4FE93C(&ui) == 10 &&
        isle_player_ui_func_8035E508_4FE918(&ui) == 10,"real film updater did not consume diagnostic canonical 50/60 roll boundary");
    const auto red = c.draw("film-diagnostic-roll-boundary-red-10");
    auto* ones = c.services.view(ui.D_803B0A14_550E24);
    auto* tens = c.services.view(ui.D_803B0A18_550E28);
    require(ones && tens && ones->red == 255 && ones->green == 0 && ones->blue == 0 &&
        tens->red == 255 && !(tens->attr&ISLE_SP_HIDDEN) && changes(film60,red,{255,20,305,45}) > 5,
        "source film ten-remaining transition failed native digit visibility/color and actual GPU output");
    unchanged_outside(film60,red,{255,20,305,45});
    auto* film = ui.D_803B09D8_550DE8;
    require(c.services.send_command(film,-4),"real film -4 enqueue failed"); c.tick();
    IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(c.services.game,film,&metadata));
    require(metadata.flags&1,"default native -4 did not hide actual film owner");
    const auto hidden = c.draw("film-default-minus4-hidden");
    require(changes(red,hidden,{255,20,305,45}) > 5,"native film -4 had no actual framebuffer effect");
    require(c.services.send_command(film,-5),"real film -5 enqueue failed"); c.tick();
    native(isle_game_object_metadata(c.services.game,film,&metadata));
    require(!(metadata.flags&1),"default native -5 did not reveal actual film owner");
    const auto shown = c.draw("film-default-minus5-shown");
    require(changes(red,shown) == 0,"real film -5 failed exact framebuffer restoration");
    require(c.services.send_command(film,-4),"film hide before screen scenarios failed"); c.tick();
    c.evidence("original Icons native shrink/flute/zoom/photo-icon processes; film60 and diagnostic canonical-roll50 red10; Services -4/-5 native FIFO actual GPU hide/restore");
}

IsleScreenTransitionObject* last_worker(screen_transition::Owner& owner) {
    IsleScreenTransitionObject* result = nullptr;
    for (auto* record = owner.objects; record; record = record->next)
        if (!record->joined && record->callback) result = record;
    require(result,"actual source transition worker was not published"); return result;
}
void delayed_join(Consumer& c, IsleScreenTransitionObject& worker, bool delayed) {
    auto& owner = c.services.screen_transition();
    require(!isle_screen_transition_func_800A7460(&owner.source),"source busy flag remained set at terminal transition");
    if (delayed) {
        require(!worker.joined && !owner.retirement_ready(),"busy=false was incorrectly treated as joined delayed worker");
        for (unsigned n = 0; n < 2; ++n) {
            c.tick(); require(!worker.joined,"source delayed worker joined before its three genuine OM waits");
        }
        c.tick(); require(worker.joined,"source delayed worker did not join on its third genuine OM update");
    } else require(worker.joined,"nondelayed source worker did not actually join");
    require(owner.retirement_ready(),"native transition joins left an outstanding Owner borrower");
}
void screens(Consumer& c) {
    auto& owner = c.services.screen_transition(); auto& source = owner.source;
    native(isle_screen_transition_func_800A7F68(&source,0,1));
    c.ui.source().player->screen = &source;
    IsleGameObjectMetadata camera_object{};
    native(isle_game_object_metadata(c.services.game,source.camera_object,&camera_object));
    const IsleGameCameraParameters* camera = nullptr;
    native(isle_game_camera_parameters(c.services.game,source.D_800BE2C8,&camera));
    require(camera_object.id == 5 && source.camera_flags == 5 && source.camera_mask == 1 &&
        source.camera_buffer_mask == 1 && camera->perspective.fovy == 45 &&
        std::abs(camera->perspective.aspect-321.0f/241.0f) < 0.00001f &&
        camera->perspective.near_plane == 8 && camera->perspective.far_plane == 1024 &&
        camera->look_at.eye[0] == 290 && camera->look_at.eye[1] == 0 && camera->look_at.eye[2] == 0 &&
        camera->look_at.at[0] == 0 && camera->look_at.at[1] == 0 && camera->look_at.at[2] == 0 &&
        camera->look_at.up[0] == 0 && camera->look_at.up[1] == 0 && camera->look_at.up[2] == 1,
        "actual 7F68 source camera metadata/perspective/eye/up differs");
    const auto blank = c.draw("screen-source-camera-clear",true);
    // Reveal first, cover last: original cover naturally leaves alpha255 for
    // the subsequent original opaque shape passes. No screen-state patch.
    for (const bool reveal : {true,false}) {
        const std::string name = reveal ? "screen-solid-reveal" : "screen-solid-cover";
        require(c.services.begin_fade(reveal,0.2f,{208,40,88},true),c.services.error());
        auto& worker = *last_worker(owner);
        require(worker.callback == (reveal ? isle_screen_transition_func_800A7760 : isle_screen_transition_func_800A7660),
            "Services fade did not create the actual direction-specific C worker");
        const auto start = c.draw(name+"-start");
        c.ticks(6); const auto middle = c.draw(name+"-middle");
        require(source.D_800AF060 > 0 && source.D_800AF060 < 255 && middle.triangle_packets > 0,
            "actual source alpha process failed to reach an intermediate triangle draw");
        for (unsigned n = 0; isle_screen_transition_func_800A7460(&source); ++n) {
            require(n < 32,"source solid transition never cleared busy"); c.tick();
        }
        const auto terminal = c.draw(name+"-busy-false-worker-live");
        require(changes(start,middle) > 10000 && changes(middle,terminal) > 10000,
            "source solid alpha transition produced no meaningful actual color evolution");
        require(reveal ? distance(terminal,blank) < distance(middle,blank) && distance(middle,blank) < distance(start,blank) :
                         distance(start,blank) < distance(middle,blank) && distance(middle,blank) < distance(terminal,blank),
            "actual native alpha output moved in the wrong reveal/cover direction");
        delayed_join(c,worker,true);
        const auto joined = c.draw(name+"-joined",true);
        require(changes(joined,blank) == 0,"joined original solid retained an actual framebuffer overlay");
    }
    std::array<Capture,2> middles;
    for (unsigned kind = 0; kind < 2; ++kind) {
        const bool delayed = kind != 0;
        const std::string name = kind ? "screen-shape-diamond-offset-negative-delayed" : "screen-shape-round-offset-positive";
        native(isle_screen_transition_func_800A7470(&source,208,40,88));
        native(delayed ? isle_screen_transition_func_800A7F40(&source,-56,28,1,0.4f) :
                         isle_screen_transition_func_800A7CC4(&source,56,-28,0,0.4f));
        auto& worker = *last_worker(owner);
        require(worker.callback == isle_screen_transition_func_800A7BAC && worker.render_kind == 1 &&
            worker.display_list == (kind ? 0x800af020u : 0x800aeee0u),"shape API failed to bind its real source list/worker");
        IsleGameModelTransform pose{}; native(isle_screen_transition_transform(&worker,&pose,0));
        require(pose.position.x == 1 && pose.position.y == (kind ? -56 : 56) &&
            pose.position.z == (kind ? 28 : -28),"source shape offset/direction did not reach canonical native model");
        c.draw(name+"-start"); c.ticks(12); middles[kind] = c.draw(name+"-middle");
        require(middles[kind].triangle_packets > 0 && changes(blank,middles[kind]) > 100 &&
                middles[kind].depth != blank.depth,"actual source aperture produced no native color/depth geometry");
        for (unsigned n = 0; isle_screen_transition_func_800A7460(&source); ++n) {
            require(n < 40,"source shape worker never cleared busy"); c.tick();
        }
        if (delayed) {
            require(!worker.joined && source.D_800BE2D0,"delayed shape lost its real paired opaque model before join");
            c.draw(name+"-busy-false-worker-live");
        }
        delayed_join(c,worker,delayed);
        require(source.D_800BE2D0 == nullptr,"actual shape join retained its paired opaque model");
        const auto joined = c.draw(name+"-joined",true);
        require(changes(joined,blank) == 0,"joined source shape retained real color geometry");
    }
    require(changes(middles[0],middles[1]) > 100 && middles[0].depth != middles[1].depth,
        "round/diamond opposite offsets collapsed to the same actual color/depth output");
    c.evidence("source7F68 ID5 camera; solid reveal/cover alpha and round/diamond signed offsets; 7F40 delayed shape; busyfalse distinct from3OM-wait joined retirement; exact Owner held through completed GPU+readback");
}
void source_controls(Consumer& c, IsleGameObject* film) {
    auto& source = *c.ui.source().player;
    IsleGameSprite* sprite = nullptr; IsleSprite* view = nullptr;
    native(isle_game_object_sprite(c.services.game,film,&sprite));
    native(isle_game_sprite_view(c.services.game,sprite,&view));
    const auto attributes = view->attr;
    const std::array color{view->red,view->green,view->blue,view->alpha};
    const auto timeout = source.ThrowItemTimeout;
    IsleGameObjectMetadata metadata{};
    native(isle_game_object_metadata(c.services.game,film,&metadata));
    native(isle_game_object_set_flags(c.services.game,film,metadata.flags | 1u));
    isle_sprite_set_attribute(view,ISLE_SP_HIDDEN);
    const auto hidden = c.draw();
    SourceDispatch late{c.services,source,film};
    const auto shown = c.draw({},false,&late,true);
    require(late.early_film_absent && late.late_film_present && late.cleanups == 2 && !late.active &&
        source.ThrowItemTimeout == 42 && !source.nonwaiting_call_frame &&
        changes(hidden,shown) > 5 && hidden.depth == shown.depth,
        "late C source visibility was gathered early, crossed a C++ frame, or changed UI depth");
    SourceDispatch black{c.services,source,film}; black.mutate = false; black.black = true;
    const auto controlled = c.draw({},false,&black);
    require(black.cleanups == 2 && !black.active && changes(shown,controlled) > 50000,
        "source-owned full-screen fill was overwritten by generic camera emission");

    auto created = g::Frame::create(c.audio.execution.snapshot,{true,{12000000,4*1024*1024}});
    require(bool(created),created.message.data());
    Consumer rejected(c.services,c.ui,*created.frame,c.transitions,c.audio,c.root,c.report,c.sprite_camera,c.immutable);
    SourceDispatch failure{c.services,source,film}; failure.fail = true;
    bool failed = false;
    try { static_cast<void>(rejected.draw({},false,&failure)); }
    catch (const std::runtime_error&) { failed = true; }
    require(failed && failure.result == ISLE_GAME_INVALID_ARGUMENT &&
        source.status == failure.result && source.ThrowItemTimeout == 41 &&
        failure.cleanups == 1 && !failure.active && !source.nonwaiting_call_frame &&
        created.frame->error() == g::Error::preparation && created.frame->quiescent() &&
        !created.frame->submission() && !created.frame->completed() &&
        isle_game_status(c.services.game) == ISLE_GAME_OK && created.frame->retire(),
        "nested C failure escaped C++ sink cleanup, submitted failed work, or erased its first error");
    // Explicit component recovery from local C-fence status, not Game fault
    // recovery or production rollback. Native first-fault state was never set.
    source.status = ISLE_GAME_OK; source.ThrowItemTimeout = timeout;
    view->attr = attributes;
    isle_sprite_color(view,color[0],color[1],color[2],color[3]);
    native(isle_game_object_set_flags(c.services.game,film,metadata.flags));
    c.evidence("source control before camera; real C mutation and dispatch-time UI re-gather; actual black GPU fill without generic overwrite; nested C failure preserves first status and destroys C++ sink scope before exact failed-Frame retirement");
}
void delete_film_and_retire(Consumer& c) {
    auto& ui = c.ui.source(); auto* film = ui.D_803B09D8_550DE8;
    require(film,"film canonical owner disappeared before real -1");
    require(c.services.send_command(film,-5),"film show before deletion failed"); c.tick();
    const auto before = c.services.sprite_count();
    const auto shown = c.draw("film-before-default-minus1");
    source_controls(c,film);
    require(c.services.send_command(film,-1),"real film -1 enqueue failed"); c.tick();
    require(!ui.D_803B09D8_550DE8 && !ui.D_803B0A14_550E24 && !ui.D_803B0A18_550E28 && !ui.D_803B0A1C_550E2C &&
        c.services.sprite_count()+4 == before,"real builtin -1 failed joined canonical film/digit ownership retirement");
    const auto erased = c.draw("film-after-default-minus1");
    require(changes(shown,erased,{255,20,305,45}) > 5,"real builtin film deletion had no actual rendered effect");
    require(c.frame.retire() && c.frame.quiescent(),"actual Frame did not retire before native Ui owner");
    const auto retired = c.ui.retire();
    if (retired != ISLE_GAME_OK) {
        c.report << "FAILED\treal builtin film -1 left Ui native ownership unretirable\tresult=" << int(retired) << '\n';
        c.report.flush(); std::cerr << "native ownership defect: Ui::retire failed after real builtin film -1\n";
    }
    native(retired); // Never suppress stale ownership with fake/no-op cleanup.
    require(!ui.Msg_CameraObject && !ui.Msg_Camera && !ui.Icons_MainObject,
        "Ui::retire failed real joined message/icon owner cleanup");
    c.evidence("real Services default -1 native OM deletion; canonical film and digit pointers NULL; actual GPU/readback retired before Ui joined retirement");
}

void terminal_scene_objects(Consumer& c) {
    struct Publication {
        scene::Services* services;
        bool entered = false, rejected_inside_update = false;
        static void update(IsleGameObject*, void* user) {
            auto& self = *static_cast<Publication*>(user);
            self.entered = true;
            self.rejected_inside_update = !self.services->retire_objects();
        }
    } publication{&c.services};
    auto* object = c.services.object(0x7ffffffdu,9,1);
    require(object && c.services.process(object,Publication::update,&publication,
                ISLE_GAME_SYNCHRONOUS,1),
            "terminal scene publication/process creation failed");
    auto& owner = c.services.screen_transition();
    native(isle_screen_transition_func_800A7918(&owner.source,0,0.1f));
    auto* worker = last_worker(owner);
    require(worker && worker->callback == isle_screen_transition_func_800A7660,
            "terminal scene regression lost the actual Original black-fade worker");
    for (unsigned updates = 0; !publication.entered ||
            isle_screen_transition_func_800A7460(&owner.source); ++updates) {
        require(updates < 32,"terminal scene Source fade did not finish");
        c.tick();
    }
    require(publication.rejected_inside_update && !worker->joined &&
                !owner.retirement_ready(),
            "native update retired Services or Source busy=false forged a worker join");
    require(c.services.retire_objects(),
            "terminal Services sweep did not genuinely join live scene publications");
    IsleGameCounts counts{};
    native(isle_game_counts(c.services.game,&counts));
    require(!counts.objects && !counts.processes && !counts.models &&
                !counts.sprites && !counts.cameras && worker->joined &&
                owner.retirement_ready(),
            "terminal Services join left native callbacks/model or unjoined Source fade");
    scene::Services::ObjectPreparation prepared;
    require(!c.services.object(0x7ffffffeu,9,1) &&
                c.services.prepare_object(prepared) == ISLE_GAME_INVALID_STATE &&
                !c.services.failed() && isle_game_status(c.services.game) == ISLE_GAME_OK,
            "terminal Services reopened native admission or invented a failure");
    c.evidence("terminal Services joins a live native publication and Original delayed black-fade worker after GPU retirement; rejects sweep inside OM and closes new admission without destroying callback owners");
}

void run(const std::filesystem::path& romfile, const std::filesystem::path& root) {
    require(root.is_absolute() && std::filesystem::is_directory(root) && !std::filesystem::is_symlink(root) &&
        std::filesystem::is_empty(root),"output must be an existing absolute empty nonsymlink directory; never reuse a failed root");
    require(std::filesystem::create_directory(root/"profile"),"private actual GameSession profile directory creation failed");
    std::ofstream report(root/"semantic-report.tsv"); require(bool(report),"private semantic report creation failed");
    report << "scope\tprivate real UI/Screen component GPU consumer; NOT PlayerFSM/photo capture/physical input/Original/full game/Windows11\n"
           << "input\texplicit canonical six-scalar pre-OM boundary; one native tick timeline; no SDL hardware\n"
           << "film-fixture\tcanonical application roll gPhotoCount=50 only; NOT fifty captured photographs\n";
    const auto snapshot = rom::read_snapshot(romfile);
    const player::UiResources ui_resources(snapshot);
    const player::PhotoResources photo_resources(snapshot);
    const photo::LightingInputs lighting(snapshot);
    screen_transition::Resources transitions(snapshot);
    sp::Producer sprites(ui_resources.roots(),ui_resources.banks());
    auto hydrated = game_audio::hydrate(snapshot); require(bool(hydrated),"actual ROM audio resource hydration failed");
    std::array<audio::effects::Section,8> sections; audio::effects::Config effects;
    audio_ok(game_audio::native_startup_effects(hydrated.resources,sections,effects));
    private_audio::Capture pcm;
    pcm.samples.resize(2048*audio::AudioService::max_frames*2);
    private_audio::Execution execution{snapshot,nullptr,{}};
    auto audio = game_audio::Session::create(std::move(hydrated.resources),effects,pcm.output(),execution.executor());
    require(bool(audio),"real AudioCapture Session creation failed");
    AudioClock audio_clock{*audio.session,execution};
    game::State state(root/"profile"); require(state.initialize(1),state.error());
    IslePlayerPhotoSource roll{};
    require(state.save(),"actual initialized GameSession missing"); native(photo_resources.bind(roll,*state.save()));
    isle_player_photo_func_8009CE00(&roll); // Application reset ONCE, not every case/frame/course.
    game_input::Service input;
    scene::Storage storage(16*1024*1024); native(storage.create());
    {
        SourceLifetime player_source(storage.game(),state.rng);
        player_source.value.photo = &roll;
        scene::Services services(storage.game(),storage.arena(),{{8,16,32,3,4},ui_resources.resources()},input,*audio.session,state);
        auto* root_object = services.object(3,0); require(root_object,"canonical Player3 root allocation failed");
        native(isle_player_source_bind(services.game,root_object,&player_source.value));
        auto* ordinary_camera_object = services.object(1,0); require(ordinary_camera_object,"native sprite camera allocation failed");
        IsleGameCamera* ordinary_camera = nullptr;
        native(isle_game_attach_camera(services.game,ordinary_camera_object,&ordinary_camera));
        player::Ui ui(ui_resources,services,player_source.value,sprites,0);
        player_source.value.ui = &ui.source();
        require(isle_player_ui_Msg_InitCamera(&ui.source()) && ui.source().Msg_Camera,"actual original Msg camera initialization failed");
        std::vector<sp::AssetSpan> immutable(sprites.assets().begin(),sprites.assets().end());
        for (const auto& span : lighting.installation_spans()) immutable.push_back({span.guest_address&0x1fffffffu,
            {reinterpret_cast<const std::uint8_t*>(span.bytes.data()),span.bytes.size()}});
        const auto transition_assets = transitions.immutable();
        immutable.insert(immutable.end(),transition_assets.begin(),transition_assets.end());
        // Frame is declared AFTER Ui: every actual graphics worker/mapping dies
        // before the Ui/Services/native context even on an exception.
        auto created = g::Frame::create(snapshot,{true,{12000000,4*1024*1024}});
        require(bool(created),created.message.data());
        Consumer consumer(services,ui,*created.frame,transitions,audio_clock,root,report,ordinary_camera,immutable);
        messages(consumer); icons_and_film(consumer);
        auto& ui_source = ui.source();
        native(isle_player_ui_Icons_Show(&ui_source));
        require(services.send_command(ui_source.D_803B09D8_550DE8,-5),
                "film show before shared-arena regression enqueue failed");
        consumer.tick();
        const auto unshared = consumer.draw();
        std::uint32_t exact_shared_capacity = 0;
        for (unsigned repeat = 0; repeat < 3; ++repeat) {
            const auto shared = consumer.draw({},false,nullptr,true);
            require(shared.rgba == unshared.rgba && shared.color == unshared.color &&
                    shared.depth == unshared.depth &&
                    shared.rectangle_packets == unshared.rectangle_packets,
                    "interleaved shared sprite lists lost ordered contents or failed per-frame cursor reset");
            exact_shared_capacity = 0;
            for (const auto bytes : shared.shared_list_bytes) exact_shared_capacity += bytes;
        }
        consumer.evidence("shared sprite arena: ordered A/B/A native inventory, actual empty draw, skipped control, repeated preparations across both framebuffer contexts match disjoint GPU color/depth/rectangles");
        const auto exact = consumer.draw({},false,nullptr,true,exact_shared_capacity);
        require(exact.rgba == unshared.rgba && exact.color == unshared.color &&
                exact.depth == unshared.depth,"exact-fit shared arena changed GPU output");
        // Failed Frames are sticky; isolate each failure in an actual Frame
        // with the same native consumers rather than retrying a failed one.
        for (const unsigned overlap : {0u,1u,2u,3u}) {
            auto isolated = g::Frame::create(snapshot,{true,{12000000,4*1024*1024}});
            require(bool(isolated),isolated.message.data());
            Consumer rejected(services,ui,*isolated.frame,transitions,audio_clock,root,report,
                              ordinary_camera,immutable);
            rejected.draw({},false,nullptr,true,
                          overlap ? ordinary_commands.size / 2 : exact_shared_capacity - 8,
                          true,overlap);
        }
        native(isle_player_ui_Icons_Hide(&ui_source));
        require(services.send_command(ui_source.D_803B09D8_550DE8,-4),
                "film hide after shared-arena regression enqueue failed");
        consumer.tick();
        screens(consumer); delete_film_and_retire(consumer);
        require(services.screen_transition().retirement_ready(),"source screen workers/Owner did not actually retire");
        terminal_scene_objects(consumer);
        report << "completed_frames\t" << consumer.frames << '\n';
    } // Services joins all remaining camera/root/native callbacks, THEN deactivate.
    native(storage.destroy());
    audio_ok(audio.session->stop());
    for (unsigned n = 0; n < 240 && audio.session->state() != game_audio::Session::State::draining; ++n) audio_clock.tick();
    require(audio_clock.tasks && pcm.used && audio.session->service().voices_retired() &&
        audio.session->state() == game_audio::Session::State::draining,"real audio RSP/owned PCM did not retire source voices");
    audio_ok(audio.session->poll_drain()); audio_ok(audio.session->poll_drain());
    require(audio.session->state() == game_audio::Session::State::drained && pcm.finishes == 1 && pcm.polls == 2,
        "actual retained PCM output did not acknowledge real drain");
    std::vector<std::byte> pcm_bytes(pcm.used*2);
    for (std::size_t n = 0; n < pcm.used; ++n) {
        const auto value = static_cast<std::uint16_t>(pcm.samples[n]);
        pcm_bytes[n*2] = std::byte(value&255); pcm_bytes[n*2+1] = std::byte(value>>8);
    }
    write_bytes(root/"owned-source-audio-stereo-s16le.pcm",pcm_bytes);
    report << "passed\tactual source audio RSP tasks=" << audio_clock.tasks << " stereo_frames=" << pcm.used/2
           << " rate=" << game_audio::source_rate << "; retained PCM+joined drain; audible UI sounds not claimed\n";
    report.flush(); require(bool(report),"semantic report persistence failed");
    std::cout << "actual UI CMsg/Icons/film native FIFO and SourceScreen camera/alpha/shape CPU RSP/RDP color+depth/Owner/readback retirement passed; "
        "component boundary only, no PlayerFSM/photo capture/Original/fullgame/Windows11 claim\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 3,"usage: player-ui-screen-rsp-smoke PRIVATE_ROM ABSOLUTE_EMPTY_OUTPUT_DIRECTORY");
        run(argv[1],argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "player-ui-screen-rsp-smoke: " << error.what() << '\n'; return EXIT_FAILURE;
    }
}
