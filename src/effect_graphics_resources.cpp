#include "effect_graphics_resources.hpp"
#include "effect_bank_resources.hpp"
#include <algorithm>
#include <limits>

namespace isle::menu {
namespace {
void need(bool value, Error error, std::uint32_t address = 0) {
    if (!value) throw EffectResourceError({error,address});
}
}
void EffectGraphicsResources::require_live() const {
    need(snapshot_.canonical().data()==source_ && snapshot_.canonical().size()==rom::us_size,
         Error::invalid_snapshot);
}
EffectGraphicsResources::EffectGraphicsResources(const rom::Snapshot& snapshot,
    std::span<const EffectBank* const> selected, RamRange arena)
    : snapshot_(snapshot),source_(snapshot.canonical().data()) {
    require_live();
    need(!(arena.offset&7u) && arena.offset<=rdram_size && arena.size<=rdram_size-arena.offset,Error::extent);
    struct Payload { std::size_t begin,end; std::uint32_t* address; };
    std::vector<Payload> payloads;
    std::array<bool,8> occupied{};
    std::size_t payload_count=0;
    for (const auto* bank:selected) {
        need(bank && bank->bank_id<occupied.size() && !occupied[bank->bank_id],Error::malformed);
        occupied[bank->bank_id]=true;
        for (const auto& image:bank->images) {
            for (const auto count:{image.frames.size(),image.palettes.size()}) {
                need(count<=payloads.max_size()-payload_count,Error::extent,bank->bank_id);
                payload_count+=count;
            }
        }
    }
    payloads.reserve(payload_count);spans_.reserve(payload_count);ranges_.reserve(payload_count);
    bank_assets_.resize(selected.size());banks_.resize(selected.size());
    const auto source_address=reinterpret_cast<std::uintptr_t>(source_);
    const auto source_size=snapshot_.canonical().size();
    const auto collect=[&](const rom::GuestView& value,std::uint32_t& address) {
        const auto at=reinterpret_cast<std::uintptr_t>(value.bytes.data());
        need(!value.bytes.empty() && at>=source_address && at-source_address<=source_size &&
             value.bytes.size()<=source_size-(at-source_address),Error::invalid_snapshot);
        const auto begin=static_cast<std::size_t>(at-source_address);
        payloads.push_back({begin,begin+value.bytes.size(),&address});
    };
    for (std::size_t bank=0;bank<selected.size();++bank) {
        const auto& input=*selected[bank];auto& storage=bank_assets_[bank];
        storage.addresses.resize(input.images.size());storage.images.resize(input.images.size());
        for (std::size_t image=0;image<input.images.size();++image) {
            const auto& from=input.images[image];auto& to=storage.addresses[image];
            to.frames.resize(from.frames.size());to.palettes.resize(from.palettes.size());
            for (std::size_t frame=0;frame<from.frames.size();++frame) collect(from.frames[frame],to.frames[frame]);
            for (std::size_t palette=0;palette<from.palettes.size();++palette) collect(from.palettes[palette],to.palettes[palette]);
            storage.images[image]={to.frames,to.palettes};
        }
        banks_[bank]={input.bank_id,storage.images};
    }
    std::sort(payloads.begin(),payloads.end(),[](const Payload& a,const Payload& b) {
        return a.begin<b.begin || (a.begin==b.begin && a.end<b.end);
    });
    auto next=arena.offset;const auto end_arena=arena.offset+arena.size;
    for (std::size_t first=0;first<payloads.size();) {
        const auto begin=payloads[first].begin;auto end=payloads[first].end;auto last=first+1;
        // Preserve exact, nested and partial source aliases with one union upload.
        while (last<payloads.size() && payloads[last].begin<end) {
            end=std::max(end,payloads[last].end);++last;
        }
        const auto extent=end-begin;
        need(next<=end_arena && extent<=end_arena-next,Error::extent,static_cast<std::uint32_t>(next));
        const auto padded=(extent+7u)&~std::size_t(7u);
        need(padded<=end_arena-next,Error::extent,static_cast<std::uint32_t>(next));
        for (auto item=first;item<last;++item) {
            const auto offset=payloads[item].begin-begin;
            need(!(offset&7u),Error::malformed,static_cast<std::uint32_t>(begin));
            *payloads[item].address=static_cast<std::uint32_t>(next+offset);
        }
        const auto view=snapshot_.canonical().subspan(begin,extent);
        spans_.push_back({static_cast<std::uint32_t>(next),
            {reinterpret_cast<const std::uint8_t*>(view.data()),view.size()}});
        ranges_.push_back({next,extent});next+=padded;first=last;
    }
}
std::span<const EffectBankBinding> EffectGraphicsResources::bindings() const { require_live();return banks_; }
std::span<const sprite::AssetSpan> EffectGraphicsResources::installation_spans() const { require_live();return spans_; }
std::span<const RamRange> EffectGraphicsResources::reservations() const { require_live();return ranges_; }
} // namespace isle::menu
