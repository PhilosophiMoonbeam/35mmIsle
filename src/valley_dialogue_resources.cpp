#include "valley_dialogue_resources.hpp"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace isle::valley {
namespace {
constexpr std::uint32_t base = 0x802d2740;
constexpr std::array<std::uint32_t,8> resource_ids{2150531864u,2150533400u,2150534936u,2150527256u,2150528792u,2150530328u,0x802e6b18u,0x802ec4d8u};
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
    const auto row = [&](IsleValleyDialogue& target, std::uint32_t address, std::uint32_t text, unsigned a, unsigned b, std::uint32_t operand, const char* exact, std::size_t bytes, bool name) {
        if (word(address)!=text || (*bank)[address-base+4]!=a || (*bank)[address-base+5]!=b || word(address+8)!=operand || text<base || std::size_t(text-base)+bytes>bank->size() || std::memcmp(bank->data()+text-base,exact,bytes))
            throw std::invalid_argument("Valley qualified dialogue prefix");
        target.unk_00 = reinterpret_cast<const char*>(bank->data()+text-base);
        target.unk_04 = static_cast<std::uint8_t>(a); target.unk_05 = static_cast<std::uint8_t>(b);
        target.argument_kind = name ? ISLE_VALLEY_DIALOGUE_TEXT : ISLE_VALLEY_DIALOGUE_NO_ARGUMENT;
        if (name) target.argument.text = source.D_8034E080_827610;
    };
    row(out->main_[0],2150548764u,2150551040u,0,0,2150948992u,"\x5c\x42\x4f\x68\x21\x20\x48\x69\x2c\x20\x25\x73\x21\x0a\x57\x68\x61\x74\x20\x61\x20\x73\x75\x72\x70\x72\x69\x73\x65\x20\x74\x6f\x20\x73\x65\x65\x20\x79\x6f\x75\x20\x68\x65\x72\x65\x21\x00",47,true);
    row(out->main_[1],2150548776u,2150551088u,0,0,0u,"\x49\x20\x73\x75\x70\x70\x6f\x73\x65\x20\x79\x6f\x75\x27\x64\x20\x6c\x69\x6b\x65\x20\x74\x6f\x0a\x6b\x6e\x6f\x77\x20\x77\x68\x79\x20\x49\x20\x61\x6d\x20\x68\x65\x72\x65\x3f\x0a\x41\x68\x68\x2c\x20\x77\x65\x6c\x6c\x2e\x2e\x2e\x00",57,false);
    row(out->main_[2],2150548788u,2150551148u,0,0,0u,"\x49\x20\x72\x65\x63\x65\x69\x76\x65\x64\x20\x73\x6f\x6d\x65\x20\x69\x6e\x74\x65\x72\x65\x73\x74\x69\x6e\x67\x0a\x69\x6e\x66\x6f\x72\x6d\x61\x74\x69\x6f\x6e\x20\x61\x74\x0a\x74\x68\x65\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x20\x4c\x61\x62\x2e\x00",61,false);
    row(out->main_[3],2150548800u,2150551212u,0,1,0u,"\x49\x74\x20\x73\x65\x65\x6d\x73\x20\x74\x68\x61\x74\x20\x74\x68\x65\x72\x65\x20\x61\x72\x65\x20\x73\x69\x78\x0a\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x73\x20\x68\x69\x64\x64\x65\x6e\x20\x6f\x6e\x20\x74\x68\x69\x73\x0a\x69\x73\x6c\x61\x6e\x64\x2e\x00",62,false);
    row(out->main_[4],2150548812u,2150551276u,0,0,0u,"\x54\x68\x65\x79\x20\x68\x61\x76\x65\x20\x73\x6f\x6d\x65\x74\x68\x69\x6e\x67\x20\x74\x6f\x20\x64\x6f\x0a\x77\x69\x74\x68\x20\x69\x6d\x70\x6f\x72\x74\x61\x6e\x74\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x0a\x74\x68\x61\x74\x20\x61\x72\x65\x20\x68\x65\x72\x65\x2e\x00",65,false);
    row(out->main_[5],2150548824u,2150551344u,0,0,0u,"\x54\x68\x65\x20\x53\x69\x67\x6e\x73\x20\x61\x72\x65\x20\x73\x63\x65\x6e\x65\x72\x79\x20\x77\x68\x69\x63\x68\x0a\x73\x65\x65\x6d\x20\x74\x6f\x20\x72\x65\x73\x65\x6d\x62\x6c\x65\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x2e\x00",55,false);
    row(out->main_[6],2150548836u,2150551400u,0,0,0u,"\x4f\x6e\x65\x20\x53\x69\x67\x6e\x20\x69\x73\x20\x61\x20\x6d\x6f\x75\x6e\x74\x61\x69\x6e\x20\x74\x68\x61\x74\x0a\x6c\x6f\x6f\x6b\x73\x20\x6c\x69\x6b\x65\x20\x44\x55\x47\x54\x52\x49\x4f\x2e\x20\x49\x20\x63\x61\x6d\x65\x0a\x68\x65\x72\x65\x20\x74\x6f\x20\x6c\x6f\x6f\x6b\x20\x66\x6f\x72\x20\x69\x74\x2e\x00",76,false);
    row(out->main_[7],2150548848u,2150551476u,0,1,0u,"\x48\x6f\x77\x65\x76\x65\x72\x2c\x20\x49\x20\x63\x61\x6e\x27\x74\x20\x67\x6f\x20\x61\x6e\x79\x0a\x64\x65\x65\x70\x65\x72\x20\x69\x6e\x74\x6f\x20\x74\x68\x65\x20\x69\x73\x6c\x61\x6e\x64\x2c\x0a\x73\x6f\x20\x49\x20\x63\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x69\x74\x2e\x00",68,false);
    row(out->main_[8],2150548860u,2150551544u,0,0,2150948992u,"\x48\x61\x76\x65\x20\x79\x6f\x75\x20\x73\x65\x65\x6e\x20\x61\x20\x6d\x6f\x75\x6e\x74\x61\x69\x6e\x20\x74\x68\x61\x74\x0a\x6c\x6f\x6f\x6b\x73\x20\x6c\x69\x6b\x65\x20\x44\x55\x47\x54\x52\x49\x4f\x3f\x0a\x4f\x68\x21\x20\x49\x20\x68\x61\x76\x65\x20\x61\x6e\x20\x69\x64\x65\x61\x21\x00",70,false);
    row(out->main_[9],2150548872u,2150551616u,0,0,0u,"\x49\x66\x20\x79\x6f\x75\x20\x66\x69\x6e\x64\x20\x74\x68\x69\x73\x20\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x2c\x0a\x74\x61\x6b\x65\x20\x61\x20\x70\x69\x63\x74\x75\x72\x65\x20\x6f\x66\x20\x69\x74\x20\x6c\x69\x6b\x65\x20\x79\x6f\x75\x0a\x77\x6f\x75\x6c\x64\x20\x61\x20\x50\x6f\x6b\xa6\xc5\x6d\x6f\x6e\x2e\x00",76,false);
    row(out->main_[10],2150548884u,2150551692u,0,0,0u,"\x57\x65\x20\x6d\x69\x67\x68\x74\x20\x64\x69\x73\x63\x6f\x76\x65\x72\x20\x63\x6c\x75\x65\x73\x20\x66\x72\x6f\x6d\x0a\x74\x68\x65\x20\x70\x69\x63\x74\x75\x72\x65\x73\x20\x61\x6e\x64\x20\x74\x68\x65\x20\x70\x6c\x61\x63\x65\x73\x0a\x77\x68\x65\x72\x65\x20\x79\x6f\x75\x20\x74\x6f\x6f\x6b\x20\x74\x68\x65\x6d\x2e\x00",78,false);
    row(out->main_[11],2150548896u,2150551772u,1,0,0u,"\x53\x68\x61\x6c\x6c\x20\x49\x20\x72\x65\x70\x65\x61\x74\x20\x74\x68\x69\x73\x20\x69\x6e\x66\x6f\x72\x6d\x61\x74\x69\x6f\x6e\x3f\x0a\x5c\x61\x20\x59\x65\x73\x2c\x20\x70\x6c\x65\x61\x73\x65\x2e\x0a\x5c\x62\x20\x4e\x6f\x2c\x20\x74\x68\x61\x6e\x6b\x20\x79\x6f\x75\x2e\x00",67,false);
    row(out->main_[12],2150548908u,2150551840u,0,0,0u,"\x5c\x51\x57\x61\x69\x74\x2e\x2e\x2e\x0a\x49\x20\x77\x69\x6c\x6c\x20\x61\x64\x64\x20\x61\x20\x70\x61\x67\x65\x20\x66\x6f\x72\x20\x50\x4b\x4d\x4e\x20\x53\x69\x67\x6e\x73\x0a\x69\x6e\x20\x74\x68\x65\x20\x50\x4b\x4d\x4e\x20\x52\x65\x70\x6f\x72\x74\x2e\x00",63,false);
    row(out->main_[13],2150548920u,2150551904u,0,0,0u,"\x53\x65\x65\x20\x79\x6f\x75\x20\x6c\x61\x74\x65\x72\x21\x00",15,false);
    row(out->empty_[0],2150548944u,2150551920u,0,0,0u,"\x54\x68\x65\x72\x65\x27\x73\x20\x6e\x6f\x20\x6f\x6e\x65\x20\x74\x68\x65\x72\x65\x2e\x2e\x2e\x00",24,false);
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
