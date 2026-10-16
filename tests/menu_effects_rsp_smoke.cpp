// Private actual RSP/RDP consumer. Workers author but do not execute this file.
// Original Title scripts0/3 remain real selector/Catalog consumers. The separate
// format/cache stimulus below is authored native IR, NOT claimed source-script
// reachability: it borrows unchanged Common/Beach images via EffectResources.
#include "effect_bank_resources.hpp"
#include "menu_effects.hpp"
#include "rom_snapshot.hpp"
#include "rsp_fixture.hpp"
#include "rsp_render.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <utility>
namespace {
namespace probe = isle::rsp_probe;
using namespace isle::menu;
constexpr std::uint32_t depth=0x310000,asset_begin=0x1b0000,asset_end=0x1e0000;
constexpr std::uint32_t poison_palette=0x1a8000,effect_commands=0x1a0000;
void require(bool value,const char* why) { if(!value) throw std::runtime_error(why); }
void ok(EffectsStatus s) { require(bool(s),"source effects transition rejected"); }
bool world(void*,EffectAttachment,EffectWorld& out) noexcept {
    for(unsigned r=0;r<4;++r) for(unsigned c=0;c<4;++c) out.matrix[r][c]=r==c?1.f:0.f;
    return true;
}
struct Wire {
    std::span<std::uint32_t> ram;
    std::uint32_t next=effect_commands;
    void cmd(std::uint32_t a,std::uint32_t b) { ram[next/4]=a; ram[next/4+1]=b; next+=8; }
    void byte(std::uint32_t at,std::uint8_t b) {
        auto& w=ram[at/4]; const unsigned shift=(3-(at&3))*8;
        w=(w&~(255u<<shift))|(std::uint32_t(b)<<shift);
    }
    void fill(std::uint32_t target,std::uint32_t value) {
        cmd(0xe7000000,0); cmd(0xff10003f,target); cmd(0xed000000,0x001000c0);
        cmd(0xef300000,0); cmd(0xf7000000,value); cmd(0xf60fc0bc,0);
    }
};
struct InstalledBank {
    std::vector<std::vector<std::uint32_t>> frames,palettes;
    std::vector<EffectImageBinding> images;
};
struct Capture {
    std::vector<std::byte> color,z;
    std::size_t rectangles=0,palette_loads=0;
    std::vector<std::array<unsigned,3>> texture_shapes;
};
Capture draw(isle::rsp::CanonicalExecutor& executor,const isle::rom::Snapshot& snapshot,
             std::span<const EffectBank* const> banks,const EffectRuntime& effects,
             bool cold,std::uint8_t poison,bool occluded,bool palette_roundtrip=false,bool point_pixels=false) {
    auto preparation=executor.begin_preparation();
    if(cold) probe::initialize_fixture_memory(preparation.words(),snapshot.canonical());
    probe::Fixture fixture(preparation.words(),probe::Case::matrix);
    Wire wire{preparation.words()}; fixture.task.commands_address=effect_commands;
    for(std::uint32_t at=asset_begin;at<asset_end;++at) wire.byte(at,poison);
    std::vector<InstalledBank> installed(banks.size());
    std::vector<EffectBankBinding> bindings; bindings.reserve(banks.size());
    std::vector<RamRange> reservations;
    std::uint32_t address=asset_begin;
    const auto install=[&](const isle::rom::GuestView& view,std::uint32_t& output) {
        output=address; reservations.push_back({address,view.bytes.size()});
        for(std::size_t k=0;k<view.bytes.size();++k) wire.byte(address+std::uint32_t(k),std::to_integer<std::uint8_t>(view.bytes[k]));
        // The tail is genuinely unowned RAM; it is never a fabricated palette.
        address+=std::uint32_t((view.bytes.size()+7)&~std::size_t(7))+512;
        require(address<asset_end,"private smoke asset extent");
    };
    for(std::size_t b=0;b<banks.size();++b) {
        const auto& bank=*banks[b]; auto& storage=installed[b];
        storage.frames.resize(bank.images.size()); storage.palettes.resize(bank.images.size()); storage.images.resize(bank.images.size());
        for(std::size_t t=0;t<bank.images.size();++t) {
            const auto& image=bank.images[t]; storage.frames[t].resize(image.frames.size()); storage.palettes[t].resize(image.palettes.size());
            for(std::size_t f=0;f<image.frames.size();++f) install(image.frames[f],storage.frames[t][f]);
            for(std::size_t f=0;f<image.palettes.size();++f) install(image.palettes[f],storage.palettes[t][f]);
            storage.images[t]={storage.frames[t],storage.palettes[t]};
        }
        bindings.push_back({bank.bank_id,storage.images});
    }
    std::vector<std::uint32_t> assets_before(preparation.words().begin()+asset_begin/4,preparation.words().begin()+asset_end/4);
    for(unsigned i=0;i<512;++i) wire.byte(poison_palette+i,poison);
    wire.fill(depth,occluded?0:0xfffcfffc); wire.fill(probe::color_address,0x00010001);
    wire.cmd(0xe7000000,0); wire.cmd(0xfe000000,depth); wire.cmd(0xef000cf0,0); wire.cmd(0xdb060000,0);
    // Poison all prior TLUT entries. Only the reached sixteen may be replaced.
    wire.cmd(0xfd100000,poison_palette); wire.cmd(0xe8000000,0);
    wire.cmd(0xf5000100,0x07000000); wire.cmd(0xe6000000,0);
    wire.cmd(0xf0000000,0x073fc000); wire.cmd(0xe7000000,0);
    EffectCamera camera{}; camera.projection[0][0]=camera.projection[1][1]=0.04f;
    camera.projection[2][2]=0.001f; camera.projection[3][3]=1;
    camera.viewport_scale={128,96,511}; camera.viewport_translate={128,96,511};
    if(point_pixels) {
        camera.viewport_scale[1]=128;
        // Public othermode wire: caller selects G_TF_POINT. Texture filtering is
        // inherited scene state, not a second effect renderer.
        wire.cmd(0xe3001201,0);
    }
    const auto packet_begin=wire.next/4;
    const auto result=effects.render(camera,{preparation.words().subspan(packet_begin,EffectRuntime::max_render_words),bindings,reservations});
    ok(result.status); require(result.particles>0,"actual effect smoke has no visible particles");
    Capture capture;
    for(std::size_t i=0;i<result.words;i+=2) {
        const auto a=preparation.words()[packet_begin+i],b=preparation.words()[packet_begin+i+1];
        if(a>>24==0xf0) { require(((b>>14)&1023)==15,"CI4 must load exactly16 palette entries"); ++capture.palette_loads; }
        if(a>>24==0xfd) {
            // Classify only borrowed FRAME addresses, not TLUT loads. This is a
            // consumer assertion against actual bindings, not source text.
            for(std::size_t bank=0;bank<banks.size();++bank) for(std::size_t image=0;image<banks[bank]->images.size();++image)
                if(std::find(installed[bank].frames[image].begin(),installed[bank].frames[image].end(),b)!=installed[bank].frames[image].end()) {
                    const auto& descriptor=banks[bank]->images[image];
                    capture.texture_shapes.push_back({unsigned(descriptor.format),unsigned(descriptor.size_code),unsigned(descriptor.width)});
                }
        }
    }
    if(palette_roundtrip) {
        const std::vector<std::array<unsigned,3>> expected{{2,0,32},{4,1,64},{2,0,32}};
        require(capture.texture_shapes==expected,"canonical renderer changed CI4P -> Common I8_64 -> same CI4P order");
        require(capture.palette_loads==2,"large texture load must invalidate equal-address cached TLUT");
    }
    wire.next+=std::uint32_t(result.words*4); wire.cmd(0xe9000000,0); wire.cmd(0xdf000000,0);
    require(wire.next<=effect_commands+0x6000,"actual smoke command reservation exceeded");
    auto completed=executor.execute(std::move(preparation),fixture.task,snapshot.canonical().subspan(probe::boot_rom_begin,probe::boot_bytes));
    require(bool(completed.rsp_result())&&completed.rsp_result().full_syncs==1,"actual RSP effect packet did not complete");
    for(const auto& packet:completed.packets().packets()) {
        const auto words=completed.packets().words().subspan(packet.offset,packet.count);
        if(((words[0]>>24)&63)==0x24) { require(words.size()==4,"canonical effect texrect packet shape"); ++capture.rectangles; }
    }
    require(capture.rectangles==result.particles,"actual RSP lost/changed particle texture rectangles");
    const auto ram=completed.rdram();
    capture.color.assign(ram.begin()+probe::color_address,ram.begin()+probe::color_address+64*48*2);
    capture.z.assign(ram.begin()+depth,ram.begin()+depth+64*48*2);
    for(std::uint32_t at=asset_begin;at<asset_end;++at) {
        const auto expected=std::uint8_t(assets_before[(at-asset_begin)/4]>>((3-(at&3))*8));
        require(ram[at^3u]==std::byte(expected),"effect rendering wrote assets/unowned adjacent RAM");
    }
    for(std::size_t i=0;i<capture.z.size();++i)
        require(capture.z[i]==(occluded?std::byte(0):std::byte((i&1)?0xff:0xfc)),"translucent source mode updated depth");
    return capture;
}
void title(isle::rsp::CanonicalExecutor& executor,const isle::rom::Snapshot& snapshot,const Catalog& catalog) {
    bool cold=true; const std::array banks{&catalog.effects()};
    for(std::size_t script:{std::size_t(0),std::size_t(3)}) {
        isle_rng rng{0x12345678}; EffectRuntime effects(banks,rng); ok(effects.status());
        const EffectAttachment attachment{animation::InstanceHandle{1,1,2},0};
        for(unsigned i=0;i<10;++i) { const auto e=effects.create_effect(0,script,attachment); ok(e.status); require(e.spawned,"source Title smoke emitter spawn"); }
        for(unsigned tick=0;tick<3;++tick) { ok(effects.update_particles()); ok(effects.update_effects(world,nullptr)); }
        const auto first=draw(executor,snapshot,banks,effects,cold,0x5a,false); cold=false;
        const auto second=draw(executor,snapshot,banks,effects,false,0xa5,false);
        require(first.color==second.color&&first.z==second.z,"Title CI4 output depended on unowned palette tail/prior TMEM");
        const auto blocked=draw(executor,snapshot,banks,effects,false,0x3c,true);
        require(first.color!=blocked.color,"Title particles ignored primitive Z test or painted no color");
        for(std::size_t i=0;i<blocked.color.size();++i) require(blocked.color[i]==std::byte((i&1)?0:1),"occluded source particles wrote framebuffer");
        for(unsigned tick=0;tick<5;++tick) ok(effects.update_particles());
        const auto animated=draw(executor,snapshot,banks,effects,false,0xc3,false);
        require(first.color!=animated.color,"actual Title animation did not change rendered colors");
        effects.detach_instance(attachment.instance); for(unsigned tick=0;tick<80;++tick) ok(effects.update_particles());
        require(effects.particle_count()==0&&effects.effect_count()==0,"Title smoke teardown retained pools");
        require(executor.quiescent(),"GPU lease not retired before Title runtime teardown");
    }
}
// Explicit native renderer stimulus, never a fabricated ROM or source fixture.
// All descriptors/payload spans are selector-derived; only bytecode/lifetimes,
// texture selection and particle placement are authored by this test consumer.
struct FormatStimulus {
    std::array<std::byte,3> bytes{std::byte(0xfc),std::byte(0x1f),std::byte(0xfd)};
    std::array<std::byte,5> frame9{std::byte(0x40),std::byte(9),std::byte(0xfc),std::byte(0x1f),std::byte(0xfd)};
    std::array<std::byte,9> black{std::byte(0xcf),std::byte(0),std::byte(0),std::byte(0),std::byte(0),std::byte(255),std::byte(0xfc),std::byte(0x1f),std::byte(0xfd)};
    std::array<std::byte,11> black9{std::byte(0x40),std::byte(9),std::byte(0xcf),std::byte(0),std::byte(0),std::byte(0),std::byte(0),std::byte(255),std::byte(0xfc),std::byte(0x1f),std::byte(0xfd)};
    EffectBank common,course;
    explicit FormatStimulus(const EffectResources& resources,EffectGroup group=EffectGroup::beach) {
        common.bank_id=3; common.images=resources.bank(EffectGroup::common).images;
        course.bank_id=0; course.images=resources.bank(group).images;
        for(auto* bank:{&common,&course}) for(std::size_t image=0;image<bank->images.size();++image) {
            EffectScript s{}; s.texture=std::uint16_t(image); s.particle_lifetime=10;
            s.parameters={0,1,0,0,0,0,0,-1,4}; s.bytecode={0,0,bytes};
            s.instructions={{0,1,no_index,0xfc},{1,1,no_index,0x1f},{2,1,1,0xfd}};
            if(group==EffectGroup::river&&bank==&course&&image==3) {
                s.bytecode={0,0,frame9}; s.instructions={{0,2,no_index,0x40},{2,1,no_index,0xfc},{3,1,no_index,0x1f},{4,1,3,0xfd}};
            }
            bank->scripts.push_back(std::move(s));
        }
        for(auto* bank:{&common,&course}) for(std::size_t image=0;image<bank->images.size();++image) {
            auto s=bank->scripts[image]; s.bytecode={0,0,black};
            s.instructions={{0,6,no_index,0xcf},{6,1,no_index,0xfc},{7,1,no_index,0x1f},{8,1,7,0xfd}};
            if(group==EffectGroup::river&&bank==&course&&image==3) {
                s.bytecode={0,0,black9};
                s.instructions={{0,2,no_index,0x40},{2,6,no_index,0xcf},{8,1,no_index,0xfc},{9,1,no_index,0x1f},{10,1,9,0xfd}};
            }
            bank->scripts.push_back(std::move(s));
        }
    }
};
void spawn_at(EffectRuntime& effects,unsigned bank,std::size_t image,Vec3 position) {
    const auto result=effects.create_particle(std::uint16_t(bank),image,position,{}); ok(result.status); require(result.spawned,"authored renderer stimulus particle drop");
}
void course_formats(isle::rsp::CanonicalExecutor& executor,const isle::rom::Snapshot& snapshot) {
    EffectResources resources(snapshot); FormatStimulus stimulus(resources);
    const auto& common=resources.bank(EffectGroup::common); const auto& beach=resources.bank(EffectGroup::beach);
    require(common.bank_id==3&&beach.bank_id==0&&common.images.size()==2&&beach.images.size()==6,"typed source resource identity");
    require(common.images[0].frames[0].bytes.size()==4096&&beach.images[2].width==48&&beach.images[3].format==0&&beach.images[3].size_code==2,"actual course format extents");
    const std::array banks{&stimulus.common,&stimulus.course}; isle_rng rng{0x12345678}; EffectRuntime effects(banks,rng); ok(effects.status());
    // Head insertion reverses these calls. One canonical packet must reload the
    // SAME CI4 palette after Common's4096-byte load overwrites its TMEM region.
    spawn_at(effects,0,4,{12.5f,0,0}); spawn_at(effects,3,0,{0,0,0}); spawn_at(effects,0,4,{-12.5f,0,0});
    const auto first=draw(executor,snapshot,banks,effects,false,0x5a,false,true);
    const auto repeat=draw(executor,snapshot,banks,effects,false,0xa5,false,true);
    require(first.color==repeat.color&&first.z==repeat.z,"multi-bank CI output depended on unowned RAM/TLUT tail");
    const auto blocked=draw(executor,snapshot,banks,effects,false,0x3c,true,true);
    require(first.color!=blocked.color,"multi-bank format packet produced no depth-tested color");
    // Both CI sprites have identical texture/palette and exact16-pixel center
    // separation from the middle. Compare all corresponding left/right pixels,
    // not merely nonempty output or command counts.
    unsigned colored=0;
    for(unsigned y=0;y<48;++y) for(unsigned x=8;x<24;++x) for(unsigned byte=0;byte<2;++byte) {
        const auto left=first.color[(y*64+x)*2+byte],right=first.color[(y*64+x+32)*2+byte];
        require(left==right,"equal CI4 sprites diverged after large-I8 TMEM overwrite");
        if(left!=blocked.color[(y*64+x)*2+byte]) ++colored;
    }
    require(colored>0,"CI4 equality oracle sampled no painted pixels");
    effects.reset();
    // Exercise real I8_32, non-power-of-two I4_48 and RGBA16_32 in actual GPU
    // execution. No source script is claimed to reference Beach image3.
    spawn_at(effects,0,1,{-12.5f,0,0}); spawn_at(effects,0,2,{0,0,0}); spawn_at(effects,0,3,{12.5f,0,0});
    const auto mixed=draw(executor,snapshot,banks,effects,false,0x9c,false);
    const std::vector<std::array<unsigned,3>> expected{{0,2,32},{4,0,48},{4,1,32}};
    require(mixed.texture_shapes==expected&&mixed.rectangles==3,"real course format/dimension command consumers");
    const auto mixed_blocked=draw(executor,snapshot,banks,effects,false,0x63,true);
    for(unsigned region=0;region<3;++region) {
        unsigned differences=0;
        for(unsigned y=0;y<48;++y) for(unsigned x=region*21;x<(region+1)*21;++x) for(unsigned b=0;b<2;++b)
            differences+=mixed.color[(y*64+x)*2+b]!=mixed_blocked.color[(y*64+x)*2+b];
        require(differences>0,"one actual I8/I4_48/RGBA16 sprite painted no color");
    }
    for(unsigned tick=0;tick<10;++tick) ok(effects.update_particles());
    require(effects.particle_count()==0&&effects.effect_count()==0&&executor.quiescent(),"course runtime/resource/GPU teardown");
}
void remaining_formats(isle::rsp::CanonicalExecutor& executor,const isle::rom::Snapshot& snapshot) {
    EffectResources resources(snapshot);
    for(const auto group:{EffectGroup::volcano,EffectGroup::valley,EffectGroup::river}) {
        // Each resource group installs into a fresh slot0 scene. Never publish
        // different course groups simultaneously under the same numeric slot.
        FormatStimulus stimulus(resources,group);
        const std::array banks{&stimulus.common,&stimulus.course}; isle_rng rng{123}; EffectRuntime effects(banks,rng); ok(effects.status());
        const std::size_t image=group==EffectGroup::river?3:0;
        const unsigned format=group==EffectGroup::volcano?3:0,size=group==EffectGroup::volcano?1:group==EffectGroup::valley?3:2;
        const auto& source=resources.bank(group).images[image];
        require(source.format==format&&source.size_code==size&&source.width==32&&source.height==32,"extra format typed descriptor identity");
        if(group==EffectGroup::river) require(source.frames.size()==10,"River ten-frame resource binding");
        spawn_at(effects,0,image,{0,0,0}); EffectParticleView p{}; require(effects.particle(0,p),"extra format particle");
        if(group==EffectGroup::river) require(p.frame==9,"actual tenth borrowed frame selected");
        const auto visible=draw(executor,snapshot,banks,effects,false,0x5a,false);
        const std::vector<std::array<unsigned,3>> expected{{format,size,32}};
        require(visible.texture_shapes==expected&&visible.rectangles==1,"actual IA8/RGBA32/tenth-frame packet");
        const auto alternate=draw(executor,snapshot,banks,effects,false,0xa5,false);
        require(visible.color==alternate.color&&visible.z==alternate.z,"extra format depended on unowned payload tails");
        const auto blocked=draw(executor,snapshot,banks,effects,false,0x3c,true);
        if(group!=EffectGroup::river) require(visible.color!=blocked.color,"actual extra-format sprite produced no depth-tested color");
        effects.reset(); spawn_at(effects,0,image+stimulus.course.images.size(),{0,0,0});
        const auto black=draw(executor,snapshot,banks,effects,false,0x96,false);
        require(black.color==blocked.color&&black.z==visible.z,"actual texture * black primitive modulation or translucent depth changed");
        for(unsigned tick=0;tick<10;++tick) ok(effects.update_particles());
        require(effects.particle_count()==0&&executor.quiescent(),"extra format resources retired before GPU quiescence");
    }
}
std::array<unsigned,4> rgba16(unsigned value) {
    const auto expand=[](unsigned v) { return (v<<3)|(v>>2); };
    return {expand((value>>11)&31),expand((value>>6)&31),expand((value>>1)&31),(value&1)?255u:0u};
}
std::array<unsigned,4> texel(const EffectImage& image,unsigned frame,unsigned x,unsigned y) {
    const auto bytes=image.frames[frame].bytes;
    const auto byte=[&](std::size_t at) { return std::to_integer<unsigned>(bytes[at]); };
    const auto index=std::size_t(y)*image.width+x;
    if(image.format==4) {
        const unsigned intensity=image.size_code==0?((byte(index/2)>>((index&1)?0:4))&15)*17:byte(index);
        return {intensity,intensity,intensity,intensity};
    }
    if(image.format==3) {
        const auto value=byte(index); const auto intensity=(value>>4)*17;
        return {intensity,intensity,intensity,(value&15)*17};
    }
    if(image.format==2) {
        const auto palette=image.palettes[(image.flags&1)?0:frame].bytes;
        const auto entry=(byte(index/2)>>((index&1)?0:4))&15;
        return rgba16((std::to_integer<unsigned>(palette[entry*2])<<8)|std::to_integer<unsigned>(palette[entry*2+1]));
    }
    if(image.size_code==2) return rgba16((byte(index*2)<<8)|byte(index*2+1));
    return {byte(index*4),byte(index*4+1),byte(index*4+2),byte(index*4+3)};
}
void sampled_format_oracles(isle::rsp::CanonicalExecutor& executor,const isle::rom::Snapshot& snapshot) {
    EffectResources resources(snapshot);
    struct Case { EffectGroup group; unsigned bank,image; };
    const std::array cases{
        Case{EffectGroup::beach,3,0}, // Common I8_64 in the shared course scene
        Case{EffectGroup::beach,0,2}, // I4_48
        Case{EffectGroup::beach,0,3}, // RGBA16
        Case{EffectGroup::beach,0,4}, // CI4 with real per-frame RGBA16 palette
        Case{EffectGroup::volcano,0,0},Case{EffectGroup::valley,0,0},Case{EffectGroup::river,0,3}};
    for(const auto test:cases) {
        FormatStimulus stimulus(resources,test.group);
        auto& bank=test.bank==3?stimulus.common:stimulus.course;
        const auto& image=bank.images[test.image];
        // An exact binary32 size/camera construction yields one source texel per
        // framebuffer pixel. The64x64 case is vertically scissored, not rescaled.
        bank.scripts[test.image].parameters[8]=float(image.width)*0.390625f;
        const std::array banks{&stimulus.common,&stimulus.course}; isle_rng rng{7}; EffectRuntime effects(banks,rng); ok(effects.status());
        spawn_at(effects,test.bank,test.image,{0,0,0}); EffectParticleView p{}; require(effects.particle(0,p),"format-oracle particle");
        const auto actual=draw(executor,snapshot,banks,effects,false,0x5a,false,false,true);
        const int left=32-int(image.width)/2,top=24-int(image.height)/2;
        unsigned compared=0;
        for(int y=2;y<46;++y) for(int x=2;x<62;++x) {
            const int sx=x-left,sy=y-top;
            if(sx<2||sy<2||sx>=int(image.width)-2||sy>=int(image.height)-2) continue;
            const auto expected=texel(image,p.frame,unsigned(sx),unsigned(sy));
            const unsigned at=unsigned((y*64+x)*2);
            // Completed-memory bytes retain the canonical executor's word XOR3
            // view; source payloads above are ordinary borrowed big-endian bytes.
            const unsigned pixel=(std::to_integer<unsigned>(actual.color[at^3u])<<8)|
                                  std::to_integer<unsigned>(actual.color[(at+1)^3u]);
            const std::array channels{(pixel>>11)&31,(pixel>>6)&31,(pixel>>1)&31};
            for(unsigned c=0;c<3;++c) {
                if(expected[3]<8) require(channels[c]==0,"decoded transparent texel painted RGB");
                else {
                    // White primitive MODULATEIA, black destination, full
                    // interior coverage. Allow two RGB5 levels for the RDP's
                    // combiner/blender rounding; never substitute another format.
                    const unsigned wanted=((expected[c]*expected[3]+127)/255)>>3;
                    const unsigned difference=channels[c]>wanted?channels[c]-wanted:wanted-channels[c];
                    require(difference<=2,"completed RGB disagrees with independently decoded source texel/alpha");
                }
            }
            ++compared;
        }
        require(compared>=128,"format oracle did not compare a meaningful source-pixel region");
        for(unsigned tick=0;tick<10;++tick) ok(effects.update_particles());
        require(effects.particle_count()==0&&executor.quiescent(),"format-oracle GPU/resource retirement");
    }
}
}
int main(int argc,char** argv) {
    try {
        require(argc==2,"expected one explicit private ROM path");
        const auto snapshot=isle::rom::read_snapshot(std::filesystem::path(argv[1]));
        const auto hydrated=isle::menu::hydrate(snapshot); require(bool(hydrated),"menu resource admission failed");
        isle::rsp::CanonicalExecutor executor(true,{1000000,probe::capture_words});
        title(executor,snapshot,*hydrated.catalog); course_formats(executor,snapshot); remaining_formats(executor,snapshot);
        sampled_format_oracles(executor,snapshot);
        std::cout<<"shared_effect_actual_rsp_rdp=passed Title_scripts=0,3 course_formats=I8,I4_48,RGBA16,CI4,IA8,RGBA32 tenth_frame=yes "
                    "same_palette_after_large_I8=reload pal16_tail_independent=yes translucent_depth_unchanged=yes teardown=yes Original_parity=unqualified\n";
        return 0;
    } catch(const std::filesystem::filesystem_error&) { std::cerr<<"private ROM operation failed\n"; }
      catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; }
    return 1;
}
