#include "course_model_producer.hpp"
#include "graphics_task.hpp"
#include <stdexcept>

namespace isle::course {
namespace {
constexpr std::size_t lane_bytes=5*8,required_bytes=4*lane_bytes;
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
bool overlaps(menu::RamRange a,menu::RamRange b) {
    return a.offset<=b.offset?b.offset-a.offset<a.size:a.offset-b.offset<b.size;
}
}
ModelProducer::ModelProducer(world::Producer& world,actor::Producer& actors,menu::RamRange lists)
    : world_(world),actors_(actors),lists_(lists) {
    require(!(lists.offset&7u)&&lists.offset>=graphics::bss_end&&lists.offset<=menu::rdram_size&&
        lists.size>=required_bytes&&lists.size<=menu::rdram_size-lists.offset,
        "course model join has invalid explicit command storage");
    for (auto range:world.reservations()) require(!overlaps(lists,range),"course join overlaps World producer");
    for (auto range:actors.reservations()) require(!overlaps(lists,range),"course join overlaps actor producer");
}
scene::render::Prepared ModelProducer::prepare(std::span<std::uint32_t> words,std::uint64_t epoch,
    const scene::render::Pass& pass) {
    require(words.size()==menu::rdram_size/4,"course join requires complete canonical RAM");
    const auto ram=reinterpret_cast<std::uintptr_t>(words.data());
    const auto self=reinterpret_cast<std::uintptr_t>(this);
    require(ram<=self?self-ram>=words.size_bytes():ram-self>=sizeof(*this),
        "course join aliases its canonical RAM");
    // Both existing producers perform their own complete native/RAM preflight.
    // Do not write wrapper lists until both have completed successfully.
    const auto world=world_.prepare(words,epoch,pass);
    const auto actors=actors_.prepare(words,epoch,pass);
    graphics::Memory memory(words);scene::render::Prepared result;
    for (std::size_t lane=0;lane<4;++lane) {
        const bool have_world=world.lanes[lane].commands>1;
        const bool have_actors=actors.lanes[lane].commands>1;
        if (lane&&!have_world&&!have_actors) continue;
        const auto begin=static_cast<std::uint32_t>(lists_.offset+lane*lane_bytes);auto cursor=begin;
        const auto command=[&](std::uint32_t first,std::uint32_t second) {
            memory.word(cursor,first);memory.word(cursor+4,second);cursor+=8;
        };
        if (have_world) {
            command(0xde000000,world.camera.address);
            command(0xde000000,world.lanes[lane].address);
        }
        if (have_actors) {
            // Reset actual projection/modelview state after World direct sky.
            command(0xde000000,actors.camera.address);
            command(0xde000000,actors.lanes[lane].address);
        }
        command(0xdf000000,0);
        result.lanes[lane]={begin,(cursor-begin)/4};
    }
    return result;
}
bool ModelProducer::prepare_pass(void* context,std::span<std::uint32_t> words,std::uint64_t epoch,
    const scene::render::Pass& pass,scene::render::Prepared& output) {
    if (!context) return false;
    try { output=static_cast<ModelProducer*>(context)->prepare(words,epoch,pass);return true; }
    catch (...) { return false; }
}
} // namespace isle::course
