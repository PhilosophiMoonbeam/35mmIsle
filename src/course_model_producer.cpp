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
        cart_ = items->cart; prefix_object_ = items->prefix_object;
        global_source_order_ = items->global_source_order || cart_ || prefix_object_;
        // Four lanes, at most one camera call plus one unpushed branch per
        // Services entry, followed by one EndDL. No hot command-vector growth.
        if (global_source_order_)
            require(source_join_bytes(items->render_order_capacity) <= lists.size,
                "course source continuation storage is below cold worst-case plan");
        if (cart_) for (auto range : cart_->reservations())
            require(!overlaps(lists,range),"course join overlaps Cart producer");
        source_order_.resize(items->render_order_capacity);
        commands_.resize(lists.size/8);
        patches_.resize(lists.size/8);
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
    disjoint(patches_.data(),patches_.size()*sizeof(Patch));
    std::size_t source_count = 0;
    if (services_) {
        disjoint(services_,sizeof(*services_));
        require(!services_->failed(),"course source Services failed");
        for (std::uint8_t link = 3; link <= (global_source_order_ ? 5 : 3); ++link) {
            std::size_t written = 0;
            require(services_->render_order(link,std::span(source_order_).subspan(source_count),written),
                "course global projection exceeds explicit capacity");
            source_count += written;
        }
    }
    // Each producer performs complete native/RAM preflight. A downstream failure
    // invalidates this scene lease, not a pretend cross-producer rollback. The
    // wrapper itself publishes only after all model/list/order checks succeed.
    bool prefix_visible = true;
    if (prefix_object_) {
        IsleGameObjectMetadata metadata{};
        require(isle_game_object_metadata(services_->game,prefix_object_,&metadata)==ISLE_GAME_OK,
            "course actual PhotoScene prefix is not live");
        prefix_visible = !(metadata.flags&1u);
    }
    const auto world = world_.prepare(words,epoch,pass);
    const auto items = items_ ? items_->prepare(words,epoch,pass) : menu::model::PacketFrame{};
    const auto actors = actors_.prepare(words,epoch,pass,prefix_visible);
    const auto cart = cart_ ? cart_->prepare(words,epoch,pass) : player::cart::ProducerFrame{};
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
    const auto validate_camera = [&](const auto& frame) {
        if (!frame.revision) return;
        const auto range = frame.camera;
        require(range.commands && !(range.address&7u) && range.address <= menu::rdram_size &&
            range.commands <= (menu::rdram_size-range.address)/8,
            "course provider camera escapes RAM");
        bool owned = false;
        for (const auto reservation : frame.reservations)
            if (range.address >= reservation.offset &&
                range.address-reservation.offset <= reservation.size &&
                std::size_t(range.commands)*8 <= reservation.size-(range.address-reservation.offset))
                owned = true;
        require(owned && !overlaps(lists_,{range.address,std::size_t(range.commands)*8}),
            "course provider camera escapes reservation");
    };
    validate_camera(world); validate_camera(items); validate_camera(actors); validate_camera(cart);
    std::size_t binding_count = 0;
    const auto identity = [&](const auto& object) {
        if constexpr (requires { object.kind; })
            if (object.kind == actor::FragmentKind::detector_prefix) {
                require(!object.object && prefix_object_,"course prefix requires actual PhotoScene object");
                return prefix_object_;
            }
        require(object.object != nullptr,"course callback has no actual object");
        return object.object;
    };
    const auto validate = [&](const auto& objects, const auto& reservations) {
        disjoint(objects.data(),objects.size_bytes());
        for (const auto& object : objects) {
            const auto owner = identity(object);
            std::size_t matches = 0;
            for (const auto& entry : order) if (entry.object == owner) {
                require(same(entry.render,object.render),"course callback metadata differs from Services");
                ++matches;
            }
            require(matches == 1,"course callback absent from Services order");
            ++binding_count;
            for (unsigned lane = 0; lane < 4; ++lane) {
                const auto range = object.lanes[lane];
                const auto slot = object.continuation_slots[lane];
                require(!(range.address&7u) && range.address <= menu::rdram_size &&
                    range.commands <= (menu::rdram_size-range.address)/8,
                    "course callback range escapes RAM");
                if (!range.commands) { require(!slot,"empty callback owns continuation"); continue; }
                require(slot == range.address+range.commands*8 && slot <= menu::rdram_size-8,
                    "course continuation is not owned callback tail");
                bool owned = false;
                for (const auto reservation : reservations)
                    if (range.address >= reservation.offset &&
                        range.address-reservation.offset <= reservation.size &&
                        (std::size_t(range.commands)+1)*8 <=
                            reservation.size-(range.address-reservation.offset)) owned = true;
                require(owned && !overlaps(lists_,{range.address,(std::size_t(range.commands)+1)*8}),
                    "course callback/continuation escapes provider reservation");
                require(words[slot/4] == 0 && words[slot/4+1] == 0,
                    "course continuation was not freshly prepared native NOP");
            }
        }
    };
    validate(world.objects,world.reservations); validate(items.objects,items.reservations);
    if (global_source_order_) {
        validate(actors.fragments,actors.reservations); validate(cart.objects,cart.reservations);
        std::size_t previous = 0;
        bool first = true;
        for (const auto& fragment : actors.fragments) {
            std::size_t index = 0;
            while (index < order.size() && order[index].object != identity(fragment)) ++index;
            require(first || index > previous,
                "course Actor/prefix collection order differs from actual Services ties");
            previous = index; first = false;
        }
    }
    const auto visit = [&](const auto& callback) {
        for (const auto& object : world.objects) callback(object);
        for (const auto& object : items.objects) callback(object);
        if (global_source_order_) {
            for (const auto& object : actors.fragments) callback(object);
            for (const auto& object : cart.objects) callback(object);
        }
    };
    visit([&](const auto& object) {
        for (unsigned lane = 0; lane < 4; ++lane) {
            const auto range = object.lanes[lane];
            if (!range.commands) continue;
            visit([&](const auto& other) {
                for (unsigned other_lane = 0; other_lane < 4; ++other_lane) {
                    if (identity(object) == identity(other) && lane == other_lane) continue;
                    const auto next = other.lanes[other_lane];
                    if (next.commands)
                        require(!overlaps({range.address,(std::size_t(range.commands)+1)*8},
                            {next.address,(std::size_t(next.commands)+1)*8}),
                            "course callback/continuation overlaps another raw callback");
                }
            });
        }
    });
    // Every actual owner belongs to exactly one provider, including the explicit
    // prefix identity. Validate before any lane/pass absence can hide a binding.
    for (const auto& entry : order) {
        std::size_t matches = 0;
        const auto count = [&](const auto& objects) {
            for (const auto& object : objects) if (identity(object) == entry.object) ++matches;
        };
        count(world.objects); count(items.objects);
        if (global_source_order_) { count(actors.fragments); count(cart.objects); }
        require(matches <= 1,"course callback belongs to multiple providers");
    }
    require(binding_count <= source_count,"course callback count exceeds Services projection");
    if (!global_source_order_) {
        // This diagnostic owns an inline join: its consumers may prepare the
        // providers again before submission. Never patch their borrowed tails.
        std::size_t cursor = 0;
        const auto command = [&](std::uint32_t a, std::uint32_t b) {
            require(cursor < commands_.size(),"course mixed callback commands exceed explicit list reservation");
            commands_[cursor++] = {a,b};
        };
        for (unsigned lane = 0; lane < 4; ++lane) {
            const auto begin = cursor;
            bool have_models = false;
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
                command(0xde000000,actors.camera.address);
                command(0xde000000,actors.lanes[lane].address);
            }
            command(0xdf000000,0);
            result.lanes[lane] = {static_cast<std::uint32_t>(lists_.offset+begin*8),(cursor-begin)*2};
        }
        for (std::size_t i = 0; i < cursor; ++i) {
            const auto at = static_cast<std::uint32_t>(lists_.offset+i*8);
            memory.word(at,commands_[i].w0); memory.word(at+4,commands_[i].w1);
        }
        return result;
    }
    std::size_t cursor = 0, patch_count = 0;
    const auto command = [&](std::uint32_t a, std::uint32_t b) {
        require(cursor < commands_.size(),"course continuation blocks exceed cold list capacity");
        commands_[cursor++] = {a,b};
    };
    for (unsigned lane = 0; lane < 4; ++lane) {
        const auto begin = cursor;
        std::uint32_t previous_slot = 0, camera = 0;
        for (const auto& entry : order) {
            const auto append = [&](const auto& objects, auto provider_camera) {
                for (const auto& object : objects) if (identity(object) == entry.object) {
                    const auto range = object.lanes[lane];
                    if (!range.commands) return;
                    const auto block = static_cast<std::uint32_t>(lists_.offset+cursor*8);
                    if (previous_slot) {
                        require(patch_count < patches_.size(),"course continuation patch capacity exceeded");
                        patches_[patch_count++] = {previous_slot,block};
                    }
                    if (camera != provider_camera.address) command(0xde000000,provider_camera.address);
                    command(0xde010000,range.address); // G_DL NOPUSH: no extra call-stack level.
                    camera = provider_camera.address;
                    if constexpr (requires { object.resets_camera; })
                        if (object.resets_camera) camera = 0;
                    previous_slot = object.continuation_slots[lane];
                    return;
                }
            };
            append(world.objects,world.camera);
            append(items.objects,items.camera);
            if (global_source_order_) {
                append(actors.fragments,actors.camera);
                append(cart.objects,cart.camera);
            }
        }
        const bool aggregate_actors = !global_source_order_ && actors.lanes[lane].commands > 1;
        if (lane && !previous_slot && !aggregate_actors) continue;
        const auto terminal = static_cast<std::uint32_t>(lists_.offset+cursor*8);
        if (aggregate_actors) {
            command(0xde000000,actors.camera.address);
            command(0xde000000,actors.lanes[lane].address);
        }
        command(0xdf000000,0);
        if (previous_slot) {
            require(patch_count < patches_.size(),"course terminal patch capacity exceeded");
            patches_[patch_count++] = {previous_slot,terminal};
        }
        // Prepared counts are 32-bit words; raw provider counts are 64-bit commands.
        result.lanes[lane] = {static_cast<std::uint32_t>(lists_.offset+begin*8),(cursor-begin)*2};
    }
    // Publish only after every source binding, tail and bounded block is checked.
    for (std::size_t i = 0; i < cursor; ++i) {
        const auto at = static_cast<std::uint32_t>(lists_.offset+i*8);
        memory.word(at,commands_[i].w0); memory.word(at+4,commands_[i].w1);
    }
    for (std::size_t i = 0; i < patch_count; ++i) {
        memory.word(patches_[i].slot,0xde010000); memory.word(patches_[i].slot+4,patches_[i].target);
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
