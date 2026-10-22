// Explicit private-ROM constructor admission regression; never CTest/default discovery.
#include "scene_graphics.hpp"
#include "menu_resources.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
void run(const char* filename) {
    auto snapshot=isle::rom::read_snapshot(filename);
    auto resources=isle::menu::hydrate(snapshot);
    require(bool(resources),"actual Catalog hydration failed");
    namespace graphics=isle::scene::graphics;
    std::array<graphics::Config,6> conflicts{};
    for(auto& config:conflicts) config.require_cpu=true;
    // Each candidate must be a legal independent core plan, but it aliases
    // actual startup producer, framebuffer or fixed source closure storage.
    conflicts[0].memory.commands.address=0x110000;
    conflicts[1].memory.workspace.address=0x110000;
    conflicts[2].memory.output.address=0x110000;
    conflicts[3].memory.depth=0x110000;
    conflicts[4].memory.commands={0xaec40,0x540}; // Actual fixed fade source, outside cold BSS.
    conflicts[5].memory.depth=0x3b5000; // Real startup framebuffer cannot also be depth.
    for(std::size_t index=0;index<conflicts.size();++index) {
        auto rejected=graphics::Frame::create(*resources.catalog,snapshot,conflicts[index]);
        if(rejected) {
            require(rejected.frame->retire()&&rejected.frame->quiescent(),"misadmitted layout failed actual executor retirement");
            std::cerr<<"accepted_conflicting_startup_core_kind="<<index<<'\n';
            throw std::runtime_error("startup core storage overlapped its real producer arena");
        }
        require(rejected.error==graphics::Error::invalid&&!rejected.frame,
                "startup overlap failed for an unrelated reason rather than invalid layout admission");
        auto independent=graphics::Frame::create(snapshot,conflicts[index]);
        require(bool(independent),independent.message.data());
        require(independent.frame->retire()&&independent.frame->quiescent(),
                "startup restriction rejected or damaged an independent generic core plan");
    }
    std::cout<<"startup_core_producer_overlap_rejection=passed generic_independent_core=preserved GPU_frames_Original_Windows11=not_claimed\n";
}
}
int main(int argc,char** argv) {
    try { require(argc==2,"usage: scene-layout-rom-smoke PRIVATE_ROM"); run(argv[1]); return EXIT_SUCCESS; }
    catch(const std::exception& error) { std::cerr<<"scene-layout-rom-smoke: "<<error.what()<<'\n'; return EXIT_FAILURE; }
}
