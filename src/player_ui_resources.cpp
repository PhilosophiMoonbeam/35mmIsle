#include "player_ui_resources.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
namespace isle::player {
namespace {
constexpr std::uint32_t source_roots[] = {
 0x80366df0,0x80367580,0x80367d10,0x80367dc0,0x80368038,
 0x803706e0,0x80378d88,0x80378ed8,0x8037a9a0,0x8037c468,
 0x8037d2a0,0x8037e0d8,0x8037ef10,0x8037fd48,0x80380b80,0x803819b8,0x80381b58,
 0x80383b48,0x803841c8,0x80384e68,0x80385b08,0x803867a8,0x80387448,0x803880e8,
 0x80388e00,0x80388f60,0x803890c0,0x80389220,0x8038c010,0x80393628
};
}
std::uint32_t UiResources::word(std::uint32_t address) const {
 if(address<bank_base || address-bank_base>bank_->size()-4) throw std::invalid_argument("Player UI metadata outside admitted bank");
 const auto* p=bank_->data()+address-bank_base;
 return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
UiResources::UiResources(const rom::Snapshot& rom) {
 const auto input=rom.canonical();
 if(input.size()<rom_end) throw std::invalid_argument("Player UI requires admitted complete ROM");
 auto writable=std::make_shared<sprite::Bank>(rom_end-rom_begin);
 std::memcpy(writable->data(),input.data()+rom_begin,writable->size());
 bank_=std::move(writable); banks_[0]={bank_,bank_base};
 resources_.reserve(std::size(source_roots));
 for(auto address:source_roots) {
  auto result=sprite::hydrate(bank_,bank_base,address);
  if(!result) throw std::invalid_argument("Player UI source sprite hydration failed");
  resources_.push_back({address,std::move(result.asset)});
 }
 // Film's writable buf words are originally rebound to these ten exact pixel
 // identities. Prepare ten immutable native descriptor variants once instead.
 const auto& prototype=sprite(0x80388f60);
 roots_.reserve(resources_.size()+10+glyphs_.size());
 for(const auto& r:resources_) roots_.push_back(r.sprite.view());
 for(unsigned i=0;i<10;++i) {
  const auto pixel=word(0x8038a034+i*4);
  auto& bitmap=digit_bitmaps_[i]; bitmap=*prototype.bitmap;
  if(pixel<bank_base || pixel-bank_base>bank_->size()-bitmap.byte_count)
   throw std::invalid_argument("Player film digit pixels outside admitted bank");
  bitmap.buf=bank_->data()+pixel-bank_base;
  auto& digit=digit_sprites_[i]; digit=prototype; digit.bitmap=&bitmap;
  digits_[i]=roots_.size(); roots_.push_back(&digit);
 }
 // Msg's 14x14 S load is pre-swapped with a 64-bit-padded row pitch.
 // Retain the real 16x13 source plane; Producer emits the exact 196-texel
 // partial load and 14x14 nonclamping tile, not an invented repacked image.
 const auto& rgb=sprite(0x8038c010); const auto& mono=sprite(0x80393628);
 for(unsigned i=0;i<glyphs_.size();++i) {
  const bool color=i<16;
  const auto& original=(color?rgb:mono);
  const auto& source=original.bitmap[color?i:i-16];
  auto& b=glyph_bitmaps_[i]; b=source;
  b.width=14; b.s=b.t=0;
  if(!b.buf || b.width_img!=16 || b.actualHeight!=13)
   throw std::invalid_argument("Player source glyph plane metadata mismatch");
  auto& s=glyphs_[i]; s=original; s.width=s.height=14;
  s.attr=ISLE_SP_TRANSPARENT|ISLE_SP_SCALE|ISLE_SP_TEXSHUF; s.istart=0; s.istep=1;
  s.nbitmaps=1; s.bmheight=s.bmHreal=14; s.bitmap=&b;
  s.scalex=s.scaley=1; s.LUT=nullptr; s.LUT_bytes=0; s.nTLUT=0;
  roots_.push_back(&s);
 }
 for(unsigned i=0;i<wide_.size();++i) {
  const auto offset=0x8039379c-bank_base+i*2;
  wide_[i]=std::bit_cast<std::int16_t>(std::uint16_t(((*bank_)[offset]<<8)|(*bank_)[offset+1]));
 }
}
const IsleSprite& UiResources::sprite(std::uint32_t identity) const {
 const auto it=std::find_if(resources_.begin(),resources_.end(),[&](const auto& r){return r.identity==identity;});
 if(it==resources_.end()) throw std::invalid_argument("unknown Player UI source sprite");
 return *it->sprite.view();
}
const IsleSprite& UiResources::digit(unsigned value) const {
 if(value>=10) throw std::invalid_argument("Player film digit outside original table");
 return *roots_[digits_[value]];
}
std::size_t UiResources::index(const IsleSprite& view) const {
 for(std::size_t i=0;i<roots_.size();++i) if(roots_[i]==&view ||
   (roots_[i]->bitmap==view.bitmap && roots_[i]->bmfmt==view.bmfmt && roots_[i]->bmsiz==view.bmsiz)) return i;
 throw std::invalid_argument("unregistered Player UI sprite descriptor");
}
std::size_t UiResources::glyph_index(const std::uint8_t* pixels) const {
 for(std::size_t i=0;i<glyphs_.size();++i) if(glyph_bitmaps_[i].buf==pixels) return resources_.size()+10+i;
 throw std::invalid_argument("unregistered Player message glyph");
}
const IsleSprite& UiResources::glyph(const std::uint8_t* pixels) const {return *roots_[glyph_index(pixels)];}
void UiResources::initialize(IslePlayerUiSource& ui) const noexcept {
 ui.Msg_TextColorR=ui.Msg_TextColorG=ui.Msg_TextColorB=ui.Msg_TextColorA=255;
 ui.Msg_LineSpacing=4;ui.Msg_ShadowEnabled=1;ui.Msg_BlinkAlpha=1;
 ui.Message_ScaleX=ui.Message_ScaleY=1;
 ui.font_rgb_bitmaps=sprite(0x8038c010).bitmap;
 ui.font_mono_bitmaps=sprite(0x80393628).bitmap;
 ui.D_803936B8_533AC8=bank_->data()+0x803936b8-bank_base;
 ui.D_80393738_533B48=bank_->data()+0x80393738-bank_base;
 ui.D_8039379C_533BAC=wide_.data();
 constexpr std::uint32_t positions[9][2]={{148,104},{124,80},{148,80},{172,80},{124,104},{172,104},{124,128},{148,128},{172,128}};
 for(unsigned i=0;i<9;++i){ui.Icons_IconObjects[i].x=positions[i][0];ui.Icons_IconObjects[i].y=positions[i][1];}
 ui.D_80388204_528614[0]=163;ui.D_80388204_528614[1]=162;ui.D_80388204_528614[2]=161;
 ui.Icons_IsShown=1;ui.Icons_NumItemsAvailable=3;
 ui.Icons_IconDefs[0]={210,189,0,0,2,&sprite(0x80387448)};
 ui.Icons_IconDefs[1]={182,185,0,0,0,&sprite(0x803867a8)};
 ui.Icons_IconDefs[2]={238,185,0,0,4,&sprite(0x803880e8)};
 ui.Icons_IconDefs[3]={268,189,0,1,6,&sprite(0x80383b48)};
 ui.Icons_IconDefs[4]={268,155,0,1,8,&sprite(0x80384e68)};
 ui.Icons_IconDefs[5]={316,155,1,1,4,&sprite(0x80384e68)};
 ui.Icons_IconDefs[6]={238,233,1,0,0,&sprite(0x80385b08)};
 ui.Icons_IconDefs[7]={316,189,1,1,2,&sprite(0x803841c8)};
 ui.D_80388E00_529210=&sprite(0x80388e00);ui.D_80388F60_529370=&sprite(0x80388f60);
 ui.D_803890C0_5294D0=&sprite(0x803890c0);ui.D_80389220_529630=&sprite(0x80389220);
}
}
