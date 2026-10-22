#include "sprite_producer.hpp"
#include "scene_render.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
namespace sp = isle::sprite;
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F&& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
struct Fixture {
    std::shared_ptr<sp::Bank> bytes = std::make_shared<sp::Bank>(128, 0);
    std::array<IsleBitmap, 2> bitmap{};
    IsleSprite sprite{};
    Fixture() {
        for (unsigned i = 0; i < 24; ++i) (*bytes)[8 + i] = static_cast<std::uint8_t>(i + 1);
        bitmap[0] = {4, 4, 0, 0, bytes->data() + 8, 16, 2, 0};
        bitmap[1] = {4, 4, 0, 0, bytes->data() + 24, 8, 1, 0};
        sprite.width = 4; sprite.height = 3; sprite.scalex = sprite.scaley = 1;
        sprite.attr = ISLE_SP_TEXSHUF; sprite.red = sprite.green = sprite.blue = sprite.alpha = 255;
        sprite.istep = 1; sprite.nbitmaps = 2; sprite.bmheight = sprite.bmHreal = 2;
        sprite.bmfmt = 0; sprite.bmsiz = 2; sprite.bitmap = bitmap.data();
    }
};
std::vector<std::uint32_t> op(std::span<const std::uint32_t> words, unsigned code) {
    std::vector<std::uint32_t> result;
    for (std::size_t i = 0; i < words.size(); i += 2)
        if ((words[i] >> 24) == code) { result.push_back(words[i]); result.push_back(words[i + 1]); }
    return result;
}
void installation_and_transactions() {
    Fixture f;
    const std::array<const IsleSprite*, 1> roots{&f.sprite};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4, 0xa5a5a5a5);
    const auto original = ram;
    const std::array<sp::RamRange, 1> collision{{{0x300010, 8}}};
    rejects([&] { producer.install(ram, 7, collision); }, "asset collision must reject");
    check(ram == original, "failed install writes nothing");
    producer.install(ram, 7, {});
    check(ram[0x300004 / 4] == 0xa5a5a5a5 && ram[0x300020 / 4] == 0xa5a5a5a5,
          "exact selected asset spans preserve adjacent guards");
    check(ram[0x300008 / 4] == 0x01020304 && ram[0x30001c / 4] == 0x15161718,
          "canonical source bytes become exact word-swapped RDRAM words");
    const std::array<sp::Draw, 1> draws{{{0, &f.sprite}}};
    const auto installed = ram;
    rejects([&] { producer.prepare(ram, 8, draws, {}, {0x150000, 4096}); }, "wrong RAM epoch rejects");
    rejects([&] { producer.prepare(ram, 7, draws, {}, {0x150000, 8}); }, "command exhaustion rejects");
    auto invalid = f.sprite; invalid.attr |= ISLE_SP_EXTERN;
    const std::array<sp::Draw, 2> late_bad{{{0, &f.sprite}, {0, &invalid}}};
    rejects([&] { producer.prepare(ram, 7, late_bad, {}, {0x150000, 4096}); }, "late invalid draw rejects");
    check(ram == installed, "all preflight failures preserve every installed and unowned RAM word");
    auto wrong = f.sprite; wrong.bmsiz = 3;
    const std::array<sp::Draw, 1> bad_layout{{{0, &wrong}}};
    rejects([&] { producer.prepare(ram, 7, bad_layout, {}, {0x150000, 4096}); }, "mutated layout cannot use stale plan");
    const auto list = producer.prepare(ram, 7, draws, {}, {0x150000, 4096});
    check(list.rectangles == 2, "short final piece draws only actual stored height");
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    const auto loads = op(words, 0xf3);
    check(loads.size() == 4 && loads[1] == 0x07007000 && loads[3] == 0x07003000,
          "pre-shuffled load blocks use exact short extents and zero dxt");
    const auto rectangles = op(words, 0xe4);
    check(rectangles.size() == 4 && rectangles[0] == 0xe4010008 && rectangles[2] == 0xe401000c &&
          rectangles[3] == 8, "piece placement advances in source row-major order");
    check(ram[(list.address + static_cast<std::uint32_t>(list.words * 4)) / 4] == 0xa5a5a5a5,
          "command end preserves its immediate guard");
}
void bitmap_step_preserves_source_geometry() {
    Fixture f;
    f.sprite.width = 8; f.sprite.height = 6; f.sprite.istep = 0;
    const std::array<const IsleSprite*, 1> roots{&f.sprite};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4, 0xa5a5a5a5);
    producer.install(ram, 1, {});
    auto view = f.sprite;
    const std::array<sp::Draw, 1> draws{{{0, &view}}};
    const auto prepare = [&] {
        return producer.prepare(ram, 1, draws, {}, {0x150000, 4096});
    };
    const auto repeated = prepare();
    const auto words = std::span(ram).subspan(repeated.address / 4, repeated.words);
    const auto rectangles = op(words, 0xe4), textures = op(words, 0xfd);
    const std::vector<std::uint32_t> expected{
        0xe4010008, 0x00000000, 0xe4020008, 0x00010000,
        0xe4010010, 0x00000008, 0xe4020010, 0x00010008,
        0xe4010018, 0x00000010, 0xe4020018, 0x00010010};
    check(rectangles == expected,
          "zero bitmap step fills every source tile across the complete 8x6 extent");
    check(textures == std::vector<std::uint32_t>{0xfd100000, 0x300008},
          "consecutive repeated tiles share one admitted texture upload and unchanged tile setup");
    check(repeated.words == 82, "six repeated rectangles use one setup and four commands per subsequent tile");
    const auto installed = ram;
    rejects([&] {
        producer.prepare(ram, 1, draws, {},
                         {0x150000, static_cast<std::uint32_t>(repeated.words * 4 - 8)});
    }, "zero-step repetition preflights the entire command extent");
    check(ram == installed, "zero-step command exhaustion publishes no partial geometry");

    view.istep = 1;
    const auto advancing = prepare();
    const auto advancing_words = std::span(ram).subspan(advancing.address / 4, advancing.words);
    check(op(advancing_words, 0xe4) == std::vector<std::uint32_t>{
              0xe4010008, 0x00000000, 0xe4020004, 0x00010000},
          "positive step preserves inventory termination and the short second bitmap height");
    const auto advancing_textures = op(advancing_words, 0xfd);
    check(advancing_textures.size() == 4 && advancing_textures[1] == 0x300008 &&
          advancing_textures[3] == 0x300018,
          "positive step advances to the next admitted bitmap bytes");
    view.istep = 2;
    const auto skipped = prepare();
    check(op(std::span(ram).subspan(skipped.address / 4, skipped.words), 0xe4) ==
              std::vector<std::uint32_t>{0xe4010008, 0x00000000},
          "larger positive bitmap step preserves source inventory skipping");
    view.istep = 0; view.istart = 1;
    const auto offset = prepare();
    const auto offset_words = std::span(ram).subspan(offset.address / 4, offset.words);
    const auto offset_textures = op(offset_words, 0xfd);
    check(op(offset_words, 0xe4) == std::vector<std::uint32_t>{
              0xe4010004, 0x00000000, 0xe4020004, 0x00010000,
              0xe401000c, 0x00000008, 0xe402000c, 0x00010008,
              0xe4010014, 0x00000010, 0xe4020014, 0x00010010},
          "zero step keeps the selected short bitmap and original row stride");
    check(offset_textures == std::vector<std::uint32_t>{0xfd100000, 0x300018},
          "zero step honors a nonzero starting bitmap with one consecutive texture upload");
    view.istep = 1;
    const auto bounded = prepare();
    check(bounded.rectangles == 1,
          "positive step with nonzero start stays inside the admitted native inventory");
    view.istart = 0; view.istep = 0; view.width = 6; view.height = 3;
    const auto nondivisible = prepare();
    check(op(std::span(ram).subspan(nondivisible.address / 4, nondivisible.words), 0xe4) ==
              std::vector<std::uint32_t>{0xe4010008, 0},
          "Source pre-draw wrap admits one complete 4x2 bitmap into a 6x3 sprite");
    view.width = 8; view.height = 6; view.attr |= ISLE_SP_SCALE;
    view.scalex = 2; view.scaley = 3; view.expx = 3; view.expy = 5;
    const auto expanded = prepare();
    const auto expansion = op(std::span(ram).subspan(expanded.address / 4, expanded.words), 0xe4);
    check(expanded.rectangles == 6 && expansion[2] == 0xe404c018 &&
          expansion[3] == 0x0002c000 && expansion[4] == 0xe4020044 && expansion[5] == 44,
          "non-unit scale applies explosion offsets in destination pixels, after scaling, on each column and row");
}
void actual_height_is_not_row_stride() {
    Fixture f;
    f.sprite.bmheight = 1; f.sprite.width = 4; f.sprite.height = 3; f.sprite.istep = 0;
    const std::array<const IsleSprite*, 1> roots{&f.sprite};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4);
    producer.install(ram, 1, {});
    auto view = f.sprite;
    const std::array<sp::Draw, 1> draws{{{0, &view}}};
    const auto list = producer.prepare(ram, 1, draws, {}, {0x150000, 4096});
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    check(op(words, 0xe4) == std::vector<std::uint32_t>{
              0xe4010008, 0, 0xe401000c, 4},
          "actualHeight taller than bmheight draws full overlapping pieces, then rejects the next incomplete row");
    check(op(words, 0xfd).size() == 2,
          "overlapping source rows do not invalidate an unchanged resident texture");
    view.height = 1; view.istep = 1; view.nbitmaps = f.sprite.nbitmaps;
    const auto first = producer.prepare(ram, 1, draws, {}, {0x150000, 4096});
    check(op(std::span(ram).subspan(first.address / 4, first.words), 0xe4) ==
              std::vector<std::uint32_t>{0xe4010008, 0},
          "Source row-fit test occurs after wrapping, not as an invented initial-row crop");
}
void consecutive_texture_setup_and_transitions() {
    auto bank = std::make_shared<sp::Bank>(64, 0xff);
    const std::array<IsleBitmap, 6> bitmap{{
        {4,4,0,0,bank->data(),32,4,0},
        {2,4,1,0,bank->data(),32,4,0},
        {4,8,0,0,bank->data(),32,2,0},
        {4,4,0,0,bank->data(),32,4,0},
        {4,4,0,0,bank->data()+32,32,4,0},
        {4,4,0,0,bank->data()+32,32,4,0}}};
    IsleSprite s{};
    s.width = 22; s.height = s.bmheight = s.bmHreal = 4;
    s.scalex = s.scaley = 1; s.red = s.green = s.blue = s.alpha = 255;
    s.bitmap = bitmap.data(); s.nbitmaps = 6; s.istep = 1; s.bmfmt = 0; s.bmsiz = 2;
    const std::array<const IsleSprite*, 1> roots{&s};
    const std::array<sp::BankMapping, 1> banks{{{bank, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4, 0xa5a5a5a5);
    producer.install(ram, 1, {});
    const std::array<sp::Draw, 2> draws{{{0, &s}, {0, &s}}};
    const auto list = producer.prepare(ram, 1, draws, {}, {0x150000, 4096});
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    const auto images = op(words, 0xfd), tiles = op(words, 0xf2), origins = op(words, 0xe1);
    check(list.rectangles == 12 && images.size() == 16 && tiles.size() == 16,
          "equal resident setup reuses TMEM, changed width/height reloads, and every Sprite resets the cache");
    for (std::size_t i = 0; i < 8; ++i)
        check(images[i * 2 + 1] == (i % 4 == 3 ? 0x300020u : 0x300000u),
              "A/A/B/A/C/C texture transition preserves exact address order in both Sprites");
    check(tiles[1] == 0x0000c00c && tiles[3] == 0x0001c004 && tiles[5] == 0x0000c00c,
          "same address with a changed TMEM interpretation cannot reuse the preceding render tile");
    check(origins[3] == 0x00200000,
          "resident texture reuse still emits the selected descriptor's independent S origin");
    const auto before = ram;
    rejects([&] {
        producer.prepare(ram, 1, draws, {},
                         {0x150000, static_cast<std::uint32_t>(list.words * 4 - 8)});
    }, "texture transitions and cache resets participate in complete capacity preflight");
    check(ram == before, "late cached-list capacity rejection cannot partially write any RAM");
}
void camera_sized_repetition_is_compact() {
    auto bank = std::make_shared<sp::Bank>(16 * 16 * 2, 0xff);
    IsleBitmap bitmap{16,16,0,0,bank->data(),bank->size(),16,0};
    IsleSprite s{};
    s.width = 640; s.height = 480; s.bmheight = s.bmHreal = 16;
    s.scalex = s.scaley = 1; s.red = s.green = s.blue = s.alpha = 255;
    s.bitmap = &bitmap; s.nbitmaps = 1; s.istep = 0; s.bmfmt = 0; s.bmsiz = 2;
    const std::array<const IsleSprite*, 1> roots{&s};
    const std::array<sp::BankMapping, 1> banks{{{bank, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4, 0xa5a5a5a5);
    producer.install(ram, 1, {});
    const std::array<sp::Draw, 1> draws{{{0, &s}}};
    const auto list = producer.prepare(ram, 1, draws, {}, {0x150000, 0x5550});
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    check(list.rectangles == 300 && list.words == 2434 && op(words, 0xfd).size() == 2,
          "Camera-shaped 16x16 source repetition clips to 300 rectangles with exactly one texture setup");
    const auto rectangles = op(words, 0xe4);
    check(rectangles.front() == 0xe4040040 && rectangles[rectangles.size() - 2] == 0xe45003c0 &&
          rectangles.back() == 0x004c0380,
          "compact repeated texture commands retain the full decoded first-to-last source rectangle sequence");
    const auto before = ram;
    rejects([&] {
        producer.prepare(ram, 1, draws, {},
                         {0x150000, static_cast<std::uint32_t>(list.words * 4 - 8)});
    }, "Camera-shaped compact commands still require their complete exact arena");
    check(ram == before, "compact Camera-shaped capacity failure preserves RAM atomically");
}
void repeated_bitmap_nonprogress_rejects_atomically() {
    for (const bool invalid_row : {false, true}) {
        Fixture f;
        f.sprite.istep = 0;
        if (invalid_row) f.sprite.bmheight = 0;
        else { f.bitmap[0].width = -1; f.bitmap[0].buf = nullptr; }
        const std::array<const IsleSprite*, 1> roots{&f.sprite};
        const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
        sp::Producer producer(roots, banks);
        std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4, 0xa5a5a5a5);
        producer.install(ram, 1, {});
        auto hidden = f.sprite; hidden.attr |= ISLE_SP_HIDDEN;
        const std::array<sp::Draw, 2> draws{{{0, &hidden}, {0, &f.sprite}}};
        const auto before = ram;
        rejects([&] { producer.prepare(ram, 1, draws, {}, {0x150000, 4096}); },
                "nonprogressing repeated bitmap geometry rejects");
        check(ram == before, "nonprogress rejection preserves every RAM word before publication");
    }
    Fixture terminal;
    terminal.sprite.istep=0; terminal.sprite.bmheight=0;
    terminal.bitmap[0].width=0; terminal.bitmap[0].buf=nullptr;
    const std::array<const IsleSprite*,1> roots{&terminal.sprite};
    const std::array<sp::BankMapping,1> banks{{{terminal.bytes,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4,0xa5a5a5a5);
    producer.install(ram,1,{});
    const std::array<sp::Draw,1> draws{{{0,&terminal.sprite}}};
    const auto stopped=producer.prepare(ram,1,draws,{},{0x150000,4096});
    const auto words=std::span(ram).subspan(stopped.address/4,stopped.words);
    check(op(words,0xe4).empty() && op(words,0xfd).empty(),
          "zero-width bitmap terminates before requiring repeated geometry progress");
}
void mutable_state_and_scissor() {
    Fixture f;
    const std::array<const IsleSprite*, 1> roots{&f.sprite};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8 * 1024 * 1024 / 4);
    producer.install(ram, 1, {});
    auto view = f.sprite;
    view.x = -1; view.y = 10; view.attr |= ISLE_SP_SCALE | ISLE_SP_TRANSPARENT;
    view.scalex = 2; view.scaley = 2; view.red = 31; view.green = 67; view.blue = 127; view.alpha = 91;
    const std::array<sp::Draw, 1> draws{{{0, &view}}};
    const auto list = producer.prepare(ram, 1, draws, {{0,0,320,240}, {0,10,6,14}}, {0x150000,4096});
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    check(list.rectangles == 1, "scissor removes whole second piece");
    const auto color = op(words, 0xfa);
    check(color.size() == 2 && color[1] == 0x1f437f5b, "shallow mutable tint and alpha reach primitive state");
    const auto rectangle = op(words, 0xe4), start = op(words, 0xe1), derivative = op(words, 0xf1);
    check(rectangle[0] == (0xe4000000 | (24u << 12) | 56u) && rectangle[1] == 40,
          "scaled bounds intersect viewport/scissor before encoding");
    check(start[1] == 0x00100000 && derivative[1] == 0x02000200,
          "left clip advances S and scales hardware derivatives");
    view.attr |= ISLE_SP_HIDDEN;
    check(producer.prepare(ram, 1, draws, {}, {0x150000,4096}).rectangles == 0,
          "hidden scalar suppresses texture rectangles without rehydration");
    view.attr &= ~ISLE_SP_HIDDEN; view.x = 20; view.scalex = -1;
    const auto flipped = producer.prepare(ram, 1, draws, {}, {0x150000,4096});
    const auto flip_words = std::span(ram).subspan(flipped.address / 4, flipped.words);
    check(op(flip_words,0xf1)[1] == 0xfc000200 && op(flip_words,0xe1)[1] == 0x00600000,
          "negative scale flips S using signed derivatives and last-texel origin");
}
void ordinary_sprite_edges_and_fractional_origins() {
    auto bank = std::make_shared<sp::Bank>(16, 0xff);
    const std::array<IsleBitmap,4> bitmap{{
        {1,1,0,0,nullptr,0,2,0}, {4,4,0,0,bank->data(),16,2,0},
        {1,1,0,0,nullptr,0,2,0}, {4,4,0,0,bank->data(),16,2,0}}};
    IsleSprite source{};
    source.x = source.y = 10;
    source.width = 5; source.height = 3; source.bmheight = 1; source.bmHreal = 2;
    source.scalex = source.scaley = 1.5f; source.attr = ISLE_SP_SCALE;
    source.red = source.green = source.blue = source.alpha = 255;
    source.nbitmaps = 4; source.istep = 1; source.bitmap = bitmap.data();
    source.bmfmt = 0; source.bmsiz = 2;
    const std::array<const IsleSprite*,1> roots{{&source}};
    const std::array<sp::BankMapping,1> banks{{{bank,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    producer.install(ram,1,{});
    auto view = source;
    const std::array<sp::Draw,1> draws{{{0,&view}}};
    const auto prepare = [&](sp::Camera camera) {
        const auto list = producer.prepare(ram,1,draws,camera,{0x150000,4096});
        return std::span<const std::uint32_t>(ram).subspan(list.address/4,list.words);
    };
    const auto words = prepare({});
    check(op(words,0xe4) == std::vector<std::uint32_t>{
              0xe4048034,0x00030028,0xe404803c,0x00030030},
          "half-pixel bitmap origins produce integer destination edges on both source rows");
    check(op(words,0xf1) == std::vector<std::uint32_t>{
              0xf1000000,0x02ab02ab,0xf1000000,0x02ab02ab},
          "a 1.5 scale rounds the ordinary reciprocal to 683, not 682");
    check(op(words,0xe1) == std::vector<std::uint32_t>{
              0xe1000000,0x000b0000,0xe1000000,0x000b000b},
          "rounded half-pixel origins retain eleven thirty-seconds of fractional source phase");
    const auto clipped = prepare({{0,0,320,240},{13,13,17,15}});
    check(op(clipped,0xe4) == std::vector<std::uint32_t>{0xe404403c,0x00034034} &&
          op(clipped,0xe1) == std::vector<std::uint32_t>{0xe1000000,0x00200020},
          "integer scissoring advances the already rounded origin independently on both axes");
    view.attr |= ISLE_SP_TEXSHIFT | ISLE_SP_FRACPOS;
    view.frac_s = -7; view.frac_t = 3;
    view.expx = 3; view.expy = 5;
    const auto shifted = prepare({});
    check(op(shifted,0xe4) == std::vector<std::uint32_t>{
              0xe4054034,0x0003c028,0xe4054050,0x0003c044} &&
          op(shifted,0xe1) == std::vector<std::uint32_t>{
              0xe1000000,0x00140013,0xe1000000,0x0014001e},
          "explosion moves integer edges without changing the independent fractional and half-texel phase");
    view = source; view.y = -1;
    check(op(prepare({}),0xe1)[1] == 0x000b0015,
          "a negative integer position preserves biased-cast correction rather than substituting ceil");
    view = source; view.scalex = 2.00005f;
    const auto near_integer = op(prepare({}),0xe4);
    check((near_integer[1] >> 12) == 48 && ((near_integer[0] >> 12) & 0xfff) == 84,
          "the source edge bias retains positions just above an integer instead of applying unconditional ceil");
}
void message_derivatives_remain_independent() {
    auto bank = std::make_shared<sp::Bank>(208,0xff);
    IsleBitmap bitmap{14,16,0,0,bank->data(),bank->size(),13,0};
    IsleSprite source{};
    source.x = source.y = 10; source.width = source.height = 14;
    source.bmheight = source.bmHreal = 13;
    source.scalex = source.scaley = 1.5f;
    source.attr = ISLE_SP_SCALE | ISLE_SP_TEXSHUF;
    source.red = source.green = source.blue = source.alpha = 255;
    source.nbitmaps = source.istep = 1; source.bitmap = &bitmap;
    source.bmfmt = 4; source.bmsiz = 1;
    const std::array<const IsleSprite*,1> roots{{&source}};
    const std::array<sp::BankMapping,1> banks{{{bank,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    producer.install(ram,1,{});
    const std::array<sp::Draw,1> draws{{{0,&source,0,sp::DrawMode::message_intensity,30,30}}};
    const auto list = producer.prepare(ram,1,draws,{}, {0x150000,4096});
    const auto words = std::span(ram).subspan(list.address/4,list.words);
    check(op(words,0xf1) == std::vector<std::uint32_t>{0xf1000000,0x02aa02aa} &&
          op(words,0xe4) == std::vector<std::uint32_t>{0xe4078078,0x00028028},
          "Msg retains its separate truncating reciprocal and caller-lowered integer endpoints");
}
void minification_capacity_is_atomic() {
    Fixture fixture;
    fixture.bitmap[0] = {64,64,0,0,fixture.bytes->data(),128,1,0};
    fixture.sprite.width = 64; fixture.sprite.height = 1;
    fixture.sprite.nbitmaps = fixture.sprite.bmheight = fixture.sprite.bmHreal = 1;
    fixture.sprite.attr |= ISLE_SP_SCALE;
    const std::array<const IsleSprite*,1> roots{&fixture.sprite};
    const std::array<sp::BankMapping,1> banks{{{fixture.bytes,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    producer.install(ram,1,{});
    auto tiny = fixture.sprite;
    tiny.x = 10; tiny.y = 10;
    tiny.scalex = 0.02f; tiny.scaley = 1;
    const std::array<sp::Draw,1> minified{{{0,&tiny}}};
    const auto minified_list = producer.prepare(ram,1,minified,{}, {0x150000,4096});
    check(minified_list.rectangles > 0, "visible reciprocal overflow is lowered, not rejected");
    const auto minified_words = std::span(ram).subspan(minified_list.address/4,minified_list.words);
    const auto lowered_derivatives = op(minified_words,0xf1);
    for (std::size_t i=1; i<lowered_derivatives.size(); i+=2)
        check(lowered_derivatives[i] == 0x00000400,
              "only the overflowing X derivative is replaced by sample-local constant S");
    const auto before_capacity_failure = ram;
    rejects([&]{producer.prepare(ram,1,minified,{},
                                 {0x150000,static_cast<std::uint32_t>(minified_list.words*4-8)});},
            "minified geometry still requires complete command capacity");
    check(ram == before_capacity_failure, "minified capacity failure preserves all RAM atomically");
}
void minification_axes_and_coordinate_boundaries() {
    auto bank = std::make_shared<sp::Bank>(60*68*2,0xff);
    const std::array<IsleBitmap,2> bitmap{{
        {60,60,0,0,bank->data(),60*34*2,34,0},
        {60,60,0,0,bank->data()+60*34*2,60*34*2,34,0}}};
    IsleSprite sprite{};
    sprite.width=60; sprite.height=68; sprite.bmheight=sprite.bmHreal=34;
    sprite.scalex=sprite.scaley=1; sprite.red=sprite.green=sprite.blue=sprite.alpha=255;
    sprite.nbitmaps=2; sprite.istep=1; sprite.bitmap=bitmap.data();
    sprite.bmfmt=0; sprite.bmsiz=2; sprite.attr=ISLE_SP_SCALE;
    sprite.x=120; sprite.y=134;
    const std::array<const IsleSprite*,1> roots{{&sprite}};
    const std::array<sp::BankMapping,1> banks{{{bank,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4,0xa5a5a5a5);
    producer.install(ram,1,{});
    const std::array<sp::Draw,1> draws{{{0,&sprite}}};
    const auto prepare = [&] {
        return producer.prepare(ram,1,draws,{}, {0x150000,65536});
    };
    for (const auto scale : {1.0f/60.0f,-1.0f/60.0f}) {
        sprite.scalex=sprite.scaley=scale;
        const auto list=prepare();
        check(list.rectangles > 0, "source 60x68 split-piece fade keeps covered samples");
        const auto derivatives=op(std::span(ram).subspan(list.address/4,list.words),0xf1);
        for (std::size_t i=1; i<derivatives.size(); i+=2)
            check(derivatives[i] == 0, "both overflowing axes lower to independently rebased samples");
    }
    for (const bool overflow_y : {false,true}) {
        sprite.scalex=overflow_y ? 1.0f : 1.0f/32.0f;
        sprite.scaley=overflow_y ? 1.0f/32.0f : 1.0f;
        const auto list=prepare();
        check(list.rectangles > 0, "positive signed-derivative boundary remains drawable");
        const auto derivatives=op(std::span(ram).subspan(list.address/4,list.words),0xf1);
        for (std::size_t i=1; i<derivatives.size(); i+=2)
            check(derivatives[i] == (overflow_y ? 0x04000000u : 0x00000400u),
                  "anisotropic lowering preserves the representable axis");
    }
    sprite.scalex=-1.0f/32.0f; sprite.scaley=1;
    const auto negative_boundary=prepare();
    const auto derivatives=op(std::span(ram).subspan(negative_boundary.address/4,negative_boundary.words),0xf1);
    for (std::size_t i=1; i<derivatives.size(); i+=2)
        check(derivatives[i] == 0x80000400u, "negative -32768 derivative keeps its normal encoded path");
    sprite.scalex=sprite.scaley=1.0f/60.0f;
    sprite.attr |= ISLE_SP_FRACPOS | ISLE_SP_TEXSHIFT;
    sprite.frac_s=32767;
    const auto before_coordinate_failure=ram;
    rejects([&]{prepare();}, "genuinely unrepresentable rebased sample coordinate rejects");
    check(ram == before_coordinate_failure, "late sample-coordinate rejection writes no RAM");
}
void minified_bands_load_consumed_neighbors() {
    auto bank=std::make_shared<sp::Bank>(128*128*2,0xff);
    IsleBitmap bitmap{128,128,0,0,bank->data(),bank->size(),128,0};
    IsleSprite sprite{};
    sprite.width=sprite.height=sprite.bmheight=sprite.bmHreal=128;
    sprite.scalex=1.0f/64.0f; sprite.scaley=1;
    sprite.red=sprite.green=sprite.blue=sprite.alpha=255;
    sprite.nbitmaps=sprite.istep=1; sprite.bitmap=&bitmap;
    sprite.bmfmt=0; sprite.bmsiz=2; sprite.attr=ISLE_SP_SCALE;
    sprite.x=sprite.y=10;
    const std::array<const IsleSprite*,1> roots{{&sprite}};
    const std::array<sp::BankMapping,1> banks{{{bank,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    producer.install(ram,1,{});
    const std::array<sp::Draw,1> draws{{{0,&sprite}}};
    for (const bool flipped : {false,true}) {
        sprite.scaley=flipped ? -1.0f/64.0f : 1.0f/64.0f;
        const auto list=producer.prepare(ram,1,draws,{}, {0x150000,65536});
        const auto words=std::span(ram).subspan(list.address/4,list.words);
        int load_first=0, load_rows=0, top=0;
        bool checked=false;
        for (std::size_t i=0; i<words.size(); i+=2) {
            const auto opcode=words[i]>>24;
            if (opcode == 0xfd)
                load_first=static_cast<int>((words[i+1]-0x300000)/256);
            else if (opcode == 0xf4)
                load_rows=static_cast<int>((words[i+1]&0xfff)/4+1);
            else if (opcode == 0xe4)
                top=static_cast<int>(words[i+1]&0xfff)/4;
            else if (opcode == 0xe1 && top == (flipped ? 9 : 11)) {
                const int packed=static_cast<int>(words[i+1]&0xffff);
                const int local_t=packed < 32768 ? packed : packed-65536;
                const int expected_row=flipped ? 63 : 64;
                check(load_first*32+local_t == expected_row*32,
                      "fractional band anchor preserves independently derived global source T");
                check(load_first <= expected_row && load_first+load_rows > expected_row+1,
                      "minified band load contains both consumed sample neighbors without artificial clamp");
                checked=true;
            }
        }
        check(checked, "minified oversized source exercises the fractional-anchor row");
    }
}
void ci_palette_and_odd_rows() {
    auto bank = std::make_shared<sp::Bank>(128, 0);
    (*bank)[8] = 0x12; (*bank)[9] = 0x30; (*bank)[10] = 0x32; (*bank)[11] = 0x10;
    IsleBitmap bitmap{3,3,0,0,bank->data()+8,4,2,3};
    IsleSprite s{};
    s.width=3; s.height=2; s.scalex=s.scaley=1; s.red=s.green=s.blue=s.alpha=255;
    s.bitmap=&bitmap; s.nbitmaps=1; s.istep=1; s.bmheight=s.bmHreal=2; s.bmfmt=2; s.bmsiz=0;
    s.LUT=bank->data()+32; s.LUT_bytes=32; s.startTLUT=48; s.nTLUT=16;
    const std::array<const IsleSprite*, 1> roots{&s};
    const std::array<sp::BankMapping,1> banks{{{bank,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4,0xcccccccc);
    producer.install(ram,1,{});
    check(ram[0x30000c/4]==0xcccccccc, "odd nibble asset install preserves exact final guard");
    auto wrong_bitmap=bitmap;
    wrong_bitmap.LUToffset=2;
    auto wrong=s;
    wrong.bitmap=&wrong_bitmap;
    const std::array<const IsleSprite*,1> wrong_roots{&wrong};
    rejects([&]{sp::Producer invalid(wrong_roots,banks);},
            "CI4 palette bank outside the loaded partial TLUT is rejected");
    std::array<IsleBitmap, 3> palette_pieces{bitmap, bitmap, bitmap};
    palette_pieces[1].LUToffset = palette_pieces[2].LUToffset = 4;
    auto palette_sprite = s;
    palette_sprite.width = 9; palette_sprite.nbitmaps = 3;
    palette_sprite.bitmap = palette_pieces.data();
    palette_sprite.nTLUT = 32; palette_sprite.LUT_bytes = 64;
    const std::array<const IsleSprite*, 1> palette_roots{&palette_sprite};
    sp::Producer palette_producer(palette_roots, banks);
    palette_producer.install(ram, 2, {});
    const std::array<sp::Draw, 1> draws{{{0, &palette_sprite}}};
    const auto list = palette_producer.prepare(ram, 2, draws, {}, {0x150000,4096});
    const auto words = std::span(ram).subspan(list.address / 4, list.words);
    check(list.rectangles == 3 && op(words, 0xf4).size() == 4 && op(words, 0xf2).size() == 4,
          "CI4 same-buffer palette-bank change invalidates the resident setup, then equal setup reuses it");
    const auto tiles = op(words, 0xf5);
    check((tiles[5] & 0x00f00000u) == 0x00300000u &&
          (tiles[9] & 0x00f00000u) == 0x00400000u,
          "consecutive CI4 cache keys preserve the exact render-tile palette bank");
}
void oversized_bitmap_bands() {
    auto bank = std::make_shared<sp::Bank>(32 * 68 * 2, 0xff);
    IsleBitmap bitmap{32, 32, 0, 0, bank->data(), bank->size(), 68, 0};
    IsleSprite s{};
    s.width = 32; s.height = s.bmheight = s.bmHreal = 68;
    s.scalex = s.scaley = 1; s.red = s.green = s.blue = s.alpha = 255;
    s.nbitmaps = s.istep = 1; s.bitmap = &bitmap; s.bmfmt = 0; s.bmsiz = 2;
    s.attr = ISLE_SP_TEXSHUF | ISLE_SP_FRACPOS;
    s.frac_t = -16;
    const std::array<const IsleSprite*, 1> roots{{&s}};
    const std::array<sp::BankMapping, 1> banks{{{bank, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    producer.install(ram, 1, {});
    const std::array<sp::Draw, 1> draws{{{0, &s}}};
    const auto list = producer.prepare(ram, 1, draws, {}, {0x150000,4096});
    const auto words = std::span(ram).subspan(list.address/4, list.words);
    const auto loads = op(words, 0xf3), images = op(words, 0xfd), rectangles = op(words, 0xe4);
    const auto origins = op(words, 0xe1);
    check(list.rectangles == 2 && loads[1] == 0x077ff000 && loads[3] == 0x0713f000,
          "4352-byte bitmap splits into bounded 4096/640-byte loads retaining both borders");
    check(images[1] == 0x300000 && images[3] == 0x300e80,
          "second band loads preceding source rows with even interleave parity");
    check(rectangles[0] == (0xe4000000u | (128u<<12) | 240u) &&
          rectangles[2] == (0xe4000000u | (128u<<12) | 272u) && rectangles[3] == 240,
          "upper/lower stored borders do not duplicate screen coverage");
    check(origins[1] == 0x0000fff0 && origins[3] == 0x00000030,
          "negative half-texel T samples retained preceding row rather than an artificial band clamp");
    s.width = 64; s.istep = 0;
    const auto repeated = producer.prepare(ram, 1, draws, {}, {0x150000,4096});
    const auto repeated_words = std::span(ram).subspan(repeated.address / 4, repeated.words);
    const auto repeated_images = op(repeated_words, 0xfd);
    check(repeated.rectangles == 4 && repeated_images == std::vector<std::uint32_t>{
              0xfd100000, 0x300000, 0xfd100000, 0x300e80,
              0xfd100000, 0x300000, 0xfd100000, 0x300e80},
          "repeating one bitmap must reload its first band after the preceding tile leaves its final band resident");
    check(op(repeated_words, 0xe1) == std::vector<std::uint32_t>{
              0xe1000000, 0x0000fff0, 0xe1000000, 0x00000030,
              0xe1000000, 0x0000fff0, 0xe1000000, 0x00000030},
          "band reloads preserve source-local origins and filtered-border interpretation on every repetition");
}
void rgba32_copy_rejection_is_atomic() {
    auto bank = std::make_shared<sp::Bank>(64, 0xff);
    IsleBitmap bitmap{4, 4, 0, 0, bank->data(), bank->size(), 4, 0};
    IsleSprite normal{};
    normal.width = normal.height = normal.bmheight = normal.bmHreal = 4;
    normal.scalex = normal.scaley = 1;
    normal.red = normal.green = normal.blue = normal.alpha = 255;
    normal.nbitmaps = normal.istep = 1; normal.bitmap = &bitmap;
    normal.bmfmt = 0; normal.bmsiz = 3; normal.attr = ISLE_SP_TEXSHUF;
    const std::array<const IsleSprite*, 1> roots{{&normal}};
    const std::array<sp::BankMapping, 1> banks{{{bank, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4, 0xa5a5a5a5);
    producer.install(ram, 1, {});
    const auto installed = ram;
    auto copy = normal; copy.attr |= ISLE_SP_FASTCOPY;
    const std::array<sp::Draw, 2> draws{{{0, &normal}, {0, &copy}}};
    rejects([&] { producer.prepare(ram, 1, draws, {}, {0x150000,4096}); },
            "RGBA32 COPY must reject even after an earlier valid draw");
    check(ram == installed, "unsupported RGBA32 COPY changes neither command arena nor any RAM guard");
}
void composed_residency_requires_exact_source_coverage() {
    Fixture f;
    const std::array<const IsleSprite*, 1> roots{{&f.sprite}};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4, 0xa5a5a5a5);
    const auto untouched = ram;
    const std::array<sp::Draw, 1> draws{{{0,&f.sprite}}};
    const std::array<sp::AssetSpan, 1> merged{{{0x300008,{f.bytes->data()+8,24}}}};
    const std::array<sp::AssetSpan, 2> split{{
        {0x300008,{f.bytes->data()+8,8}}, {0x300010,{f.bytes->data()+16,16}}}};
    {
        auto discarded = producer.validate_residency(ram, 9, merged);
        check(bool(discarded), "validated merged residency produces a bound token");
    }
    rejects([&] { producer.prepare(ram,9,draws,{}, {0x150000,4096}); },
            "abandoned preflight must not publish residency");
    rejects([&] { producer.validate_residency(ram,9,std::span(split).first(1)); },
            "a composed union omitting selected texture bytes must reject");
    const std::vector<std::uint8_t> equal_copy(f.bytes->begin()+8,f.bytes->begin()+32);
    const std::array<sp::AssetSpan, 1> different_owner{{{0x300008,equal_copy}}};
    rejects([&] { producer.validate_residency(ram,9,different_owner); },
            "equal payload bytes from different immutable source identities are not a collision whitelist");
    const std::array<sp::AssetSpan, 2> conflicting_overlap{{merged[0],different_owner[0]}};
    rejects([&] { producer.validate_residency(ram,9,conflicting_overlap); },
            "an overlapping foreign source cannot hide behind an otherwise complete matching union");
    check(ram == untouched, "residency preflight and rejection never write RAM");
    auto prepared = producer.validate_residency(ram,9,split);
    auto moved = std::move(prepared);
    check(!prepared && bool(moved), "moving residency transfers its single commit authority");
    rejects([&] { producer.prepare(ram,9,draws,{}, {0x150000,4096}); },
            "uncommitted split-span preflight does not publish residency");
    // Caller-owned union writer, not Producer::install: each byte is installed
    // once despite the source plan splitting one sprite asset across spans.
    for(const auto& span:split) for(std::size_t i=0;i<span.bytes.size();++i) {
        const auto at=span.address+static_cast<std::uint32_t>(i);
        const auto shift=(3u-(at&3u))*8u;
        ram[at/4]=(ram[at/4]&~(255u<<shift)) | (std::uint32_t(span.bytes[i])<<shift);
    }
    const auto installed = ram;
    producer.commit_residency(std::move(moved));
    check(!moved && ram == installed, "commit consumes authority without copying or changing any RAM byte");
    check(producer.prepare(ram,9,draws,{}, {0x150000,4096}).rectangles == 2,
          "exact caller-installed split/merged source coverage satisfies normal producer residency");
}

std::shared_ptr<sp::Raster> dynamic_raster(std::uint32_t address = 0x400000,
                                         std::weak_ptr<void>* lifetime = nullptr) {
    auto primary = std::make_shared<std::vector<std::uint8_t>>(32, 0x11);
    auto alternate = std::make_shared<std::vector<std::uint8_t>>(32, 0x77);
    if (lifetime) *lifetime = primary;
    IsleSprite s{};
    s.width = s.height = s.bmheight = s.bmHreal = 4;
    s.scalex = s.scaley = 1; s.red = s.green = s.blue = s.alpha = 255;
    s.nbitmaps = s.istep = 1; s.bmfmt = 0; s.bmsiz = 2;
    const std::array<sp::RasterPiece, 1> pieces{{{{4,4,0,0,nullptr,32,4,0},0}}};
    return std::make_shared<sp::Raster>(s, pieces, std::array<sp::RasterStorage, 2>{{
        {primary, *primary, address}, {alternate, *alternate, address + 0x100}}});
}
void dynamic_union_revision_and_rebind() {
    Fixture f;
    const std::array<const IsleSprite*, 1> roots{{&f.sprite}};
    const std::array<sp::BankMapping, 1> banks{{{f.bytes, 0x80300000}}};
    sp::Producer producer(roots, banks);
    auto raster = dynamic_raster();
    const auto binding = producer.register_raster(raster);
    std::vector<sp::AssetSpan> union_plan(producer.assets().begin(), producer.assets().end());
    std::array<std::uint8_t, 8> other{1,2,3,4,5,6,7,8};
    union_plan.push_back({0x500000, other});
    std::vector<std::uint32_t> ram(8*1024*1024/4, 0xa5a5a5a5);
    producer.install_composed(ram, 1, union_plan);
    check(ram[0x400000/4] == 0x11111111 && ram[0x400100/4] == 0x77777777 &&
          ram[0x500000/4] == 0x01020304, "one residency includes immutable plus both mutable variants");
    const auto* stable_descriptor = raster->view(sp::RasterSource::primary).bitmap;
    auto pixels = raster->pixels(sp::RasterSource::primary);
    pixels[1] = 0xee;
    raster->publish(sp::RasterSource::primary, 1, 1);
    check(raster->revision(sp::RasterSource::primary) == 2 &&
          raster->view(sp::RasterSource::primary).bitmap == stable_descriptor,
          "publication advances revision without topology allocation");
    const auto before_rejection = ram;
    union_plan.push_back({0x400100, other});
    rejects([&] { producer.install_composed(ram, 1, union_plan); },
            "late whole-union immutable/dynamic overlap rejects");
    check(ram == before_rejection, "late whole-union rejection preserves every RAM byte");
    union_plan.pop_back();
    const std::array<sp::RamRange, 1> task_collision{{{0x400008, 8}}};
    rejects([&] { producer.install_composed(ram, 1, union_plan, task_collision); },
            "dynamic allocation colliding with a task rejects before changed uploads");
    check(ram == before_rejection, "reservation rejection does not partially publish changed rasters");
    const std::array<sp::Draw,1> primary{{{binding.resources[0],
        &raster->view(sp::RasterSource::primary),binding.generation}}};
    {
        auto borrow = producer.borrow();
        rejects([&] { producer.prepare(ram,1,primary,{}, {0x150000,4096}); },
                "uninstalled changed revision cannot render stale texels");
    }
    // Sentinels inside unchanged spans make accidental whole-plane/per-frame
    // uploads observable independently of matching source content.
    ram[0x400008/4] = 0x12345678;
    ram[0x400100/4] = 0x87654321;
    producer.install_composed(ram, 1, union_plan);
    check(ram[0x400000/4] == 0x11ee1111 && ram[0x400008/4] == 0x12345678 &&
          ram[0x400100/4] == 0x87654321,
          "only changed published bytes upload, preserving neighbor bytes and untouched alternate");
    {
        auto borrow = producer.borrow();
        const auto list = producer.prepare(ram,1,primary,{}, {0x150000,4096});
        check(op(std::span(ram).subspan(list.address/4,list.words),0xfd)[1] == 0x400000,
              "primary binding uses its actual physical texture");
        const std::array<sp::Draw,1> alternate{{{binding.resources[1],
            &raster->view(sp::RasterSource::alternate),binding.generation}}};
        const auto rebound = producer.prepare(ram,1,alternate,{}, {0x150000,4096});
        check(op(std::span(ram).subspan(rebound.address/4,rebound.words),0xfd)[1] == 0x400100,
              "explicit alternate rebind uses separately registered source");
        const std::array<sp::Draw,1> bad_rebind{{{binding.resources[0],
            &raster->view(sp::RasterSource::alternate),binding.generation}}};
        const auto before = ram;
        rejects([&] { producer.prepare(ram,1,bad_rebind,{}, {0x150000,4096}); },
                "resource index and descriptor variant must agree");
        check(before == ram, "bad rebind is command-write atomic");
        rejects([&] { raster->publish(sp::RasterSource::primary,0,1); }, "live graphics pins revision");
        rejects([&] { raster->pixels(sp::RasterSource::primary); }, "live graphics denies mutable span acquisition");
        rejects([&] { producer.unregister_raster(binding); }, "real unretired borrower prevents release");
    }
    auto overlap = dynamic_raster(0x400000);
    rejects([&] { producer.register_raster(overlap); }, "physical raster allocation overlap rejects");
    producer.unregister_raster(binding);
    const auto replacement = producer.register_raster(overlap);
    check(replacement.resources == binding.resources && replacement.generation != binding.generation,
          "retired slots reuse bounded topology storage with a new generation");
    producer.install_composed(ram,1,union_plan);
    {
        auto borrow = producer.borrow();
        rejects([&] { producer.prepare(ram,1,primary,{}, {0x150000,4096}); },
                "old native draw binding cannot alias a reused registration");
    }
    rejects([&] { producer.unregister_raster(binding); }, "stale retirement cannot release a replacement");
    producer.unregister_raster(replacement);
}
void dynamic_retirement_and_preflight_authority() {
    sp::Producer producer({}, {});
    std::weak_ptr<void> storage;
    auto raster = dynamic_raster(0x400000, &storage);
    const auto binding = producer.register_raster(raster);
    raster.reset();
    check(!storage.expired(), "producer owns actual mutable backing storage");
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    {
        auto token = producer.validate_residency(ram,1,{});
        rejects([&] { producer.unregister_raster(binding); }, "uncommitted plan pins mutable allocation");
        auto moved = std::move(token);
        check(!token && bool(moved), "dynamic plan authority remains move-only");
    }
    producer.unregister_raster(binding);
    check(storage.expired(), "last retired registration reclaims actual backing allocation");
    auto first = producer.validate_residency(ram,2,{});
    auto stale = producer.validate_residency(ram,2,{});
    producer.commit_residency(std::move(first));
    rejects([&] { producer.commit_residency(std::move(stale)); }, "stale plan cannot overwrite newer residency");
    sp::Producer other({}, {});
    auto foreign = other.validate_residency(ram,3,{});
    rejects([&] { producer.commit_residency(std::move(foreign)); }, "foreign plan authority rejects");
}
void source_camera_masks_and_tags() {
    isle::scene::render::Pass pass;
    pass.link_mask = 2; pass.camera_tag = UINT32_MAX;
    pass.tag_match = isle::scene::render::TagMatch::equality;
    check(pass.includes({1,0x80000000u,UINT32_MAX}), "source sprite camera admits DL1/tag-1");
    check(!pass.includes({2,0x80000000u,UINT32_MAX}), "source sprite camera rejects other render links");
    check(!pass.includes({1,0x80000000u,1}), "flag8 camera requires exact tag equality, not intersection");
    pass.tag_match = isle::scene::render::TagMatch::intersection;
    pass.camera_tag = 0x80000000u;
    check(pass.includes({1,0,0x80000001u}), "ordinary camera uses nonzero32-bit tag intersection");
    check(!pass.includes({1,0,1}), "ordinary camera rejects disjoint tags");
    check(!pass.includes({64,0,UINT32_MAX}), "invalid render links never shift by64");
}
void dynamic_native_formats_and_bank_alias_rejection() {
    Fixture immutable;
    const std::array<const IsleSprite*,1> roots{{&immutable.sprite}};
    const std::array<sp::BankMapping,1> banks{{{immutable.bytes,0x80300000}}};
    sp::Producer producer(roots,banks);
    std::vector<std::uint32_t> ram(8*1024*1024/4);
    for (const auto format_size : {std::array<unsigned,2>{3,1}, {3,2}, {0,3}}) {
        const auto extent = 4u * 4u * (1u << format_size[1]) / 2u;
        auto primary = std::make_shared<std::vector<std::uint8_t>>(extent,0xff);
        auto alternate = std::make_shared<std::vector<std::uint8_t>>(extent,0x77);
        IsleSprite s{};
        s.width = s.height = s.bmheight = s.bmHreal = 4;
        s.scalex = s.scaley = 1; s.red = s.green = s.blue = s.alpha = 255;
        s.nbitmaps = s.istep = 1;
        s.bmfmt = static_cast<std::uint8_t>(format_size[0]);
        s.bmsiz = static_cast<std::uint8_t>(format_size[1]);
        const std::array<sp::RasterPiece,1> pieces{{{{4,4,0,0,nullptr,extent,4,0},0}}};
        auto raster = std::make_shared<sp::Raster>(s,pieces,std::array<sp::RasterStorage,2>{{
            {primary,*primary,0x400000},{alternate,*alternate,0x400100}}});
        const auto binding = producer.register_raster(raster);
        producer.install(ram,1,{});
        {
            auto borrow = producer.borrow();
            const std::array<sp::Draw,1> draw{{{binding.resources[0],
                &raster->view(sp::RasterSource::primary),binding.generation}}};
            check(producer.prepare(ram,1,draw,{}, {0x150000,4096}).rectangles == 1,
                  "native dynamic IA8/IA16/RGBA32 uses the existing sprite generator");
        }
        producer.unregister_raster(binding);
        if (format_size == std::array<unsigned,2>{3,2}) {
            auto alias = std::make_shared<sp::Raster>(s,pieces,std::array<sp::RasterStorage,2>{{
                {immutable.bytes,std::span(*immutable.bytes).first(extent),0x400000},
                {alternate,*alternate,0x400100}}});
            rejects([&] { producer.register_raster(alias); },
                    "mutable dynamic allocation cannot alias any immutable bank bytes");
        }
    }
}
} // namespace
int main() {
    try {
        installation_and_transactions(); mutable_state_and_scissor(); ci_palette_and_odd_rows();
        ordinary_sprite_edges_and_fractional_origins();
        message_derivatives_remain_independent();
        minification_capacity_is_atomic();
        bitmap_step_preserves_source_geometry();
        actual_height_is_not_row_stride();
        consecutive_texture_setup_and_transitions();
        camera_sized_repetition_is_compact();
        repeated_bitmap_nonprogress_rejects_atomically();
        minification_axes_and_coordinate_boundaries();
        minified_bands_load_consumed_neighbors();
        oversized_bitmap_bands(); rgba32_copy_rejection_is_atomic();
        composed_residency_requires_exact_source_coverage();
        dynamic_union_revision_and_rebind(); dynamic_retirement_and_preflight_authority();
        source_camera_masks_and_tags();
        dynamic_native_formats_and_bank_alias_rejection();
        return 0;
    } catch(const std::exception& error) { std::cerr << "sprite producer: " << error.what() << '\n'; return 1; }
}
