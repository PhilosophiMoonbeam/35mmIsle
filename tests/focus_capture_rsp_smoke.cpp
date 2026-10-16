// Private-ROM opt-in actual canonical RSP/RDP consumer. Never registered in CTest.
// Standalone copy evidence only: not actor capture, detector, photo snapshot or queue.
#include "focus_capture.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace isle;
void require(bool v,const char* message) { if (!v) throw std::runtime_error(message); }
constexpr std::uint32_t top=0x610000,stack=0x620000,yield=0x624000,task=0x626000,
    boot=0x627000,output=0x640000,output_end=0x680000;
constexpr std::array<std::uint32_t,2> framebuffer{0x300000,0x330000};
constexpr std::size_t plane_bytes=320*240*2;
constexpr focus::CaptureLayout layout{{0x600000,0x100},{0x600100,0x100},{0x600240,20*128}};
constexpr std::uint32_t sentinel=0x59c7a63d;
std::uint16_t pixel(unsigned x,unsigned y,unsigned seed) {
    // Independent original framebuffer positions, varied RGB and BOTH bit0 states.
    return static_cast<std::uint16_t>((x*173u+y*997u+seed*3251u) ^ (x<<9) ^ (y<<3));
}
enum class Postprocess { none, writeback, abandon };
void fill_framebuffers(std::span<std::uint32_t> words,unsigned seed) {
    graphics::Memory memory(words);
    for (auto fb:framebuffer)
        for (unsigned y=0;y<240;++y) for (unsigned x=0;x<320;++x)
            memory.half(fb+(y*320+x)*2,pixel(x,y,seed+(fb==framebuffer[1]?7u:0u)));
}
std::uint16_t read_half(std::span<const std::byte> bytes,std::uint32_t address) {
    return static_cast<std::uint16_t>((std::to_integer<unsigned>(bytes[address^3u])<<8)|
                                      std::to_integer<unsigned>(bytes[(address+1)^3u]));
}
void admission(const focus::RegionCopies& copies) {
    std::array<std::uint32_t,focus::RegionCopies::command_words+4> storage;
    storage.fill(sentinel); const auto before=storage;
    require(!copies.emit_region(storage,20,{0,0,320,240}),"invalid row accepted");
    require(storage==before,"invalid row wrote storage");
    require(!copies.emit_region(storage,0,{5,0,4,240}),"inverted scissor accepted");
    require(storage==before,"invalid scissor wrote storage");
    require(!copies.emit_region(storage,0,{0,0,1024,240}),"unrepresentable scissor accepted");
    require(storage==before,"large scissor wrote storage");
    require(!copies.emit_region(std::span(storage).first(focus::RegionCopies::command_words-1),0,{0,0,320,240}),"short suffix accepted");
    require(storage==before,"short suffix wrote storage");
    std::vector<std::uint32_t> short_ram((layout.regions.offset+layout.regions.size)/4-1,sentinel);
    require(copies.install(short_ram)==focus::CaptureStatus::insufficient_storage,"short RAM accepted");
    require(std::all_of(short_ram.begin(),short_ram.end(),[](auto w){return w==sentinel;}),"failed installation wrote RAM");
    for (unsigned kind=0;kind<4;++kind) {
        auto bad=layout;
        if (kind==0) bad.copy_list=bad.tile_list;
        if (kind==1) ++bad.regions.offset;
        if (kind==2) bad.regions.size=20*128-1;
        if (kind==3) bad.tile_list.offset=8*1024*1024;
        bool rejected=false;
        try { focus::RegionCopies invalid(bad); } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"invalid ownership layout accepted");
    }
}
void consume(rsp::CanonicalExecutor& executor,const rom::Snapshot& snapshot,
             const focus::RegionCopies& copies,bool cold,unsigned context,unsigned seed,
             Postprocess postprocess=Postprocess::none,bool initialize_sources=true) {
    require(executor.quiescent(),"GPU/CPU owner not retired before row overwrite");
    auto preparation=executor.begin_preparation(); auto words=preparation.words();
    if (cold) {
        graphics::validate_cold_inputs(words,snapshot,snapshot.canonical().data(),boot);
        graphics::install_cold_inputs(words,snapshot,boot);
        require(copies.install(words)==focus::CaptureStatus::ok,"static list installation failed");
    }
    graphics::Memory memory(words);
    // All twenty rows plus adjacent external guards; only three selected rows may change.
    constexpr auto region_begin=static_cast<std::uint32_t>(layout.regions.offset);
    for (std::uint32_t a=region_begin-64;a<region_begin+20*128+64;a+=4) memory.word(a,sentinel);
    if (initialize_sources) fill_framebuffers(words,seed);
    constexpr std::uint32_t outside=0x7f0000;
    for (unsigned i=0;i<64;++i) memory.word(outside+i*4,sentinel);
    // The full declared reservations (including unused list padding) stay CPU-owned.
    std::uint32_t cursor=top;
    auto command=[&](std::uint32_t a,std::uint32_t b) {
        memory.word(cursor,a); memory.word(cursor+4,b); cursor+=8;
    };
    command(0xdb060000,0); command(0xdb06003c,framebuffer[context]);
    // Explicit inherited modes; primitive itself sets all source COPY requirements.
    command(0xef000000,0);
    constexpr std::array<unsigned,3> rows{0,7,19};
    for (auto row:rows) {
        const auto result=copies.emit_region(words.subspan(cursor/4,focus::RegionCopies::command_words),row,{3,5,317,235});
        require(bool(result) && result.written_words==focus::RegionCopies::command_words,"SaveRegion emission failed");
        cursor+=static_cast<std::uint32_t>(result.written_words*4);
    }
    command(0xe9000000,0); command(0xdf000000,0);
    const auto prepared=graphics::finish_task(words,snapshot,boot,stack,yield,task,output,output_end,top);
    auto completed=executor.execute(std::move(preparation),prepared.task,prepared.boot);
    const auto ram=completed.rdram();
    for (unsigned row=0;row<20;++row) {
        const bool selected=std::find(rows.begin(),rows.end(),row)!=rows.end();
        for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x) {
            const auto address=copies.region_offset(row)+(y*8+x)*2;
            const auto expected=selected?pixel(156+x,116+y,seed+(context==1?7u:0u)):
                static_cast<std::uint16_t>((address&2u)?sentinel:sentinel>>16);
            require(read_half(ram,address)==expected,"actual GPU region differs from full16bit original center/untouched row");
        }
    }
    for (unsigned i=0;i<32;++i) {
        const auto expected=static_cast<std::uint16_t>((i&1u)?sentinel:sentinel>>16);
        require(read_half(ram,region_begin-64+i*2)==expected,"leading region guard changed");
        require(read_half(ram,region_begin+20*128+i*2)==expected,"trailing region guard changed");
    }
    for (auto fb:framebuffer) for (unsigned y=0;y<240;++y) for (unsigned x=0;x<320;++x)
        require(read_half(ram,fb+(y*320+x)*2)==pixel(x,y,seed+(fb==framebuffer[1]?7u:0u)),"copy changed source/other framebuffer");
    for (unsigned i=0;i<128;++i)
        require(read_half(ram,outside+i*2)==static_cast<std::uint16_t>((i&1u)?sentinel:sentinel>>16),"GPU changed outside caller guard");
    // All data comparison/CPU analysis finishes BEFORE release/next shared-row overwrite.
    if (postprocess!=Postprocess::none) {
        auto writable=completed.postprocess_words();
        fill_framebuffers(writable,seed+100);
        if (postprocess==Postprocess::writeback) {
            auto moved=std::move(completed);
            moved.finish_postprocess();
            completed=std::move(moved);
        }
    }
    completed.release(); require(executor.quiescent(),"completed GPU readback owner survived release");
    if (postprocess!=Postprocess::none) {
        bool rejected=false;
        try { static_cast<void>(completed.postprocess_words()); }
        catch (const std::runtime_error&) { rejected=true; }
        require(rejected,"released completed lease allowed a new CPU writer");
    }
    if (postprocess==Postprocess::abandon) {
        bool rejected=false;
        try { auto invalid=executor.begin_preparation(); }
        catch (const std::runtime_error&) { rejected=true; }
        require(rejected,"abandoned completed CPU writes did not poison RAM reuse");
    }
}
int run(const std::filesystem::path& path) {
    const focus::RegionCopies copies(layout); admission(copies);
    const auto snapshot=rom::read_snapshot(path);
    rsp::CanonicalExecutor executor(true,{16000000,4*1024*1024});
    consume(executor,snapshot,copies,true,0,1,Postprocess::writeback);
    consume(executor,snapshot,copies,false,0,101,Postprocess::none,false);
    consume(executor,snapshot,copies,false,1,2);
    consume(executor,snapshot,copies,false,0,3);
    consume(executor,snapshot,copies,true,1,4); // explicit reinstall after cold reset
    rsp::CanonicalExecutor fresh(true,{16000000,4*1024*1024});
    consume(fresh,snapshot,copies,true,0,5,Postprocess::abandon);
    std::cout<<"actual standalone center-copy full16bit/guards/both-contexts passed tasks=6 "
        "completed_CPU_writeback_move_retained_GPU_data_abandon_poison=passed\n";
    return 0;
}
}
int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::runtime_error("usage: focus-capture-rsp-smoke PRIVATE_ROM");
        return run(argv[1]);
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
