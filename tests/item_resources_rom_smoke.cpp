#include "item_resources.hpp"
#include "beach_world_resources.hpp"
#include "menu_animation.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

// Opt-in actual admitted assets and real shared animator only. No source item
// factory/physics, native OM callback, GPU, input or audio execution is claimed.
namespace i=isle::items;
namespace m=isle::menu;
namespace a=isle::menu::animation;
namespace {
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void accepted(a::Status status,const char* operation) {
    if (!status) {
        std::cerr<<operation<<" error="<<static_cast<int>(status.error)<<" guest=0x"<<std::hex<<status.address<<std::dec<<'\n';
        throw std::runtime_error(operation);
    }
}
template<class F> void rejects(F&& action,const char* message) {
    bool rejected=false;try { action(); } catch (const std::exception&) { rejected=true; }
    require(rejected,message);
}
bool same(m::Vec3 x,m::Vec3 y) { return x.x==y.x && x.y==y.y && x.z==y.z; }
void animate(const i::Resources& resources) {
    const auto models=resources.models();
    std::array<a::SlotCapacity,4> capacities{};
    for (const auto& definition:resources.definitions())
        capacities[definition.model]=a::slot_capacity(models[definition.model]);
    // At most28 source commands on a finite splash's immediate update; only
    // positive-duration self-cycles on ball/apple. Source callbacks are NULL.
    auto made=a::Runtime::create({models,resources.materials(),resources.animations(),m::no_index},
                                capacities,{},{64,0});
    require(bool(made),"All four actual item families failed shared animator compilation");
    auto& runtime=*made.runtime;
    std::array<a::InstanceHandle,4> instances{};
    for (const auto& definition:resources.definitions()) {
        const auto acquired=runtime.acquire(definition.model,definition.model,false);
        require(bool(acquired),"Actual item instance acquisition failed");
        instances[definition.model]=acquired.handle;
        auto root=runtime.model(acquired.handle).nodes[0].transform;
        root.scale=definition.root_scale; // Genuine source assignment, not multiplication.
        accepted(runtime.set_node_transform(acquired.handle,0,root),"item root scale assignment");
        accepted(runtime.set_texture_speed(acquired.handle,definition.texture_speed),"item source texture speed");
    }
    std::array<std::uint32_t,2> image_domains{};
    std::array<float,2> terminal_time{};
    // Fixed diagnostic envelope, not advance-until-passing. Preserve source
    // texture0.4/model1 speeds and splash0.9 speeds across every update.
    for (unsigned frame=0;frame<256;++frame) {
        for (const auto& definition:resources.definitions()) {
            const auto handle=instances[definition.model];
            accepted(runtime.step_model(handle),"item source animation step");
            const auto state=runtime.model(handle);
            require(bool(state),"Active source item state became stale");
            require(runtime.events().empty(),"Item source NULL fnAnimCallback acquired Pokemon/FX callbacks");
            require(same(state.nodes[0].transform.scale,definition.root_scale),
                    "Source item root scale assignment was normalized or overwritten");
            if (definition.family==i::Family::pester_ball || definition.family==i::Family::apple) {
                const auto& material=state.materials[0];
                const auto& source=resources.materials()[material.resource];
                require(material.image_index<source.images.size() && material.image_index<32,
                        "Actual item texture animation escaped admitted image domain");
                require(state.nodes[0].clock.speed==1.0f && material.clock.speed==0.4f,
                        "Texture-only source speed setter changed model clock");
                image_domains[definition.family==i::Family::pester_ball ? 0 : 1]|=1u<<material.image_index;
            } else {
                // Own Items_UpdateFume276-288 consumes object animationTime.
                // At source speed0.9, tick66 first satisfies60-time<0.9.
                // FINISHED is deliberately transient inside the same update;
                // sys/anim.c662-664 disables it after applying final values.
                require((60.0f-state.animation_time<0.9f)==(frame>=66),
                        "Actual source splash fume cutoff changed");
                const auto index=definition.family==i::Family::water_splash ? 0u : 1u;
                if (frame==68) terminal_time[index]=state.animation_time;
                if (frame==255)
                    require(state.animation_time==terminal_time[index]&&state.animation_time>=60.0f&&state.animation_time<60.9f,
                            "Finite splash END failed to freeze source object animation time");
            }
        }
    }
    require(image_domains[0]==0xff && image_domains[1]==0x3f,
            "Source ball/apple STEP cycles did not visit their complete8/6 image domains");
    // Pure animator: no native objects, GPU borrows or attached effects exist.
    // This release is not offered as proof of any external/native/GPU fence.
    for (const auto handle:instances) {
        accepted(runtime.begin_retire(handle),"item pure-animator retirement");
        accepted(runtime.release_retired(handle),"item pure-animator release");
    }
}
void run(const char* filename) {
    auto snapshot=isle::rom::read_snapshot(filename);
    const auto world=isle::beach::world_descriptors(snapshot).world;
    i::Resources resources(snapshot,world.fog_rgb,world.fog_min,world.fog_max);
    animate(resources);
    const auto* original=resources.models()[resources.definition(i::Family::water_splash).model].tree.bytes.data();
    rejects([&] { static_cast<void>(resources.definition(static_cast<i::Family>(4))); },"Foreign item family admitted");
    auto moved=std::move(snapshot);
    rejects([&] { static_cast<void>(resources.models()); },"Moved-from item Snapshot access admitted");
    rejects([&] { i::Resources invalid(snapshot,world.fog_rgb,world.fog_min,world.fog_max); },"Moved-from item construction admitted");
    i::Resources rebound(moved,world.fog_rgb,world.fog_min,world.fog_max);
    require(rebound.models()[rebound.definition(i::Family::water_splash).model].tree.bytes.data()==original,
            "Rebound item resource copied or detached original asset storage");
    std::cout<<"Actual item assets: models="<<rebound.models().size()<<" materials="<<rebound.materials().size()
             <<" programs="<<rebound.animations().size()<<" installation_spans="<<rebound.installation_spans().size()<<'\n';
}
}
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: item-resources-rom-smoke <admitted-original.z64>\n";return EXIT_FAILURE; }
    try { run(argv[1]);std::cout<<"Actual four-family item image cycles, finite END, NULL callbacks, split speeds and borrowed lifetime passed\n";return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return EXIT_FAILURE; }
}
