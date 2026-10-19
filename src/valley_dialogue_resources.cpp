#include "valley_dialogue_resources.hpp"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace isle::valley {
namespace {
constexpr std::uint32_t base = 0x802d2740;
constexpr std::array<std::uint32_t,14> resource_ids{
    2150531864u,2150533400u,2150534936u,2150527256u,2150528792u,2150530328u,
    0x802e6b18u,0x802ec4d8u,
    0x802e97b8u,0x802ea058u,0x802ea8f8u,0x802eb198u,0x802eba38u,0x802ec2d8u};
}
std::shared_ptr<const DialogueResources> DialogueResources::load(const rom::Snapshot& snapshot, IsleValleyActorSource& source) {
    constexpr std::array<std::uint8_t,20> admitted{0xed,0xc7,0xc4,0x9c,0xc5,0x68,0xc0,0x45,0xfe,0x48,0xbe,0x0d,0x18,0x01,0x1c,0x30,0xf3,0x93,0xcb,0xaf};
    if (snapshot.identity().sha1 != admitted || snapshot.identity().size != rom::us_size || snapshot.canonical().size() != rom::us_size)
        throw std::invalid_argument("Valley resource snapshot admission");
    auto out = std::shared_ptr<DialogueResources>(new DialogueResources);
    const auto input = snapshot.canonical().subspan(0x7abcd0,0x825e30-0x7abcd0);
    auto bank = std::make_shared<sprite::Bank>(input.size());
    std::memcpy(bank->data(),input.data(),input.size());
    out->bank_ = bank;
    out->banks_[0] = {out->bank_,base};
    const auto word = [&](std::uint32_t address) {
        if (address < base || std::size_t(address-base)+4 > bank->size()) throw std::out_of_range("Valley resource word");
        const auto* b = bank->data() + (address-base);
        return (std::uint32_t(b[0])<<24)|(std::uint32_t(b[1])<<16)|(std::uint32_t(b[2])<<8)|b[3];
    };
    // Text stays in the caller's exact admitted ROM. Only pointer/extent/row
    // metadata belongs in the executable, never a duplicate asset byte string.
    const auto row = [&](IsleValleyDialogue& target, std::uint32_t address, std::uint32_t text, unsigned a, unsigned b, std::uint32_t operand, std::size_t bytes, bool name) {
        if (word(address)!=text || (*bank)[address-base+4]!=a || (*bank)[address-base+5]!=b ||
            word(address+8)!=operand || text<base || !bytes || std::size_t(text-base)+bytes>bank->size())
            throw std::invalid_argument("Valley qualified dialogue metadata");
        const auto* value = reinterpret_cast<const char*>(bank->data()+text-base);
        if (value[bytes-1]!='\0' || std::memchr(value,'\0',bytes-1))
            throw std::invalid_argument("Valley dialogue extent");
        target.unk_00 = value;
        target.unk_04 = static_cast<std::uint8_t>(a); target.unk_05 = static_cast<std::uint8_t>(b);
        target.argument_kind = name ? ISLE_VALLEY_DIALOGUE_TEXT : ISLE_VALLEY_DIALOGUE_NO_ARGUMENT;
        if (name) target.argument.text = source.D_8034E080_827610;
    };
    row(out->main_[0],2150548764u,2150551040u,0,0,2150948992u,47,true);
    row(out->main_[1],2150548776u,2150551088u,0,0,0u,57,false);
    row(out->main_[2],2150548788u,2150551148u,0,0,0u,61,false);
    row(out->main_[3],2150548800u,2150551212u,0,1,0u,62,false);
    row(out->main_[4],2150548812u,2150551276u,0,0,0u,65,false);
    row(out->main_[5],2150548824u,2150551344u,0,0,0u,55,false);
    row(out->main_[6],2150548836u,2150551400u,0,0,0u,76,false);
    row(out->main_[7],2150548848u,2150551476u,0,1,0u,68,false);
    row(out->main_[8],2150548860u,2150551544u,0,0,2150948992u,70,false);
    row(out->main_[9],2150548872u,2150551616u,0,0,0u,76,false);
    row(out->main_[10],2150548884u,2150551692u,0,0,0u,78,false);
    row(out->main_[11],2150548896u,2150551772u,1,0,0u,67,false);
    row(out->main_[12],2150548908u,2150551840u,0,0,0u,63,false);
    row(out->main_[13],2150548920u,2150551904u,0,0,0u,15,false);
    row(out->empty_[0],2150548944u,2150551920u,0,0,0u,24,false);
    if (word(0x802ec5c4) || word(0x802ec5dc)) throw std::invalid_argument("Valley dialogue terminator");
    for (std::size_t i=0;i<resource_ids.size();++i) {
        const auto decoded = sprite::hydrate(out->bank_,base,resource_ids[i]);
        if (!decoded) throw std::invalid_argument("Valley sprite hydration");
        out->sprites_[i] = decoded.asset;
        out->resources_[i] = {resource_ids[i],decoded.asset};
        out->roots_[i] = decoded.asset.view();
    }
    constexpr std::array<std::size_t,5> order{0,1,2,1,0};
    for (std::size_t i=0;i<5;++i) {
        if (word(0x802ec5f0+static_cast<std::uint32_t>(4*i))!=resource_ids[order[i]] || word(0x802ec608+static_cast<std::uint32_t>(4*i))!=resource_ids[3+order[i]]) throw std::invalid_argument("Valley sprite prefix");
        out->first_[i]=out->sprites_[order[i]].view();
        out->second_[i]=out->sprites_[3+order[i]].view();
    }
    if (word(0x802ec604) || word(0x802ec61c)) throw std::invalid_argument("Valley sprite terminator");
    for (std::size_t i=0;i<out->third_.size();++i) {
        if (word(0x802ec620+static_cast<std::uint32_t>(4*i))!=resource_ids[8+i])
            throw std::invalid_argument("Valley talking-face sprite table");
        out->third_[i]=out->sprites_[8+i].view();
    }
    return out;
}
IsleGameResult DialogueResources::dialogue(std::uint32_t id,std::size_t index,const IsleValleyDialogue** out) const noexcept {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    if (id==0x802ec51c && index<main_.size()) *out=&main_[index];
    else if (id==0x802ec5d0 && index<empty_.size()) *out=&empty_[index];
    else return ISLE_GAME_INVALID_ARGUMENT;
    return ISLE_GAME_OK;
}
IsleGameResult DialogueResources::sprite(std::uint32_t id,const IsleSprite** out) const noexcept {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    for (std::size_t i=0;i<resource_ids.size();++i) if (resource_ids[i]==id) { *out=sprites_[i].view(); return ISLE_GAME_OK; }
    return ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult DialogueResources::sprite_list(std::uint32_t id,const IsleSprite* const** out) const noexcept {
    if (!out) return ISLE_GAME_INVALID_ARGUMENT;
    *out=nullptr;
    if (id==0x802ec5f0) *out=first_.data();
    else if (id==0x802ec608) *out=second_.data();
    else if (id==0x802ec620) *out=third_.data();
    else return ISLE_GAME_INVALID_ARGUMENT;
    return ISLE_GAME_OK;
}
IsleGameResult DialogueResources::dialogue_format(char* buffer,std::size_t capacity,const IsleValleyDialogue* row,std::int32_t* count) noexcept {
    if (!count) return ISLE_GAME_INVALID_ARGUMENT;
    *count=0;
    if (!row || !row->unk_00 || (!buffer && capacity)) return ISLE_GAME_INVALID_ARGUMENT;
    // Two passes: reject malformed typed operands before publishing any bytes.
    std::size_t total=0;
    for (unsigned pass=0;pass<2;++pass) {
        std::size_t written=0; bool consumed=false;
        const auto emit=[&](char c) { if (pass && capacity && written<capacity-1) buffer[written]=c; ++written; };
        for (const char* p=row->unk_00;*p;++p) {
            if (*p!='%') { emit(*p); continue; }
            ++p;
            if (*p=='%') { emit('%'); continue; }
            if (consumed) return ISLE_GAME_INVALID_ARGUMENT;
            consumed=true;
            if (*p=='s' && row->argument_kind==ISLE_VALLEY_DIALOGUE_TEXT && row->argument.text) {
                for (const char* q=row->argument.text;*q;++q) emit(*q);
            } else if ((*p=='d' || *p=='i') && row->argument_kind==ISLE_VALLEY_DIALOGUE_INTEGER) {
                char digits[12];
                const auto result=std::to_chars(digits,digits+sizeof(digits),row->argument.integer);
                for (const char* q=digits;q!=result.ptr;++q) emit(*q);
            } else return ISLE_GAME_INVALID_ARGUMENT;
        }
        if (consumed != (row->argument_kind!=ISLE_VALLEY_DIALOGUE_NO_ARGUMENT) || written>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) return ISLE_GAME_INVALID_ARGUMENT;
        total=written;
    }
    if (capacity) buffer[std::min(total,capacity-1)]='\0';
    *count=static_cast<std::int32_t>(total);
    return ISLE_GAME_OK;
}
}
