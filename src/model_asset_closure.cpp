#include "model_asset_closure.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace isle::model_assets {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint32_t advance(std::uint32_t address, std::size_t bytes) {
    require(bytes <= std::numeric_limits<std::uint32_t>::max() - address,
            "Model resource address overflow");
    return address + static_cast<std::uint32_t>(bytes);
}
// A source set is a borrowed immutable material identity, never a span into a
// copied state's own scalar. All fields of the TIMG tuple travel together.
enum class Source : std::uint8_t { incoming, direct, images, palettes };
struct State {
    Source source = Source::incoming;
    std::size_t identity = 0;
    std::uint8_t format = 0, size = 0;
    std::uint16_t width = 0;
    const std::vector<std::size_t>* binding = nullptr;
    std::uint32_t latch = 0;
    bool latch_known = false;
    friend bool operator==(const State&, const State&) = default;
};
struct Key {
    std::uint32_t root;
    State state;
    friend bool operator==(const Key&, const Key&) = default;
};
struct Hash {
    std::size_t operator()(const Key& key) const noexcept {
        std::size_t h = key.root;
        const auto add = [&](std::size_t v) { h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2); };
        const auto& s = key.state;
        add(static_cast<std::size_t>(s.source)); add(s.identity); add(s.format);
        add(s.size); add(s.width); add(reinterpret_cast<std::uintptr_t>(s.binding));
        add(s.latch); add(s.latch_known);
        return h;
    }
};
struct Command { std::uint32_t address, w0, w1; };
using States = std::vector<State>;
} // namespace

Reader::Reader(const rom::Snapshot& snapshot, std::span<const Mapping> mappings)
    : Reader(snapshot.canonical(), mappings) {}
Reader::Reader(std::span<const std::byte> bytes, std::span<const Mapping> mappings)
    : bytes_(bytes), mappings_(mappings) {
    require(!bytes.empty(), "Empty model resource source");
    for (std::size_t i = 0; i < mappings.size(); ++i) {
        const auto& a = mappings[i];
        require(a.bytes && a.guest_begin >= 0x80000000u && a.guest_begin < 0x80800000u &&
                a.bytes <= 0x80800000ull - a.guest_begin &&
                a.rom_begin <= bytes.size() && a.bytes <= bytes.size() - a.rom_begin,
                "Invalid model resource mapping");
        for (std::size_t j = 0; j < i; ++j) {
            const auto& b = mappings[j];
            require(static_cast<std::uint64_t>(a.guest_begin) + a.bytes <= b.guest_begin ||
                    static_cast<std::uint64_t>(b.guest_begin) + b.bytes <= a.guest_begin,
                    "Ambiguous model guest mapping");
            require(a.rom_begin + a.bytes <= b.rom_begin || b.rom_begin + b.bytes <= a.rom_begin,
                    "Ambiguous model ROM mapping");
        }
    }
}
rom::GuestView Reader::view(std::uint32_t address, std::size_t bytes, std::size_t alignment) const {
    require(bytes && alignment && !(alignment & (alignment - 1)) && address % alignment == 0,
            "Empty or misaligned model resource");
    for (const auto& m : mappings_) {
        if (address < m.guest_begin) continue;
        const auto delta = static_cast<std::size_t>(address - m.guest_begin);
        if (delta >= m.bytes) continue;
        require(bytes <= m.bytes - delta, "Model resource escapes initialized mapping");
        return {address, m.rom_begin + delta, bytes_.subspan(m.rom_begin + delta, bytes)};
    }
    throw std::runtime_error("Model resource outside initialized mappings: " + std::to_string(address));
}
std::size_t Reader::remaining_bytes(std::uint32_t address) const {
    for (const auto& mapping : mappings_) {
        if (address >= mapping.guest_begin && address - mapping.guest_begin < mapping.bytes)
            return mapping.bytes - (address - mapping.guest_begin);
    }
    throw std::runtime_error("Model resource outside initialized mappings: " + std::to_string(address));
}

struct Builder {
    Closure& out;
    const Reader& reader;
    std::span<const menu::Material> materials;
    Limits limits;
    std::size_t decoded = 0, evaluated = 0, model_index = menu::no_index;
    std::uint32_t lane = 0;
    std::unordered_map<std::uint32_t, std::vector<Command>> code;
    std::unordered_map<Key, States, Hash> cache;
    std::vector<std::uint32_t> active;
    Builder(Closure& closure, const Reader& input, std::span<const menu::Material> values, Limits bounds)
        : out(closure), reader(input), materials(values), limits(bounds) {}

    void append(States& states, const State& state) const {
        if (std::find(states.begin(), states.end(), state) == states.end()) {
            require(states.size() < limits.alternatives, "Model closure alternative bound exhausted");
            states.push_back(state);
        }
    }
    void append(States& states, const States& values) const {
        for (const auto& state : values) append(states, state);
    }
    void obligation(Parameter parameter, Command c) {
        const Obligation value{parameter, model_index, lane, c.address, c.w0, c.w1};
        if (std::find(out.obligations_.begin(), out.obligations_.end(), value) == out.obligations_.end()) {
            require(out.obligations_.size() < limits.obligations, "Model closure obligation bound exhausted");
            out.obligations_.push_back(value);
        }
    }
    const std::vector<Command>& program(std::uint32_t root) {
        if (const auto found = code.find(root); found != code.end()) return found->second;
        std::vector<Command> commands;
        for (auto pc = root;; pc = advance(pc, 8)) {
            require(++decoded <= limits.commands, "Model closure decoded-command bound exhausted");
            const auto wire = reader.view(pc, 8, 8);
            const auto w0 = wire.u32(0), w1 = wire.u32(4);
            commands.push_back({pc, w0, w1});
            if ((w0 >> 24) == 0xdf || w0 == 0xde010000u) break;
        }
        const auto wire = reader.view(root, commands.size() * 8, 8);
        out.lists_.push_back({wire}); out.installation_.push_back(wire);
        return code.emplace(root, std::move(commands)).first->second;
    }
    void image(const State& state, Command command, std::size_t bytes, menu::ImageKind kind) {
        require(bytes, "Empty model image demand");
        if (state.source == Source::incoming) { obligation(Parameter::image, command); return; }
        const auto record = [&](std::uint32_t address) {
            require(address, "NULL image in consumed material domain");
            const auto wire = reader.view(address, bytes, 8);
            const auto old = std::find_if(out.images_.begin(), out.images_.end(), [&](const auto& value) {
                return value.command_identity == command.address && value.wire.guest_address == address &&
                    value.wire.bytes.size() == bytes && value.kind == kind && value.format == state.format &&
                    value.source_size_code == state.size && value.source_width == state.width;
            });
            if (old == out.images_.end()) {
                out.images_.push_back({wire, command.address, kind, state.format, state.size, state.width});
                out.installation_.push_back(wire);
            }
        };
        if (state.source == Source::direct) record(static_cast<std::uint32_t>(state.identity));
        else {
            require(state.identity < materials.size(), "Invalid material source identity");
            const auto& material = materials[state.identity];
            const auto& sources = state.source == Source::images ? material.images : material.palettes;
            require(!sources.empty(), "Consumed material has no bounded image domain");
            for (const auto address : sources) {
                // Source-declared image holes have no payload allocation.
                // The packet kernel still rejects actually selecting NULL.
                if (!address && state.source==Source::images) continue;
                record(address);
            }
        }
    }
    State material(Command command, State state) {
        require((command.w1 >> 24) == 0x0e && !(command.w1 & 7u), "Malformed Segment14 material slot");
        if (!state.binding) {
            obligation(Parameter::segment14, command);
            state.source = Source::incoming; state.identity = 0;
            state.format = 0; state.size = 0; state.width = 0;
            return state;
        }
        const auto slot = (command.w1 & 0xffffffu) / 8;
        require(slot < state.binding->size(), "Segment14 slot outside actual attachment table");
        const auto index = (*state.binding)[slot];
        require(index < materials.size(), "Invalid native material attachment");
        const auto& m = materials[index];
        const menu::DynamicMaterialCall call{command.address, command.w1, index};
        const auto old = std::find_if(out.dynamic_calls_.begin(), out.dynamic_calls_.end(), [&](const auto& c) {
            return c.command_identity == call.command_identity && c.segmented_target == call.segmented_target && c.material_index == index;
        });
        if (old == out.dynamic_calls_.end()) out.dynamic_calls_.push_back(call);
        const auto flags = m.flags ? m.flags : 0xa1;
        require(m.format <= 4 && m.size_code <= 3 && m.auxiliary_format <= 4 && m.auxiliary_size_code <= 3,
                "Malformed material image tuple");
        const auto set = [&](Source source, std::uint8_t format, std::uint8_t size) {
            state.source = source; state.identity = index; state.format = format; state.size = size; state.width = 1;
        };
        if (flags & 4) {
            set(Source::palettes, 0, 2);
            if (flags & 3) image(state, command, m.size_code == 1 ? 512 : 32, menu::ImageKind::palette);
        }
        if (flags & 0x12) {
            set(Source::images, m.auxiliary_format, static_cast<std::uint8_t>(m.auxiliary_size_code == 3 ? 3 : 2));
            if (flags & 0x11) {
                const auto texels = static_cast<std::size_t>(m.block_width) * m.block_height;
                require(texels, "Empty auxiliary material block");
                // Source emits rounded 16/32-bit loader units, not a main-image size conversion.
                const auto source_bits = 4u << m.auxiliary_size_code;
                const auto loader_bits = 4u << state.size;
                const auto units = (texels * source_bits + loader_bits - 1) / loader_bits;
                require(units && units <= 4096, "Auxiliary material load exceeds packet extent");
                image(state, command, units * loader_bits / 8, menu::ImageKind::texture_block);
            }
        }
        if (flags & 0x11) set(Source::images, m.format, m.size_code);
        return state; // Primitive-color-only flags0x200 preserve the complete tuple.
    }
    void load(State state, Command c) {
        const auto opcode = c.w0 >> 24;
        // Validate packet-only fields even when its entry tuple is symbolic.
        if (opcode == 0xf0) {
            require(c.w0 == 0xf0000000 && !(c.w1 & 0xf8003fffu) && ((c.w1 >> 14) & 1023) < 256,
                    "Malformed LOADTLUT packet");
            if (state.source == Source::incoming) { obligation(Parameter::image, c); return; }
            require(state.format == 0 && state.size == 2 && state.width == 1, "Invalid TLUT image tuple");
            image(state, c, (((c.w1 >> 14) & 1023) + 1) * 2, menu::ImageKind::palette);
            return;
        }
        require(!(c.w1 & 0xf8000000u), "Malformed texture-load tile selector");
        const auto sl = (c.w0 >> 12) & 4095, tl = c.w0 & 4095;
        require(!((sl | tl) & 3), "Unsupported fractional texture-load origin");
        if (opcode == 0xf4) {
            const auto sh = (c.w1 >> 12) & 4095, th = c.w1 & 4095;
            require(sh >= sl && th >= tl && !((sh | th) & 3), "Malformed LOADTILE coordinates");
        }
        if (state.source == Source::incoming) { obligation(Parameter::image, c); return; }
        require(state.width && state.format <= 4 && state.size <= 3 && (sl >> 2) < state.width,
                "Malformed incoming texture tuple/origin");
        std::size_t texels;
        if (opcode == 0xf3) {
            texels = static_cast<std::size_t>(tl >> 2) * state.width + (sl >> 2) + ((c.w1 >> 12) & 4095) + 1;
        } else {
            const auto sh = (c.w1 >> 12) & 4095, th = c.w1 & 4095;
            require((sh >> 2) < state.width, "LOADTILE escapes image row");
            texels = static_cast<std::size_t>(th >> 2) * state.width + (sh >> 2) + 1;
        }
        image(state, c, (texels * (4u << state.size) + 7) / 8,
              opcode == 0xf3 ? menu::ImageKind::texture_block : menu::ImageKind::texture_tile);
    }
    const States& execute(std::uint32_t root, State entry) {
        const Key key{root, entry};
        if (const auto found = cache.find(key); found != cache.end()) return found->second;
        require(active.size() < limits.call_depth && std::find(active.begin(), active.end(), root) == active.end(),
                "Cyclic or overdeep model display-list graph");
        active.push_back(root);
        const auto& commands = program(root);
        States states{entry}, exits, next;
        for (const auto c : commands) {
            next.clear();
            for (auto state : states) {
                require(++evaluated <= limits.evaluations, "Model closure evaluation bound exhausted");
                bool continuation = true;
                switch (c.w0 >> 24) {
                case 0x01: {
                    const auto n = (c.w0 >> 12) & 255, end = (c.w0 & 4095) >> 1;
                    require(n && end >= n && end <= 32 && (c.w0 & 0xfff00fffu) == (0x01000000u | end * 2),
                            "Malformed model vertex cache range");
                    const auto wire = reader.view(c.w1, n * 16, 8);
                    const auto old = std::find_if(out.vertices_.begin(), out.vertices_.end(), [&](const auto& v) { return v.command_identity == c.address; });
                    if (old == out.vertices_.end()) {
                        out.vertices_.push_back({wire, c.address, static_cast<std::uint8_t>(end - n), static_cast<std::uint8_t>(n)});
                        out.installation_.push_back(wire);
                    }
                    break;
                }
                case 0x03: {
                    const auto first = c.w0 & 0xffff, last = c.w1;
                    require((c.w0 & 0xffff0000u) == 0x03000000u && !((first | last) & 1) && first <= last && last <= 62,
                            "Malformed CULLDL range");
                    append(exits, state); // Early return uses state AT this instruction.
                    break;
                }
                case 0xe1: require(c.w0 == 0xe1000000u, "Malformed RDPHALF_1"); state.latch = c.w1; state.latch_known = true; break;
                case 0x04: {
                    const auto five = (c.w0 >> 12) & 4095, two = c.w0 & 4095;
                    require(!(two & 1) && two / 2 < 32 && five == (two / 2) * 5, "Malformed BRANCH_Z vertex encodings");
                    if (state.latch_known) append(exits, execute(state.latch, state));
                    else obligation(Parameter::branch_target, c);
                    // Taken is NOPUSH. False alone continues through this list.
                    break;
                }
                case 0xde: {
                    require(c.w0 == 0xde000000u || c.w0 == 0xde010000u, "Malformed G_DL");
                    if ((c.w1 >> 24) == 0x0e) {
                        const auto returned = material(c, state);
                        if (c.w0 == 0xde010000u) append(exits, returned); else append(next, returned);
                    } else {
                        const auto& returned = execute(c.w1, state);
                        if (c.w0 == 0xde010000u) append(exits, returned); else append(next, returned);
                    }
                    continuation = false; break;
                }
                case 0xdf:
                    require(c.w0 == 0xdf000000u && !c.w1, "Malformed ENDDL");
                    append(exits, state); continuation = false; break;
                case 0xfd:
                    require(((c.w0 >> 21) & 7) <= 4 && !(c.w0 & 0x0007f000u), "Malformed SETTIMG");
                    reader.view(c.w1, 1, 8);
                    state.source = Source::direct; state.identity = c.w1;
                    state.format = static_cast<std::uint8_t>((c.w0 >> 21) & 7);
                    state.size = static_cast<std::uint8_t>((c.w0 >> 19) & 3);
                    state.width = static_cast<std::uint16_t>((c.w0 & 4095) + 1); break;
                case 0xf0: case 0xf3: case 0xf4: load(state, c); break;
                case 0xdb:
                    require((c.w0 & 0xffff0000u) == 0xdb0a0000u &&
                            ((c.w0 & 65535) == 0 || (c.w0 & 65535) == 4 || (c.w0 & 65535) == 24 || (c.w0 & 65535) == 28),
                            "Unsupported address-bearing MOVEWORD"); break;
                case 0x02: case 0x05: case 0x06: case 0xd7: case 0xd9:
                case 0xe2: case 0xe3: case 0xe6: case 0xe7: case 0xe8:
                case 0xf2: case 0xf5: case 0xf9: case 0xfa: case 0xfb: case 0xfc: break;
                default: throw std::runtime_error("Unsupported model resource opcode at " + std::to_string(c.address));
                }
                if (continuation) append(next, state);
            }
            states.swap(next);
            if (states.empty()) break;
        }
        active.pop_back();
        return cache.emplace(key, std::move(exits)).first->second;
    }
    States root(std::uint32_t address, const States& entries) {
        States exits;
        for (const auto& entry : entries) append(exits, execute(address, entry));
        return exits;
    }
    States node(const ModelPlan& plan, std::size_t index, States entries) {
        const auto& model = *plan.model;
        const auto& n = model.nodes[index];
        const auto domain = plan.visibility_domains.empty() ? 1u : plan.visibility_domains[index];
        require(domain && !(domain & ~15u), "Invalid node visibility domain");
        States exits;
        for (std::uint32_t flags = 0; flags < 4; ++flags) {
            if (!(domain & (1u << flags))) continue;
            auto states = !(domain & (domain - 1)) ? std::move(entries) : entries;
            if (!(flags & 2)) {
                if (!(flags & 1)) for (const auto& p : n.payloads) {
                    require(p.display_list_id < 4, "Invalid model payload lane");
                    if (p.display_list_id != lane) continue;
                    // J only emits pre when draw is present; I permits pre-only.
                    const bool j = model.render_type == menu::RenderType::j_fogged;
                    if (p.pre_list && (!j || p.draw_list)) states = root(p.pre_list, states);
                    if (p.draw_list) {
                        // renLoadTextures returns without rebinding when mobjList is NULL.
                        if (!n.materials.empty()) for (auto& state : states) state.binding = &n.materials;
                        states = root(p.draw_list, states);
                    }
                }
                for (std::size_t child = index + 1; child < model.nodes.size(); ++child)
                    if (model.nodes[child].parent == index) states = node(plan, child, std::move(states));
            }
            append(exits, states);
        }
        return exits;
    }
    void model(const ModelPlan& plan) {
        require(plan.model && (plan.visibility_domains.empty() || plan.visibility_domains.size() == plan.model->nodes.size()),
                "Invalid model closure plan");
        const auto type = plan.model->render_type;
        require(type == menu::RenderType::a || type == menu::RenderType::b || type == menu::RenderType::b_fogged || type == menu::RenderType::a_fogged ||
                type == menu::RenderType::d_fogged || type == menu::RenderType::i_fogged ||
                type == menu::RenderType::i_no_fog || type == menu::RenderType::j_fogged,
                "Unsupported model resource render selector");
        require((type != menu::RenderType::a && type != menu::RenderType::a_fogged) || plan.model->nodes.size() == 1,
                "TypeA resource renderer consumes exactly one root");
        for (std::size_t i = 0; i < plan.model->nodes.size(); ++i) {
            const auto parent = plan.model->nodes[i].parent;
            require(parent == menu::no_index || parent < i, "Invalid resource model parent order");
        }
        // Source gtl combine order. Symbolic entries are deliberately independent:
        // no assumption that this model's lane0 immediately precedes its lane1.
        for (const auto selected : {0u, 2u, 1u, 3u}) {
            lane = selected; cache.clear();
            States states{State{}};
            for (std::size_t i = 0; i < plan.model->nodes.size(); ++i)
                if (plan.model->nodes[i].parent == menu::no_index) states = node(plan, i, std::move(states));
        }
    }
    void merge() {
        auto& spans = out.installation_;
        std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) { return a.guest_address < b.guest_address; });
        std::size_t count = 0;
        for (const auto& span : spans) {
            if (count) {
                auto& last = spans[count - 1];
                const auto end = static_cast<std::uint64_t>(last.guest_address) + last.bytes.size();
                if (span.guest_address <= end) {
                    const auto next = static_cast<std::uint64_t>(span.guest_address) + span.bytes.size();
                    if (next <= end) continue;
                    const auto same_mapping = std::any_of(reader.mappings_.begin(), reader.mappings_.end(), [&](const auto& mapping) {
                        return last.guest_address >= mapping.guest_begin &&
                               next <= static_cast<std::uint64_t>(mapping.guest_begin) + mapping.bytes;
                    });
                    if (same_mapping) {
                        last = reader.view(last.guest_address, static_cast<std::size_t>(next - last.guest_address), 1);
                        continue;
                    }
                    require(span.guest_address == end, "Overlapping installation mappings disagree");
                }
            }
            spans[count++] = span;
        }
        spans.resize(count);
    }
};

Closure::Closure(const Reader& reader, std::span<const ModelPlan> plans, std::span<const menu::Material> materials,
                 std::span<const std::uint32_t> extra_roots, std::span<const rom::GuestView> extra_installation, Limits limits) {
    require(limits.commands && limits.evaluations && limits.alternatives && limits.call_depth && limits.obligations,
            "Zero model closure bound");
    Builder builder{*this, reader, materials, limits};
    for (std::size_t i = 0; i < plans.size(); ++i) { builder.model_index = i; builder.model(plans[i]); }
    builder.model_index = menu::no_index; builder.lane = 0;
    for (const auto root : extra_roots) { builder.cache.clear(); builder.execute(root, State{}); }
    for (const auto& span : extra_installation) {
        const auto actual = reader.view(span.guest_address, span.bytes.size(), 1);
        require(actual.rom_offset == span.rom_offset && actual.bytes.data() == span.bytes.data(), "Invalid extra installation provenance");
        installation_.push_back(span);
    }
    if (obligations_.empty()) builder.merge();
    else installation_.clear(); // Partial demand diagnostics are NOT an installation plan.
}
void Closure::require_complete() const {
    if (!obligations_.empty()) {
        const auto& first = obligations_.front();
        throw std::runtime_error("Unresolved model resource entry parameter at " + std::to_string(first.command_identity) +
                                 " model " + std::to_string(first.model_index) + " lane " + std::to_string(first.lane));
    }
}
std::span<const rom::GuestView> Closure::installation_spans() const { require_complete(); return installation_; }
} // namespace isle::model_assets
