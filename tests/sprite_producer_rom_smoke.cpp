// Explicit private-ROM proof source; never a production renderer/executor owner.
// Native producer -> actual GAME RSP -> actual RDP -> completed mapped RDRAM.
#include "sprite_producer.hpp"
#include "menu_resources.hpp"
#include "graphics_task.hpp"
#include "rsp_render.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
namespace sp = isle::sprite;
constexpr std::uint32_t color = 0x3b5000, depth = 0x600000;
constexpr sp::RamRange commands{0x150000, 0x50000};
constexpr std::array<sp::RamRange, 5> reservations{{
    {0, 0x150000}, {0x1a0000, 0xa0000}, {color, 320*240*2},
    {0x3da800, 320*240*2}, {depth,320*240*2}}};
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct StoredCoverage { unsigned x,y,value; };
struct Proof {
    isle::rsp::CanonicalExecutor executor{true, {2000000, 262144}};
    const isle::rom::Snapshot& snapshot;
    bool cold = true;
    std::uint64_t epoch = 1;
    explicit Proof(const isle::rom::Snapshot& s) : snapshot(s) {}
    std::vector<std::byte> render(sp::Producer& producer, std::span<const sp::Draw> draws,
                                  sp::Camera camera = {}, std::span<const StoredCoverage> coverage = {}) {
        auto preparation = executor.begin_preparation();
        auto graphics_borrow = producer.borrow();
        auto ram = preparation.words();
        if (cold) {
            isle::graphics::validate_cold_inputs(ram, snapshot, snapshot.canonical().data(), 0x147000);
        }
        producer.install(ram, epoch, reservations);
        if (cold) {
            isle::graphics::install_cold_inputs(ram, snapshot, 0x147000);
        }
        const auto before_rejection = std::vector<std::uint32_t>(ram.begin(), ram.end());
        bool rejected = false;
        try { producer.prepare(ram, epoch, draws, camera, {commands.address, 8}, reservations); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected && std::equal(ram.begin(), ram.end(), before_rejection.begin()),
                "exhausted sprite arena changed mapped RAM");
        const auto list = producer.prepare(ram, epoch, draws, camera, commands, reservations);
        isle::graphics::Memory memory(ram);
        std::uint32_t at = 0x100000;
        const auto cmd = [&](std::uint32_t a, std::uint32_t b) {
            memory.word(at, a); memory.word(at+4,b); at+=8;
        };
        cmd(0xdb060000,0); // Segment zero: every native texture/sublist uses physical offsets.
        cmd(0xe7000000,0); cmd(0xed000000,(1280u<<12)|960u);
        cmd(0xff10013f,depth); cmd(0xef300cf0,0); cmd(0xf7000000,0xfffcfffc);
        cmd(0xf6000000 | (1276u<<12)|956u,0);
        cmd(0xe7000000,0); cmd(0xff10013f,color); cmd(0xf7000000,0x00010001);
        cmd(0xf6000000 | (1276u<<12)|956u,0); cmd(0xe7000000,0); cmd(0xfe000000,depth);
        cmd(0xde000000,list.address); cmd(0xe9000000,0); cmd(0xdf000000,0);
        const auto task = isle::graphics::finish_task(ram,snapshot,0x147000,0x144000,0x145000,
                                                     0x146000,0x200000,0x240000,0x100000);
        const auto before_gpu = std::vector<std::uint32_t>(ram.begin(),ram.end());
        auto completed = executor.execute(std::move(preparation),task.task,task.boot);
        cold=false;
        require(bool(completed.rsp_result()) && completed.rsp_result().full_syncs==1,
                "sprite task did not complete actual RSP/FullSync");
        const auto bytes=completed.rdram();
        const std::array<sp::RamRange,5> gpu_writes{{
            {0x140000,0x10000},{0x200000,0x40000},{color,320*240*2},
            {depth,320*240*2},{0,0}}};
        for(std::size_t i=0;i<before_gpu.size();++i) {
            const auto address=static_cast<std::uint32_t>(i*4);
            bool writable=false;
            for(const auto r:gpu_writes) writable=writable ||
                (address>=r.address && address-r.address<r.size);
            if(!writable) {
                const auto before=before_gpu[i];
                for(unsigned j=0;j<4;++j)
                    require(bytes[(address+j)^3u]==std::byte((before>>((3-j)*8))&255u),
                            "actual sprite task changed an unowned RDRAM guard");
            }
        }
        const auto hidden=completed.hidden();
        for(const auto expected:coverage) {
            require(expected.x<320&&expected.y<240&&expected.value<8,
                    "invalid independently authored coverage expectation");
            const auto index=expected.y*320+expected.x;
            const auto address=color+index*2;
            require(color/2+index<hidden.size(),"completed hidden coverage extent is short");
            const auto word=(unsigned(bytes[address^3u])<<8)|unsigned(bytes[(address+1)^3u]);
            const auto stored=((word&1u)<<2)|(unsigned(hidden[color/2+index])&3u);
            require(stored==expected.value,
                    "actual framebuffer/hidden coverage differs from independently derived quarter geometry");
        }
        std::vector<std::byte> image(bytes.begin()+color,bytes.begin()+color+320*240*2);
        completed.release();
        return image;
    }
};
std::uint16_t pixel(const std::vector<std::byte>& image,unsigned x,unsigned y) {
    const auto at=(y*320+x)*2;
    return static_cast<std::uint16_t>((unsigned(image[at^3u])<<8)|unsigned(image[(at+1)^3u]));
}
void visible(const std::vector<std::byte>& image) {
    std::size_t count=0;
    for(unsigned y=0;y<240;++y) for(unsigned x=0;x<320;++x) count+=pixel(image,x,y)!=1;
    require(count>8,"actual private sprite produced no visible texels");
}
void private_scene(Proof& proof,const isle::menu::Catalog& catalog,isle::menu::Scene scene,
                   std::uint32_t identity) {
    const auto source=catalog.sprites(scene);
    std::vector<const IsleSprite*> roots;
    for(const auto& r:source) roots.push_back(r.sprite.view());
    const auto& bank=catalog.bank(scene);
    const std::array<sp::BankMapping,1> banks{{{bank.owner,bank.guest_begin}}};
    sp::Producer producer(roots,banks);
    std::size_t selected=0;
    while(selected<source.size() && source[selected].identity!=identity) ++selected;
    require(selected<source.size(),"private sprite selection missing from complete catalog");
    auto view=*roots[selected];
    // GAME overwrites descriptor defaults before its first visible draw:
    // AA0650.c:88 (logo), A0BE70.c:255 (RGBA32 title), A08E30.c:238-241
    // (menu chain). In particular, the RGBA32 descriptor's 0x228 default is
    // never a request to execute unsupported COPY32 in these source paths.
    view.attr=ISLE_SP_TEXSHUF|ISLE_SP_TRANSPARENT;
    view.red=view.green=view.blue=view.alpha=255;
    if(scene==isle::menu::Scene::logo) {
        view.x=96; view.y=64;
    } else if(identity==0x803051f0) {
        view.x=98; view.y=68;
    } else {
        require(identity==0x802db328,"private scalar probe requires an exact source initialization");
        view.x=42; view.y=51;
        view.attr|=ISLE_SP_SCALE|ISLE_SP_HIDDEN;
        view.scalex=view.scaley=1.2f;
        // Explicitly reveal the source-initially-hidden chain for its visible
        // scalar boundary; the reverse HIDDEN transition is exercised below.
        view.attr&=~ISLE_SP_HIDDEN;
    }
    const auto runtime_view=view;
    const std::array<sp::Draw,1> draws{{{selected,&view}}};
    const auto initial=proof.render(producer,draws); visible(initial);
    view.red=37; view.green=113; view.blue=239;
    const auto tinted=proof.render(producer,draws);
    require(tinted!=initial,"mutable GAME sprite tint did not affect actual RDP image");
    view.alpha=73;
    require(proof.render(producer,draws)!=tinted,"mutable alpha did not affect actual RDP blending");
    view.red=view.green=view.blue=view.alpha=255;
    require(proof.render(producer,draws)==initial,"restored sprite scalar failed exact framebuffer restoration");
    view.attr|=ISLE_SP_SCALE; view.scalex=0.75f; view.scaley=1.25f;
    require(proof.render(producer,draws)!=initial,"sprite scale did not affect actual RDP image");
    view=runtime_view;
    const sp::Bounds scissor{view.x+8,view.y+8,view.x+24,view.y+24};
    const auto clipped=proof.render(producer,draws,{{0,0,320,240},scissor});
    for(unsigned y=0;y<240;++y) for(unsigned x=0;x<320;++x)
        if(x<unsigned(scissor.left) || x>=unsigned(scissor.right) ||
           y<unsigned(scissor.top) || y>=unsigned(scissor.bottom))
            require(pixel(clipped,x,y)==1,"sprite scissor modified an outside framebuffer guard");
    view.attr|=ISLE_SP_HIDDEN;
    const auto hidden=proof.render(producer,draws);
    for(unsigned y=0;y<240;++y) for(unsigned x=0;x<320;++x)
        require(pixel(hidden,x,y)==1,"hidden sprite still modified framebuffer");
    std::cout<<"native-private-sprite scene="<<(scene==isle::menu::Scene::logo?"logo":"menu")
             <<" complete_roots="<<source.size()<<" changed_restore_scale_scissor_hidden_guards=pass\n";
    ++proof.epoch;
}
void band_window_comparison(Proof& proof, const IsleSprite& source, sp::BankMapping bank,
                            bool require_interpolation) {
    // Same real packed texels, two actual GPU paths: an oversized plane with
    // internal TMEM bands, and one independently bounded source-row window
    // fitting in TMEM. No normalized image or CPU-rendered oracle is submitted.
    const std::array<const IsleSprite*,1> roots{{&source}};
    const std::array<sp::BankMapping,1> banks{{bank}};
    sp::Producer full(roots,banks);
    auto view = source;
    view.x = 48; view.y = 40; view.width = 32;
    view.attr = ISLE_SP_TEXSHUF | ISLE_SP_FRACPOS;
    view.frac_s = 0; view.frac_t = -16;
    view.red = view.green = view.blue = view.alpha = 255;
    const std::array<sp::Draw,1> draws{{{0,&view}}};
    const auto divided = proof.render(full,draws);

    const auto& original = source.bitmap[0];
    require(source.bmfmt == 0 && source.bmsiz == 2 && original.width_img == 32 &&
            original.actualHeight == 68, "expected admitted oversized RGBA16 source piece");
    auto bitmap = original;
    constexpr int window_first = 56, window_rows = 12;
    bitmap.buf += window_first * 64;
    bitmap.byte_count = window_rows * 64;
    bitmap.actualHeight = window_rows;
    auto window = view;
    window.y += window_first;
    window.height = window.bmheight = window.bmHreal = window_rows;
    window.nbitmaps = 1; window.bitmap = &bitmap;
    const std::array<const IsleSprite*,1> reference_roots{{&window}};
    sp::Producer reference(reference_roots,banks);
    const std::array<sp::Draw,1> reference_draws{{{0,&window}}};
    const auto continuous = proof.render(reference,reference_draws);
    // Cover both the new draw boundary at row 60 and the old one at row 62.
    // Rows at the reference window's own clamp edges are deliberately excluded.
    for(unsigned y=98;y<107;++y) for(unsigned x=48;x<80;++x)
        require(pixel(divided,x,y)==pixel(continuous,x,y),
                "fractional oversized sprite band introduced an artificial filtering seam");
    if(require_interpolation) {
        const auto mixed = pixel(divided,50,100);
        require(mixed != 0xf801 && mixed != 0x07c1 && mixed != 1,
                "fractional seam proof did not interpolate distinct neighboring source rows");
    }
    ++proof.epoch;
}
void oversized_private_and_fractional_bands(Proof& proof, const isle::menu::Catalog& catalog) {
    const auto& bank = catalog.bank(isle::menu::Scene::title_menu);
    for(const auto identity : {0x8030e020u,0x80310080u,0x803120e0u})
        band_window_comparison(proof,*catalog.sprite(isle::menu::Scene::title_menu,identity).view(),
                               {bank.owner,bank.guest_begin},false);
    // A distinct-row fixture makes the former half-texel upper-edge clamp
    // observable even if a particular private image happens to repeat rows.
    auto bytes = std::make_shared<sp::Bank>(32*68*2);
    for(unsigned row=0;row<68;++row) for(unsigned x=0;x<32;++x) {
        const std::uint16_t rgba = row&1 ? 0xf801 : 0x07c1;
        (*bytes)[(row*32+x)*2] = static_cast<std::uint8_t>(rgba>>8);
        (*bytes)[(row*32+x)*2+1] = static_cast<std::uint8_t>(rgba);
    }
    IsleBitmap bitmap{32,32,0,0,bytes->data(),bytes->size(),68,0};
    IsleSprite sprite{};
    sprite.width=32; sprite.height=sprite.bmheight=sprite.bmHreal=68;
    sprite.scalex=sprite.scaley=1; sprite.nbitmaps=sprite.istep=1;
    sprite.bmfmt=0; sprite.bmsiz=2; sprite.bitmap=&bitmap; sprite.attr=ISLE_SP_TEXSHUF;
    band_window_comparison(proof,sprite,{bytes,0x802b5000},true);
    std::cout<<"native-private-sprite oversized_GAME_pieces=3 fractional_band_window_comparison=pass\n";
}
void synthetic_ci_boundary(Proof& proof) {
    // Independent public-format fixture; NOT a claimed CI asset in GAME's menu.
    // Private selected GAME roots above are RGBA16/32 and IA16, not CI.
    auto bank=std::make_shared<sp::Bank>(96,0);
    std::fill(bank->begin(),bank->begin()+8,0x11);
    (*bank)[18]=0xf8; (*bank)[19]=1;
    (*bank)[50]=0; (*bank)[51]=0x3f;
    IsleBitmap bitmap{4,4,0,0,bank->data(),8,4,0};
    IsleSprite view{};
    view.x=100; view.y=100; view.width=view.height=4;
    view.scalex=view.scaley=1; view.red=view.green=view.blue=view.alpha=255;
    view.nbitmaps=1; view.istep=1; view.bmheight=view.bmHreal=4; view.bitmap=&bitmap;
    view.bmfmt=2; view.bmsiz=0; view.LUT=bank->data()+16; view.LUT_bytes=32; view.nTLUT=16;
    auto blue_bitmap=bitmap;
    blue_bitmap.LUToffset=3;
    auto blue_view=view;
    blue_view.x=120;
    blue_view.bitmap=&blue_bitmap;
    blue_view.LUT=bank->data()+48;
    blue_view.startTLUT=48;
    const std::array<const IsleSprite*,2> roots{{&view,&blue_view}};
    const std::array<sp::BankMapping,1> banks{{{bank,0x802b5000}}};
    sp::Producer producer(roots,banks);
    const std::array<sp::Draw,2> draws{{{0,&view},{1,&blue_view}}};
    const auto red=proof.render(producer,draws);
    const auto observed=pixel(red,101,101);
    if(observed!=0xf801)
        std::cerr<<"CI4 observed RGBA5551 center="<<std::hex<<observed
                 <<" first="<<pixel(red,100,100)<<" last="<<pixel(red,103,103)<<std::dec<<'\n';
    require(observed==0xf801,"CI4/TLUT native RDP boundary did not produce RGBA5551 red");
    require(pixel(red,121,101)==0x003f,
            "CI4 nonzero partial TLUT bank did not preserve its independent blue color");
    view.red=0; view.green=255; view.blue=255;
    const auto black=proof.render(producer,draws);
    require(pixel(black,101,101)==1,"CI4 primitive tint did not modulate actual palette color");
    view.red=255;
    require(proof.render(producer,draws)==red,"CI4 scalar restore did not restore exact framebuffer");
}
void mutable_raster_boundary(Proof& proof) {
    auto primary = std::make_shared<std::vector<std::uint8_t>>(32);
    auto alternate = std::make_shared<std::vector<std::uint8_t>>(32);
    const auto solid = [](std::span<std::uint8_t> pixels, std::uint16_t rgba) {
        for (std::size_t i = 0; i < pixels.size(); i += 2) {
            pixels[i] = static_cast<std::uint8_t>(rgba >> 8);
            pixels[i + 1] = static_cast<std::uint8_t>(rgba);
        }
    };
    solid(*primary, 0xf801); solid(*alternate, 0x07c1);
    IsleSprite prototype{};
    prototype.x = prototype.y = 100;
    prototype.width = prototype.height = prototype.bmheight = prototype.bmHreal = 4;
    prototype.scalex = prototype.scaley = 1;
    prototype.red = prototype.green = prototype.blue = prototype.alpha = 255;
    prototype.nbitmaps = prototype.istep = 1; prototype.bmfmt = 0; prototype.bmsiz = 2;
    const std::array<sp::RasterPiece,1> pieces{{{{4,4,0,0,nullptr,32,4,0},0}}};
    auto raster = std::make_shared<sp::Raster>(prototype,pieces,std::array<sp::RasterStorage,2>{{
        {primary,*primary,0x400000},{alternate,*alternate,0x400100}}});
    sp::Producer producer({}, {});
    const auto binding = producer.register_raster(raster);
    std::array<sp::Draw,1> draw{{{binding.resources[0],
        &raster->view(sp::RasterSource::primary),binding.generation}}};
    const auto red = proof.render(producer,draw);
    require(pixel(red,101,101)==0xf801,"dynamic primary did not reach actual RSP/RDP");
    solid(raster->pixels(sp::RasterSource::primary),0x003f);
    raster->publish(sp::RasterSource::primary,0,16);
    raster->publish(sp::RasterSource::primary,16,16);
    const auto blue = proof.render(producer,draw);
    require(pixel(blue,101,101)==0x003f && blue!=red,
            "published dynamic revision failed actual hardware-format rendering");
    draw[0] = {binding.resources[1],&raster->view(sp::RasterSource::alternate),binding.generation};
    const auto green = proof.render(producer,draw);
    require(pixel(green,101,101)==0x07c1 && green!=blue,
            "dynamic alternate rebind did not select its distinct physical storage");
    draw[0] = {binding.resources[0],&raster->view(sp::RasterSource::primary),binding.generation};
    solid(raster->pixels(sp::RasterSource::primary),0xf801);
    raster->publish(sp::RasterSource::primary,0,32);
    require(proof.render(producer,draw)==red,"dynamic source restoration changed the actual framebuffer");
    require(proof.executor.quiescent(),"dynamic retirement preceded actual GPU readback retirement");
    producer.unregister_raster(binding);
    ++proof.epoch;
    std::cout<<"native-private-sprite dynamic_revision_rebind_restore_actual_rsp_rdp_guards=pass\n";
}

// Independent public-format sources, not substitute output images. These
// unshuffled RGBA16 planes travel through the same ROM RSP and canonical RDP
// executor as the private GAME roots above.
struct MinificationPlane {
    std::shared_ptr<sp::Bank> bank;
    std::vector<IsleBitmap> bitmaps;
    IsleSprite source{};
    MinificationPlane(unsigned width, unsigned height, unsigned piece_rows = 0)
        : bank(std::make_shared<sp::Bank>(width * height * 2)) {
        if (!piece_rows) piece_rows = height;
        for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
            // Low five bits distinguish neighboring columns/rows; blue
            // distinguishes the 32-texel blocks, including every true edge.
            const auto rgba = static_cast<std::uint16_t>(
                ((x & 31u) << 11) | ((y & 31u) << 6) |
                ((1u + (x >> 5) + 4u * (y >> 5)) << 1) | 1u);
            const auto at = (y * width + x) * 2;
            (*bank)[at] = static_cast<std::uint8_t>(rgba >> 8);
            (*bank)[at + 1] = static_cast<std::uint8_t>(rgba);
        }
        for (unsigned row = 0; row < height; row += piece_rows) {
            const auto rows = std::min(piece_rows, height - row);
            bitmaps.push_back({static_cast<std::int16_t>(width),
                static_cast<std::int16_t>(width), 0, 0, bank->data() + row * width * 2,
                rows * width * 2, static_cast<std::int16_t>(rows), 0});
        }
        source.x = source.y = 80;
        source.width = static_cast<std::int16_t>(width);
        source.height = static_cast<std::int16_t>(height);
        source.bmheight = source.bmHreal = static_cast<std::int16_t>(piece_rows);
        source.scalex = source.scaley = 1;
        source.red = source.green = source.blue = source.alpha = 255;
        source.nbitmaps = static_cast<std::int16_t>(bitmaps.size());
        source.istep = 1; source.bmfmt = 0; source.bmsiz = 2;
        source.attr = ISLE_SP_SCALE;
        source.bitmap = bitmaps.data();
    }
    std::vector<std::byte> render(Proof& proof, const IsleSprite& view,
                                 sp::Camera camera = {}, std::span<const StoredCoverage> coverage = {}) const {
        const std::array<const IsleSprite*,1> roots{{&view}};
        const std::array<sp::BankMapping,1> banks{{{bank,0x802b5000}}};
        sp::Producer producer(roots,banks);
        const std::array<sp::Draw,1> draws{{{0,&view}}};
        auto image = proof.render(producer,draws,camera,coverage);
        ++proof.epoch;
        return image;
    }
};
struct ExpectedSample { unsigned x, y; std::uint16_t color_rgba5551; };
void minification_pixels(const std::vector<std::byte>& image, sp::Bounds extent,
                         std::initializer_list<ExpectedSample> samples) {
    // Bounds and authored sample colors are independent of emitted commands.
    // In particular, no command count, derivative or producer UV is an oracle.
    // Framebuffer bit 0 stores coverage, NOT source texture alpha. Sampling
    // checks RGB5 only; direct color+hidden coverage and CUTOUT checks below
    // assert those independent properties instead of pinning incidental alpha.
    for (unsigned y = 0; y < 240; ++y) for (unsigned x = 0; x < 320; ++x)
        if (int(x) < extent.left || int(x) >= extent.right ||
            int(y) < extent.top || int(y) >= extent.bottom)
            require(pixel(image,x,y) == 1,
                    "minification changed a sample outside its independent coverage bounds");
    for (const auto expected : samples) {
        const auto observed=pixel(image,expected.x,expected.y);
        if((observed>>1)!=(expected.color_rgba5551>>1))
            std::cerr<<"Minification sample "<<expected.x<<','<<expected.y
                     <<" expected RGB5="<<std::hex<<(expected.color_rgba5551>>1)
                     <<" observed RGB5="<<(observed>>1)<<std::dec
                     <<" extent="<<extent.left<<','<<extent.top<<','<<extent.right<<','<<extent.bottom<<'\n';
        require((observed>>1)==(expected.color_rgba5551>>1),
                "minification selected the wrong authored RGB5 source sample");
    }
}
void native_minification_samples(Proof& proof) {
    // Pinned consumer authority: parallel-rdp rdp_device.cpp:735-771,
    // shaders/span_setup.comp:111-148, interpolation.h:95-125 and
    // shading.h:167-199. Non-AA coverage selects integer samples in
    // [ceil(quarterLeft/4),ceil(quarterRight/4)). X removes fractional-left
    // phase; Y anchors interpolation at floor(quarterTop/4), not quarterTop/4.
    // The following tables are logical source coordinates worked out under
    // those equations, not echoes of the native producer's encoded commands.
    MinificationPlane plane(128,128);
    auto view = plane.source;
    view.scalex = view.scaley = 1.0f / 64.0f;
    // Both overflow: source (0,0), (64,0), (0,64), (64,64).
    // Nominal bands leave quarter-height rectangles at both consumed Y rows:
    // coverage count2 -> opaque CLAMP stored coverage1, not texture alpha1.
    constexpr std::array<StoredCoverage,4> quarter_coverage{{
        {80,80,1},{81,80,1},{80,81,1},{81,81,1}}};
    minification_pixels(plane.render(proof,view,{},quarter_coverage),{80,80,82,82},{
        {80,80,0x0003},{81,80,0x0007},{80,81,0x0013},{81,81,0x0017}});

    view.height = 8; view.scaley = 0.5f;
    // Only S overflows; the representable T step must remain two source rows.
    minification_pixels(plane.render(proof,view),{80,80,82,84},{
        {80,80,0x0003},{81,80,0x0007},{80,81,0x0083},{81,81,0x0087},
        {80,82,0x0103},{81,82,0x0107},{80,83,0x0183},{81,83,0x0187}});
    view = plane.source; view.width = 8;
    view.scalex = 0.5f; view.scaley = 1.0f / 64.0f;
    // Only T overflows; the representable S step must remain two columns.
    minification_pixels(plane.render(proof,view),{80,80,84,82},{
        {80,80,0x0003},{81,80,0x1003},{82,80,0x2003},{83,80,0x3003},
        {80,81,0x0013},{81,81,0x1013},{82,81,0x2013},{83,81,0x3013}});

    view = plane.source;
    view.scalex = 1.0f / 32.0f; view.scaley = 1.0f / 64.0f;
    // +32768 cannot be encoded as signed S5.10. Wrapping it reverses these
    // columns, while zeroing it without rebasing repeats the first column.
    minification_pixels(plane.render(proof,view),{80,80,84,82},{
        {80,80,0x0003},{81,80,0x0005},{82,80,0x0007},{83,80,0x0009},
        {80,81,0x0013},{81,81,0x0015},{82,81,0x0017},{83,81,0x0019}});
    view.x = 84; view.y = 82;
    view.scalex = -1.0f / 32.0f; view.scaley = -1.0f / 64.0f;
    // -32768 IS representable. Preserve that axis while lowering negative T.
    // Consumed columns 127,95,63,31 and rows 127,63 include opposite edges.
    minification_pixels(plane.render(proof,view),{80,80,84,82},{
        {80,80,0xffe1},{81,80,0xffdf},{82,80,0xffdd},{83,80,0xffdb},
        {80,81,0xffd1},{81,81,0xffcf},{82,81,0xffcd},{83,81,0xffcb}});
    view.x = 82; view.scalex = -1.0f / 64.0f;
    minification_pixels(plane.render(proof,view),{80,80,82,82},{
        {80,80,0xffe1},{81,80,0xffdd},{80,81,0xffd1},{81,81,0xffcd}});

    view = plane.source; view.scalex = view.scaley = 1.0f / 64.0f;
    // Scissor clipping must not restart the sample phase at the new origin.
    minification_pixels(plane.render(proof,view,{{0,0,320,240},{81,81,82,82}}),
                        {81,81,82,82},{{81,81,0x0017}});
    // A skipped source column creates a genuine quarter-pixel left edge
    // without changing the public integer position API. Samples at x=81,82
    // consume source columns 48,112, not 0,64 or the rounded geometry edge.
    const std::array<IsleBitmap,2> quarter_columns{{
        {16,16,0,0,nullptr,0,128,0},plane.bitmaps[0]}};
    view.width = 144; view.nbitmaps = 2; view.bitmap = quarter_columns.data();
    minification_pixels(plane.render(proof,view),{81,80,83,82},{
        {81,80,0x8005},{82,80,0x8009},{81,81,0x8015},{82,81,0x8019}});

    MinificationPlane narrow(8,128);
    view = narrow.source; view.scaley = 1.0f / 64.0f;
    // Nonzero bitmap S/T origins remain relative to their stored plane.
    auto origin_bitmap = narrow.bitmaps[0];
    origin_bitmap.s = 2; origin_bitmap.t = 4;
    view.width = 6; view.height = view.bmheight = 124;
    view.bitmap = &origin_bitmap;
    minification_pixels(narrow.render(proof,view),{80,80,86,82},{
        {80,80,0x1103},{85,80,0x3903},{80,81,0x1113},{85,81,0x3913}});
    // A real quarter-pixel TOP is deliberately NOT symmetric with LEFT.
    // This whole 8x128 plane fits TMEM: no artificial band can hide the Y
    // anchor. TOP=80.25 consumes rows 64,128 (the latter clamps to edge127),
    // not geometrically snapped rows48,112. Both columns remain distinct.
    const std::array<IsleBitmap,2> quarter_rows{{
        {8,8,0,0,nullptr,0,128,0},narrow.bitmaps[0]}};
    view = narrow.source; view.height = 256; view.nbitmaps = 2;
    view.expy = -112; view.bitmap = quarter_rows.data();
    view.scaley = 1.0f / 64.0f;
    minification_pixels(narrow.render(proof,view),{80,81,88,83},{
        {80,81,0x0013},{87,81,0x3813},{80,82,0x07db},{87,82,0x3fdb}});

    // Exact reached source fade, with the actual 60x68 / 60x34 topology.
    // The final source scalar is deliberately not replaced by a safe clamp.
    MinificationPlane route(60,68,34);
    view = route.source; view.x = 120; view.y = 134;
    view.scalex = view.scaley = 1.0f - 59.0f / 60.0f;
    minification_pixels(route.render(proof,view),{120,134,121,135},{{120,134,0x0003}});
    // Positive quarter-area but no non-AA sample: the second 34-row piece
    // at 134.5..135 must not reject or manufacture a framebuffer write.
    // At 0.001 even the quarter geometry is empty; a following visible draw
    // proves that this valid source scalar does not abort the whole task.
    auto tiny = view; tiny.scalex = tiny.scaley = 0.001f;
    auto witness = route.source;
    witness.x = witness.y = 100; witness.width = witness.height = 2;
    const std::array<const IsleSprite*,1> roots{{&route.source}};
    const std::array<sp::BankMapping,1> banks{{{route.bank,0x802b5000}}};
    sp::Producer producer(roots,banks);
    const std::array<sp::Draw,2> draws{{{0,&tiny},{0,&witness}}};
    minification_pixels(proof.render(producer,draws),{100,100,102,102},{
        {100,100,0x0003},{101,100,0x0803},{100,101,0x0043},{101,101,0x0843}});
    ++proof.epoch;
    std::cout<<"native-private-sprite minification_samples_axes_signed_quarters_clipping_empty=pass\n";
}
void native_minification_filter_alpha(Proof& proof) {
    MinificationPlane whole(8,128), split(8,128,64);
    auto view = whole.source;
    view.scaley = 1.0f / 64.0f;
    view.attr |= ISLE_SP_FRACPOS; view.frac_t = -16;
    const auto continuous = whole.render(proof,view);
    minification_pixels(continuous,{80,80,88,82},{{80,80,0x0003},{87,80,0x3803}});
    // At source63.5, independently authored neighboring rows have G=31,0
    // and B=5,9. Bilerp gives approximately G16/B7 after 5551 quantization.
    const auto mixed = pixel(continuous,80,81);
    require((mixed >> 11) == 0 && ((mixed >> 6) & 31u) >= 15 &&
            ((mixed >> 6) & 31u) <= 16 && ((mixed >> 1) & 31u) >= 7 &&
            ((mixed >> 1) & 31u) <= 8,
            "minification lost the unsplit source's distinct bilerp neighbors");
    view = split.source; view.scaley = 1.0f / 64.0f;
    view.attr |= ISLE_SP_FRACPOS; view.frac_t = -16;
    const auto divided = split.render(proof,view);
    // A true bitmap edge, unlike an artificial TMEM band, MUST clamp. The
    // second piece starts at logical row64; it must not restart at row0.
    minification_pixels(divided,{80,80,88,82},{
        {80,80,0x0003},{87,80,0x3803},{80,81,0x0013},{87,81,0x3813}});
    require(mixed != pixel(divided,80,81),
            "split and unsplit minification ignored their distinct source borders");
    // A high-contrast authored quad makes half-texel phase observable even
    // after 5551 quantization: red/green over blue/white at the piece origin.
    constexpr std::array<std::uint16_t,4> quad{{0xf801,0x07c1,0x003f,0xffff}};
    for (unsigned y = 0; y < 2; ++y) for (unsigned x = 0; x < 2; ++x) {
        const auto at = ((64 + y) * 8 + x) * 2;
        (*split.bank)[at] = static_cast<std::uint8_t>(quad[y * 2 + x] >> 8);
        (*split.bank)[at + 1] = static_cast<std::uint8_t>(quad[y * 2 + x]);
    }
    view.attr &= ~ISLE_SP_FRACPOS; view.frac_t = 0;
    view.attr |= ISLE_SP_TEXSHIFT;
    const auto shifted = split.render(proof,view);
    // TEXSHIFT supplies +0.5 on both axes, including the new piece origin.
    // Native 3-tap filtering on the upper triangle averages green and blue,
    // not the unshifted red texel and not a CPU-style four-tap gray average.
    const auto half = pixel(shifted,80,81);
    require((half >> 11) == 0 && ((half >> 6) & 31u) >= 15 &&
            ((half >> 6) & 31u) <= 16 && ((half >> 1) & 31u) >= 15 &&
            ((half >> 1) & 31u) <= 16,
            "minification dropped TEXSHIFT at the split row origin");
    minification_pixels(shifted,{80,80,88,82},{});

    MinificationPlane alpha(128,8);
    for (unsigned y = 0; y < 8; ++y) for (unsigned x = 0; x < 128; ++x) {
        // Transparent red at the first consumed column, opaque green at the
        // second. A wrapped/zeroed S derivative cannot pass both outcomes.
        const std::uint16_t rgba = x < 32 ? 0xf800 : 0x07c1;
        const auto at = (y * 128 + x) * 2;
        (*alpha.bank)[at] = static_cast<std::uint8_t>(rgba >> 8);
        (*alpha.bank)[at + 1] = static_cast<std::uint8_t>(rgba);
    }
    view = alpha.source; view.scalex = 1.0f / 64.0f;
    view.attr |= ISLE_SP_CUTOUT;
    constexpr std::array<StoredCoverage,4> full_coverage{{
        {80,80,7},{81,80,7},{80,87,7},{81,87,7}}};
    const auto cutout=alpha.render(proof,view,{},full_coverage);
    minification_pixels(cutout,{80,80,82,88},{
        {80,80,1},{81,80,0x07c1},{80,87,1},{81,87,0x07c1}});
    require(pixel(cutout,80,80)==1&&pixel(cutout,80,87)==1,
            "CUTOUT must preserve the exact existing clear pixels, not write opaque black");
    view.attr |= ISLE_SP_TRANSPARENT; view.alpha = 128;
    const auto blended = alpha.render(proof,view,{},full_coverage);
    minification_pixels(blended,{80,80,82,88},{{80,80,1},{80,87,1}});
    require(pixel(blended,80,80)==1&&pixel(blended,80,87)==1,
            "transparent CUTOUT must preserve the exact existing clear pixels");
    for (const unsigned y : {80u,87u}) {
        const auto p = pixel(blended,81,y);
        require((p & 0xf83eu) == 0 && ((p >> 6) & 31u) >= 15 &&
                ((p >> 6) & 31u) <= 16,
                "minified CUTOUT/primitive alpha did not reach actual RDP blending");
    }
    std::cout<<"native-private-sprite minification_split_borders_bilerp_texshift_cutout_alpha=pass\n";
}
} // namespace
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try {
        require(argc==2,"expected explicit private ROM path");
        const auto snapshot=isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        auto resources=isle::menu::hydrate(snapshot);
        require(bool(resources),"private menu resource hydration failed");
        Proof proof(snapshot);
        private_scene(proof,*resources.catalog,isle::menu::Scene::logo,0x802bc0c0);
        private_scene(proof,*resources.catalog,isle::menu::Scene::title_menu,0x803051f0);
        private_scene(proof,*resources.catalog,isle::menu::Scene::title_menu,0x802db328);
        oversized_private_and_fractional_bands(proof,*resources.catalog);
        synthetic_ci_boundary(proof);
        mutable_raster_boundary(proof);
        native_minification_samples(proof);
        native_minification_filter_alpha(proof);
        return 0;
    } catch(const std::filesystem::filesystem_error&) {
        std::cerr<<"sprite smoke: private filesystem operation failed\n";
    } catch(const std::exception& error) { std::cerr<<"sprite smoke: "<<error.what()<<'\n'; }
    return 1;
}
