#include "photo_subjects.hpp"
#include "course_resources.hpp"
#include "egg_source.h"
#include "model_data_decoder.hpp"

#include <algorithm>
#include <bit>
#include <exception>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace isle::photo::subjects {
namespace {
constexpr std::array<std::uint8_t,20> admitted_sha1{
    0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,
    0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
constexpr model_assets::Mapping lookup_mapping{0x800ac0e0,0x57a90,0x2450};
void require(bool value, const char* message, std::uint32_t address = 0) {
    if (!value) throw std::runtime_error(std::string(message)+" at guest "+std::to_string(address));
}
float scalar(const rom::GuestView& wire, std::size_t offset) {
    const auto value = wire.f32(offset);
    require(std::isfinite(value),"Nonfinite photo resource scalar",wire.guest_address);
    return value;
}
bool scored(std::int32_t id) noexcept {
    return (id > 0 && id <= 151) || id == 600 || id == 601 || id == 602 || id == 603;
}
menu::RenderType render_type(std::uint32_t identity) {
    switch (identity) {
    case 0x800a15d8: return menu::RenderType::b_fogged;
    case 0x800a1608: return menu::RenderType::d_fogged;
    case 0x800a1650: return menu::RenderType::i_fogged;
    case 0x800a1680: return menu::RenderType::i_no_fog;
    case 0x800a16b0: return menu::RenderType::j_fogged;
    default: throw std::runtime_error("Unclosed original photo render callback at guest "+std::to_string(identity));
    }
}
std::size_t tree_nodes(const model_assets::Reader& reader, std::uint32_t tree) {
    const auto available = reader.remaining_bytes(tree);
    for (std::size_t i = 0; i <= available/44; ++i) {
        const auto offset = i*44;
        require(offset <= available && available-offset >= 4,"Photo tree lacks mapped sentinel",tree);
        const auto id = reader.view(tree+static_cast<std::uint32_t>(offset),4).u32(0);
        if (id == 18) { require(i != 0,"Empty photo model tree",tree); return i; }
    }
    throw std::runtime_error("Photo tree lacks mapped sentinel");
}
std::vector<Definition> definitions(const model_assets::Reader& reader, std::uint32_t begin,
                                    std::uint32_t end, bool scenery) {
    std::vector<Definition> result;
    const auto stride = scenery ? 28u : 20u;
    for (auto address = begin; address <= end && end-address >= 4; address += stride) {
        const auto id = reader.view(address,4).u32(0);
        if (!id) return result;
        require(end-address >= stride,"Photo definition escapes exact table domain",address);
        Definition value{}; value.wire = reader.view(address,stride);
        value.id = std::bit_cast<std::int32_t>(id); value.scale = scalar(value.wire,4);
        value.tree = value.wire.u32(8); value.materials = value.wire.u32(12);
        value.callback = value.wire.u32(16);
        require(value.tree && value.callback,"Photo definition lacks tree/render identity",address);
        if (scenery) { value.model_animations = value.wire.u32(20); value.material_animations = value.wire.u32(24); }
        result.push_back(value);
    }
    throw std::runtime_error("Photo definition table lacks source sentinel");
}
void decode_animation_sets(const model_assets::Reader& reader, std::uint32_t address, std::size_t count,
                    std::vector<AnimationSet>& output) {
    for (std::size_t i = 0; i < count; ++i) {
        AnimationSet set{}; set.wire = reader.view(address+static_cast<std::uint32_t>(i*12),12);
        set.id = std::bit_cast<std::int32_t>(set.wire.u32(0));
        const auto length = std::bit_cast<std::int32_t>(set.wire.u32(8));
        require(length >= 0,"Negative photo variant count",set.wire.guest_address);
        const auto table = set.wire.u32(4);
        if (length) {
            require(table && std::size_t(length) <= reader.remaining_bytes(table)/12,
                    "Photo variants escape source mapping",table);
            set.variants.reserve(static_cast<std::size_t>(length));
            for (std::int32_t j = 0; j < length; ++j) {
                const auto row = reader.view(table+static_cast<std::uint32_t>(j*12),12);
                set.variants.push_back({row,scalar(row,0),row.u32(4),row.u32(8)});
            }
        }
        output.push_back(std::move(set));
    }
}
void put16(std::vector<std::byte>& bytes, std::size_t at, std::int16_t value) {
    const auto bits = static_cast<std::uint16_t>(value);
    bytes[at] = std::byte(bits>>8); bytes[at+1] = std::byte(bits&255);
}
void decode_egg_vertices(const rom::GuestView& wire, std::span<IsleEggVertex> vertices) {
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto at = i*16;
        auto& vertex = vertices[i];
        vertex.unk_00 = std::bit_cast<std::int16_t>(wire.u16(at));
        vertex.unk_02 = std::bit_cast<std::int16_t>(wire.u16(at+2));
        vertex.unk_04 = std::bit_cast<std::int16_t>(wire.u16(at+4));
        for (std::size_t j = 0; j < 6; ++j) vertex.unk_06[j] = wire.u8(at+6+j);
        vertex.unk_0C = std::bit_cast<std::int8_t>(wire.u8(at+12));
        vertex.unk_0D = std::bit_cast<std::int8_t>(wire.u8(at+13));
        vertex.unk_0E = std::bit_cast<std::int8_t>(wire.u8(at+14));
        vertex.unk_0F = wire.u8(at+15);
    }
}
struct EggReplayScratch {
    std::array<IsleEggVertex,336> base, endpoint, destination;
    std::size_t patch;
    bool endpoint_ready = false;
    EggReplayScratch(const model_assets::Reader& reader, std::uint32_t guest,
                     std::size_t count, std::size_t patch_index)
        : patch(patch_index) {
        decode_egg_vertices(reader.view(guest,count*16,8),std::span(base).first(count));
    }
};
void egg_patch(std::vector<VertexPatch>& patches, std::optional<EggReplayScratch>& scratch,
               const model_assets::Reader& reader, std::uint32_t destination,
               std::uint32_t endpoint, std::size_t vertices, bool morph, float time) {
    if (!scratch) {
        scratch.emplace(reader,destination,vertices,patches.size());
        patches.push_back({destination,std::vector<std::byte>(vertices*16)});
    }
    auto& work = *scratch;
    const IsleEggVertex* output = work.base.data();
    if (morph) {
        if (!work.endpoint_ready) {
            decode_egg_vertices(reader.view(endpoint,vertices*16,8),std::span(work.endpoint).first(vertices));
            // AB050 leaves opaque6/tail untouched; seed those exactly once.
            std::copy_n(work.base.begin(),vertices,work.destination.begin());
            work.endpoint_ready = true;
        }
        // Original GAME 47380.c1202-1235 consumes the recorded GObj clock,
        // not the live Egg timer (which advances at different course rates).
        float amount = time < 0 || time > 60.0f ? 1.0f : time/60.0f;
        if (amount < 0.0f) amount = 0.0f;
        if (amount > 1.0f) amount = 1.0f;
        isle_egg_original_func_800AB050(amount,static_cast<std::int32_t>(vertices),
            work.destination.data(),work.base.data(),work.endpoint.data());
        output = work.destination.data();
    }
    // No SDK/guest overlay: CPU Source-width semantics become canonical BE
    // positions, unchanged opaque6, signed normal3 bit patterns and tail.
    auto& bytes = patches[work.patch].bytes;
    for (std::size_t i = 0; i < vertices; ++i) {
        const auto at = i*16;
        const auto& vertex = output[i];
        put16(bytes,at,vertex.unk_00);
        put16(bytes,at+2,vertex.unk_02);
        put16(bytes,at+4,vertex.unk_04);
        for (std::size_t j = 0; j < 6; ++j) bytes[at+6+j] = std::byte(vertex.unk_06[j]);
        bytes[at+12] = std::byte(static_cast<std::uint8_t>(vertex.unk_0C));
        bytes[at+13] = std::byte(static_cast<std::uint8_t>(vertex.unk_0D));
        bytes[at+14] = std::byte(static_cast<std::uint8_t>(vertex.unk_0E));
        bytes[at+15] = std::byte(vertex.unk_0F);
    }
}
IsleGameModelTransform transform(const menu::animation::Transform& t) {
    return {{t.position.x,t.position.y,t.position.z},{t.rotation.x,t.rotation.y,t.rotation.z},
            {t.scale.x,t.scale.y,t.scale.z}};
}
menu::animation::Transform transform(const IsleGameModelTransform& t) {
    return {{t.position.x,t.position.y,t.position.z},{t.rotation.x,t.rotation.y,t.rotation.z},
            {t.scale.x,t.scale.y,t.scale.z}};
}
}
float frozen_time(float time) {
    require(std::isfinite(time),"Nonfinite recorded world time");
    while (!(time < 100.0f)) {
        const float next = time-100.0f;
        require(next < time,"Recorded world time cannot advance source subtraction");
        time = next;
    }
    return time;
}
Resources::Resources(const rom::Snapshot& snapshot, const world::Resources& world,
    const IslePlayerPhotoData& record)
    : snapshot_(&snapshot), source_data_(snapshot.canonical().data()), world_(&world), record_(record) {
    require(snapshot.canonical().size() == rom::us_size && snapshot.identity().size == rom::us_size &&
            snapshot.identity().sha1 == admitted_sha1,"Photo resources require admitted US Snapshot");
    world.require_live();
    const auto& layout = course::layout(record.levelID);
    require(world.setup().wire.guest_address == layout.world_setup,"Photo/world course identity mismatch");
    std::vector<model_assets::Mapping> mappings(layout.mappings.begin(),layout.mappings.end());
    mappings.push_back(lookup_mapping);
    const model_assets::Reader reader(snapshot,mappings);
    subjects_ = definitions(reader,0x800adbec,0x800ae27c,false);
    scenery_ = definitions(reader,0x800ada64,0x800adbec,true);
    animation_sets_.reserve(83);
    decode_animation_sets(reader,0x800ad4b8,67,animation_sets_);
    decode_animation_sets(reader,0x800ad9a4,16,animation_sets_);
    constexpr std::array<std::pair<std::int32_t,std::uint32_t>,3> identity_tables{{
        {113,0x800acd9c},{601,0x800ad444},{602,0x800ad474}}};
    for (std::size_t i = 0; i < identity_tables.size(); ++i) {
        const auto [id,address] = identity_tables[i];
        const auto set = std::find_if(animation_sets_.begin(),animation_sets_.end(),
            [id](const auto& value) { return value.id == id; });
        require(set != animation_sets_.end(),"Payload identity table lacks source variant domain",address);
        auto& table = identities_[i]; table.id = id;
        table.variants.reserve(set->variants.size());
        for (std::size_t variant = 0; variant < set->variants.size(); ++variant)
            table.variants.push_back(reader.view(address+static_cast<std::uint32_t>(variant*12),4));
    }
    model_assets::DataDecoder decoder(reader,materials_,animations_,{},&paths_);
    struct Hydration {
        const Definition* definition;
        std::uint32_t model_animations, material_animations;
        std::size_t nodes;
        float time;
    };
    std::vector<Hydration> hydration;
    auto hydrate = [&](const Definition& definition, Instance instance,
                       std::uint32_t model_anims, std::uint32_t material_anims) {
        // Close callback admission before retaining a reconstruction plan.
        render_type(definition.callback);
        instance.callback = definition.callback;
        instances_.push_back(instance);
        hydration.push_back({&definition,model_anims,material_anims,tree_nodes(reader,definition.tree),instance.time});
    };
    // Source enterFirstBlock visits current, then previous, then next. Secondary
    // blocks have no neighbours. Primary repeated-first tables form a ring.
    const auto blocks = world.blocks();
    const auto current = static_cast<std::size_t>(record.unk_00_16&15u);
    require(current < blocks.size(),"Recorded block is absent from original course");
    std::array<std::size_t,3> selected{current,menu::no_index,menu::no_index};
    if (current < world.primary_count()) {
        if (current) selected[1] = current-1;
        else if (world.primary_loop()) selected[1] = world.primary_count()-1;
        if (current+1 < world.primary_count()) selected[2] = current+1;
        else if (world.primary_loop()) selected[2] = 0;
    }
    const auto& origin = blocks[current].metadata.world_position;
    for (const auto block_index : selected) {
        if (block_index == menu::no_index) continue;
        const auto& block = blocks[block_index];
        for (const auto& spawn : block.spawns) {
            const auto found = std::find_if(scenery_.begin(),scenery_.end(),[&](const auto& d) { return d.id == spawn.id; });
            if (found == scenery_.end()) continue; // Original scenery table filter.
            Instance instance{}; instance.kind = Kind::scenery; instance.id = found->id;
            instance.time = frozen_time(record.unk_04.f32); instance.scale = found->scale;
            instance.position = {(spawn.position.x+(block.metadata.world_position.x-origin.x))*100.0f,
                (spawn.position.y+(block.metadata.world_position.y-origin.y))*100.0f,
                (spawn.position.z+(block.metadata.world_position.z-origin.z))*100.0f};
            instance.yaw = spawn.rotation.y;
            instance.hide_first_child = instance.id == 1018 && (record.unk_00_16&0xe0u) != 0x80u;
            // Source only binds scenery materials inside the non-NULL model branch.
            hydrate(*found,instance,found->model_animations,found->model_animations ? found->material_animations : 0);
        }
    }
    // GAME 47380.c988/998 saves the freshly loaded course data once. These
    // cold semantic arrays model those immutable replay bases, not live Egg
    // frame0 BSS or a preceding record's residual morph destination.
    std::array<std::optional<EggReplayScratch>,2> egg_scratch;
    for (std::size_t i = 0; i < 12; ++i) {
        const auto& recorded = record.pokemons[i];
        if (recorded.pokemonID < 0) break;
        const auto found = std::find_if(subjects_.begin(),subjects_.end(),[&](const auto& d) { return d.id == recorded.pokemonID; });
        if (found == subjects_.end()) continue; // Source does not render unmatched IDs.
        require(recorded.unk_00_13 < 64 && std::isfinite(recorded.animationTime),"Invalid recorded subject variant/time");
        const auto first = recorded.pokemonID < 1000 ? animation_sets_.begin() : animation_sets_.begin()+67;
        const auto last = recorded.pokemonID < 1000 ? animation_sets_.begin()+67 : animation_sets_.end();
        const auto set = std::find_if(first,last,[&](const auto& s) { return s.id == recorded.pokemonID; });
        require(set != last,"Photo subject lacks exact source animation set",found->wire.guest_address);
        require(recorded.unk_00_13 < set->variants.size(),"Recorded variant escapes exact source animation set",set->wire.guest_address);
        const auto& variant = set->variants[recorded.unk_00_13];
        const auto models = variant.model_animations, materials = variant.material_animations;
        Instance instance{}; instance.id = recorded.pokemonID; instance.record_index = i;
        instance.time = recorded.animationTime; instance.scale = found->scale;
        instance.position = recorded.position; instance.yaw = recorded.yaw;
        instance.dl_priority = instance.id == 82 ? 1u : 0x80000000u;
        if (instance.id == 113)
            instance.half_scale = identities_[0].variants[recorded.unk_00_13].u32(0) == 0x80186e20;
        if (instance.id == 602) {
            const bool morph = identities_[2].variants[recorded.unk_00_13].u32(0) == 0x80188cb0;
            egg_patch(patches_,egg_scratch[0],reader,0x80188d28,0x8018a6f0,0x150,morph,instance.time);
        } else if (instance.id == 601) {
            const bool morph = identities_[1].variants[recorded.unk_00_13].u32(0) == 0x8017a710;
            egg_patch(patches_,egg_scratch[1],reader,0x8017a780,0x8017c090,0x144,morph,instance.time);
        }
        hydrate(*found,instance,models,materials);
    }
    // A Texture can be shared across selected variants and attachments. Admit
    // the exact fresh stream/time samples reconstruction will execute before
    // caching it; never infer an unbounded timeline or scan adjacent pointers.
    struct MaterialUse {
        std::uint32_t address;
        std::vector<menu::animation::TextureSample> samples;
    };
    std::vector<MaterialUse> material_uses;
    for (const auto& plan : hydration) {
        if (!plan.definition->materials) continue;
        const auto table = reader.view(plan.definition->materials,plan.nodes*4);
        for (std::size_t n = 0; n < plan.nodes; ++n) {
            const auto list = table.u32(n*4);
            if (!list) continue;
            const auto available = reader.remaining_bytes(list)/4;
            std::uint32_t animation_list = 0;
            if (plan.material_animations)
                animation_list = reader.view(plan.material_animations+static_cast<std::uint32_t>(n*4),4).u32(0);
            bool terminated = false;
            for (std::size_t j = 0; j < available; ++j) {
                const auto address = reader.view(list+static_cast<std::uint32_t>(j*4),4).u32(0);
                if (!address) { terminated = true; break; }
                auto use = std::find_if(material_uses.begin(),material_uses.end(),
                    [address](const auto& value) { return value.address == address; });
                if (use == material_uses.end()) { material_uses.push_back({address,{}}); use = material_uses.end()-1; }
                if (animation_list) {
                    const auto stream = decoder.animation(reader.view(animation_list+static_cast<std::uint32_t>(j*4),4).u32(0),
                        menu::AnimationKind::texture,false);
                    if (stream != menu::no_index && std::none_of(use->samples.begin(),use->samples.end(),
                        [&](const auto& sample) { return sample.program == stream && sample.first == plan.time; }))
                        // Reconstruction creates fresh AObjs, seeks the actual
                        // recorded/frozen time, sets speed0 and updates once.
                        use->samples.push_back({stream,plan.time,plan.time,0,true});
                }
            }
            require(terminated,"Photo material attachments lack mapped NULL sentinel",list);
        }
    }
    for (const auto& use : material_uses)
        decoder.sampled_material(use.address,use.samples);
    for (std::size_t i = 0; i < hydration.size(); ++i) {
        const auto& plan = hydration[i];
        auto& instance = instances_[i];
        const auto& definition = *plan.definition;
        const auto type = render_type(definition.callback);
        const auto found = std::find_if(models_.begin(),models_.end(),[&](const auto& model) {
            return model.tree.guest_address == definition.tree && model.material_table.guest_address == definition.materials &&
                model.render_type == type && model.animation_table.guest_address == plan.model_animations &&
                model.material_animation_table.guest_address == plan.material_animations;
        });
        if (found == models_.end()) {
            auto model = decoder.model(definition.tree,definition.materials,plan.nodes,type);
            model.matrix_kinds = {0x1c,0,0};
            model.fog_rgb = world.setup().fog_rgb;
            model.fog_min = world.setup().fog_min; model.fog_max = world.setup().fog_max;
            decoder.bind_initial_animations(model,plan.model_animations,plan.material_animations,false);
            instance.model = models_.size(); models_.push_back(std::move(model));
        } else instance.model = static_cast<std::size_t>(found-models_.begin());
    }
    visibility_.resize(models_.size());
    std::vector<model_assets::ModelPlan> plans; plans.reserve(models_.size());
    for (std::size_t i = 0; i < models_.size(); ++i) {
        auto& domain = visibility_[i]; domain.assign(models_[i].nodes.size(),1);
        for (std::size_t n = 0; n < models_[i].nodes.size(); ++n) {
            const auto stream = models_[i].nodes[n].animation;
            if (stream != menu::no_index) for (const auto& command : animations_[stream].commands)
                if (command.opcode == 15) domain[n] |= static_cast<std::uint8_t>(1u<<(command.mask&3u));
        }
        for (const auto& instance : instances_) if (instance.model == i && instance.hide_first_child) {
            const auto child = std::find_if(models_[i].nodes.begin(),models_[i].nodes.end(),[](const auto& n) { return n.parent == 0; });
            require(child != models_[i].nodes.end(),"Photo sign hiding requires original first child");
            const auto index = static_cast<std::size_t>(child-models_[i].nodes.begin());
            domain[index] |= static_cast<std::uint8_t>((domain[index]&5u)<<1);
        }
        plans.push_back({&models_[i],domain});
    }
    for (const auto& instance : instances_) capacities_.push_back(menu::animation::slot_capacity(models_[instance.model]));
    closure_.emplace(reader,plans,materials_);
    closure_->require_complete();
    // Publish one disjoint immutable installation, not "ROM install plus a
    // mutable patch" that a later frame installer could silently overwrite.
    std::sort(patches_.begin(),patches_.end(),[](const auto& a, const auto& b) { return a.guest < b.guest; });
    for (const auto& span : closure_->installation_spans()) {
        std::size_t cursor = 0;
        const auto end = std::uint64_t(span.guest_address)+span.bytes.size();
        for (const auto& patch : patches_) {
            const auto patch_end = std::uint64_t(patch.guest)+patch.bytes.size();
            const auto current = std::uint64_t(span.guest_address)+cursor;
            if (patch_end <= current || patch.guest >= end) continue;
            if (patch.guest > current) {
                const auto bytes = static_cast<std::size_t>(patch.guest-current);
                installation_.push_back({static_cast<std::uint32_t>(current),span.rom_offset+cursor,
                    span.bytes.subspan(cursor,bytes)});
            }
            cursor = static_cast<std::size_t>(std::min(patch_end,end)-span.guest_address);
        }
        if (cursor < span.bytes.size())
            installation_.push_back({span.guest_address+static_cast<std::uint32_t>(cursor),span.rom_offset+cursor,
                span.bytes.subspan(cursor)});
    }
    for (const auto& patch : patches_)
        // Like decompressed assets, these source-derived bytes have no literal
        // one-to-one ROM offset. Their immutable owner is this Resources.
        installation_.push_back({patch.guest,menu::decompressed_rom_offset,patch.bytes});
    std::sort(installation_.begin(),installation_.end(),
        [](const auto& a, const auto& b) { return a.guest_address < b.guest_address; });
}
void Resources::require_live() const {
    require(snapshot_->canonical().size() == rom::us_size && snapshot_->canonical().data() == source_data_,
            "Photo resources borrow moved-from Snapshot");
    world_->require_live();
}
void Resources::install(std::span<std::uint32_t> words, std::span<const menu::RamRange> reservations) const {
    require_live();
    require(words.data() && words.size_bytes() == menu::rdram_size,"Photo assets require exclusive canonical RDRAM");
    const auto begin = reinterpret_cast<std::uintptr_t>(words.data());
    require(begin <= UINTPTR_MAX-words.size_bytes(),"Photo RAM pointer overflow");
    const auto end = begin+words.size_bytes();
    auto disjoint = [&](const void* pointer, std::size_t bytes) {
        if (!bytes) return;
        const auto at = reinterpret_cast<std::uintptr_t>(pointer);
        require(pointer && at <= UINTPTR_MAX-bytes && (at+bytes <= begin || end <= at),
                "Photo RAM aliases retained resource owner");
    };
    disjoint(this,sizeof(*this));
    disjoint(snapshot_->canonical().data(),snapshot_->canonical().size());
    disjoint(reservations.data(),reservations.size_bytes());
    for (const auto range : reservations)
        require(range.size && range.offset <= menu::rdram_size && range.size <= menu::rdram_size-range.offset,
                "Photo asset reservation escapes RDRAM");
    auto validate = [&](std::uint32_t guest, std::span<const std::byte> bytes) {
        require(guest >= 0x80000000u && guest-0x80000000u <= menu::rdram_size &&
            bytes.size() <= menu::rdram_size-(guest-0x80000000u),"Photo asset write escapes RDRAM",guest);
        disjoint(bytes.data(),bytes.size());
        const auto physical = std::size_t(guest-0x80000000u);
        for (const auto range : reservations)
            require(physical+bytes.size() <= range.offset || range.offset+range.size <= physical,
                    "Photo asset write overlaps caller reservation",guest);
    };
    const auto spans = installation_spans();
    disjoint(spans.data(),spans.size_bytes());
    disjoint(patches_.data(),patches_.size()*sizeof(VertexPatch));
    for (const auto& span : spans) validate(span.guest_address,span.bytes);
    auto ram = std::as_writable_bytes(words);
    auto upload = [&](std::uint32_t guest, std::span<const std::byte> bytes) {
        const auto physical = std::size_t(guest-0x80000000u);
        for (std::size_t i = 0; i < bytes.size(); ++i) ram[(physical+i)^3u] = bytes[i];
    };
    for (const auto& span : spans) upload(span.guest_address,span.bytes);
}

namespace {
std::size_t seek_commands(const menu::Animation& stream, float time,
                          std::span<std::optional<float>> scratch) {
    float left = -time;
    std::size_t cursor = 0, count = 0;
    auto visited = scratch.first(stream.commands.size());
    std::fill(visited.begin(),visited.end(),std::nullopt);
    for (;;) {
        require(cursor < stream.commands.size() && count != SIZE_MAX,"Photo animation seek escapes decoded program");
        const auto& command = stream.commands[cursor++];
        ++count;
        if (!command.opcode) return count;
        if (command.opcode == 1 || command.opcode == 14) {
            const auto target = command.wire.u32(4);
            if (!target) return count;
            const auto found = std::find_if(stream.commands.begin(),stream.commands.end(),
                [target](const auto& c) { return c.wire.guest_address == target; });
            require(found != stream.commands.end(),"Photo animation jump lacks decoded boundary",target);
            cursor = static_cast<std::size_t>(found-stream.commands.begin());
            if (visited[cursor]) require(left > *visited[cursor],"Photo animation cycle cannot advance source clock",target);
            visited[cursor] = left;
        }
        switch (command.opcode) {
        case 2: case 3: case 5: case 8: case 10: case 15: case 16: case 17: case 18: case 20: case 23:
            left += static_cast<float>(command.duration); break;
        case 22: left = static_cast<float>(command.duration); break;
        default: break;
        }
        if (left > 0) return count;
    }
}
menu::animation::ExecutionCapacity execution_capacity(const Resources& resources) {
    std::size_t maximum = 0;
    std::size_t maximum_stream = 0;
    for (const auto& stream : resources.animations()) maximum_stream = std::max(maximum_stream,stream.commands.size());
    std::vector<std::optional<float>> scratch(maximum_stream);
    for (const auto& instance : resources.instances()) {
        std::size_t commands = 0;
        auto charge = [&](std::size_t stream) {
            if (stream == menu::no_index) return;
            const auto count = seek_commands(resources.animations()[stream],instance.time,scratch);
            require(count <= SIZE_MAX-commands,"Photo animation command capacity overflow");
            commands += count;
        };
        for (const auto& node : resources.models()[instance.model].nodes) {
            charge(node.animation);
            for (const auto stream : node.material_animations) charge(stream);
        }
        maximum = std::max(maximum,commands);
    }
    return {maximum,0}; // No live Pokemon/FX/sound callback is installed.
}
}
struct Reconstruction::Impl {
    struct NativeOwner {
        Impl* owner = nullptr;
        std::size_t index = 0;
        bool observing = false, requested = false, joined = false, retiring = false;
    };
    IsleGame* game;
    const Resources& resources;
    LinkOwnership links;
    std::unique_ptr<menu::animation::Runtime> runtime;
    std::vector<Object> objects;
    std::vector<menu::model::PacketBinding> bindings;
    std::vector<NativeOwner> native_owners;
    std::vector<IsleGameModelNodeInit> scratch;
    std::array<ScoredSlot,12> scores{};
    std::size_t count = 0, score_count = 0;
    bool started = false, clearing = false;
    IsleGameResult failure = ISLE_GAME_OK;
    IsleGameResult reject(IsleGameResult result) noexcept {
        if (failure == ISLE_GAME_OK) failure = result;
        return result;
    }
    static void joined(IsleGameObject*, void* context) noexcept {
        auto& native = *static_cast<NativeOwner*>(context);
        auto& s = *native.owner;
        native.joined = true;
        if (!native.requested) s.reject(ISLE_GAME_INVALID_STATE);
        s.objects[native.index].binding.object = nullptr;
        s.objects[native.index].binding.model = nullptr;
        s.bindings[native.index] = {};
    }
    Impl(IsleGame* g, const Resources& r, LinkOwnership l) : game(g), resources(r), links(l) {}
};
Reconstruction::Reconstruction(IsleGame* game, const Resources& resources, LinkOwnership links)
    : impl_(std::make_unique<Impl>(game,resources,links)) {
    require(game && links.object_link < 32 && links.dl_link < 32,"Photo reconstruction requires actual Game/link ownership");
    resources.require_live();
    auto& s = *impl_;
    s.objects.resize(resources.instances().size());
    s.bindings.resize(resources.instances().size());
    s.native_owners.resize(resources.instances().size());
    for (std::size_t i = 0; i < s.native_owners.size(); ++i) s.native_owners[i] = {&s,i};
    std::size_t maximum = 0;
    for (const auto capacity : resources.capacities()) maximum = std::max(maximum,capacity.nodes);
    s.scratch.resize(maximum);
    const auto capacities = resources.capacities();
    auto created = menu::animation::Runtime::create(
        {resources.models(),resources.materials(),resources.animations(),menu::no_index,resources.paths()},
        capacities,{},execution_capacity(resources));
    require(bool(created),"Photo frozen animation runtime admission rejected");
    s.runtime = std::move(created.runtime);
}
Reconstruction::~Reconstruction() { if (impl_->count) std::terminate(); }
IsleGameResult Reconstruction::publish() noexcept {
    auto& s = *impl_;
    if (s.started || s.clearing || s.failure != ISLE_GAME_OK) return s.reject(ISLE_GAME_INVALID_STATE);
    s.started = true;
    try {
        s.resources.require_live();
        const auto instances = s.resources.instances();
        const auto models = s.resources.models();
        for (std::size_t i = 0; i < instances.size(); ++i) {
            const auto& instance = instances[i];
            const auto acquired = s.runtime->acquire(i,instance.model,false);
            if (!acquired) return s.reject(ISLE_GAME_INVALID_STATE);
            auto& output = s.objects[i]; output = {};
            output.instance = i; output.record_index = instance.record_index;
            output.render = {s.links.dl_link,instance.dl_priority,s.links.camera_tag};
            output.binding.slot = i; output.binding.runtime = s.runtime.get();
            output.binding.state = s.runtime->model(acquired.handle);
            output.binding.pose = menu::model::PoseSource::native;
            output.binding.root = menu::model::RootRecipe::source;
            // Count the acquired slot before native allocation, so a rejected
            // native operation can be explicitly rolled back by clear().
            ++s.count;
            if (!s.runtime->restart_model(acquired.handle,instance.time) ||
                !s.runtime->set_model_speed(acquired.handle,0) ||
                !s.runtime->set_texture_speed(acquired.handle,0) ||
                !s.runtime->step_model(acquired.handle)) return s.reject(ISLE_GAME_INVALID_STATE);
            auto state = s.runtime->model(acquired.handle);
            IsleGameModelTransform root{};
            if (instance.kind == Kind::subject) {
                const auto unbaked = transform(state.nodes[0].transform);
                const auto result = isle_game_model_bake_photo_transform(&unbaked,&instance.position,
                    instance.yaw,instance.scale,&root);
                if (result != ISLE_GAME_OK) return s.reject(result);
                if (instance.half_scale) {
                    root.scale.x *= 0.5f; root.scale.y *= 0.5f; root.scale.z *= 0.5f;
                }
            } else {
                const float scale = instance.scale*0.1f;
                root = {instance.position,{0,instance.yaw,0},{scale,scale,scale}};
            }
            if (!s.runtime->set_node_transform(acquired.handle,0,transform(root))) return s.reject(ISLE_GAME_INVALID_STATE);
            output.binding.state = s.runtime->model(acquired.handle);
            auto result = isle_game_add_object(s.game,128,s.links.object_link,0x80000000u,nullptr,nullptr,&output.binding.object);
            if (result != ISLE_GAME_OK) return s.reject(result);
            auto& owner = s.native_owners[i];
            result = isle_game_object_set_retire_callback(s.game,output.binding.object,Impl::joined,&owner);
            if (result != ISLE_GAME_OK) return s.reject(result);
            owner.observing = true;
            const auto attached = menu::model::attach_source_model(s.game,output.binding.object,
                models[instance.model],output.binding.state,menu::model::RootRecipe::source,s.scratch,
                &output.binding.model);
            if (!attached) return s.reject(ISLE_GAME_INVALID_STATE);
            state = s.runtime->model(acquired.handle);
            for (std::size_t n = 0; n < state.nodes.size(); ++n) {
                auto flags = state.nodes[n].flags;
                if (instance.hide_first_child && state.nodes[n].parent == 0) {
                    // Only the first child, not all direct children.
                    bool first = true;
                    for (std::size_t p = 1; p < n; ++p) if (state.nodes[p].parent == 0) first = false;
                    if (first) flags |= 1u;
                }
                result = isle_game_model_set_flags(s.game,output.binding.model,n,flags);
                if (result != ISLE_GAME_OK) return s.reject(result);
            }
            s.bindings[i] = {output.binding,output.render};
            if (instance.kind == Kind::subject && scored(instance.id)) {
                s.scores[s.score_count++] = {output.binding.object,static_cast<std::uint8_t>(instance.record_index)};
            }
        }
        return ISLE_GAME_OK;
    } catch (...) { return s.reject(ISLE_GAME_INVALID_ARGUMENT); }
}
IsleGameResult Reconstruction::clear() noexcept {
    auto& s = *impl_;
    s.clearing = true;
    while (s.count) {
        auto& object = s.objects[s.count-1];
        auto& owner = s.native_owners[s.count-1];
        if (object.binding.object) {
            if (!owner.observing) {
                const auto result = isle_game_object_set_retire_callback(s.game,object.binding.object,Impl::joined,&owner);
                if (result != ISLE_GAME_OK) return s.reject(result);
                owner.observing = true;
            }
            if (!owner.requested) {
                owner.requested = true;
                const auto result = isle_game_delete_object(s.game,object.binding.object);
                if (result != ISLE_GAME_OK) { owner.requested = false; return s.reject(result); }
            }
            if (!owner.joined) return ISLE_GAME_OK;
        }
        const auto handle = object.binding.state.instance;
        if (handle) {
            if (!owner.retiring) {
                if (!s.runtime->begin_retire(handle)) return s.reject(ISLE_GAME_INVALID_STATE);
                owner.retiring = true;
            }
            if (!s.runtime->release_retired(handle)) return s.reject(ISLE_GAME_INVALID_STATE);
        }
        object = {};
        s.bindings[s.count-1] = {};
        --s.count;
    }
    s.score_count = 0;
    return ISLE_GAME_OK;
}
bool Reconstruction::rearm() noexcept {
    auto& s = *impl_;
    if (!cleared() || s.failure != ISLE_GAME_OK) return false;
    for (auto& owner : s.native_owners) owner = {&s,owner.index};
    s.started = s.clearing = false;
    return true;
}
bool Reconstruction::clearing() const noexcept { return impl_->clearing; }
bool Reconstruction::cleared() const noexcept { return impl_->clearing && impl_->count == 0; }
bool Reconstruction::failed() const noexcept { return impl_->failure != ISLE_GAME_OK; }
IsleGameResult Reconstruction::error() const noexcept { return impl_->failure; }
bool Reconstruction::active() const noexcept {
    return impl_->count != 0 && !impl_->clearing && impl_->failure == ISLE_GAME_OK;
}
std::span<const Object> Reconstruction::objects() const noexcept {
    return std::span(impl_->objects).first(impl_->count);
}
std::span<const menu::model::PacketBinding> Reconstruction::bindings() const noexcept {
    return impl_->bindings;
}
std::span<const ScoredSlot> Reconstruction::scored_slots() const noexcept {
    return std::span(impl_->scores).first(impl_->score_count);
}

} // namespace isle::photo::subjects
