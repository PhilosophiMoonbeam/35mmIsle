#include "menu_animation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

// GAME semantic authority df8815e75f3d79618466e01e0827c6165b93d11c:
// sys/anim.c:106-214,217-1200,1270-1347,1617-1691,2016-2366;
// sys/om.c:965-1093; app_level/pokemon.c:320-400.
// Original native implementation. No GAME/SDK object layout or producer import.
// Compile without fast-math and with FP contraction disabled: binary32 expression
// grouping differs intentionally between model/material and camera cubic paths.
namespace isle::menu::animation {
namespace {
struct Invalid { Status status; };
void require(bool ok, Error error, std::uint32_t address = 0) {
    if (!ok) throw Invalid{{error, address}};
}
struct Track {
    unsigned kind = 0;
    float inv_duration = 1, time = 0, initial = 0, target = 0, rate = 0, target_rate = 0;
    std::uint32_t initial_word = 0, target_word = 0; // Packed RGBA, never float arithmetic.
    std::size_t path = no_index;
};
struct Command {
    std::uint8_t opcode = 0;
    std::uint16_t mask = 0, duration = 0;
    std::uint32_t address = 0;
    std::array<std::uint32_t, 20> operands{};
    std::size_t path = no_index;
};
struct Program {
    AnimationKind kind{};
    std::vector<Command> commands;
    std::size_t callbacks = 0;
};
struct Player {
    std::size_t program = no_index, cursor = 0;
    std::array<Track, 15> tracks{};
};
bool same(float a, float b) noexcept { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
bool same(Vec3 a, Vec3 b) noexcept { return same(a.x,b.x) && same(a.y,b.y) && same(a.z,b.z); }
float value(const Track& a, bool camera) noexcept {
    if (a.kind == 2) return a.initial + a.time * a.rate;
    if (a.kind == 1) return a.inv_duration <= a.time ? a.target : a.initial;
    if (camera) { // animGetAObjInterpValue, not model updater's rearrangement.
        const float t2 = a.time * a.time;
        const float t18 = a.inv_duration * a.inv_duration;
        const float t16 = t2 * a.time * t18;
        const float t10 = 2.0f * t16 * a.inv_duration;
        const float s14 = 3.0f * t2 * t18;
        const float s18 = t2 * a.inv_duration;
        const float s10 = t16 - s18;
        return ((t10-s14)+1.0f)*a.initial + a.target*(s14-t10) +
               a.rate*((s10-s18)+a.time) + a.target_rate*s10;
    }
    const float t16 = a.inv_duration * a.inv_duration;
    const float t12 = a.time * a.time;
    const float t18 = a.inv_duration * t12;
    const float t14 = a.time * t12 * t16;
    const float t20 = 2.0f * t14 * a.inv_duration;
    const float t22 = 3.0f * t12 * t16;
    const float t24 = t14 - t18;
    return a.initial*((t20-t22)+1.0f) + a.target*(t22-t20) +
           a.rate*((t24-t18)+a.time) + a.target_rate*t24;
}
std::uint32_t color(const Track& a) noexcept {
    const auto initial = a.initial_word;
    const auto target = a.target_word;
    if (a.kind == 1) return a.inv_duration <= a.time ? target : initial;
    // GAME's paired-byte ColorPack arithmetic is per-channel 8.8 interpolation,
    // with truncation before the documented [0,256] clamp (not float RGBA lerp).
    const float weight = a.time * a.inv_duration * 256.0f;
    const int interp = weight < 0 ? 0 : weight > 256 ? 256 : static_cast<int>(weight);
    std::uint32_t result = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        const auto x = (initial >> shift) & 255u, y = (target >> shift) & 255u;
        result |= ((x * static_cast<unsigned>(256-interp) + y * static_cast<unsigned>(interp)) >> 8) << shift;
    }
    return result;
}
Program compile(const Animation& input, std::span<const AnimationPath> paths) {
    Program output;
    output.kind = input.kind;
    require(input.kind == AnimationKind::model || input.kind == AnimationKind::texture ||
            input.kind == AnimationKind::camera, Error::malformed, input.wire.guest_address);
    require(!input.commands.empty(), Error::malformed, input.wire.guest_address);
    std::size_t offset = 0;
    for (std::size_t i = 0; i < input.commands.size(); ++i) {
        const auto& source = input.commands[i];
        const auto address = source.wire.guest_address;
        require(source.wire.bytes.size() >= 4 && source.wire.bytes.size()%4 == 0, Error::extent, address);
        require(offset <= input.wire.bytes.size() && source.wire.bytes.size() <= input.wire.bytes.size()-offset,
                Error::extent, address);
        require(source.wire.bytes.data() == input.wire.bytes.data()+offset &&
                address == input.wire.guest_address+offset, Error::malformed, address);
        const auto word = source.wire.u32(0);
        Command c;
        c.opcode = static_cast<std::uint8_t>(word >> 25);
        c.mask = static_cast<std::uint16_t>((word >> 15) & 0x3ffu);
        c.duration = static_cast<std::uint16_t>(word & 0x7fffu);
        c.address = address;
        require(c.opcode == source.opcode && c.mask == source.mask && c.duration == source.duration,
                Error::malformed, address);
        const bool basic = c.opcode >= 3 && c.opcode <= 11;
        const bool extra = c.opcode >= 18 && c.opcode <= 21;
        const bool model = input.kind == AnimationKind::model;
        const bool texture = input.kind == AnimationKind::texture;
        require(c.opcode == 0 || c.opcode == 2 || basic ||
                (model && (c.opcode == 12 || c.opcode == 13 || c.opcode == 15 || c.opcode == 16 || c.opcode == 17)) ||
                (texture && extra), Error::unsupported, address);
        require(!extra || !(c.mask & ~0x1fu), Error::unsupported, address);
        if (basic && input.kind == AnimationKind::camera)
            require(!(c.mask & 0x88u), Error::unsupported, address);
        if (c.opcode == 13) {
            require(source.path_index < paths.size(), Error::extent, address);
            c.path = source.path_index;
        }
        const unsigned count = (basic || extra || c.opcode == 17) ?
            static_cast<unsigned>(std::popcount(static_cast<unsigned>(c.mask))) : 0;
        const unsigned operands = c.opcode == 13 ? 1 :
            count * ((c.opcode == 5 || c.opcode == 6) ? 2 : 1);
        require(source.wire.bytes.size() == 4u+4u*operands, Error::malformed, address);
        for (unsigned j = 0; j < operands; ++j) {
            c.operands[j] = source.wire.u32(4u+4u*j);
            if (!extra && c.opcode != 13)
                require(std::isfinite(std::bit_cast<float>(c.operands[j])), Error::malformed, address);
        }
        if (c.opcode == 0) require(word == 0 && i+1 == input.commands.size(), Error::malformed, address);
        else require(i+1 != input.commands.size(), Error::malformed, address);
        if (c.opcode == 16) ++output.callbacks;
        if (c.opcode == 17) output.callbacks += count;
        output.commands.push_back(c);
        offset += source.wire.bytes.size();
    }
    require(offset == input.wire.bytes.size(), Error::extent, input.wire.guest_address);
    return output;
}
void restart(Player& p, Clock& clock, float skip) noexcept {
    for (auto& a : p.tracks) a.kind = 0; // animSet retains previous targets/rates.
    p.cursor = 0;
    clock.time_left = 0;
    clock.time_passed = skip;
    clock.phase = p.program == no_index ? Phase::disabled : Phase::changed;
}
} // namespace

struct Runtime::Impl {
    struct ModelStorage {
        std::vector<NodeState> nodes;
        std::vector<MaterialState> materials;
        std::vector<Player> node_players, material_players;
        float animation_time = 0;
        std::uint32_t flags = 0;
    };
    Resources resources;
    CameraState initial_camera, camera;
    Player camera_player;
    std::vector<Program> programs;
    std::vector<ModelStorage> models;
    std::vector<CallbackEvent> event_storage;
    std::size_t event_count = 0;
    std::uint64_t generation = 1, event_order = 0;
    bool retired = false;

    void emit(std::size_t model, std::size_t node, int id, float argument, std::uint32_t address) noexcept {
        event_storage[event_count++] = {model,node,id,argument,address,event_order++};
    }
    void process(Player& p, Clock& clock, std::size_t mi, std::size_t ni, bool receiver,
                 std::uint32_t* node_flags, float* animation_time) noexcept {
        if (clock.phase == Phase::disabled) return;
        if (clock.phase == Phase::changed) {
            clock.time_left = -clock.time_passed;
            clock.phase = Phase::running;
        } else {
            clock.time_left -= clock.speed;
            clock.time_passed += clock.speed;
            if (animation_time) *animation_time = clock.time_passed;
            if (clock.time_left > 0) return;
        }
        const auto& program = programs[p.program];
        do {
            const auto& c = program.commands[p.cursor++];
            const float duration = static_cast<float>(c.duration);
            if (c.opcode == 0) {
                for (auto& a : p.tracks) if (a.kind) a.time += clock.speed + clock.time_left;
                if (program.kind == AnimationKind::camera) clock.time_passed = clock.time_left;
                if (animation_time && program.kind == AnimationKind::model) *animation_time = clock.time_passed;
                clock.phase = Phase::finished;
                if (receiver && resources.models[mi].receives_effect_callback) emit(mi,ni,-1,0,c.address);
                return;
            }
            if (c.opcode == 2 || c.opcode == 15 || c.opcode == 16) {
                if (c.opcode == 15) *node_flags = c.mask;
                if (c.opcode == 16 && resources.models[mi].receives_effect_callback)
                    emit(mi,ni,c.mask >> 8,static_cast<float>(c.mask & 255u),c.address);
                clock.time_left += duration;
                continue;
            }
            if (c.opcode == 12) {
                for (unsigned channel = 0; channel < 10; ++channel)
                    if (c.mask & (1u << channel)) p.tracks[channel].time += duration;
                continue;
            }
            if (c.opcode == 13) { p.tracks[3].path = c.path; continue; }
            if (c.opcode == 17) {
                unsigned operand = 0;
                for (unsigned channel = 0; channel < 10; ++channel) {
                    if (!(c.mask & (1u << channel))) continue;
                    const float argument = std::bit_cast<float>(c.operands[operand++]);
                    if (resources.models[mi].receives_effect_callback) emit(mi,ni,static_cast<int>(channel+4),argument,c.address);
                }
                clock.time_left += duration;
                continue;
            }
            const bool extra = c.opcode >= 18;
            unsigned operand = 0;
            for (unsigned channel = 0; channel < (extra ? 5u : 10u); ++channel) {
                if (!(c.mask & (1u << channel))) continue;
                auto& a = p.tracks[channel + (extra ? 10 : 0)];
                const auto word = c.operands[operand++];
                if (extra) {
                    a.initial_word = a.target_word;
                    a.target_word = word;
                } else {
                    const float next = std::bit_cast<float>(word);
                    if (c.opcode == 7) { a.target_rate = next; continue; }
                    a.initial = a.target;
                    a.target = next;
                }
                if (c.opcode == 3 || c.opcode == 4) {
                    a.kind = 2;
                    if (duration != 0) a.rate = (a.target-a.initial)/duration;
                    a.target_rate = 0;
                } else if (c.opcode == 10 || c.opcode == 11 || c.opcode == 18 || c.opcode == 19) {
                    a.kind = 1;
                    a.inv_duration = duration;
                    if (!extra) a.target_rate = 0;
                } else if (extra) {
                    a.kind = 2;
                    if (duration != 0) a.inv_duration = 1.0f/duration;
                } else {
                    a.kind = 3;
                    a.rate = a.target_rate;
                    a.target_rate = (c.opcode == 5 || c.opcode == 6) ?
                        std::bit_cast<float>(c.operands[operand++]) : 0.0f;
                    if (duration != 0) a.inv_duration = 1.0f/duration;
                }
                a.time = -clock.time_left-clock.speed;
            }
            if (c.opcode == 3 || c.opcode == 5 || c.opcode == 8 || c.opcode == 10 ||
                c.opcode == 18 || c.opcode == 20) clock.time_left += duration;
        } while (clock.time_left <= 0);
    }
    static void advance(Player& p, const Clock& clock) noexcept {
        if (clock.phase != Phase::finished)
            for (auto& a : p.tracks) if (a.kind) a.time += clock.speed;
    }
    void restore_material(MaterialState& state, Player& player, std::size_t resource,
                          float speed, float palette) noexcept {
        const auto revision = state.revision+1;
        const auto& material = resources.materials[resource];
        state = {};
        state.resource = resource;
        state.revision = revision;
        state.palette_index = palette;
        state.parameters = material.parameters;
        state.parameters[4] = material.parameters[0];
        state.parameters[5] = material.parameters[2];
        state.color_and_trailing_words = material.color_and_trailing_words;
        state.lod_level = static_cast<float>(material.color_and_trailing_words[1] >> 24)/255.0f;
        // Validated before publication; no throwing wire access in mutations.
        const auto b = material.wire.bytes;
        state.word_4c = (std::to_integer<std::uint32_t>(b[0x4c]) << 24) |
            (std::to_integer<std::uint32_t>(b[0x4d]) << 16) |
            (std::to_integer<std::uint32_t>(b[0x4e]) << 8) | std::to_integer<std::uint32_t>(b[0x4f]);
        state.clock.speed = speed;
        player = {};
    }
    void initialize(float skip) noexcept {
        event_count = 0;
        event_order = 0;
        camera = initial_camera;
        camera.view_dirty = camera.projection_dirty = true;
        camera.written_channels = 0;
        camera.clock.speed = 0.5f;
        camera_player = {};
        camera_player.program = resources.camera_animation;
        restart(camera_player,camera.clock,skip);
        for (std::size_t mi = 0; mi < models.size(); ++mi) {
            auto& m = models[mi];
            const auto& source = resources.models[mi];
            m.animation_time = skip;
            m.flags = 0;
            bool receiver = true;
            for (std::size_t ni = 0; ni < m.nodes.size(); ++ni) {
                const auto& node = source.nodes[ni];
                auto& n = m.nodes[ni];
                n.transform = {node.position,node.rotation,node.scale};
                n.parent = node.parent;
                n.flags = 0;
                ++n.revision;
                n.callback_receiver = node.animation != no_index && receiver;
                if (node.animation != no_index) receiver = false;
                n.clock.speed = source.animation_speed;
                auto& p = m.node_players[ni];
                p = {};
                p.program = node.animation;
                restart(p,n.clock,skip);
                for (std::size_t j = 0; j < n.material_count; ++j) {
                    const auto index = n.first_material+j;
                    auto& state = m.materials[index];
                    auto& mp = m.material_players[index];
                    restore_material(state,mp,node.materials[j],source.animation_speed,0);
                    mp.program = node.material_animations.empty() ? no_index : node.material_animations[j];
                    restart(mp,state.clock,skip);
                }
            }
        }
    }
};

Runtime::Runtime(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Runtime::~Runtime() = default;
CreateResult Runtime::create(const Catalog& catalog, CameraState camera) noexcept {
    try { return create({catalog.models(),catalog.materials(),catalog.animations(),catalog.camera_animation()},camera); }
    catch (...) { return {{Error::malformed,0},nullptr}; }
}
CreateResult Runtime::create(Resources resources, CameraState camera) noexcept {
    try {
        require(std::isfinite(camera.eye.x) && std::isfinite(camera.eye.y) && std::isfinite(camera.eye.z) &&
                std::isfinite(camera.at.x) && std::isfinite(camera.at.y) && std::isfinite(camera.at.z) &&
                std::isfinite(camera.roll) && std::isfinite(camera.fov_y), Error::malformed);
        auto impl = std::make_unique<Impl>();
        impl->resources = resources;
        impl->initial_camera = camera;
        for (const auto& input : resources.animations) impl->programs.push_back(compile(input,resources.paths));
        auto check_program = [&](std::size_t index, AnimationKind kind) {
            if (index != no_index) require(index < impl->programs.size() && impl->programs[index].kind == kind, Error::malformed);
        };
        check_program(resources.camera_animation,AnimationKind::camera);
        std::size_t max_callbacks = 0;
        for (const auto& program : impl->programs)
            if (program.kind == AnimationKind::model) max_callbacks = std::max(max_callbacks,program.callbacks);
        impl->models.resize(resources.models.size());
        std::size_t event_capacity = 0;
        for (std::size_t mi = 0; mi < resources.models.size(); ++mi) {
            const auto& source = resources.models[mi];
            auto& target = impl->models[mi];
            require(std::isfinite(source.animation_speed) && source.animation_speed >= 0, Error::malformed);
            target.nodes.resize(source.nodes.size());
            target.node_players.resize(source.nodes.size());
            std::size_t materials = 0;
            for (std::size_t ni = 0; ni < source.nodes.size(); ++ni) {
                const auto& node = source.nodes[ni];
                require(node.parent == no_index || node.parent < ni, Error::malformed,node.wire.guest_address);
                // A pre-order array cannot return into a retired sibling subtree.
                if (node.parent != no_index && ni) {
                    auto ancestor = ni-1;
                    while (ancestor != no_index && ancestor != node.parent) ancestor = source.nodes[ancestor].parent;
                    require(ancestor == node.parent,Error::malformed,node.wire.guest_address);
                }
                require(std::isfinite(node.position.x) && std::isfinite(node.position.y) && std::isfinite(node.position.z) &&
                        std::isfinite(node.rotation.x) && std::isfinite(node.rotation.y) && std::isfinite(node.rotation.z) &&
                        std::isfinite(node.scale.x) && std::isfinite(node.scale.y) && std::isfinite(node.scale.z),Error::malformed);
                check_program(node.animation,AnimationKind::model);
                // Future bindings may select any precompiled model program.
                require(node.material_animations.empty() || node.material_animations.size() == node.materials.size(),Error::malformed);
                target.nodes[ni].first_material = materials;
                target.nodes[ni].material_count = node.materials.size();
                materials += node.materials.size();
                for (std::size_t j = 0; j < node.materials.size(); ++j) {
                    require(node.materials[j] < resources.materials.size(), Error::extent);
                    require(resources.materials[node.materials[j]].wire.bytes.size() >= 0x78,Error::extent);
                    if (!node.material_animations.empty()) check_program(node.material_animations[j],AnimationKind::texture);
                }
            }
            target.materials.resize(materials);
            target.material_players.resize(materials);
            if (source.receives_effect_callback && !source.nodes.empty()) {
                require(max_callbacks <= (std::numeric_limits<std::size_t>::max()-1)/source.nodes.size(),Error::extent);
                event_capacity = std::max(event_capacity,max_callbacks*source.nodes.size()+1);
            }
        }
        // Every node may bind the most callback-rich program; only one receives
        // END. Reserve for arbitrary later bindings, not only initial streams.
        impl->event_storage.resize(event_capacity);
        impl->initialize(0);
        return {{},std::unique_ptr<Runtime>(new Runtime(std::move(impl)))};
    } catch (const Invalid& e) { return {e.status,nullptr}; }
      catch (const std::bad_alloc&) { return {{Error::allocation,0},nullptr}; }
      catch (...) { return {{Error::malformed,0},nullptr}; }
}

Status Runtime::step_model(std::size_t index) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    auto& model = s.models[index];
    s.event_count = 0;
    ++s.generation;
    for (std::size_t ni = 0; ni < model.nodes.size(); ++ni) {
        auto& n = model.nodes[ni];
        auto& p = model.node_players[ni];
        bool changed = false;
        const auto flags = n.flags;
        s.process(p,n.clock,index,ni,n.callback_receiver,&n.flags,&model.animation_time);
        if (n.clock.phase != Phase::disabled) {
            Impl::advance(p,n.clock);
            if (!(model.flags & 2u)) for (unsigned j = 0; j < 10; ++j) {
                const auto& a = p.tracks[j];
                if (!a.kind) continue;
                const float v = value(a,false);
                float* field = nullptr;
                switch (j) {
                case 0: field = &n.transform.rotation.x; break;
                case 1: field = &n.transform.rotation.y; break;
                case 2: field = &n.transform.rotation.z; break;
                case 4: field = &n.transform.position.x; break;
                case 3: {
                    if (a.path >= s.resources.paths.size()) return {Error::extent,0};
                    Vec3 position;
                    try { position = path_position(s.resources.paths[a.path],std::clamp(v,0.0f,1.0f)); }
                    catch (...) { return {Error::malformed,0}; }
                    changed |= !same(n.transform.position,position);
                    n.transform.position = position;
                    break;
                }
                case 5: field = &n.transform.position.y; break;
                case 6: field = &n.transform.position.z; break;
                case 7: field = &n.transform.scale.x; break;
                case 8: field = &n.transform.scale.y; break;
                case 9: field = &n.transform.scale.z; break;
                }
                if (field) { changed |= !same(*field,v); *field = v; }
            }
            if (n.clock.phase == Phase::finished) n.clock.phase = Phase::disabled;
        }
        if (changed || flags != n.flags) ++n.revision;
        for (std::size_t j = 0; j < n.material_count; ++j) {
            const auto k = n.first_material+j;
            auto& m = model.materials[k];
            auto& mp = model.material_players[k];
            s.process(mp,m.clock,index,ni,false,nullptr,nullptr);
            if (m.clock.phase == Phase::disabled) continue;
            Impl::advance(mp,m.clock);
            bool material_changed = false;
            for (unsigned c = 0; c < 10; ++c) {
                if (!mp.tracks[c].kind) continue;
                const float v = value(mp.tracks[c],false);
                if (c == 0 || c == 5) {
                    auto& image = c == 0 ? m.image_index : m.next_image_index;
                    const auto next = static_cast<std::uint16_t>(v);
                    material_changed |= image != next;
                    image = next;
                    continue;
                }
                float* field = nullptr;
                switch (c) {
                case 1: field = &m.parameters[0]; break;
                case 2: field = &m.parameters[1]; break;
                case 3: field = &m.parameters[2]; break;
                case 4: field = &m.parameters[3]; break;
                case 6: field = &m.parameters[6]; break;
                case 7: field = &m.parameters[7]; break;
                case 8: field = &m.lod_level; break;
                case 9: field = &m.palette_index; break;
                }
                material_changed |= !same(*field,v);
                *field = v;
            }
            for (unsigned c = 0; c < 5; ++c) {
                if (!mp.tracks[10+c].kind) continue;
                const auto v = color(mp.tracks[10+c]);
                // prim=50, env=58, blend=5c, lights=60/64; 54 is lod seed.
                const unsigned word = c == 0 ? 0 : c+1;
                material_changed |= m.color_and_trailing_words[word] != v;
                m.color_and_trailing_words[word] = v;
            }
            if (material_changed) ++m.revision;
            if (m.clock.phase == Phase::finished) m.clock.phase = Phase::disabled;
        }
    }
    return {};
}
Status Runtime::step_camera(std::uint32_t flags) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    s.event_count = 0;
    ++s.generation;
    auto& c = s.camera;
    const auto eye = c.eye, at = c.at;
    const auto roll = c.roll, fov = c.fov_y;
    const bool initial = c.clock.phase == Phase::changed;
    c.view_dirty = c.projection_dirty = false;
    c.written_channels = 0;
    s.process(s.camera_player,c.clock,0,0,false,nullptr,nullptr);
    if (c.clock.phase != Phase::disabled) {
        Impl::advance(s.camera_player,c.clock);
        if (!(flags & 2u)) for (unsigned j = 0; j < 10; ++j) {
            const auto& a = s.camera_player.tracks[j];
            if (!a.kind) continue;
            const float v = value(a,true);
            c.written_channels |= static_cast<std::uint16_t>(1u << j);
            switch (j) {
            case 0: c.eye.x = v; break;
            case 1: c.eye.y = v; break;
            case 2: c.eye.z = v; break;
            case 4: c.at.x = v; break;
            case 5: c.at.y = v; break;
            case 6: c.at.z = v; break;
            case 8: c.roll = v; break;
            case 9: c.fov_y = v; break;
            }
        }
        if (c.clock.phase == Phase::finished) c.clock.phase = Phase::disabled;
    }
    c.view_dirty = initial || !same(eye,c.eye) || !same(at,c.at) || !same(roll,c.roll);
    c.projection_dirty = initial || !same(fov,c.fov_y);
    return {};
}
ModelState Runtime::model(std::size_t index) const noexcept {
    if (index >= impl_->models.size() || impl_->retired) return {};
    const auto& m = impl_->models[index];
    return {m.nodes,m.materials,m.animation_time,m.flags};
}
const CameraState& Runtime::camera() const noexcept { return impl_->camera; }
std::span<const CallbackEvent> Runtime::events() const noexcept { return {impl_->event_storage.data(),impl_->event_count}; }
std::size_t Runtime::model_count() const noexcept { return impl_->models.size(); }
std::uint64_t Runtime::generation() const noexcept { return impl_->generation; }
Status Runtime::set_object_flags(std::size_t index, std::uint32_t flags) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (index >= impl_->models.size()) return {Error::extent,0};
    impl_->models[index].flags = flags;
    return {};
}
Status Runtime::restart_model(std::size_t index, float skip) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    auto& m = s.models[index];
    for (std::size_t i = 0; i < m.nodes.size(); ++i) restart(m.node_players[i],m.nodes[i].clock,skip);
    for (std::size_t i = 0; i < m.materials.size(); ++i) {
        // animSetModelTreeTextureAnimation leaves null entries untouched.
        if (m.material_players[i].program != no_index) restart(m.material_players[i],m.materials[i].clock,skip);
    }
    m.animation_time = skip;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::bind_model(std::size_t index, std::span<const std::size_t> bindings,
                           float skip, ModelBindPose pose) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    auto& m = s.models[index];
    if (bindings.size() != m.nodes.size()) return {Error::extent,0};
    if (!std::isfinite(skip) || (pose != ModelBindPose::preserve && pose != ModelBindPose::restore))
        return {Error::malformed,0};
    for (const auto program : bindings)
        if (program != no_index && (program >= s.programs.size() || s.programs[program].kind != AnimationKind::model))
            return {Error::malformed,0};
    bool receiver = true;
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        auto& n = m.nodes[i];
        if (bindings[i] != no_index) {
            auto& p = m.node_players[i];
            p.program = bindings[i];
            restart(p,n.clock,skip);
            n.callback_receiver = receiver;
            receiver = false;
        } else {
            // Source null entries disable only; the old AObjs/list/time remain.
            n.clock.phase = Phase::disabled;
            n.callback_receiver = false;
        }
        if (pose == ModelBindPose::restore) {
            const auto& source = s.resources.models[index].nodes[i];
            const Transform bind{source.position,source.rotation,source.scale};
            if (!same(n.transform.position,bind.position) || !same(n.transform.rotation,bind.rotation) ||
                !same(n.transform.scale,bind.scale)) ++n.revision;
            n.transform = bind;
        }
    }
    m.animation_time = skip;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::bind_materials(std::size_t index, std::span<const std::size_t> bindings, float skip) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    auto& m = s.models[index];
    if (!bindings.empty() && bindings.size() != m.materials.size()) return {Error::extent,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    for (const auto program : bindings)
        if (program != no_index && (program >= s.programs.size() || s.programs[program].kind != AnimationKind::texture))
            return {Error::malformed,0};
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i] == no_index) continue;
        auto& p = m.material_players[i];
        p.program = bindings[i];
        restart(p,m.materials[i].clock,skip);
    }
    m.animation_time = skip;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::reset_materials(std::size_t index, std::span<const std::size_t> bindings) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    auto& m = s.models[index];
    if (!bindings.empty() && bindings.size() != m.materials.size()) return {Error::extent,0};
    for (const auto resource : bindings) {
        if (resource == no_index) continue;
        if (resource >= s.resources.materials.size()) return {Error::extent,0};
        if (s.resources.materials[resource].wire.bytes.size() < 0x78) return {Error::extent,0};
    }
    for (std::size_t ni = 0; ni < m.nodes.size(); ++ni) {
        const auto& n = m.nodes[ni];
        for (std::size_t j = 0; j < n.material_count; ++j) {
            const auto k = n.first_material+j;
            const auto resource = bindings.empty() ? s.resources.models[index].nodes[ni].materials[j] : bindings[k];
            if (resource == no_index) continue;
            auto& state = m.materials[k];
            s.restore_material(state,m.material_players[k],resource,1,state.palette_index);
        }
    }
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::set_model_speed(std::size_t index, float speed) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    if (!std::isfinite(speed)) return {Error::malformed,0};
    for (auto& n : s.models[index].nodes) n.clock.speed = speed;
    return {};
}
Status Runtime::set_texture_speed(std::size_t index, float speed) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (index >= s.models.size()) return {Error::extent,0};
    if (!std::isfinite(speed)) return {Error::malformed,0};
    for (auto& m : s.models[index].materials) m.clock.speed = speed;
    return {};
}
Status Runtime::restart_camera(float skip) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    restart(impl_->camera_player,impl_->camera.clock,skip);
    impl_->event_count = 0;
    ++impl_->generation;
    return {};
}
Status Runtime::reset(float skip) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    impl_->initialize(skip);
    ++impl_->generation;
    return {};
}
void Runtime::retire() noexcept {
    impl_->retired = true;
    impl_->event_count = 0;
    ++impl_->generation;
}
} // namespace isle::menu::animation
