#include "audio_synth_internal.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace isle::audio::detail {
namespace {
template<class T> struct SpanHash {
    std::size_t operator()(std::span<const T> values) const noexcept {
        std::size_t hash = 2166136261u;
        for (const auto value : values) {
            hash ^= static_cast<std::size_t>(value);
            hash *= 16777619u;
        }
        return hash;
    }
};
template<class T> struct SpanEqual {
    bool operator()(std::span<const T> a, std::span<const T> b) const noexcept {
        return a.size() == b.size() &&
               (a.data() == b.data() || std::equal(a.begin(), a.end(), b.begin()));
    }
};
template<class T> using PackedSpans =
    std::unordered_map<std::span<const T>, std::uint32_t, SpanHash<T>, SpanEqual<T>>;

struct LayoutFailure { Error error; };

// All arithmetic is checked before narrowing. Each source has eight guard bytes
// beyond its logical extent for the aligned ABI1 DMA loads used by the decoder.
std::uint32_t reserve(Catalog& catalog, std::size_t bytes, bool dma_guard = false) {
    constexpr auto limit = AudioService::rdram_bytes;
    const auto remaining = limit - catalog.resource_end;
    const std::size_t guard = dma_guard ? 8u : 0u;
    if (bytes > remaining || guard > remaining - bytes)
        throw LayoutFailure{Error::address_range};
    const auto padded = (bytes + guard + 7u) & ~std::size_t{7u};
    if (padded > remaining) throw LayoutFailure{Error::address_range};
    const auto address = catalog.resource_end;
    catalog.resource_end += static_cast<std::uint32_t>(padded);
    return address;
}

template<class T>
std::uint32_t pack(Catalog& catalog, PackedSpans<T>& spans,
                   std::span<const T> values, bool dma_guard = false) {
    const auto found = spans.find(values);
    if (found != spans.end()) return found->second;
    const auto address = reserve(catalog, values.size_bytes(), dma_guard);
    spans.emplace(values, address);
    return address;
}

Error admit(const Wave& wave, CatalogWave& entry) noexcept {
    if (wave.type != WaveType::adpcm) return Error::unsupported_wave;
    if (wave.book == nullptr || wave.book->order != 2 ||
        wave.book->predictors <= 0 || wave.book->predictors > 8 ||
        wave.book->coefficients.size() != static_cast<std::size_t>(wave.book->predictors) * 16u)
        return Error::unsupported_book;
    // The BankFile already guarantees valid typed extents and loop bounds.
    // Predictor indices, unlike the wire graph, are a DSP admission constraint.
    const auto frames = wave.encoded.size() / 9u;
    if (frames > std::numeric_limits<std::uint32_t>::max() / 16u)
        return Error::invalid_resource;
    for (std::size_t i = 0; i < frames; ++i)
        if ((std::to_integer<unsigned>(wave.encoded[i * 9u]) & 15u) >=
            static_cast<unsigned>(wave.book->predictors))
            return Error::invalid_resource;
    if (wave.loop != nullptr && wave.loop->count != 0 && !wave.loop->adpcm_history)
        return Error::unsupported_loop;
    entry.sample_frames = static_cast<std::uint32_t>(frames * 16u);
    return Error::none;
}
} // namespace

Error build_catalog(Context& context, std::span<const BankFile> owners) noexcept {
    if (context.memory == nullptr || context.owner == 0 || context.lease != Lease::none ||
        !context.catalog.owners.empty() || !context.catalog.waves.empty() ||
        !context.catalog.sounds.empty() ||
        context.catalog.resource_end != AudioService::resource_address)
        return Error::invalid_state;
    if (owners.empty() || std::any_of(owners.begin(), owners.end(),
                                    [](const BankFile& owner) { return !owner; }))
        return Error::invalid_resource;
    try {
        Catalog pending;
        pending.owners.assign(owners.begin(), owners.end());
        std::unordered_set<const Bank*> banks;
        std::unordered_set<const Instrument*> instruments;
        std::unordered_set<const Sound*> sounds;
        std::unordered_map<const Wave*, std::uint32_t> waves;
        PackedSpans<std::int16_t> books, histories;
        PackedSpans<std::byte> samples;
        const auto visit_instrument = [&](const Instrument* instrument) {
            if (instrument == nullptr || !instruments.insert(instrument).second) return;
            for (const auto* sound : instrument->sounds) {
                if (sound == nullptr || !sounds.insert(sound).second) continue;
                if (pending.sounds.size() >= std::numeric_limits<std::uint32_t>::max())
                    throw LayoutFailure{Error::identity_exhausted};
                auto wave = waves.find(sound->wave);
                if (wave == waves.end()) {
                    if (pending.waves.size() >= std::numeric_limits<std::uint32_t>::max())
                        throw LayoutFailure{Error::identity_exhausted};
                    CatalogWave entry;
                    entry.resource = sound->wave;
                    entry.admission = admit(*entry.resource, entry);
                    if (entry.admission == Error::none) {
                        const auto& resource = *entry.resource;
                        entry.book_bytes = static_cast<std::uint32_t>(resource.book->coefficients.size() * 2u);
                        entry.book_address = pack(pending, books,
                            std::span<const std::int16_t>(resource.book->coefficients));
                        if (resource.loop != nullptr && resource.loop->count != 0)
                            entry.loop_address = pack(pending, histories,
                                std::span<const std::int16_t>(resource.loop->history));
                        entry.sample_address = pack(pending, samples, resource.encoded, true);
                    }
                    const auto index = static_cast<std::uint32_t>(pending.waves.size());
                    pending.waves.push_back(entry);
                    wave = waves.emplace(sound->wave, index).first;
                }
                pending.sounds.push_back({sound, wave->second});
            }
        };
        for (const auto& owner : pending.owners)
            for (const auto* bank : owner.banks()) {
                if (bank == nullptr || !banks.insert(bank).second) continue;
                visit_instrument(bank->percussion);
                for (const auto* instrument : bank->instruments) visit_instrument(instrument);
            }

        // No allocation, failure or publication occurs after the first write.
        // Zero every alignment/guard byte even if the caller's RAM was nonzero.
        for (auto address = AudioService::resource_address; address < pending.resource_end; ++address)
            context.put8(address, 0);
        for (const auto& [values, address] : books)
            for (std::size_t i = 0; i < values.size(); ++i)
                context.put16(address + static_cast<std::uint32_t>(i * 2u),
                              static_cast<std::uint16_t>(values[i]));
        for (const auto& [values, address] : histories)
            for (std::size_t i = 0; i < values.size(); ++i)
                context.put16(address + static_cast<std::uint32_t>(i * 2u),
                              static_cast<std::uint16_t>(values[i]));
        for (const auto& [values, address] : samples)
            for (std::size_t i = 0; i < values.size(); ++i)
                context.put8(address + static_cast<std::uint32_t>(i),
                             std::to_integer<std::uint8_t>(values[i]));
        context.catalog = std::move(pending);
        return Error::none;
    } catch (const LayoutFailure& failure) {
        return failure.error;
    } catch (const std::bad_alloc&) {
        return Error::allocation_failure;
    } catch (const std::length_error&) {
        return Error::allocation_failure;
    }
}

Error find_sound(const Context& context, const Sound& sound, SoundId& result) noexcept {
    for (std::size_t i = 0; i < context.catalog.sounds.size(); ++i)
        if (context.catalog.sounds[i].resource == &sound) {
            result = {context.owner, static_cast<std::uint32_t>(i)};
            return Error::none;
        }
    return Error::foreign_resource;
}

Error catalog_sound(const Context& context, SoundId id, const CatalogSound*& result) noexcept {
    if (id.owner == 0 || id.owner != context.owner) return Error::foreign_resource;
    if (id.index >= context.catalog.sounds.size()) return Error::invalid_resource;
    result = &context.catalog.sounds[id.index];
    return Error::none;
}

Error catalog_wave(const Context& context, WaveId id, const CatalogWave*& result) noexcept {
    if (id.owner == 0 || id.owner != context.owner) return Error::foreign_resource;
    if (id.index >= context.catalog.waves.size()) return Error::invalid_resource;
    result = &context.catalog.waves[id.index];
    return Error::none;
}
} // namespace isle::audio::detail
