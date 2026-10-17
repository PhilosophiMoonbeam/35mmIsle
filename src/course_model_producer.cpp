#include "course_model_producer.hpp"
#include "graphics_task.hpp"
#include <algorithm>
#include <stdexcept>

namespace isle::course {
namespace {
constexpr std::size_t lane_bytes = 5*8, required_bytes = 4*lane_bytes;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool overlaps(menu::RamRange a, menu::RamRange b) {
    return a.size && b.size && (a.offset <= b.offset ? b.offset-a.offset < a.size : a.offset-b.offset < b.size);
}
bool same(scene::RenderMetadata a, scene::RenderMetadata b) {
    return a.link == b.link && a.priority == b.priority && a.camera_tag == b.camera_tag;
}
}
ModelProducer::ModelProducer(world::Producer& world, actor::Producer& actors, menu::RamRange lists,
    const ItemComposition* items)
    : world_(world),actors_(actors),lists_(lists),services_(items ? &items->services : nullptr),
      items_(items ? &items->producer : nullptr) {
    require(!(lists.offset&7u) && lists.offset >= graphics::bss_end &&
        lists.offset <= menu::rdram_size && lists.size >= required_bytes &&
        lists.size <= menu::rdram_size-lists.offset,"course model join has invalid explicit command storage");
    for (auto range : world.reservations()) require(!overlaps(lists,range),"course join overlaps World producer");
    for (auto range : actors.reservations()) require(!overlaps(lists,range),"course join overlaps Actor producer");
    if (items) {
        require(items->render_order_capacity,"course Item composition requires explicit source order scratch capacity");
        for (auto range : items_->reservations()) require(!overlaps(lists,range),"course join overlaps Item producer");
        source_order_.resize(items->render_order_capacity);
        commands_.resize(lists.size/8);
    }
}
scene::render::Prepared ModelProducer::prepare(std::span<std::uint32_t> words, std::uint64_t epoch,
    const scene::render::Pass& pass) {
    require(words.data() && words.size_bytes() == menu::rdram_size,"course join requires complete canonical RAM");
    const auto ram = reinterpret_cast<std::uintptr_t>(words.data());
    require(ram <= UINTPTR_MAX-words.size_bytes(),"course join RAM pointer extent invalid");
    const auto end = ram+words.size_bytes();
    const auto disjoint = [&](const void* pointer, std::size_t bytes) {
        if (!bytes) return;
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        require(pointer && at <= UINTPTR_MAX-bytes && (end <= at || at+bytes <= ram),
            "course join aliases its canonical RAM");
    };
    disjoint(this,sizeof(*this)); disjoint(&pass,sizeof(pass));
    disjoint(source_order_.data(),source_order_.size()*sizeof(scene::RenderOrderEntry));
    disjoint(commands_.data(),commands_.size()*sizeof(menu::model::Command));
    std::size_t source_count = 0;
    if (services_) {
        disjoint(services_,sizeof(*services_));
        require(!services_->failed() && services_->render_order(3,source_order_,source_count),
            "course global DL3 projection exceeds explicit capacity or scene failed");
    }
    // Each producer performs complete native/RAM preflight. A downstream failure
    // invalidates this scene lease, not a pretend cross-producer rollback. The
    // wrapper itself publishes only after all model/list/order checks succeed.
    const auto world = world_.prepare(words,epoch,pass);
    const auto items = items_ ? items_->prepare(words,epoch,pass) : ::isle::items::ProducerFrame{};
    const auto actors = actors_.prepare(words,epoch,pass);
    graphics::Memory memory(words);
    scene::render::Prepared result;
    if (!items_) {
        // Preserve the exact no-Item World+Actor/PhotoScene command stream.
        for (std::size_t lane = 0; lane < 4; ++lane) {
            const bool have_world = world.lanes[lane].commands > 1;
            const bool have_actors = actors.lanes[lane].commands > 1;
            if (lane && !have_world && !have_actors) continue;
            const auto begin = static_cast<std::uint32_t>(lists_.offset+lane*lane_bytes);
            auto cursor = begin;
            const auto command = [&](std::uint32_t first, std::uint32_t second) {
                memory.word(cursor,first); memory.word(cursor+4,second); cursor += 8;
            };
            if (have_world) {
                command(0xde000000,world.camera.address);
                command(0xde000000,world.lanes[lane].address);
            }
            if (have_actors) {
                command(0xde000000,actors.camera.address);
                command(0xde000000,actors.lanes[lane].address);
            }
            command(0xdf000000,0);
            result.lanes[lane] = {begin,(cursor-begin)/4};
        }
        return result;
    }
    const auto order = std::span(source_order_).first(source_count);
    const auto validate_objects = [&](const auto& objects) {
        for (std::size_t i = 0; i < objects.size(); ++i) {
            const auto& object = objects[i];
            require(object.object && object.render.link == 3,"course model escaped actual shared source DL3");
            for (std::size_t j = 0; j < i; ++j)
                require(objects[j].object != object.object,"course producer repeated an actual object identity");
            std::size_t matches = 0;
            for (const auto& entry : order) if (entry.object == object.object) {
                require(same(entry.render,object.render),"course model metadata differs from actual Services order");
                ++matches;
            }
            require(matches == 1,"course model missing from actual Services global DL3 list");
            for (const auto range : object.lanes) {
                require(!(range.address&7u) && range.address <= menu::rdram_size &&
                    range.commands <= (menu::rdram_size-range.address)/8,
                    "course raw callback range escapes canonical RAM");
                require(!overlaps(lists_,{range.address,std::size_t(range.commands)*8}),
                    "course mixed output overlaps callback input");
            }
        }
    };
    disjoint(world.objects.data(),world.objects.size_bytes());
    disjoint(items.objects.data(),items.objects.size_bytes());
    validate_objects(world.objects); validate_objects(items.objects);
    for (const auto& w : world.objects) for (const auto& i : items.objects)
        require(w.object != i.object,"course object belongs to both World and Item producers");
    std::size_t cursor = 0;
    const auto command = [&](std::uint32_t a, std::uint32_t b) {
        require(cursor < commands_.size(),"course mixed callback commands exceed explicit list reservation");
        commands_[cursor++] = {a,b};
    };
    for (unsigned lane = 0; lane < 4; ++lane) {
        const auto begin = cursor;
        bool have_models = false;
        // Project the same global list afresh for every frame. Unknown owners
        // belong to their own scene producers; never reorder the borrowed subset.
        for (const auto& entry : order) {
            const auto append = [&](const auto& objects) {
                for (const auto& object : objects) if (object.object == entry.object) {
                    const auto range = object.lanes[lane];
                    if (!range.commands) return;
                    if (!have_models) { command(0xde000000,world.camera.address); have_models = true; }
                    for (std::size_t i = 0; i < range.commands; ++i) {
                        const auto at = range.address/4+i*2;
                        command(words[at],words[at+1]);
                    }
                    return;
                }
            };
            append(world.objects); append(items.objects);
        }
        const bool have_actors = actors.lanes[lane].commands > 1;
        if (lane && !have_models && !have_actors) continue;
        if (have_actors) {
            // Preserve the actual Actor prefix/capture history and reset the
            // projection/modelview state after World direct sky rendering.
            command(0xde000000,actors.camera.address);
            command(0xde000000,actors.lanes[lane].address);
        }
        command(0xdf000000,0);
        result.lanes[lane] = {static_cast<std::uint32_t>(lists_.offset+begin*8),(cursor-begin)*2};
    }
    for (std::size_t i = 0; i < cursor; ++i) {
        const auto at = static_cast<std::uint32_t>(lists_.offset+i*8);
        if (words[at/4] != commands_[i].w0) memory.word(at,commands_[i].w0);
        if (words[at/4+1] != commands_[i].w1) memory.word(at+4,commands_[i].w1);
    }
    return result;
}
bool ModelProducer::prepare_pass(void* context, std::span<std::uint32_t> words, std::uint64_t epoch,
    const scene::render::Pass& pass, scene::render::Prepared& output) {
    if (!context) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
    const auto out = reinterpret_cast<std::uintptr_t>(&output);
    if (begin > UINTPTR_MAX-words.size_bytes() || out > UINTPTR_MAX-sizeof(output) ||
        !(begin+words.size_bytes() <= out || out+sizeof(output) <= begin)) return false;
    try { output = static_cast<ModelProducer*>(context)->prepare(words,epoch,pass); return true; }
    catch (...) { return false; }
}
} // namespace isle::course
