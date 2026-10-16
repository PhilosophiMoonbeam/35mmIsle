#include "oaks_lab_resources.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace isle::oaks_lab {
std::span<const std::uint8_t> Resources::bytes(std::uint32_t address,std::size_t size) const {
    const auto& bank=window_->banks()[2];
    if(address<bank.guest || address-bank.guest>bank.owner->size() || size>bank.owner->size()-(address-bank.guest))
        throw std::out_of_range("Oak initialized-data extent");
    return std::span(*bank.owner).subspan(address-bank.guest,size);
}
std::uint32_t Resources::word(std::uint32_t address) const {
    const auto b=bytes(address,4);
    return (std::uint32_t(b[0])<<24)|(std::uint32_t(b[1])<<16)|(std::uint32_t(b[2])<<8)|b[3];
}
float Resources::real(std::uint32_t address) const { return std::bit_cast<float>(word(address)); }
std::string_view Resources::text(std::uint32_t address) const {
    return window_->text(window::ResourceBank::Group::oak,address);
}
void Resources::buttons(std::uint32_t address,std::span<window::Button> out) {
    std::fill(out.begin(),out.end(),window::Button{});
    for(std::size_t i=0;i<out.size();++i) {
        const auto id=word(address+static_cast<std::uint32_t>(i*8));
        const auto string=word(address+static_cast<std::uint32_t>(i*8+4));
        if(id>35 || (id==34 && string)) throw std::invalid_argument("Oak source button table");
        out[i]={static_cast<int>(id),string?text(string):std::string_view{}};
        if(id==35) return;
    }
    throw std::invalid_argument("Oak source button table terminator");
}
std::shared_ptr<const Resources> Resources::load(const rom::Snapshot& snapshot,std::shared_ptr<const window::Resources> window) {
    if(!window) throw std::invalid_argument("Oak window resources");
    constexpr std::array<std::uint8_t,20> admitted{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    if(snapshot.identity().sha1!=admitted || snapshot.canonical().size()<0x59f2b)throw std::invalid_argument("Oak species-table ROM identity");
    auto out=std::shared_ptr<Resources>(new Resources);
    out->window_=std::move(window);
    const auto species=snapshot.canonical().subspan(0x59e94,151);
    std::array<bool,63> seen{};
    for(unsigned i=0;i<151;++i){
        const auto index=std::bit_cast<std::int8_t>(species[i]);
        if(index< -1||index>62||(index>=0&&seen[static_cast<unsigned>(index)]))throw std::invalid_argument("Oak source report-index table");
        out->species_[i]=index;if(index>=0)seen[static_cast<unsigned>(index)]=true;
    }
    if(std::find(seen.begin(),seen.end(),false)!=seen.end())throw std::invalid_argument("Oak source report-index closure");
    const auto all=out->window_->banks();
    if(all.size()!=3 || all[2].rom_begin!=0x8ac010 || all[2].rom_end!=0x98c330 || all[2].guest!=0x800e67f0)
        throw std::invalid_argument("Oak private initialized bank identity");
    out->banks_={sprite::BankMapping{all[0].owner,all[0].guest},sprite::BankMapping{all[2].owner,all[2].guest}};
    out->sprites_.reserve(out->window_->sprites(window::ResourceBank::Group::common).size()+out->window_->sprites(window::ResourceBank::Group::oak).size());
    for(auto group:{window::ResourceBank::Group::common,window::ResourceBank::Group::oak})
        for(const auto& resource:out->window_->sprites(group)) out->sprites_.push_back(resource);
    out->roots_.reserve(out->sprites_.size());
    out->indices_.reserve(out->sprites_.size());
    for(std::size_t i=0;i<out->sprites_.size();++i){out->roots_.push_back(out->sprites_[i].sprite.view());out->indices_.emplace_back(out->sprites_[i].identity,i);}
    std::sort(out->indices_.begin(),out->indices_.end());
    out->buttons(0x801958f0,out->main_[0]);
    out->buttons(0x801958c0,out->main_[1]);
    for(unsigned i=0;i<7;++i) out->buttons(out->word(0x80195cec+i*4),out->courses_[i]);
    // Complete reached script rodata is NUL-bounded within this same private
    // initialized bank. These are addresses, never compiled protected strings.
    for(auto address:{0x801c5530u,0x801c5550u,0x801c5810u,0x801c5840u,0x801c5850u,0x801c5880u,
                     0x801c58b0u,0x801c5950u,0x801c598cu,0x801c59c4u,0x801c5a08u,0x801c5a4cu,
                     0x801c5aa0u,0x801c5ae0u,0x801c5b24u,0x801c6a7cu,0x801c6a94u,0x801c6aacu,0x801c6ac8u})
        (void)out->text(address);
    return out;
}
scene::ResourceView Resources::view() const noexcept { return {{128,64,0,4,1024},sprites_}; }
std::span<const window::Button> Resources::main_buttons(bool report) const noexcept { return main_[report?1:0]; }
std::span<const window::Button> Resources::course_buttons(unsigned unlocked) const {
    if(unlocked>=courses_.size()) throw std::out_of_range("Oak unlocked course table");
    return courses_[unlocked];
}
std::size_t Resources::sprite_index(std::uint32_t identity) const {
    const auto it=std::lower_bound(indices_.begin(),indices_.end(),identity,[](const auto& entry,std::uint32_t key){return entry.first<key;});
    if(it==indices_.end()||it->first!=identity)throw std::out_of_range("Oak sprite resource identity");
    return it->second;
}
int Resources::species_index(unsigned id) const noexcept {return id>0&&id<=151?species_[id-1]:-1;}
} // namespace isle::oaks_lab
