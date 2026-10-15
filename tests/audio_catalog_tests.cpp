#include "audio_fixture.hpp"
#include "../src/audio_synth_internal.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace a = isle::audio;
namespace d = isle::audio::detail;
namespace fixture = isle::audio_test;
namespace {
int failures = 0;
void check(bool condition, std::string_view meaning) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << meaning << '\n'; }
}
void prepare(d::Context& context, std::uint64_t owner = 1) {
    context.owner = owner;
    context.memory = std::make_unique<std::uint32_t[]>(a::AudioService::rdram_bytes / 4u);
    std::fill_n(context.memory.get(), a::AudioService::rdram_bytes / 4u, 0xa5a5a5a5u);
}
std::uint16_t read16(const d::Context& context, std::uint32_t address) {
    return static_cast<std::uint16_t>((std::uint16_t(context.get8(address)) << 8) |
                                      context.get8(address + 1));
}
a::BankFile decode(const fixture::Fixture& input) {
    auto result = input.decode();
    check(bool(result), "authored fixture hydrates before catalog construction");
    return std::move(result.asset);
}
a::Error build(d::Context& context, const a::BankFile& owner) {
    return d::build_catalog(context, std::span<const a::BankFile>(&owner, 1));
}

void retention_traversal_and_identity() {
    fixture::Fixture input;
    input.bank->resize(320);
    auto& b = *input.bank;
    fixture::put16(b, 2, 3);
    fixture::put32(b, 8, 240); fixture::put32(b, 12, 0);
    fixture::put32(b, 24, 208); // First bank's percussion is not in its instrument list.
    fixture::put16(b, 46, 3);
    fixture::put32(b, 52, 64); fixture::put32(b, 56, 0);
    fixture::put16(b, 222, 3);
    fixture::put32(b, 224, 264); fixture::put32(b, 228, 304); fixture::put32(b, 232, 0);
    fixture::put16(b, 240, 2); fixture::put32(b, 244, 32000);
    fixture::put32(b, 252, 32); fixture::put32(b, 256, 208);
    fixture::put32(b, 264, 80); fixture::put32(b, 268, 96); fixture::put32(b, 272, 280);
    fixture::put32(b, 280, 0); fixture::put32(b, 284, 18);
    fixture::put32(b, 292, 124); fixture::put32(b, 296, 168);
    fixture::put32(b, 304, 80); fixture::put32(b, 308, 96); fixture::put32(b, 312, 104);
    auto owner = decode(input);
    if (!owner) return;
    const auto* first = owner.banks()[0];
    const auto* percussion_sound = first->percussion->sounds[0];
    const auto* shared_wave_sound = first->percussion->sounds[1];
    const auto* melodic_sound = first->instruments[0]->sounds[0];
    d::Context context; prepare(context, 17);
    std::array<a::BankFile, 2> owners{owner, owner};
    check(d::build_catalog(context, owners) == a::Error::none,
          "multiple banks, percussion, null slots and repeated owners are traversed");
    check(context.catalog.owners.size() == 2 && context.catalog.sounds.size() == 3 &&
          context.catalog.waves.size() == 2,
          "all owners retained while native sound and wave aliases share catalog entries");
    a::SoundId percussion{}, melodic{}, shared{};
    check(d::find_sound(context, *percussion_sound, percussion) == a::Error::none &&
          d::find_sound(context, *melodic_sound, melodic) == a::Error::none &&
          d::find_sound(context, *shared_wave_sound, shared) == a::Error::none &&
          melodic.owner == 17 && percussion.index != melodic.index && shared.index != melodic.index,
          "percussion-only and melodic native identities receive distinct owner-qualified IDs");
    const d::CatalogSound* selected = nullptr;
    const d::CatalogSound* alias = nullptr;
    check(d::catalog_sound(context, melodic, selected) == a::Error::none &&
          d::catalog_sound(context, shared, alias) == a::Error::none &&
          selected->resource == melodic_sound && alias->wave_index == selected->wave_index,
          "distinct sounds can retain the very same native wave identity");
    const d::CatalogWave* wave = nullptr;
    check(d::catalog_wave(context, {17, selected->wave_index}, wave) == a::Error::none &&
          wave->resource == melodic_sound->wave,
          "catalog wave lookup preserves original typed descriptor identity");
    const auto copied = *melodic_sound;
    a::SoundId unchanged{99, 88};
    check(d::find_sound(context, copied, unchanged) == a::Error::foreign_resource &&
          unchanged.owner == 99 && unchanged.index == 88,
          "a structurally identical foreign sound cannot impersonate a retained descriptor");
    d::Context other; prepare(other, 18);
    check(build(other, owner) == a::Error::none &&
          d::catalog_sound(other, melodic, selected) == a::Error::foreign_resource &&
          d::catalog_wave(other, {17, 0}, wave) == a::Error::foreign_resource,
          "even catalogs retaining the same bank reject each other's sound and wave IDs");
    check(d::catalog_sound(context, {17, 3}, selected) == a::Error::invalid_resource &&
          d::catalog_wave(context, {17, 2}, wave) == a::Error::invalid_resource &&
          d::catalog_wave(context, {}, wave) == a::Error::foreign_resource,
          "invalid indices and unqualified IDs fail instead of selecting another resource");
    other.catalog = {};
    std::weak_ptr<const a::Bytes> bank_bytes = input.bank, table_bytes = input.table;
    input.bank.reset(); input.table.reset(); owner = {}; owners = {};
    check(!bank_bytes.expired() && !table_bytes.expired() &&
          melodic_sound->wave->encoded.size() == 18,
          "catalog retains both input owners and bare graph pointers after callers release all copies");
    context.catalog = {};
    check(bank_bytes.expired() && table_bytes.expired(), "catalog destruction releases the last bank and table owners");
}

void precise_layout_and_content_deduplication() {
    fixture::Fixture first, second;
    for (auto* input : {&first, &second}) {
        fixture::put32(*input->bank, 124, 3); fixture::put32(*input->bank, 128, 31);
        fixture::put32(*input->bank, 132, 2);
        (*input->table)[1] = std::byte{0x19}; (*input->table)[17] = std::byte{0xaa};
        input->table->push_back(std::byte{0x7b}); fixture::put32(*input->bank, 108, 19);
    }
    std::array<a::BankFile, 2> owners{decode(first), decode(second)};
    if (!owners[0] || !owners[1]) return;
    d::Context context; prepare(context);
    check(d::build_catalog(context, owners) == a::Error::none, "equal independently retained resources admit");
    check(context.catalog.waves.size() == 2 && context.catalog.sounds.size() == 2,
          "content sharing does not collapse distinct native descriptor identities");
    const auto& wave = context.catalog.waves[0];
    const auto& duplicate = context.catalog.waves[1];
    constexpr auto base = a::AudioService::resource_address;
    check(wave.book_address == base && wave.book_bytes == 32 && wave.loop_address == base + 32 &&
          wave.sample_address == base + 64 && context.catalog.resource_end == base + 96,
          "book, history and guarded source occupy a precise eight-byte-aligned layout");
    check(duplicate.book_address == wave.book_address && duplicate.loop_address == wave.loop_address &&
          duplicate.sample_address == wave.sample_address,
          "equal book bytes, histories and encoded extents materialize only once across banks");
    check(wave.sample_frames == 32 && wave.resource->encoded.size() == 19 &&
          wave.resource->loop->start == 3 && wave.resource->loop->end == 31 &&
          wave.resource->loop->count == 2,
          "tail byte adds no decoded frame and partial finite-loop semantics remain untouched");
    check(read16(context, wave.book_address) == 0x8000 && read16(context, wave.book_address + 30) == 0x7fff &&
          read16(context, wave.loop_address) == 0x8000 && read16(context, wave.loop_address + 30) == 0x7fff,
          "signed book coefficients and complete loop histories are packed in RDRAM byte order");
    check(context.get8(wave.sample_address + 1) == 0x19 &&
          context.get8(wave.sample_address + 17) == 0xaa && context.get8(wave.sample_address + 18) == 0x7b,
          "the complete source extent including its optional alignment byte is retained");
    bool zero_padding = true;
    for (auto offset = 19u; offset < 32u; ++offset)
        zero_padding = zero_padding && context.get8(wave.sample_address + offset) == 0;
    check(zero_padding && context.get8(base - 1) == 0xa5 &&
          context.get8(context.catalog.resource_end) == 0xa5,
          "alignment and DMA guard bytes are initialized without touching neighboring RAM");
    check(build(context, owners[0]) == a::Error::invalid_state && context.catalog.resource_end == base + 96,
          "a published catalog cannot be rebuilt or silently append resources");
}

void independent_resources_and_inactive_loop() {
    fixture::Fixture first, second;
    (*second.table)[1] = std::byte{0x12};
    fixture::put16(*second.bank, 178, 123);
    fixture::put16(*second.bank, 138, 456);
    std::array<a::BankFile, 2> owners{decode(first), decode(second)};
    if (!owners[0] || !owners[1]) return;
    d::Context context; prepare(context);
    check(d::build_catalog(context, owners) == a::Error::none, "different resource contents admit together");
    const auto& one = context.catalog.waves[0]; const auto& two = context.catalog.waves[1];
    check(two.book_address == one.sample_address + 32 && two.loop_address == two.book_address + 32 &&
          two.sample_address == two.loop_address + 32 &&
          context.catalog.resource_end == a::AudioService::resource_address + 192,
          "different books, histories and source extents each reserve their own checked storage");
    check(read16(context, two.book_address + 2) == 123 && read16(context, two.loop_address + 2) == 456 &&
          context.get8(two.sample_address + 1) == 0x12 && context.get8(one.sample_address + 1) == 0,
          "each packed wave points to its own actual immutable data, not an alternate selected wave");

    fixture::Fixture inactive;
    fixture::put32(*inactive.bank, 132, 0);
    auto owner = decode(inactive);
    if (!owner) return;
    d::Context no_loop; prepare(no_loop);
    check(build(no_loop, owner) == a::Error::none && no_loop.catalog.waves[0].loop_address == 0 &&
          no_loop.catalog.waves[0].resource->loop->count == 0 &&
          no_loop.catalog.resource_end == a::AudioService::resource_address + 64,
          "zero-count loops retain descriptors but consume no unused history storage");
}

void unsupported_wave_and_player_policy_separation() {
    fixture::Fixture supported, raw, order_one, predictors_nine, invalid_predictor, predictors_eight;
    fixture::put32(*supported.bank, 84, 1000); // Legal graph; not SoundPlayer's indefinite sustain policy.
    (*raw.bank)[112] = std::byte{1}; fixture::put32(*raw.bank, 128, 9);
    fixture::put32(*order_one.bank, 168, 1);
    predictors_nine.bank->resize(464); fixture::put32(*predictors_nine.bank, 172, 9);
    (*invalid_predictor.table)[0] = std::byte{1};
    predictors_eight.bank->resize(432); fixture::put32(*predictors_eight.bank, 172, 8);
    (*predictors_eight.table)[0] = std::byte{7};
    std::array<a::BankFile, 6> owners{decode(supported), decode(raw), decode(order_one),
                                     decode(predictors_nine), decode(invalid_predictor), decode(predictors_eight)};
    for (const auto& owner : owners) if (!owner) return;
    d::Context context; prepare(context);
    check(d::build_catalog(context, owners) == a::Error::none && context.catalog.waves.size() == 6,
          "unsupported DSP resources remain cataloged alongside supported sources");
    const std::array<a::Error, 6> expected{a::Error::none, a::Error::unsupported_wave,
        a::Error::unsupported_book, a::Error::unsupported_book, a::Error::invalid_resource, a::Error::none};
    for (std::uint32_t i = 0; i < owners.size(); ++i) {
        a::SoundId id{}; const d::CatalogSound* sound = nullptr; const d::CatalogWave* wave = nullptr;
        const auto& original = fixture::selected(owners[i]);
        const bool found = d::find_sound(context, original, id) == a::Error::none &&
            d::catalog_sound(context, id, sound) == a::Error::none &&
            d::catalog_wave(context, {context.owner, sound->wave_index}, wave) == a::Error::none;
        check(found && wave->resource == original.wave && wave->admission == expected[i],
              "every sound resolves to its exact native wave and separate DSP admission result");
        if (found && expected[i] != a::Error::none)
            check(wave->sample_address == 0 && wave->book_address == 0 && wave->loop_address == 0 &&
                  wave->book_bytes == 0,
                  "unsupported waves allocate no unused encoded, predictor or loop copy");
    }
    check(context.catalog.waves[0].admission == a::Error::none &&
          context.catalog.waves[0].resource == fixture::selected(owners[0]).wave,
          "DSP wave admission is independent of finite-decay SoundPlayer envelope policy");
    check(context.catalog.waves[5].book_bytes == 256 &&
          context.catalog.resource_end == a::AudioService::resource_address + 384,
          "all eight predictors fit, and only the two supported waves contribute to the actual layout");
}

void arena_boundaries_and_atomic_failure() {
    constexpr auto arena = a::AudioService::rdram_bytes - a::AudioService::resource_address;
    constexpr auto fitting_encoded = arena - 64u - 8u;
    static_assert(fitting_encoded % 9u == 0 && fitting_encoded % 8u == 0);
    fixture::Fixture fitting;
    fitting.table->resize(fitting_encoded);
    fixture::put32(*fitting.bank, 108, fitting_encoded);
    auto fit_owner = decode(fitting);
    if (!fit_owner) return;
    {
        d::Context context; prepare(context);
        check(build(context, fit_owner) == a::Error::none &&
              context.catalog.resource_end == a::AudioService::rdram_bytes &&
              context.catalog.waves[0].sample_frames == (fitting_encoded / 9u) * 16u &&
              context.get8(a::AudioService::rdram_bytes - 1) == 0,
              "an actual computed source layout filling the entire immutable arena succeeds");
    }
    fixture::Fixture overflow;
    overflow.table->resize(fitting_encoded + 9u);
    fixture::put32(*overflow.bank, 108, fitting_encoded + 9u);
    auto overflow_owner = decode(overflow);
    if (!overflow_owner) return;
    d::Context context; prepare(context);
    check(build(context, overflow_owner) == a::Error::address_range && context.catalog.owners.empty() &&
          context.catalog.sounds.empty() && context.catalog.waves.empty() &&
          context.catalog.resource_end == a::AudioService::resource_address &&
          context.get8(a::AudioService::resource_address) == 0xa5 &&
          context.get8(a::AudioService::rdram_bytes - 1) == 0xa5,
          "one additional compressed frame fails atomically without publishing owners or writing RAM");
    fixture::Fixture small;
    auto small_owner = decode(small);
    std::array<a::BankFile, 2> combined{small_owner, fit_owner};
    check(d::build_catalog(context, combined) == a::Error::address_range &&
          context.catalog.owners.empty() && context.catalog.waves.empty() &&
          context.get8(a::AudioService::resource_address + 64) == 0xa5,
          "aggregate bank layout is checked before any otherwise-valid earlier wave is published or copied");
    check(build(context, small_owner) == a::Error::none && context.catalog.waves.size() == 1,
          "a failed layout leaves a pristine context available for a later valid construction");
}

void invalid_owners_fail_without_publication() {
    d::Context context; prepare(context);
    check(d::build_catalog(context, {}) == a::Error::invalid_resource && context.catalog.owners.empty(),
          "an empty owner list does not publish a catalog");
    fixture::Fixture input;
    auto owner = decode(input);
    if (!owner) return;
    std::array<a::BankFile, 2> owners{owner, {}};
    check(d::build_catalog(context, owners) == a::Error::invalid_resource && context.catalog.owners.empty() &&
          context.get8(a::AudioService::resource_address) == 0xa5,
          "an invalid later owner rejects the entire construction before resource writes");
    d::Context missing_memory; missing_memory.owner = 1;
    check(build(missing_memory, owner) == a::Error::invalid_state,
          "catalog requires the parent's preallocated RDRAM");
}
} // namespace

int main() {
    retention_traversal_and_identity();
    precise_layout_and_content_deduplication();
    independent_resources_and_inactive_loop();
    unsupported_wave_and_player_policy_separation();
    arena_boundaries_and_atomic_failure();
    invalid_owners_fail_without_publication();
    if (failures) return EXIT_FAILURE;
    std::cout << "Audio catalog ownership, identity, admission and checked-layout checks passed\n";
    return EXIT_SUCCESS;
}
