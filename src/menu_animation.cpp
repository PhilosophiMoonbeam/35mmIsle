#include "menu_animation.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

// GAME semantic authority df8815e75f3d79618466e01e0827c6165b93d11c:
// sys/anim.c:106-214,217-1200,1270-1347,1617-1691,2016-2366;
// sys/om.c:965-1093; app_level/pokemon.c:320-400; app_level/4FE330.c:79-93.
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
    std::size_t camera_at_path = no_index;
    std::size_t target = no_index;
};
struct Program {
    AnimationKind kind{};
    std::vector<Command> commands;
    std::size_t callbacks = 0;
    bool cyclic = false;
    std::size_t cycle_entry = no_index;
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
bool adds_duration(std::uint8_t opcode) noexcept {
    return opcode == 2 || opcode == 3 || opcode == 5 || opcode == 8 ||
        opcode == 10 || opcode == 15 || opcode == 16 || opcode == 17 ||
        opcode == 18 || opcode == 20 || opcode == 23;
}
// Shared command semantics for runtime execution and cold domain certification.
// time is the AObj time before the updater adds this step's speed.
void assign_tracks(Player& player, const Command& command, float time) noexcept {
    const bool extra = command.opcode >= 18;
    const float duration = static_cast<float>(command.duration);
    unsigned operand = 0;
    for (unsigned channel = 0; channel < (extra ? 5u : 10u); ++channel) {
        if (!(command.mask & (1u << channel))) continue;
        auto& a = player.tracks[channel + (extra ? 10 : 0)];
        const auto word = command.operands[operand++];
        if (extra) {
            a.initial_word = a.target_word;
            a.target_word = word;
        } else {
            const float next = std::bit_cast<float>(word);
            if (command.opcode == 7) { a.target_rate = next; continue; }
            a.initial = a.target;
            a.target = next;
        }
        if (command.opcode == 3 || command.opcode == 4) {
            a.kind = 2;
            if (duration != 0) a.rate = (a.target-a.initial)/duration;
            a.target_rate = 0;
        } else if (command.opcode == 10 || command.opcode == 11 || command.opcode == 18 || command.opcode == 19) {
            a.kind = 1;
            a.inv_duration = duration;
            if (!extra) a.target_rate = 0;
        } else if (extra) {
            a.kind = 2;
            if (duration != 0) a.inv_duration = 1.0f/duration;
        } else {
            a.kind = 3;
            a.rate = a.target_rate;
            a.target_rate = (command.opcode == 5 || command.opcode == 6) ?
                std::bit_cast<float>(command.operands[operand++]) : 0.0f;
            if (duration != 0) a.inv_duration = 1.0f/duration;
        }
        a.time = time;
    }
}
Program compile(const Animation& input, std::span<const AnimationPath> paths) {
    Program output;
    output.kind = input.kind;
    require(input.kind == AnimationKind::model || input.kind == AnimationKind::texture ||
            input.kind == AnimationKind::camera, Error::malformed, input.wire.guest_address);
    require(!input.commands.empty(), Error::malformed, input.wire.guest_address);
    const auto fragments = input.fragments.empty() ?
        std::span<const rom::GuestView>(&input.wire,1) : std::span<const rom::GuestView>(input.fragments);
    require(fragments.front().guest_address == input.wire.guest_address &&
            fragments.front().bytes.data() == input.wire.bytes.data() &&
            fragments.front().bytes.size() == input.wire.bytes.size(),Error::malformed,input.wire.guest_address);
    std::size_t fragment = 0, offset = 0;
    for (std::size_t i = 0; i < input.commands.size(); ++i) {
        const auto& source = input.commands[i];
        const auto address = source.wire.guest_address;
        require(source.wire.bytes.size() >= 4 && source.wire.bytes.size()%4 == 0 &&
                source.wire.bytes.size() <= UINT32_MAX-address, Error::extent, address);
        if (fragment < fragments.size() && offset == fragments[fragment].bytes.size()) {
            ++fragment;
            offset = 0;
        }
        require(fragment < fragments.size(),Error::extent,address);
        const auto& extent = fragments[fragment];
        require(!extent.bytes.empty() && extent.bytes.size() <= UINT32_MAX-extent.guest_address &&
                offset <= extent.bytes.size() && source.wire.bytes.size() <= extent.bytes.size()-offset,
                Error::extent,address);
        require(source.wire.bytes.data() == extent.bytes.data()+offset &&
                address == extent.guest_address+offset,Error::malformed,address);
        for (std::size_t prior_index = 0; prior_index < output.commands.size(); ++prior_index) {
            const auto& prior = output.commands[prior_index];
            const auto prior_size = input.commands[prior_index].wire.bytes.size();
            require(address+source.wire.bytes.size() <= prior.address ||
                    prior.address+prior_size <= address,Error::malformed,address);
        }
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
        const bool camera = input.kind == AnimationKind::camera;
        const bool transfer = c.opcode == 1 || c.opcode == 14;
        require(c.opcode == 0 || c.opcode == 2 || basic || transfer || c.opcode == 12 ||
                (model && (c.opcode == 13 || c.opcode == 15 || c.opcode == 16 || c.opcode == 17)) ||
                (texture && (extra || c.opcode == 22)) || (camera && (c.opcode == 13 || c.opcode == 23)),
                Error::unsupported, address);
        require(!(extra || c.opcode == 22) || !(c.mask & ~0x1fu), Error::malformed, address);
        require(!transfer || (!c.mask && !c.duration),Error::malformed,address);
        require(c.opcode != 2 || !c.mask,Error::malformed,address);
        if (c.opcode == 13) {
            if (model || (c.mask & 8)) {
                require(source.path_index < paths.size(), Error::extent, address);
                c.path = source.path_index;
            }
            if (camera) {
                require(c.mask && !(c.mask & ~0x88u), Error::malformed, address);
                if (c.mask & 128) {
                    require(source.camera_at_path_index < paths.size(), Error::extent, address);
                    c.camera_at_path = source.camera_at_path_index;
                }
            }
        }
        const unsigned count = (basic || extra || c.opcode == 17 || c.opcode == 22) ?
            static_cast<unsigned>(std::popcount(static_cast<unsigned>(c.mask))) : 0;
        const unsigned operands = c.opcode == 13 ?
            (model ? 1u : static_cast<unsigned>(std::popcount(static_cast<unsigned>(c.mask)))) :
            transfer ? 1u : c.opcode == 23 ? 2u : count * ((c.opcode == 5 || c.opcode == 6) ? 2 : 1);
        require(source.wire.bytes.size() == 4u+4u*operands, Error::malformed, address);
        for (unsigned j = 0; j < operands; ++j) {
            c.operands[j] = source.wire.u32(4u+4u*j);
            if ((basic || c.opcode == 17))
                require(std::isfinite(std::bit_cast<float>(c.operands[j])), Error::malformed, address);
        }
        if (c.opcode == 0) require(word == 0, Error::malformed, address);
        if (c.opcode == 16 || transfer) ++output.callbacks;
        if (c.opcode == 17) output.callbacks += count;
        output.commands.push_back(c);
        offset += source.wire.bytes.size();
    }
    require(fragment+1 == fragments.size() && offset == fragments.back().bytes.size(),
            Error::extent,input.wire.guest_address);
    for (std::size_t i = 0; i < output.commands.size(); ++i) {
        auto& c = output.commands[i];
        if (!c.opcode) continue;
        const bool transfer = c.opcode == 1 || c.opcode == 14;
        const auto address = transfer ? c.operands[0] :
            c.address+static_cast<std::uint32_t>(input.commands[i].wire.bytes.size());
        if (transfer && !address) continue; // Original NULL list, not an END callback.
        const auto target = std::find_if(output.commands.begin(),output.commands.end(),
            [address](const auto& command) { return command.address == address; });
        require(target != output.commands.end(),Error::malformed,c.address);
        c.target = static_cast<std::size_t>(target-output.commands.begin());
    }
    std::vector<bool> visited(output.commands.size());
    std::size_t cursor = 0, reached = 0;
    while (cursor != no_index && !visited[cursor]) {
        visited[cursor] = true;
        ++reached;
        cursor = output.commands[cursor].target;
    }
    require(reached == output.commands.size(),Error::malformed,input.wire.guest_address);
    output.cyclic = cursor != no_index;
    output.cycle_entry = cursor;
    if (output.cyclic) {
        const auto entry = cursor;
        bool progress = false;
        do {
            const auto& c = output.commands[cursor];
            progress |= (adds_duration(c.opcode) || c.opcode == 22) && c.duration != 0;
            cursor = c.target;
        } while (cursor != entry);
        require(progress,Error::malformed,output.commands[entry].address);
    }
    return output;
}
// Outward binary32 interval operations follow value()'s actual expression tree.
// Refinement is driven by actual command branches and consumed integer domains,
// not exponent/mantissa probes or polynomial command maxima. At adjacent
// binary32 skips both possible executions have been evaluated exactly.
struct Interval { float low, high; };
float rounded_down(double value) noexcept {
    const auto rounded = static_cast<float>(value);
    return static_cast<double>(rounded) > value ?
        std::nextafter(rounded,-std::numeric_limits<float>::infinity()) : rounded;
}
float rounded_up(double value) noexcept {
    const auto rounded = static_cast<float>(value);
    return static_cast<double>(rounded) < value ?
        std::nextafter(rounded,std::numeric_limits<float>::infinity()) : rounded;
}
Interval operator+(Interval a, Interval b) noexcept {
    return {rounded_down(static_cast<double>(a.low)+b.low),rounded_up(static_cast<double>(a.high)+b.high)};
}
Interval operator-(Interval a, Interval b) noexcept {
    return {rounded_down(static_cast<double>(a.low)-b.high),rounded_up(static_cast<double>(a.high)-b.low)};
}
Interval operator*(Interval a, Interval b) noexcept {
    const std::array<double,4> products{
        static_cast<double>(a.low)*b.low,static_cast<double>(a.low)*b.high,
        static_cast<double>(a.high)*b.low,static_cast<double>(a.high)*b.high};
    if (std::any_of(products.begin(),products.end(),[](double v) { return std::isnan(v); }))
        return {-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
    const auto bounds = std::minmax_element(products.begin(),products.end());
    return {rounded_down(*bounds.first),rounded_up(*bounds.second)};
}
Interval point(float value) noexcept { return {value,value}; }
Interval track_interval(const Track& a, Interval time) noexcept {
    if (a.kind == 2) return point(a.initial)+time*point(a.rate);
    const auto inv = point(a.inv_duration);
    const auto t16 = inv*inv;
    const auto t12 = time*time;
    const auto t18 = inv*t12;
    const auto t14 = time*t12*t16;
    const auto t20 = point(2)*t14*inv;
    const auto t22 = point(3)*t12*t16;
    const auto t24 = t14-t18;
    return point(a.initial)*((t20-t22)+point(1))+point(a.target)*(t22-t20)+
        point(a.rate)*((t24-t18)+time)+point(a.target_rate)*t24;
}
// GAME stores image channels and the renderer's LOD-derived indices as u16;
// palette indexing instead uses a signed s32 directly.
bool index_prefix(Interval range, unsigned channel, std::size_t& prefix) noexcept {
    if (!std::isfinite(range.low) || !std::isfinite(range.high) ||
        range.low < -2147483648.0f || range.high >= 2147483648.0f) return false;
    const auto low = static_cast<std::int64_t>(std::trunc(range.low));
    const auto high = static_cast<std::int64_t>(std::trunc(range.high));
    if (channel == 9) {
        if (low < 0) return false;
        prefix = static_cast<std::size_t>(high)+1;
        return true;
    }
    auto image_prefix=[](std::int64_t first, std::int64_t last) {
        const auto lower = first & 65535, upper = last & 65535;
        return static_cast<std::size_t>(last-first >= 65535 || upper < lower ? 65536 : upper+1);
    };
    prefix = image_prefix(low,high);
    if (channel == 8) {
        if (high == INT32_MAX) return false;
        prefix = std::max(prefix,image_prefix(low+1,high+1));
    }
    return true;
}
struct DomainObserver {
    unsigned channel;
    std::size_t prefix = 0;
    std::uint32_t address = 0;
    void sample(Track track, float time) {
        track.time = time;
        const auto scalar = value(track,false);
        std::size_t count = 0;
        require(index_prefix(point(scalar),channel,count),Error::extent,address);
        prefix = std::max(prefix,count);
    }
};
struct TrackSeed {
    float target = 0, rate = 0, target_rate = 0, inv_duration = 1;
    bool operator==(const TrackSeed&) const = default;
};
TrackSeed seed(const Track& track) noexcept {
    return {track.target,track.rate,track.target_rate,track.inv_duration};
}
bool equal_track(const Track& a, const Track& b) noexcept {
    return a.kind == b.kind && same(a.inv_duration,b.inv_duration) && same(a.time,b.time) &&
        same(a.initial,b.initial) && same(a.target,b.target) && same(a.rate,b.rate) &&
        same(a.target_rate,b.target_rate);
}
void observe_universal_program(const Program& program, unsigned channel, TrackSeed initial,
                     DomainObserver& observer, std::vector<TrackSeed>& seeds) {
    Player player;
    auto& track = player.tracks[channel];
    track.target = initial.target; track.rate = initial.rate;
    track.target_rate = initial.target_rate; track.inv_duration = initial.inv_duration;
    auto observe=[&] {
        if (!track.kind) return;
        // No restriction on source seek/update magnitudes: rounded assignment,
        // shifts and terminal compensation need not reproduce nominal time.
        // STEP ignores that arithmetic and both branches are a complete proof.
        // Other tracks need the owner's explicit first-update capability.
        require(track.kind == 1,Error::unsupported,observer.address);
        observer.sample(track,-std::numeric_limits<float>::infinity());
        observer.sample(track,track.inv_duration);
    };
    auto retain_seed=[&] {
        const auto next = seed(track);
        require(std::isfinite(next.target) && std::isfinite(next.rate) &&
                std::isfinite(next.target_rate) && std::isfinite(next.inv_duration),
                Error::malformed,observer.address);
        if (std::find(seeds.begin(),seeds.end(),next) == seeds.end()) seeds.push_back(next);
    };
    // Universal STEP proof already includes both values, so its time is also
    // irrelevant to cycle equality. Retained assignment seeds remain relevant.
    std::vector<Track> entries;
    auto cursor = std::size_t{0};
    while (cursor != no_index) {
        const auto& c = program.commands[cursor];
        observer.address = c.address;
        if (cursor == program.cycle_entry) {
            if (!track.kind) track.time = 0; // Opcode12 cannot make an inactive time observable.
            if (track.kind == 1) track.time = 0;
            if (std::any_of(entries.begin(),entries.end(),
                [&](const auto& prior) {
                    return equal_track(prior,track);
                })) break;
            entries.push_back(track);
        }
        if (!c.opcode) {
            observe();
            retain_seed();
            break;
        }
        if (c.opcode >= 3 && c.opcode <= 11) {
            assign_tracks(player,c,0);
        }
        if ((adds_duration(c.opcode) || c.opcode == 22) && c.duration) {
            observe();
            retain_seed();
        }
        if (c.target == no_index) {
            observe();
            retain_seed();
        }
        cursor = c.target;
    }
}
// Endpoint-monotone operations rounded to nearest binary32, exactly as Source
// assignments, shifts, waits, speed addition and END compensation. Unlike the
// outward polynomial evaluator, a point interval remains the exact Runtime time.
Interval time_add(Interval a, float b) noexcept { return {a.low+b,a.high+b}; }
struct SampleState { Track track; Interval time{}; bool complete = true; };
SampleState seek_sample(const Program& program, unsigned channel, TrackSeed initial,
                        float first, float last, float speed) {
    Player player;
    auto& track = player.tracks[channel];
    track.target = initial.target; track.rate = initial.rate;
    track.target_rate = initial.target_rate; track.inv_duration = initial.inv_duration;
    // Coefficients are exact Runtime assign_tracks results on this command
    // path; only time is an interval. RN addition/subtraction is monotone, so
    // endpoint propagation encloses every binary32 skip. Correlation lost at
    // END can widen a bound, never exclude a real compensated time.
    Interval left{-last,-first}, time{};
    std::size_t cursor = 0;
    bool entered_cycle = false;
    Interval prior_left{};
    for (;;) {
        if (program.cyclic && cursor == program.cycle_entry) {
            // A point seek must genuinely progress, not spin after rounded
            // duration addition stagnates. No guessed command/time limit.
            if (entered_cycle && first == last)
                require(left.low > prior_left.low,Error::extent,program.commands[cursor].address);
            entered_cycle = true;
            prior_left = left;
        }
        if (cursor == no_index || !program.commands[cursor].opcode) {
            if (track.kind) {
                const auto compensation = time_add(left,speed);
                time = {time.low+compensation.low,time.high+compensation.high};
            }
            return {track,time,true};
        }
        const auto& c = program.commands[cursor];
        const float duration = static_cast<float>(c.duration);
        cursor = c.target;
        if (c.opcode >= 3 && c.opcode <= 11) {
            assign_tracks(player,c,0);
            if (c.opcode != 7 && (c.mask & (1u << channel)))
                time = {-left.high-speed,-left.low-speed};
        } else if (c.opcode == 12 && (c.mask & (1u << channel))) {
            time = time_add(time,duration);
        }
        if (c.opcode == 22) left = point(duration);
        else if (adds_duration(c.opcode)) left = time_add(left,duration);
        require(std::isfinite(left.low) && std::isfinite(left.high),Error::extent,c.address);
        if (left.low > 0) {
            if (track.kind) time = time_add(time,speed);
            return {track,time,true};
        }
        if (left.high > 0) return {track,time,false}; // Split at the actual branch.
    }
}
void observe_sample(const Program& program, unsigned channel, TrackSeed initial,
                    const TextureSample& sampling, DomainObserver& observer,
                    std::vector<TrackSeed>& seeds) {
    auto retain=[&](const Track& track) {
        const auto next = seed(track);
        require(std::isfinite(next.target) && std::isfinite(next.rate) &&
                std::isfinite(next.target_rate) && std::isfinite(next.inv_duration),
                Error::extent,observer.address);
        if (std::find(seeds.begin(),seeds.end(),next) == seeds.end()) seeds.push_back(next);
    };
    auto visit=[&](auto&& self, float first, float last) -> void {
        for (const float skip : {first,last}) {
            const auto exact = seek_sample(program,channel,initial,skip,skip,sampling.speed);
            if (exact.track.kind) observer.sample(exact.track,exact.time.low);
            retain(exact.track);
        }
        if (first == last) return;
        const auto range = seek_sample(program,channel,initial,first,last,sampling.speed);
        if (range.complete) {
            if (!range.track.kind) return;
            if (range.track.kind == 1 && !std::isnan(range.time.low) &&
                !std::isnan(range.time.high) && range.time.low <= range.time.high) {
                observer.sample(range.track,range.time.low);
                observer.sample(range.track,range.time.high);
                retain(range.track);
                return;
            }
            std::size_t bound = 0;
            if (range.track.kind != 1 && index_prefix(track_interval(range.track,range.time),channel,bound) &&
                bound <= observer.prefix) { retain(range.track); return; }
        }
        const float middle = static_cast<float>((static_cast<double>(first)+last)*0.5);
        if (middle == first || middle == last) return; // Endpoints exhaust binary32.
        self(self,first,middle);
        self(self,middle,last);
    };
    visit(visit,sampling.first,sampling.last);
}
void restart(Player& p, Clock& clock, float skip) noexcept {
    for (auto& a : p.tracks) a.kind = 0; // animSet retains previous targets/rates.
    p.cursor = 0;
    clock.time_left = 0;
    clock.time_passed = skip;
    clock.phase = p.program == no_index ? Phase::disabled : Phase::changed;
}

TextureDomainResult domain_impl(const Material& material, std::span<const Animation> animations,
                               std::span<const std::size_t> selected,
                               std::span<const AnimationPath> paths,
                               std::span<const TextureSample> samples, bool sampled) noexcept {
    try {
        require(material.wire.bytes.size() >= 0x78,Error::extent,material.wire.guest_address);
        std::vector<Program> programs;
        std::vector<TextureSample> capabilities;
        auto append=[&](std::size_t index) {
            if (index == no_index) return;
            require(index < animations.size() && animations[index].kind == AnimationKind::texture,
                    Error::malformed,material.wire.guest_address);
            programs.push_back(compile(animations[index],paths));
        };
        if (sampled) {
            for (const auto& sample : samples) {
                require(std::isfinite(sample.first) && std::isfinite(sample.last) &&
                        sample.first <= sample.last && std::isfinite(sample.speed),
                        Error::malformed,material.wire.guest_address);
                if (sample.program == no_index) continue;
                append(sample.program);
                capabilities.push_back(sample);
            }
        } else for (const auto index : selected) append(index);
        const auto flags = material.flags ? material.flags : 0xa1u;
        TextureDomainResult result{};
        constexpr std::array<unsigned,4> channels{0,5,8,9};
        for (const auto channel : channels) {
            const bool consumed = channel == 8 ? (flags & 0x10) != 0 :
                channel == 9 ? (flags & 4) != 0 :
                !(flags & 0x10) && (flags & (channel == 0 ? 1u : 2u));
            if (!consumed) continue;
            DomainObserver observer{channel,0,material.wire.guest_address};
            Track initial;
            initial.kind = 2;
            initial.initial = channel == 8 ?
                static_cast<float>(material.color_and_trailing_words[1] >> 24)/255.0f : 0;
            observer.sample(initial,0);
            // Rebinding clears kinds but retains AObj targets/rates/duration.
            // Close over all states observable between updates of the selected
            // programs, rather than assuming every restart allocates new AObjs.
            std::vector<TrackSeed> seeds(1);
            for (std::size_t i = 0; i < seeds.size(); ++i) {
                const auto current = seeds[i];
                for (std::size_t p = 0; p < programs.size(); ++p) {
                    if (!sampled) observe_universal_program(programs[p],channel,current,observer,seeds);
                    else if (!capabilities[p].fresh || i == 0)
                        observe_sample(programs[p],channel,current,capabilities[p],observer,seeds);
                }
            }
            if (channel == 9) result.palettes = observer.prefix;
            else result.images = std::max(result.images,observer.prefix);
        }
        return result;
    } catch (const Invalid& e) { return {e.status,0,0}; }
      catch (const std::bad_alloc&) { return {{Error::allocation,0},0,0}; }
      catch (...) { return {{Error::malformed,material.wire.guest_address},0,0}; }
}
// Deliberately small lifetime language: every observable segment assigns LOD
// with LINEAR_LAST; transfers preserve the wait/track and only reset the source
// elapsed clock. A zero-duration assignment is never itself a publication.
TextureDomainResult playback_domain(const Material& material, std::span<const Program> programs,
                                    std::span<const TexturePlayback> capabilities) {
    const auto address = material.wire.guest_address;
    require(material.wire.bytes.size() >= 0x78,Error::extent,address);
    require(material.flags == 0x10 && !capabilities.empty(),Error::unsupported,address);
    DomainObserver observer{8,0,address};
    Track initial;
    initial.kind = 2;
    initial.initial = static_cast<float>(material.color_and_trailing_words[1] >> 24)/255.0f;
    observer.sample(initial,0);
    std::vector<float> seeds{0};
    float minimum = std::numeric_limits<float>::infinity(), maximum = 0;
    bool zero = false;
    for (const auto& capability : capabilities) {
        require(capability.program < programs.size() && !capability.speeds.empty(),
                Error::malformed,address);
        for (const auto speed : capability.speeds) {
            require(std::isfinite(speed) && speed >= 0,Error::unsupported,address);
            zero |= speed == 0;
            if (speed > 0) minimum = std::min(minimum,speed);
            maximum = std::max(maximum,speed);
        }
    }
    // Taking the union is conservative: permits even switches between speed
    // sets belonging to different programs, but never assumes an owner clock.
    require(zero && maximum > 0,Error::unsupported,address);
    for (const auto& capability : capabilities) {
        const auto& program = programs[capability.program];
        require(program.kind == AnimationKind::texture && program.cyclic,Error::unsupported,address);
        for (const auto& c : program.commands) {
            require((c.opcode == 3 && c.mask == 256) ||
                    (c.opcode == 14 && c.target != no_index),Error::unsupported,c.address);
            if (!c.duration) continue;
            require(maximum < static_cast<float>(c.duration),Error::unsupported,c.address);
            const auto target = std::bit_cast<float>(c.operands[0]);
            if (std::find(seeds.begin(),seeds.end(),target) == seeds.end()) seeds.push_back(target);
        }
    }
    // Compile already proves complete reachability and a positive wait in the
    // cycle. Thus every zero-assignment/transfer chain ends at a positive
    // assignment, before any fields or retained seeds can be published.
    // A rebind can retain only a published positive assignment's target (or
    // fresh target0). Its old kind/rate/time cannot affect the next publication:
    // the next positive assignment resets all three with shared assign_tracks.
    // Testing every such seed under every program therefore closes both
    // arbitrary rebind sequences and loop reentry, not merely the first cycle.
    for (const auto starting_target : seeds) for (const auto& capability : capabilities) {
        const auto& program = programs[capability.program];
        Player player;
        player.tracks[8].target = starting_target;
        std::vector<float> entries;
        auto cursor = std::size_t{0};
        for (;;) {
            auto& track = player.tracks[8];
            if (cursor == program.cycle_entry) {
                if (std::find(entries.begin(),entries.end(),track.target) != entries.end()) break;
                entries.push_back(track.target);
            }
            const auto& c = program.commands[cursor];
            cursor = c.target;
            if (c.opcode != 3) continue;
            assign_tracks(player,c,0); // Exact Runtime coefficients, including RN division.
            if (!c.duration) continue;
            observer.address = c.address;
            constexpr float epsilon = std::numeric_limits<float>::epsilon();
            constexpr float tiny = std::numeric_limits<float>::denorm_min();
            const auto duration = static_cast<float>(c.duration);
            // For left in (0,D], RN(left-s) decreases by at least this
            // amount. The extra update covers the segment-entry publication.
            // Zero updates are exact identities, not charged against this bound.
            const auto error = (point(duration)+point(maximum))*point(epsilon)+point(tiny);
            const auto decrement = rounded_down(static_cast<double>(minimum)-error.high);
            require(decrement > 0,Error::unsupported,c.address);
            const auto updates = std::ceil(std::nextafter(static_cast<double>(duration)/decrement,
                std::numeric_limits<double>::infinity()))+1;
            require(std::isfinite(updates) && updates*epsilon < 1,Error::unsupported,c.address);
            // At assignment, x=-left is in [0,max_speed]. Monotonic RN gives
            // RN(RN(x-s)+s)>=0, and the two roundings add at most 2 errors.
            // Thereafter gamma_n bounds all positive additions, with no
            // frame/cycle horizon. Every assignment establishes this anew.
            const auto count = point(static_cast<float>(updates)); // Integer < 2^23, exact.
            const auto entry = point(maximum)+point(2)*(point(epsilon)*(point(2)*point(maximum))+point(tiny));
            const auto numerator = entry+count*point(maximum)+count*point(tiny);
            const auto denominator = point(1)-count*point(epsilon);
            require(denominator.low > 0,Error::unsupported,c.address);
            const auto upper = rounded_up(std::nextafter(static_cast<double>(numerator.high)/denominator.low,
                std::numeric_limits<double>::infinity()));
            const Interval time{0,upper};
            std::size_t prefix = 0;
            require(std::isfinite(track.rate) &&
                    index_prefix(track_interval(track,time),8,prefix),Error::extent,c.address);
            observer.prefix = std::max(observer.prefix,prefix);
        }
    }
    return {{},observer.prefix,0};
}
} // namespace
TextureDomainResult texture_domain(const Material& material, std::span<const Animation> animations,
                                  std::span<const std::size_t> selected,
                                  std::span<const AnimationPath> paths) noexcept {
    return domain_impl(material,animations,selected,paths,{},false);
}
TextureDomainResult texture_sample_domain(const Material& material, std::span<const Animation> animations,
                                         std::span<const TextureSample> samples,
                                         std::span<const AnimationPath> paths) noexcept {
    return domain_impl(material,animations,{},paths,samples,true);
}
TextureDomainResult texture_playback_domain(const Material& material, std::span<const Animation> animations,
                                           std::span<const TexturePlayback> capabilities,
                                           std::span<const AnimationPath> paths) noexcept {
    try {
        std::vector<Program> programs(animations.size());
        for (const auto& capability : capabilities) {
            require(capability.program < animations.size(),Error::malformed,material.wire.guest_address);
            auto& program = programs[capability.program];
            if (program.commands.empty()) program = compile(animations[capability.program],paths);
        }
        return playback_domain(material,programs,capabilities);
    } catch (const Invalid& e) { return {e.status,0,0}; }
      catch (const std::bad_alloc&) { return {{Error::allocation,0},0,0}; }
      catch (...) { return {{Error::malformed,material.wire.guest_address},0,0}; }
}
SlotCapacity slot_capacity(const Model& model) noexcept {
    SlotCapacity result{model.nodes.size(), 0};
    for (const auto& node : model.nodes) result.material_attachments += node.materials.size();
    return result;
}

struct Runtime::Impl {
    struct ModelStorage {
        std::vector<NodeState> node_storage;
        std::vector<MaterialState> material_storage;
        std::vector<Player> node_player_storage, material_player_storage;
        std::span<NodeState> nodes;
        std::span<MaterialState> materials;
        std::span<Player> node_players, material_players;
        std::size_t resource_model = no_index;
        std::uint64_t incarnation = 0;
        enum class Life { free, prepared, active, retiring } life = Life::free;
        bool callback_enabled = false;
        float animation_time = 0;
        std::uint32_t flags = 0;
    };
    Resources resources;
    CameraState initial_camera, camera;
    Player camera_player;
    std::vector<Program> programs;
    // Cold-certified per-material capabilities; speed spans retain the Resources
    // immutable borrowed lifetime. No allocation occurs on warm mutation paths.
    std::vector<std::vector<TexturePlayback>> material_playbacks;
    bool capped(std::size_t material) const noexcept {
        return !material_playbacks[material].empty();
    }
    bool playback_program(std::size_t material, std::size_t program) const noexcept {
        if (!capped(material) || program == no_index) return true;
        for (const auto& capability : material_playbacks[material])
            if (capability.program == program) return true;
        return false;
    }
    bool playback_speed(std::size_t material, std::size_t program, float speed) const noexcept {
        if (!capped(material)) return true;
        for (const auto& capability : material_playbacks[material]) {
            if (program != no_index && capability.program != program) continue;
            for (const auto allowed : capability.speeds) if (same(allowed,speed)) return true;
        }
        return false;
    }
    std::vector<SlotCapacity> resource_capacities;
    std::vector<ModelStorage> models;
    std::vector<CallbackEvent> event_storage;
    std::size_t command_capacity = 0;
    bool needs_preflight = false;
    std::size_t event_count = 0;
    std::uint64_t generation = 1, event_order = 0;
    std::uint64_t pool = 0;
    InstanceHandle handle(std::size_t index) const noexcept {
        return {pool, models[index].incarnation, static_cast<std::uint32_t>(index)};
    }
    bool valid(InstanceHandle h, ModelStorage::Life life = ModelStorage::Life::active) const noexcept {
        return !retired && h && h.pool == pool && h.slot < models.size() &&
            models[h.slot].incarnation == h.incarnation && models[h.slot].life == life;
    }
    Status can_acquire(std::size_t slot, std::size_t resource) const noexcept {
        if (retired) return {Error::retired,0};
        if (slot >= models.size() || resource >= resources.models.size()) return {Error::extent,0};
        const auto& m = models[slot];
        if (m.life != ModelStorage::Life::free) return {Error::occupied,0};
        const auto capacity = resource_capacities[resource];
        if (capacity.nodes > m.node_storage.size() || capacity.material_attachments > m.material_storage.size())
            return {Error::capacity,0};
        if (m.incarnation == std::numeric_limits<std::uint64_t>::max()) return {Error::exhausted,0};
        return {};
    }
    AcquireResult activate(std::size_t slot, std::size_t resource, bool callbacks) noexcept {
        auto& m = models[slot];
        const auto capacity = resource_capacities[resource];
        m.nodes = std::span(m.node_storage).first(capacity.nodes);
        m.node_players = std::span(m.node_player_storage).first(capacity.nodes);
        m.materials = std::span(m.material_storage).first(capacity.material_attachments);
        m.material_players = std::span(m.material_player_storage).first(capacity.material_attachments);
        m.resource_model = resource;
        m.callback_enabled = callbacks;
        initialize_model(m,0,true);
        ++m.incarnation;
        m.life = ModelStorage::Life::active;
        event_count = 0;
        ++generation;
        return {{},handle(slot)};
    }
    bool retired = false;
    // Admission only: follow the same immutable command boundaries and binary32
    // wait additions, without touching tracks, fields, cursors or borrowed events.
    // Total work is charged across the entire public call, not separately per
    // node. This also bounds a positive mathematical cycle whose additions cease
    // to advance a very large binary32 overshoot.
    Status preflight(const Player& player, const Clock& clock, bool receiver,
                     bool callbacks, std::size_t& commands, std::size_t& events) const noexcept {
        if (clock.phase == Phase::disabled) return {};
        if (player.program == no_index) return {};
        float left = clock.phase == Phase::changed ? -clock.time_passed : clock.time_left-clock.speed;
        if (!std::isfinite(left) ||
            (clock.phase != Phase::changed && !std::isfinite(clock.time_passed+clock.speed)))
            return {Error::capacity,0};
        if (clock.phase != Phase::changed && left > 0) return {};
        const auto& program = programs[player.program];
        auto cursor = player.cursor;
        bool loop_seen = false;
        float loop_left = 0;
        do {
            if (cursor == no_index) return {};
            if (cursor == program.cycle_entry) {
                if (loop_seen && left <= loop_left) return {Error::capacity,program.commands[cursor].address};
                loop_seen = true;
                loop_left = left;
            }
            const auto& command = program.commands[cursor];
            cursor = command.target;
            if (commands == command_capacity) return {Error::capacity,command.address};
            ++commands;
            std::size_t emitted = 0;
            if (callbacks && program.kind == AnimationKind::model) {
                if ((command.opcode == 0 || command.opcode == 1 || command.opcode == 14) && receiver) emitted = 1;
                if (command.opcode == 16) emitted = 1;
                if (command.opcode == 17) emitted = static_cast<std::size_t>(std::popcount(static_cast<unsigned>(command.mask)));
            }
            if (emitted > event_storage.size()-events) return {Error::capacity,command.address};
            events += emitted;
            if (events > UINT64_MAX-event_order) return {Error::exhausted,command.address};
            if (command.opcode == 0) return {};
            if (adds_duration(command.opcode)) left += static_cast<float>(command.duration);
            if (command.opcode == 22) left = static_cast<float>(command.duration);
        } while (left <= 0);
        return {};
    }

    void emit(std::size_t model, std::size_t node, int id, float argument, std::uint32_t address) noexcept {
        event_storage[event_count++] = {handle(model),node,id,argument,address,event_order++};
    }
    void process(Player& p, Clock& clock, std::size_t mi, std::size_t ni, bool receiver,
                 std::uint32_t* node_flags, float* animation_time, MaterialState* material = nullptr) noexcept {
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
        if (p.program == no_index) {
            for (auto& a : p.tracks) if (a.kind) a.time += clock.speed + clock.time_left;
            clock.time_passed = clock.time_left;
            clock.phase = Phase::finished;
            return;
        }
        const auto& program = programs[p.program];
        do {
            if (p.cursor == no_index) {
                for (auto& a : p.tracks) if (a.kind) a.time += clock.speed + clock.time_left;
                clock.time_passed = clock.time_left;
                if (animation_time) *animation_time = clock.time_left;
                clock.phase = Phase::finished;
                return;
            }
            const auto& c = program.commands[p.cursor];
            p.cursor = c.target;
            const float duration = static_cast<float>(c.duration);
            if (c.opcode == 0) {
                for (auto& a : p.tracks) if (a.kind) a.time += clock.speed + clock.time_left;
                if (program.kind == AnimationKind::camera) clock.time_passed = clock.time_left;
                if (animation_time && program.kind == AnimationKind::model) *animation_time = clock.time_passed;
                clock.phase = Phase::finished;
                if (receiver && models[mi].callback_enabled) emit(mi,ni,-1,0,c.address);
                return;
            }
            if (c.opcode == 2 || c.opcode == 15 || c.opcode == 16 || c.opcode == 23) {
                if (c.opcode == 15) *node_flags = c.mask;
                if (c.opcode == 16 && models[mi].callback_enabled)
                    emit(mi,ni,c.mask >> 8,static_cast<float>(c.mask & 255u),c.address);
                clock.time_left += duration;
                continue;
            }
            if (c.opcode == 12) {
                for (unsigned channel = 0; channel < 10; ++channel)
                    if (c.mask & (1u << channel)) p.tracks[channel].time += duration;
                continue;
            }
            if (c.opcode == 13) {
                if (program.kind == AnimationKind::model || (c.mask & 8)) p.tracks[3].path = c.path;
                if (program.kind == AnimationKind::camera && (c.mask & 128)) p.tracks[7].path = c.camera_at_path;
                continue;
            }
            if (c.opcode == 1 || c.opcode == 14) {
                if (c.opcode == 14) {
                    clock.time_passed = -clock.time_left;
                    if (animation_time) *animation_time = -clock.time_left;
                }
                if (receiver && models[mi].callback_enabled) emit(mi,ni,-2,0,c.address);
                continue;
            }
            if (c.opcode == 17) {
                unsigned operand = 0;
                for (unsigned channel = 0; channel < 10; ++channel) {
                    if (!(c.mask & (1u << channel))) continue;
                    const float argument = std::bit_cast<float>(c.operands[operand++]);
                    if (models[mi].callback_enabled) emit(mi,ni,static_cast<int>(channel+4),argument,c.address);
                }
                clock.time_left += duration;
                continue;
            }
            if (c.opcode == 22) {
                clock.time_left = duration;
                constexpr std::array<std::size_t,5> words{no_index,7,6,9,8};
                unsigned operand = 0;
                for (unsigned bit = 0; bit < words.size(); ++bit) if (c.mask & (1u << bit)) {
                    const auto word = c.operands[operand++];
                    auto& destination = bit ? material->color_and_trailing_words[words[bit]] : material->word_4c;
                    if (destination != word) { destination = word; ++material->revision; }
                }
                continue;
            }
            assign_tracks(p,c,-clock.time_left-clock.speed);
            if (adds_duration(c.opcode)) clock.time_left += duration;
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
        for (auto& m : models) if (m.life == ModelStorage::Life::active) initialize_model(m,skip);
    }
    void initialize_model(ModelStorage& m, float skip, bool fresh = false) noexcept {
        const auto& source = resources.models[m.resource_model];
        m.animation_time = skip;
        m.flags = 0;
        bool receiver = true;
        std::size_t material_offset = 0;
        for (std::size_t ni = 0; ni < m.nodes.size(); ++ni) {
            const auto& node = source.nodes[ni];
            auto& n = m.nodes[ni];
            const auto revision = fresh ? std::uint64_t{1} : n.revision+1;
            n = {};
            n.revision = revision;
            n.transform = {node.position,node.rotation,node.scale};
            n.parent = node.parent;
            n.first_material = material_offset;
            n.material_count = node.materials.size();
            material_offset += n.material_count;
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
                if (fresh) state.revision = 1;
                mp.program = node.material_animations.empty() ? no_index : node.material_animations[j];
                restart(mp,state.clock,skip);
            }
        }
    }
};

Runtime::Runtime(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Runtime::~Runtime() = default;
CreateResult Runtime::create(const Catalog& catalog, std::span<const SlotCapacity> capacities, CameraState camera,
                             ExecutionCapacity execution) noexcept {
    try { return create({catalog.models(),catalog.materials(),catalog.animations(),catalog.camera_animation()},capacities,camera,execution); }
    catch (...) { return {{Error::malformed,0},nullptr}; }
}
CreateResult Runtime::create(Resources resources, std::span<const SlotCapacity> capacities, CameraState camera,
                             ExecutionCapacity execution) noexcept {
    try {
        require(std::isfinite(camera.eye.x) && std::isfinite(camera.eye.y) && std::isfinite(camera.eye.z) &&
                std::isfinite(camera.at.x) && std::isfinite(camera.at.y) && std::isfinite(camera.at.z) &&
                std::isfinite(camera.roll) && std::isfinite(camera.fov_y), Error::malformed);
        auto impl = std::make_unique<Impl>();
        impl->resources = resources;
        impl->initial_camera = camera;
        // Never recycle pool identities, including after Runtime destruction.
        static std::atomic<std::uint64_t> next_pool{1};
        auto identity = next_pool.load(std::memory_order_relaxed);
        do {
            require(identity != std::numeric_limits<std::uint64_t>::max(),Error::exhausted);
        } while (!next_pool.compare_exchange_weak(identity,identity+1,std::memory_order_relaxed));
        impl->pool = identity;
        require(capacities.size() <= UINT32_MAX,Error::extent);
        for (const auto& input : resources.animations) impl->programs.push_back(compile(input,resources.paths));
        impl->material_playbacks.resize(resources.materials.size());
        for (const auto& playback : resources.playbacks) {
            require(playback.material < resources.materials.size(),Error::extent);
            impl->material_playbacks[playback.material].push_back(playback.capability);
        }
        for (std::size_t i = 0; i < resources.materials.size(); ++i) {
            require(!resources.materials[i].requires_playback_capability || impl->capped(i),
                    Error::unsupported,resources.materials[i].wire.guest_address);
            if (impl->capped(i)) {
                const auto domain=playback_domain(resources.materials[i],impl->programs,impl->material_playbacks[i]);
                require(domain.images<=resources.materials[i].images.size() &&
                        domain.palettes<=resources.materials[i].palettes.size(),
                        Error::extent,resources.materials[i].wire.guest_address);
            }
        }
        require((execution.commands == no_index) == (execution.events == no_index),Error::malformed);
        const bool finite_certificate = execution.commands == no_index;
        for (const auto& program : impl->programs)
            require(!finite_certificate || !program.cyclic,
                    Error::capacity,program.commands.back().address);
        impl->needs_preflight = !finite_certificate;
        auto check_program = [&](std::size_t index, AnimationKind kind) {
            if (index != no_index) require(index < impl->programs.size() && impl->programs[index].kind == kind, Error::malformed);
        };
        check_program(resources.camera_animation,AnimationKind::camera);
        std::size_t max_callbacks = 0, model_commands = 0, material_commands = 0, camera_commands = 0;
        for (const auto& program : impl->programs) {
            if (program.kind == AnimationKind::model) {
                max_callbacks = std::max(max_callbacks,program.callbacks);
                model_commands = std::max(model_commands,program.commands.size());
            } else if (program.kind == AnimationKind::texture) {
                material_commands = std::max(material_commands,program.commands.size());
            } else camera_commands = std::max(camera_commands,program.commands.size());
        }
        impl->command_capacity = camera_commands;
        impl->models.resize(capacities.size());
        impl->resource_capacities.resize(resources.models.size());
        std::size_t event_capacity = 0;
        for (std::size_t mi = 0; mi < resources.models.size(); ++mi) {
            const auto& source = resources.models[mi];
            std::size_t materials = 0;
            require(std::isfinite(source.animation_speed) && source.animation_speed >= 0, Error::malformed);
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
                require(node.materials.size() <= std::numeric_limits<std::size_t>::max()-materials,Error::extent);
                materials += node.materials.size();
                for (std::size_t j = 0; j < node.materials.size(); ++j) {
                    require(node.materials[j] < resources.materials.size(), Error::extent);
                    require(resources.materials[node.materials[j]].wire.bytes.size() >= 0x78,Error::extent);
                    if (!node.material_animations.empty()) check_program(node.material_animations[j],AnimationKind::texture);
                    const auto program = node.material_animations.empty() ? no_index : node.material_animations[j];
                    require(impl->playback_program(node.materials[j],program),Error::unsupported);
                }
            }
            impl->resource_capacities[mi] = {source.nodes.size(),materials};
        }
        for (std::size_t slot = 0; slot < capacities.size(); ++slot) {
            const auto capacity = capacities[slot];
            auto& target = impl->models[slot];
            target.node_storage.resize(capacity.nodes);
            target.node_player_storage.resize(capacity.nodes);
            target.material_storage.resize(capacity.material_attachments);
            target.material_player_storage.resize(capacity.material_attachments);
            require(capacity.nodes == 0 || model_commands <= std::numeric_limits<std::size_t>::max()/capacity.nodes,
                    Error::extent);
            const auto node_work = model_commands*capacity.nodes;
            require(capacity.material_attachments == 0 ||
                    material_commands <= (std::numeric_limits<std::size_t>::max()-node_work)/capacity.material_attachments,
                    Error::extent);
            impl->command_capacity = std::max(impl->command_capacity,
                node_work+material_commands*capacity.material_attachments);
            if (capacity.nodes) {
                require(max_callbacks <= (std::numeric_limits<std::size_t>::max()-1)/capacity.nodes,Error::extent);
                event_capacity = std::max(event_capacity,max_callbacks*capacity.nodes+1);
            }
        }
        // Finite certification covers every node's richest possible future
        // binding and one receiver END. Cyclic callers select explicit storage.
        if (!finite_certificate) {
            impl->command_capacity = execution.commands;
            event_capacity = execution.events;
        }
        impl->event_storage.resize(event_capacity);
        impl->initialize(0);
        return {{},std::unique_ptr<Runtime>(new Runtime(std::move(impl)))};
    } catch (const Invalid& e) { return {e.status,nullptr}; }
      catch (const std::bad_alloc&) { return {{Error::allocation,0},nullptr}; }
      catch (...) { return {{Error::malformed,0},nullptr}; }
}
AcquireResult Runtime::acquire(std::size_t slot, std::size_t resource, bool callbacks) noexcept {
    const auto status = impl_->can_acquire(slot,resource);
    if (!status) return {status,{}};
    return impl_->activate(slot,resource,callbacks);
}
Status Runtime::prepare_acquire(std::size_t slot, std::size_t resource, bool callbacks) noexcept {
    const auto status = impl_->can_acquire(slot,resource);
    if (!status) return status;
    auto& m = impl_->models[slot];
    m.resource_model = resource;
    m.callback_enabled = callbacks;
    m.life = Impl::ModelStorage::Life::prepared;
    return {};
}
AcquireResult Runtime::acquire_prepared(std::size_t slot) noexcept {
    if (impl_->retired) return {{Error::retired,0},{}};
    if (slot >= impl_->models.size()) return {{Error::extent,0},{}};
    const auto& m = impl_->models[slot];
    if (m.life != Impl::ModelStorage::Life::prepared) return {{Error::handle,0},{}};
    return impl_->activate(slot,m.resource_model,m.callback_enabled);
}
Status Runtime::cancel_prepared(std::size_t slot) noexcept {
    if (slot >= impl_->models.size()) return {Error::extent,0};
    auto& m = impl_->models[slot];
    if (m.life != Impl::ModelStorage::Life::prepared) return {Error::handle,0};
    m.resource_model = no_index;
    m.callback_enabled = false;
    m.life = Impl::ModelStorage::Life::free;
    return {};
}
Status Runtime::begin_retire(InstanceHandle h) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    s.models[h.slot].life = Impl::ModelStorage::Life::retiring;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::release_retired(InstanceHandle h) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h,Impl::ModelStorage::Life::retiring)) return {Error::handle,0};
    auto& m = s.models[h.slot];
    m.life = Impl::ModelStorage::Life::free;
    m.nodes = {};
    m.materials = {};
    m.node_players = {};
    m.material_players = {};
    m.resource_model = no_index;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::set_callback_enabled(InstanceHandle h, bool enabled) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!impl_->valid(h)) return {Error::handle,0};
    impl_->models[h.slot].callback_enabled = enabled;
    impl_->event_count = 0;
    return {};
}

Status Runtime::step_model(InstanceHandle h) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    auto& model = s.models[index];
    // Capability admission precedes even model callbacks, borrowed-event
    // invalidation and generation changes. Binding at disabled reset speed1 is
    // harmless; only the speed that will actually be evaluated is constrained.
    for (std::size_t i = 0; i < model.materials.size(); ++i) {
        const auto& material = model.materials[i];
        const auto& player = model.material_players[i];
        if (material.clock.phase == Phase::disabled || player.program == no_index) continue;
        if (!s.playback_speed(material.resource,player.program,material.clock.speed))
            return {Error::unsupported,0};
    }
    std::size_t commands = 0, events = 0;
    if (s.needs_preflight) for (std::size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const auto& node = model.nodes[ni];
        const auto status = s.preflight(model.node_players[ni],node.clock,node.callback_receiver,
                                        model.callback_enabled,commands,events);
        if (!status) return status;
        for (std::size_t j = 0; j < node.material_count; ++j) {
            const auto k = node.first_material+j;
            const auto material_status = s.preflight(model.material_players[k],model.materials[k].clock,
                                                     false,false,commands,events);
            if (!material_status) return material_status;
        }
    }
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
            s.process(mp,m.clock,index,ni,false,nullptr,nullptr,&m);
            if (m.clock.phase == Phase::disabled) continue;
            Impl::advance(mp,m.clock);
            bool material_changed = false;
            for (unsigned c = 0; c < 10; ++c) {
                if (!mp.tracks[c].kind) continue;
                const float v = value(mp.tracks[c],false);
                if (c == 0 || c == 5) {
                    auto& image = c == 0 ? m.image_index : m.next_image_index;
                    const auto next = static_cast<std::uint16_t>(static_cast<std::int32_t>(v));
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
    std::size_t commands = 0, events = 0;
    if (s.needs_preflight) {
        const auto status = s.preflight(s.camera_player,s.camera.clock,false,false,commands,events);
        if (!status) return status;
    }
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
            if (j != 3 && j != 7) c.written_channels |= static_cast<std::uint16_t>(1u << j);
            switch (j) {
            case 0: c.eye.x = v; break;
            case 1: c.eye.y = v; break;
            case 2: c.eye.z = v; break;
            case 3: case 7: {
                if (a.path >= s.resources.paths.size()) return {Error::extent,0};
                Vec3 position;
                try { position = path_position(s.resources.paths[a.path],std::clamp(v,0.0f,1.0f)); }
                catch (...) { return {Error::malformed,0}; }
                if (j == 3) { c.eye = position; c.written_channels |= 0x07u; }
                else { c.at = position; c.written_channels |= 0x70u; }
                break;
            }
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
ModelState Runtime::model(InstanceHandle h) const noexcept {
    if (!impl_->valid(h)) {
        ModelState invalid;
        invalid.status = {impl_->retired ? Error::retired : Error::handle,0};
        return invalid;
    }
    const auto& m = impl_->models[h.slot];
    return {m.nodes,m.materials,m.animation_time,m.flags,h,m.resource_model,{}};
}
const CameraState& Runtime::camera() const noexcept { return impl_->camera; }
std::span<const CallbackEvent> Runtime::events() const noexcept { return {impl_->event_storage.data(),impl_->event_count}; }
std::size_t Runtime::slot_count() const noexcept { return impl_->models.size(); }
std::uint64_t Runtime::generation() const noexcept { return impl_->generation; }
Status Runtime::set_object_flags(InstanceHandle h, std::uint32_t flags) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!impl_->valid(h)) return {Error::handle,0};
    impl_->models[h.slot].flags = flags;
    return {};
}
Status Runtime::set_node_transform(InstanceHandle h, std::size_t node, const Transform& transform) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    auto& m = s.models[h.slot];
    if (node >= m.nodes.size()) return {Error::extent,0};
    if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) ||
        !std::isfinite(transform.position.z) || !std::isfinite(transform.rotation.x) ||
        !std::isfinite(transform.rotation.y) || !std::isfinite(transform.rotation.z) ||
        !std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
        !std::isfinite(transform.scale.z)) return {Error::malformed,0};
    auto& n = m.nodes[node];
    if (!same(n.transform.position,transform.position) || !same(n.transform.rotation,transform.rotation) ||
        !same(n.transform.scale,transform.scale)) {
        n.transform = transform;
        ++n.revision;
        ++s.generation;
    }
    return {};
}
Status Runtime::set_node_flags(InstanceHandle h, std::size_t node, std::uint32_t flags) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    auto& m = s.models[h.slot];
    if (node >= m.nodes.size()) return {Error::extent,0};
    auto& n = m.nodes[node];
    if (n.flags != flags) {
        n.flags = flags;
        ++n.revision;
        ++s.generation;
    }
    return {};
}
Status Runtime::set_node_time_left(InstanceHandle h, std::size_t node, float value) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    if (node >= s.models[h.slot].nodes.size()) return {Error::extent,0};
    if (!std::isfinite(value)) return {Error::malformed,0};
    s.models[h.slot].nodes[node].clock.time_left = value;
    return {};
}
Status Runtime::restart_model(InstanceHandle h, float skip) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    if (!std::isfinite(skip)) return {Error::malformed,0};
    auto& m = s.models[index];
    if (skip != 0) for (std::size_t i = 0; i < m.materials.size(); ++i)
        if (s.capped(m.materials[i].resource) && m.material_players[i].program != no_index)
            return {Error::unsupported,0};
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
Status Runtime::bind_model(InstanceHandle h, std::span<const std::size_t> bindings,
                           float skip, ModelBindPose pose) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
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
            const auto& source = s.resources.models[m.resource_model].nodes[i];
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
Status Runtime::bind_materials(InstanceHandle h, std::span<const std::size_t> bindings, float skip) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    auto& m = s.models[index];
    if (!bindings.empty() && bindings.size() != m.materials.size()) return {Error::extent,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    for (const auto program : bindings)
        if (program != no_index && (program >= s.programs.size() || s.programs[program].kind != AnimationKind::texture))
            return {Error::malformed,0};
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i] == no_index) continue; // NULL must not constrain preserved clocks.
        if (!s.playback_program(m.materials[i].resource,bindings[i]) ||
            (s.capped(m.materials[i].resource) && skip != 0)) return {Error::unsupported,0};
    }
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
Status Runtime::reset_materials(InstanceHandle h, std::span<const std::size_t> bindings) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    auto& m = s.models[index];
    if (!bindings.empty() && bindings.size() != m.materials.size()) return {Error::extent,0};
    for (const auto resource : bindings) {
        if (resource == no_index) continue;
        if (resource >= s.resources.materials.size()) return {Error::extent,0};
        if (s.resources.materials[resource].wire.bytes.size() < 0x78) return {Error::extent,0};
    }
    if (!bindings.empty()) for (std::size_t i = 0; i < bindings.size(); ++i) {
        const auto resource = bindings[i], previous = m.materials[i].resource;
        if (resource == no_index || resource == previous) continue;
        // A reset cannot discard a certificate or transplant fresh AObjs onto
        // a capped resource through an originally unrestricted attachment.
        if (s.capped(previous) || s.capped(resource)) return {Error::unsupported,0};
    }
    for (std::size_t ni = 0; ni < m.nodes.size(); ++ni) {
        const auto& n = m.nodes[ni];
        for (std::size_t j = 0; j < n.material_count; ++j) {
            const auto k = n.first_material+j;
            const auto resource = bindings.empty() ? s.resources.models[m.resource_model].nodes[ni].materials[j] : bindings[k];
            if (resource == no_index) continue;
            auto& state = m.materials[k];
            s.restore_material(state,m.material_players[k],resource,1,state.palette_index);
        }
    }
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::set_model_speed(InstanceHandle h, float speed) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    if (!std::isfinite(speed)) return {Error::malformed,0};
    for (auto& n : s.models[index].nodes) n.clock.speed = speed;
    return {};
}
Status Runtime::set_texture_speed(InstanceHandle h, float speed) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!s.valid(h)) return {Error::handle,0};
    const auto index = h.slot;
    if (!std::isfinite(speed)) return {Error::malformed,0};
    const auto& model = s.models[index];
    for (std::size_t i = 0; i < model.materials.size(); ++i)
        if (!s.playback_speed(model.materials[i].resource,model.material_players[i].program,speed))
            return {Error::unsupported,0};
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
Status Runtime::bind_camera(std::size_t program, float skip) noexcept {
    auto& s = *impl_;
    if (s.retired) return {Error::retired,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    if (program != no_index && (program >= s.programs.size() ||
        s.programs[program].kind != AnimationKind::camera)) return {Error::handle,0};
    s.camera_player.program = program;
    restart(s.camera_player,s.camera.clock,skip);
    s.camera.clock.phase = Phase::changed;
    s.event_count = 0;
    ++s.generation;
    return {};
}
Status Runtime::set_camera_speed(float speed) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!std::isfinite(speed)) return {Error::malformed,0};
    impl_->camera.clock.speed = speed;
    return {};
}
Status Runtime::reset(float skip) noexcept {
    if (impl_->retired) return {Error::retired,0};
    if (!std::isfinite(skip)) return {Error::malformed,0};
    if (skip != 0) for (const auto& active : impl_->models) {
        if (active.life != Impl::ModelStorage::Life::active) continue;
        for (const auto& node : impl_->resources.models[active.resource_model].nodes)
            for (std::size_t i = 0; i < node.materials.size(); ++i)
                if (impl_->capped(node.materials[i]) && !node.material_animations.empty() &&
                    node.material_animations[i] != no_index) return {Error::unsupported,0};
    }
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
