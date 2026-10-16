// Public authored-IR consumers: no ROM, copied source bytecode or asset fixtures.
#include "menu_effects.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace isle::menu;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void ok(EffectsStatus s) { require(bool(s), "effect operation rejected"); }
void near(float a, float b, const char* why) { require(std::abs(a-b) <= 0.00002f, why); }
void vector_near(Vec3 a, Vec3 b, const char* why) { near(a.x,b.x,why); near(a.y,b.y,why); near(a.z,b.z,why); }
struct Program {
    std::vector<std::byte> bytes;
    std::vector<EffectInstruction> instructions;
    std::size_t start = 0;
    Program& op(unsigned opcode) {
        start = bytes.size(); bytes.push_back(std::byte(opcode));
        instructions.push_back({start,1,no_index,std::uint8_t(opcode)}); return *this;
    }
    Program& u8(unsigned n) { bytes.push_back(std::byte(n)); ++instructions.back().size; return *this; }
    Program& u16(unsigned n) { return u8(n>>8).u8(n&255); }
    Program& f(float n) {
        const auto bits = std::bit_cast<std::uint32_t>(n);
        return u8(bits>>24).u8((bits>>16)&255).u8((bits>>8)&255).u8(bits&255);
    }
    Program& vec(Vec3 v) { return f(v.x).f(v.y).f(v.z); }
    // Yield, then return to the yielding instruction. The source return register
    // initially names byte offset zero; these programs place FC explicitly.
    Program& hold() { return op(0xfc).op(0x1f).op(0xfd); }
};
struct Fixture {
    EffectBank bank;
    std::vector<Program> programs;
    std::array<std::array<std::byte,512>,10> pixels{};
    explicit Fixture(unsigned bank_id = 0) {
        bank.bank_id = bank_id;
        EffectImage image{}; image.format=4; image.size_code=0; image.width=image.height=32;
        for (const auto& frame : pixels) image.frames.push_back({0,0,frame});
        bank.images.push_back(std::move(image));
    }
    std::size_t add(Program p, Vec3 velocity = {}, unsigned lifetime = 1000) {
        programs.push_back(std::move(p));
        const auto& program=programs.back();
        EffectScript s{}; s.bytecode={0,0,program.bytes}; s.instructions=program.instructions;
        s.particle_lifetime=std::uint16_t(lifetime); s.parameters={0,1,velocity.x,velocity.y,velocity.z,0,0,-1,4};
        bank.scripts.push_back(std::move(s)); return bank.scripts.size()-1;
    }
};
EffectParticleView particle(const EffectRuntime& runtime, std::size_t index=0) {
    EffectParticleView out{}; require(runtime.particle(index,out),"missing particle in source order"); return out;
}
void spawn(EffectRuntime& r, unsigned bank, std::size_t script) {
    const auto s=r.create_particle(std::uint16_t(bank),script); ok(s.status); require(s.spawned,"unexpected particle drop");
}
void masks_and_physics() {
    for (unsigned mask=0; mask<8; ++mask) {
        Fixture f;
        Program p; p.op(0x80|mask);
        for (unsigned axis=0;axis<3;++axis) if (mask&(1u<<axis)) p.f(float(axis+2));
        p.op(0x88|mask); for (unsigned axis=0;axis<3;++axis) if(mask&(1u<<axis)) p.f(10);
        p.op(0x90|mask); for (unsigned axis=0;axis<3;++axis) if(mask&(1u<<axis)) p.f(float(axis+1));
        p.op(0x98|mask); for (unsigned axis=0;axis<3;++axis) if(mask&(1u<<axis)) p.f(2);
        p.hold(); f.add(std::move(p));
        const std::array banks{&f.bank}; isle_rng rng{9}; EffectRuntime r(banks,rng); ok(r.status());
        const auto result=r.create_particle(0,0,{20,30,40},{5,6,7}); ok(result.status);
        const auto v=particle(r); const std::array original_pos{20.f,30.f,40.f}, original_vel{5.f,6.f,7.f};
        std::array<float,3> pos{},vel{};
        for(unsigned k=0;k<3;++k) { vel[k]=(mask&(1u<<k))?float(k+3):original_vel[k]; pos[k]=((mask&(1u<<k))?float(k+12):original_pos[k])+vel[k]; }
        vector_near(v.position,{pos[0],pos[1],pos[2]},"masked position set/add or immediate integration");
        vector_near(v.velocity,{vel[0],vel[1],vel[2]},"masked velocity set/add");
        require(v.lifetime==1000 && rng.seed==9,"immediate lifetime or deterministic masks consumed RNG");
    }
    Fixture f; Program p;
    p.op(0xa2).f(2).op(0xa3).f(0.5f).op(1).op(0xa2).f(0).op(0xa3).f(1).hold(); f.add(std::move(p));
    const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
    ok(r.create_particle(0,0,{1,2,3},{4,6,8}).status);
    vector_near(particle(r).velocity,{2,2,4},"gravity must precede friction");
    vector_near(particle(r).position,{3,4,7},"integration must follow gravity/friction");
    require((particle(r).flags&3)==3,"A2/A3 enable bits"); ok(r.update_particles());
    require((particle(r).flags&3)==0,"A2 zero/A3 one disable bits");
    vector_near(particle(r).velocity,{2,2,4},"disabled physics still applied");
}
void interpolation_and_waits() {
    Fixture f; Program p;
    p.op(0xa0).u8(2).f(10).op(0xcf).u8(2).u8(0).u8(60).u8(120).u8(180)
     .op(0xdf).u8(2).u8(90).u8(120).u8(150).u8(180).op(0x40).u8(2) // frame with zero delay continues
     .op(0x60).u8(0).u8(3).op(0x43).u8(4).op(0xfe);
    f.add(std::move(p)); const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
    require(particle(r).frame==4,"zero packed/extended waits must continue until nonzero delay");
    std::array<std::uint8_t,4> prim{255,255,255,255},env{};
    const std::array<unsigned,4> pt{0,60,120,180},et{90,120,150,180};
    for(unsigned tick=0;tick<3;++tick) {
        const int n=int(3-tick);
        for(unsigned k=0;k<4;++k) {
            prim[k]=std::uint8_t(((int(prim[k])<<16)+(int(pt[k])-int(prim[k]))*(65536/n))>>16);
            env[k]=std::uint8_t(((int(env[k])<<16)+(int(et[k])-int(env[k]))*(65536/n))>>16);
        }
        const auto v=particle(r); near(v.size,float(6+2*tick),"size duration+1 interpolation");
        require(v.primitive==prim && v.environment==env,"source integer16 color interpolation");
        if(tick<2) ok(r.update_particles());
    }
    ok(r.update_particles());
    require(r.particle_count()==0,"FE should retire after packed wait expires");
}
void control_and_retirement() {
    for(unsigned count : {0u,1u,3u,255u}) {
        Fixture f; Program p; p.op(0xfa).u8(count).op(0x89).f(1).op(0xfb).hold();
        p.instructions[2].branch_offset=p.instructions[1].offset;
        f.add(std::move(p)); const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
        near(particle(r).position.x,float(count?count:256),"FA zero must wrap uint8 loop count; finite no-wait loop");
    }
    { Fixture f; Program p; p.op(0x89).f(1).op(1).op(0xfd); f.add(std::move(p));
      const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
      for(unsigned tick=1;tick<=4;++tick) { near(particle(r).position.x,float(tick),"FD initial return register zero"); ok(r.update_particles()); } }
    { Fixture f; Program p; p.op(0x89).f(10).op(0xfc).op(0x89).f(1).op(0xfc).op(0x89).f(2).op(1).op(0xfd); f.add(std::move(p));
      const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0); near(particle(r).position.x,13,"FC first pass");
      ok(r.update_particles()); near(particle(r).position.x,15,"FC replaces one return register, not call stack"); }
    for(unsigned end : {0xfeu,0xffu}) { Fixture f; Program p; p.op(1).op(end); f.add(std::move(p),{1,0,0},3);
      const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0); require(particle(r).lifetime==3,"spawn lifetime+1 immediate decrement");
      ok(r.update_particles()); require(r.particle_count()==0,"FE/FF retire before position integration"); }
    { Fixture f; Program p; p.hold(); f.add(std::move(p),{},2); const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
      spawn(r,0,0); ok(r.update_particles()); require(particle(r).lifetime==1,"last live lifetime"); ok(r.update_particles()); require(r.particle_count()==0,"natural lifetime retirement"); }
}
void rotation_a9() {
    Fixture f; Program p; constexpr float angle=0.63f; p.op(0xa9).f(angle).hold(); f.add(std::move(p));
    const std::array banks{&f.bank}; isle_rng rng{0x12345678},oracle=rng;
    // Independent source expression evaluation, not the runtime helper or a
    // length-only invariant (the historical disputed sign changes z alone).
    const Vec3 input{2.25f,-3.5f,4.75f};
    const float pitch=std::atan2(input.y,input.z),sp=std::sin(pitch),cp=std::cos(pitch);
    const float yaw=std::atan2(input.x,input.y*sp+input.z*cp),sy=std::sin(yaw),cy=std::cos(yaw);
    const float length=std::sqrt(input.x*input.x+input.y*input.y+input.z*input.z);
    const float azimuth=isle_rng_next_float(&oracle)*6.2831855f;
    const float radial=std::sin(angle)*length;
    const float x=std::cos(azimuth)*radial,y=std::sin(azimuth)*radial,axial=std::cos(angle)*length;
    const Vec3 expected{x*cy+axial*sy,-x*sp*sy+y*cp+axial*sp*cy,-x*cp*sy-y*sp+axial*cp*cy};
    require(std::abs(2*y*sp)>0.1f,"A9 case does not discriminate disputed sign");
    EffectRuntime r(banks,rng); ok(r.status()); ok(r.create_particle(0,0,{1,2,3},input).status);
    const auto actual=particle(r);
    require(std::bit_cast<std::uint32_t>(actual.velocity.x)==std::bit_cast<std::uint32_t>(expected.x) &&
            std::bit_cast<std::uint32_t>(actual.velocity.y)==std::bit_cast<std::uint32_t>(expected.y) &&
            std::bit_cast<std::uint32_t>(actual.velocity.z)==std::bit_cast<std::uint32_t>(expected.z),"A9 non-axis binary32 grouping/source minus sign");
    require(rng.seed==oracle.seed,"A9 must consume exactly one RNG draw");
    vector_near(actual.position,{1+expected.x,2+expected.y,3+expected.z},"A9 immediate integration");
}
void random_operations() {
    Fixture f; Program p;
    p.op(0xa6).u16(40).u16(9).op(0xa8).vec({2,3,4}).op(0xab).f(2).op(0xac).u8(0).f(5).f(3)
     .op(0xcf).u8(0).u8(10).u8(20).u8(30).u8(40).op(0xba).u8(7).u8(8).u8(9).u8(10)
     .op(0xdf).u8(0).u8(20).u8(30).u8(40).u8(50).op(0xbb).u8(11).u8(12).u8(13).u8(14)
     .op(0xbc).u8(2).u8(4).op(0xbd).f(6).f(2).op(0xbe).vec({1,2,0.5f}).hold(); f.add(std::move(p));
    const std::array banks{&f.bank}; isle_rng rng{0x12345678},oracle=rng;
    const unsigned life=40+unsigned(isle_rng_next_float(&oracle)*9);
    const Vec3 offset{2*isle_rng_next_float(&oracle),3*isle_rng_next_float(&oracle),4*isle_rng_next_float(&oracle)};
    const float size=5+3*isle_rng_next_float(&oracle);
    std::array<std::uint8_t,4> prim{10,20,30,40},env{20,30,40,50};
    for(unsigned k=0;k<4;++k) prim[k]=std::uint8_t(float(prim[k])+float(7+k)*isle_rng_next_float(&oracle));
    for(unsigned k=0;k<4;++k) env[k]=std::uint8_t(float(env[k])+float(11+k)*isle_rng_next_float(&oracle));
    const unsigned frame=unsigned(2+4*isle_rng_next_float(&oracle)); const float speed=6+2*isle_rng_next_float(&oracle);
    const float scale=speed/std::sqrt(4.f*4.f+6.f*6.f+12.f*12.f);
    const Vec3 velocity{4*scale,6*scale*2,12*scale*0.5f};
    EffectRuntime r(banks,rng); ok(r.status()); ok(r.create_particle(0,0,{1,2,3},{2,3,6}).status); const auto v=particle(r);
    require(rng.seed==oracle.seed && v.lifetime==life-1 && v.frame==frame,"random opcode draw order/lifetime/frame bounds");
    require(v.primitive==prim && v.environment==env,"BA/BB independent channel draws or immediate target assignment"); near(v.size,size,"AC immediate random size");
    vector_near(v.velocity,velocity,"AB/BD/BE source scale order"); vector_near(v.position,{1+offset.x+velocity.x,2+offset.y+velocity.y,3+offset.z+velocity.z},"A8 independent offsets before integration");
    for(unsigned probability : {0u,100u}) {
        Fixture death; Program q; q.op(0xa7).u8(probability).hold(); death.add(std::move(q)); const std::array db{&death.bank};
        isle_rng dr{0x12345678},expected=dr; const bool killed=probability>=unsigned(isle_rng_next_float(&expected)*100);
        EffectRuntime d(db,dr); ok(d.status()); spawn(d,0,0); require(d.particle_count()==(killed?0u:1u) && dr.seed==expected.seed,"A7 inclusive integer-percent death and one draw");
    }
}
void color_masks_and_extended_duration() {
    for(unsigned mask=0;mask<16;++mask) {
        Fixture f; Program p; p.op(0xc0|mask).u8(0);
        for(unsigned k=0;k<4;++k) if(mask&(1u<<k)) p.u8(10+k);
        p.op(0xd0|mask).u8(0); for(unsigned k=0;k<4;++k) if(mask&(1u<<k)) p.u8(20+k);
        p.hold(); f.add(std::move(p)); const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
        const auto v=particle(r);
        for(unsigned k=0;k<4;++k) require(v.primitive[k]==((mask&(1u<<k))?10+k:255) &&
                                         v.environment[k]==((mask&(1u<<k))?20+k:0),"color component masks preserve unselected channels");
    }
    Fixture f; Program p; p.op(0xa0).u8(0x80).u8(0x80).f(133).op(0x61).u8(0).u8(9).op(0xff); f.add(std::move(p));
    const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
    near(particle(r).size,5,"two-byte duration decodes 128+1"); require(particle(r).frame==9,"ten-frame image upper bound");
    for(unsigned tick=1;tick<129;++tick) ok(r.update_particles());
    near(particle(r).size,133,"extended size interpolation final target");
    for(unsigned tick=129;tick<256;++tick) ok(r.update_particles());
    require(r.particle_count()==1,"extended wait decoded as zero or low byte only");
    ok(r.update_particles()); require(r.particle_count()==0,"extended wait 256 expires on exact update");
}
void child_emitters_and_sibling_order() {
    Fixture f(3); Program p; p.op(0xa4).u16(1).op(0xa4).u16(2).op(0xa5).u16(3).hold();
    f.add(std::move(p)); Program one; one.op(0x81).f(11).hold(); f.add(one);
    Program two; two.op(0x81).f(22).hold(); f.add(two);
    Program emitter; emitter.hold(); f.add(emitter); auto& s=f.bank.scripts[3];
    s.parameters[5]=-0.0f; s.parameters[6]=-0.0f; s.parameters[7]=-1; s.parameters[4]=2;
    const std::array banks{&f.bank}; isle_rng rng{123}; EffectRuntime r(banks,rng); ok(r.status());
    const auto parent=r.create_particle(11,0,{5,6,7},{9,0,0}); ok(parent.status);
    require(r.particle_count()==3 && r.effect_count()==1,"A4 siblings/A5 emitter allocation");
    near(particle(r,1).position.x,22,"later child inserted immediately after parent"); near(particle(r,2).position.x,11,"first child follows later sibling");
    ok(r.update_effects());
    const auto child=particle(r,0); require(child.effect_id==parent.effect_id && child.bank_id==11,"A5 emitter preserves encoded bank/inherited source ID");
    vector_near(child.position,{5,6,9},"A5 copies pre-integration position and retains child script velocity, not parent velocity");
    require(r.create_effect(3,3).effect_id==5,"child particle/emitter allocations consume IDs before inheritance");
    r.eject_id(parent.effect_id,1); require(r.particle_count()==0 && r.effect_count()==1,"ID ejection removes parent/children/inherited emitter, retains unrelated emitter");
}
void regular_child_traversal() {
    Fixture f; Program p; p.op(1).op(0xa4).u16(1).hold(); f.add(p);
    Program child; child.op(0x89).f(10).hold(); f.add(child,{3,0,0});
    const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
    ok(r.update_particles()); require(r.particle_count()==2,"regular tick child allocation");
    const auto v=particle(r,1); near(v.position.x,16,"regular list traversal ticks new child after its immediate tick");
    require(v.lifetime==999,"regular child immediate plus outer traversal lifetime");
}
void source_id_wrap() {
    Fixture f; Program p; p.hold(); f.add(p); const std::array banks{&f.bank}; isle_rng rng{7}; EffectRuntime r(banks,rng); ok(r.status());
    for(unsigned counter=1;counter<=65537;++counter) {
        const auto e=r.create_effect(0,0); ok(e.status); require(e.spawned&&e.effect_id==std::uint16_t(counter),"source uint16 ID wrap is separate from native handle");
        r.eject_effect(e.handle);
    }
    require(rng.seed==7&&r.effect_count()==0,"create/eject consumed random state or retained emitters");
}
void operand_bounds() {
    const auto rejects=[](Program p) {
        Fixture f; f.add(std::move(p)); const std::array banks{&f.bank}; isle_rng rng{1};
        EffectRuntime r(banks,rng); require(!r.status(),"malformed authored operand was admitted"); require(rng.seed==1,"admission consumed RNG");
    };
    for(unsigned op : {0xa4u,0xa5u,0xb9u}) { Program p; p.op(op).u16(1).hold(); rejects(std::move(p)); }
    { Program p; p.op(0xaa).u16(1).u16(0).hold(); rejects(std::move(p)); }
    { Program p; p.op(0xbc).u8(10).u8(0).hold(); rejects(std::move(p)); }
    { Program p; p.op(0x41).u8(10).hold(); rejects(std::move(p)); }
    { Program p; p.op(0xbf).u8(0).hold(); rejects(std::move(p)); }
    { Program p; p.op(0xbf).u8(9).hold(); rejects(std::move(p)); }
    { Program p; p.op(0xa0).u8(0).f(std::numeric_limits<float>::infinity()).hold(); rejects(std::move(p)); }
    { Program p; p.op(0x20).u8(0).op(0xfd); rejects(std::move(p)); } // decoded zero wait cannot certify a cycle
    { Program p; p.op(0xa8).f(1); rejects(std::move(p)); }
}
void flags() {
    constexpr std::array<unsigned,13> ops{0xa1,0xad,0xaf,0xb0,0xb1,0xae,0xb2,0xb4,0xb3,0xb5,0xb6,0xc0,0xd0};
    constexpr std::array<unsigned,13> expected{0,0x80,0xa0,0xc0,0xe0,0x80,0x280,0x680,0x280,0x380,0x280,0x280,0x280};
    Fixture f; Program p; for(auto op:ops) { p.op(op); if(op==0xa1||op==0xc0||op==0xd0) p.u8(0); p.op(1); } p.op(0xff); f.add(std::move(p));
    const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status()); spawn(r,0,0);
    for(unsigned k=0;k<ops.size();++k) { require(particle(r).flags==expected[k],"render flag transition"); ok(r.update_particles()); }
    require(r.particle_count()==0,"flag script END");
}
void children_and_lists() {
    for(unsigned child_op : {0xa4u,0xaau,0xb9u}) {
        Fixture f(3); Program parent; parent.op(child_op).u16(1); if(child_op==0xaa) parent.u16(1); parent.hold();
        f.add(std::move(parent),{2,0,0}); Program child; child.op(0x89).f(10).hold(); f.add(std::move(child),{3,0,0});
        const std::array banks{&f.bank}; isle_rng rng{7},oracle=rng; if(child_op==0xaa) isle_rng_next_float(&oracle);
        EffectRuntime r(banks,rng); ok(r.status()); const auto result=r.create_particle(11,0,{5,0,0},{2,0,0}); ok(result.status);
        require(r.particle_count()==2,"child not allocated"); const auto a=particle(r,0),b=particle(r,1);
        require(a.effect_id==result.effect_id && b.effect_id==a.effect_id && a.bank_id==11 && b.bank_id==11,"child inherited ID/encoded bank");
        near(a.position.x,7,"parent integration order"); near(b.position.x,child_op==0xb9?17.f:18.f,"child inherits position before parent integration and ticks immediately");
        require(a.lifetime==1000 && b.lifetime==1000,"child immediate tick doubled by list traversal");
        require(rng.seed==oracle.seed,"child creation RNG order"); ok(r.update_particles()); near(particle(r,1).position.x,child_op==0xb9?19.f:21.f,"child insertion after parent update order");
        require(r.create_effect(3,1).effect_id==3,"child allocation consumes source ID before inheriting parent ID");
    }
    Fixture course(0),common(3); Program hold; hold.hold(); course.add(hold,{1,0,0}); common.add(hold,{2,0,0});
    const std::array banks{&common.bank,&course.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
    spawn(r,11,0); spawn(r,0,0); spawn(r,3,0); spawn(r,120,0);
    require(particle(r,0).bank_id==3 && particle(r,1).bank_id==0 && particle(r,2).bank_id==11 && particle(r,3).bank_id==120,"sixteen encoded lists, ascending lists and head insertion");
    const auto paused=particle(r,2); r.pause_lists(1u<<1); ok(r.update_particles()); require(particle(r,2).lifetime==paused.lifetime,"list pause changed lifetime");
    r.pause_lists(0); r.pause_id(particle(r,0).effect_id,0,true); const auto stopped=particle(r,0); ok(r.update_particles());
    require(particle(r,0).position.x==stopped.position.x && particle(r,0).lifetime==stopped.lifetime,"ID pause advanced physics/timer");
    r.pause_id(stopped.effect_id,0,false); ok(r.update_particles()); near(particle(r,0).position.x,stopped.position.x+2,"ID resume");
    r.eject_id(stopped.effect_id,1); require(r.particle_count()==4,"ID eject crossed requested particle list"); r.eject_id(stopped.effect_id,0); require(r.particle_count()==3,"ID eject did not remove match");
}
struct LocalTargets {
    EffectAttachment aim{},output{}; Vec3 local{3,4,0},written{}; bool alive=true;
};
bool target_read(void* context,EffectAttachment a,Vec3& out) noexcept {
    auto& t=*static_cast<LocalTargets*>(context);
    if(!t.alive || a.instance!=t.aim.instance || a.node!=t.aim.node) return false;
    out=t.local; return true;
}
bool target_write(void* context,EffectAttachment a,Vec3 value) noexcept {
    auto& t=*static_cast<LocalTargets*>(context);
    if(!t.alive || a.instance!=t.output.instance || a.node!=t.output.node) return false;
    t.written=value; return true;
}
void target_local_services() {
    Fixture f; Program p; p.op(0xb7).u8(0).op(0xb8).u8(0).f(5).op(0xbf).u8(8).hold(); f.add(std::move(p));
    LocalTargets t{{animation::InstanceHandle{7,1,2},3},{animation::InstanceHandle{7,1,2},4}};
    const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng,{&t,target_read,target_write}); ok(r.status());
    ok(r.register_aim_target(0,t.aim)); ok(r.register_output_target(7,t.output));
    require(r.register_aim_target(1,t.aim).error!=EffectsError::none && r.register_output_target(8,t.output).error!=EffectsError::none,"target slot bounds");
    ok(r.create_particle(0,0,{0,0,0},{0,0,10}).status);
    vector_near(particle(r).velocity,{6.6f,8.8f,0},"B7 local aim preserving speed then B8 inverse squared local attraction");
    vector_near(t.written,{6.6f,8.8f,0},"BF publishes integrated local motion after B7/B8, not the pre-step position");
    t.written={101,203,307};
    r.clear_targets(animation::InstanceHandle{7,2,2}); ok(r.update_particles());
    vector_near(t.written,{13.2f,17.6f,0},"foreign incarnation incorrectly cleared the active local output target");
    t.written={101,203,307}; r.clear_targets(t.aim.instance); ok(r.update_particles());
    vector_near(t.written,{101,203,307},"cleared native incarnation still changed local target state");
    // Missing source slots do not fabricate an aim or an output destination.
    r.reset(); spawn(r,0,0);
    vector_near(particle(r).velocity,{},"null target changed zero velocity");
    vector_near(t.written,{101,203,307},"empty source target registry fabricated a local write");
    ok(r.register_output_target(7,t.output)); t.alive=false;
    require(r.update_particles().error==EffectsError::attachment,"stale registered native target was silently accepted");
}
EffectWorld identity() { EffectWorld w{}; for(unsigned k=0;k<4;++k) w.matrix[k][k]=1; return w; }
bool world(void* context,EffectAttachment,EffectWorld& out) noexcept { out=*static_cast<EffectWorld*>(context); return true; }
bool missing(void*,EffectAttachment,EffectWorld&) noexcept { return false; }
void emitters_pools_and_retirement() {
    Fixture f(0),g(3); Program p; p.hold(); f.add(p,{},50); g.add(p,{},50);
    for(auto* bank : {&f.bank,&g.bank}) { auto& s=bank->scripts[0]; s.parameters[5]=-1; s.parameters[6]=-1; s.parameters[7]=-1; s.effect_lifetime=0; }
    const std::array banks{&f.bank,&g.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
    for(unsigned k=0;k<10;++k) { const auto e=r.create_effect(k&1?3:0,0); ok(e.status); require(e.spawned && e.effect_id==k+1,"shared emitter pool/global ID"); }
    const auto seed=rng.seed; const auto drop=r.create_effect(3,0); require(bool(drop.status)&&!drop.spawned&&rng.seed==seed,"10-emitter exhaustion is successful null spawn without RNG");
    r.reset(); require(rng.seed==seed,"reset reseeded external RNG");
    for(unsigned k=0;k<100;++k) spawn(r,k&1?3:0,0);
    const auto exhausted=r.create_particle(0,0); require(bool(exhausted.status)&&!exhausted.spawned&&r.particle_count()==100&&rng.seed==seed,"common/course share 100-particle pool");
    const auto e=r.create_effect(3,0); require(e.spawned && e.effect_id==111,"exhaustion must not consume ID; reset must preserve counter");
    isle_rng expected=rng; isle_rng_next_float(&expected); // fixed negative radius/angle/rate: initial azimuth only, even on drop
    ok(r.update_effects()); require(rng.seed==expected.seed && r.particle_count()==100,"full-pool emitter geometry RNG must still advance");
    const auto stale=e.handle; r.eject_effect(e.handle); require(r.effect_count()==0,"explicit emitter ejection");
    const auto replacement=r.create_effect(0,0); r.eject_effect(stale); require(r.effect_count()==1,"stale handle removed reused emitter slot");
    require(r.configure_effect(stale,{},{ }).error==EffectsError::attachment,"stale configuration handle accepted"); r.eject_effect(replacement.handle);
    r.reset(); const EffectAttachment old{animation::InstanceHandle{7,1,2},0},newer{animation::InstanceHandle{7,2,2},0};
    const auto attached=r.create_effect(0,0,old); require(attached.spawned,"attached emitter spawn");
    {
        isle_rng failed_rng{123}; EffectRuntime failed(banks,failed_rng); ok(failed.status());
        require(failed.create_effect(0,0,old).spawned,"error-path attached emitter");
        require(failed.update_effects(missing,nullptr).error==EffectsError::attachment&&failed.particle_count()==0,
                "invalid native attachment must report error without fake particle/fallback");
    }
    auto w=identity(); w.matrix[3][0]=10; ok(r.update_effects(world,&w)); require(r.particle_count()==1,"attached emission");
    r.create_effect(3,0,newer); r.detach_instance(old.instance); require(r.effect_count()==1&&r.particle_count()==1,"detach must remove only matching incarnation emitter, not ordinary particles");
    r.detach_instance(newer.instance); require(r.effect_count()==0,"new incarnation detach");
    r.translate({7,8,9}); const auto moved=particle(r); require(moved.position.x>15,"translation did not shift live particles");
    const auto plain=r.create_effect(3,0); ok(r.configure_effect(plain.handle,{1,2,3},{})); r.translate({7,8,9}); ok(r.update_effects());
    const auto emitted=particle(r); // radius=1 can affect x/y, but source z here is exact.
    near(emitted.position.z,12,"translation must also shift emitters before subsequent emission");
    r.pause_id(plain.effect_id,0,true); const auto paused_seed=rng.seed; ok(r.update_effects()); require(rng.seed==paused_seed,"paused emitter consumed RNG");
    r.eject_id(plain.effect_id,15); require(r.effect_count()==0,"source ID ejection scans emitters independent of particle link");
    Fixture finite; finite.add(p,{},50); finite.bank.scripts[0].effect_lifetime=2;
    const std::array finite_banks{&finite.bank}; isle_rng finite_rng{1}; EffectRuntime mortal(finite_banks,finite_rng); ok(mortal.status());
    require(mortal.create_effect(0,0).spawned,"finite emitter"); ok(mortal.update_effects()); require(mortal.effect_count()==1,"emitter retired early");
    ok(mortal.update_effects()); require(mortal.effect_count()==0&&mortal.particle_count()==2,"finite emitter lifetime/ordinary-particle independence");
}
void normalized_columns() {
    Fixture f; Program p; p.hold(); f.add(p,{2,3,4}); auto& s=f.bank.scripts[0]; s.parameters[7]=-1; s.parameters[5]=-1; s.parameters[6]=-0.5f;
    const std::array banks{&f.bank}; isle_rng ar{123},br=ar; EffectRuntime a(banks,ar),b(banks,br); ok(a.status()); ok(b.status());
    const EffectAttachment attachment{animation::InstanceHandle{1,1,2},0}; a.create_effect(0,0,attachment); b.create_effect(0,0,attachment);
    auto normal=identity(),scaled=normal; scaled.matrix[0][0]=2; scaled.matrix[1][1]=3; scaled.matrix[2][2]=4;
    ok(a.update_effects(world,&normal)); ok(b.update_effects(world,&scaled));
    vector_near(particle(a).velocity,particle(b).velocity,"attachment velocity columns must normalize independently"); require(ar.seed==br.seed,"column scale changed RNG");
}
void emitter_rng_and_source_order() {
    for(unsigned kind : {0u,3u}) {
        Fixture f; Program p; p.hold(); f.add(p,{0,0,2});
        auto& s=f.bank.scripts[0]; s.kind=std::uint16_t(kind); s.parameters[5]=2; s.parameters[6]=0.5f;
        const std::array banks{&f.bank}; isle_rng rng{0x12345678},oracle=rng;
        isle_rng_next_float(&oracle); // source initial azimuth, subsequently replaced
        float fraction=isle_rng_next_float(&oracle); if(kind==3) fraction=std::sqrt(fraction);
        const float azimuth=isle_rng_next_float(&oracle)*6.2831855f;
        const float radius=2*fraction,radial=std::sin(fraction*0.5f)*2,axial=std::cos(fraction*0.5f)*2;
        Vec3 velocity{std::cos(azimuth)*radial,std::sin(azimuth)*radial,axial};
        if(kind==3) { velocity.x*=fraction; velocity.y*=fraction; velocity.z*=fraction; }
        EffectRuntime r(banks,rng); ok(r.status()); const auto e=r.create_effect(0,0); require(e.spawned,"kind0/3 emitter"); ok(r.update_effects());
        const auto v=particle(r); require(rng.seed==oracle.seed&&v.effect_id==e.effect_id,"initial/replaced azimuth draw order or emitter particle inherited ID");
        vector_near(v.velocity,velocity,"kind3 sqrt fraction and post-rotation velocity scale");
        vector_near(v.position,{std::cos(azimuth)*radius+velocity.x,std::sin(azimuth)*radius+velocity.y,velocity.z},"emitter source radius/velocity grouping");
    }
    {
        Fixture f; Program p; p.hold(); f.add(p); f.bank.scripts[0].parameters[7]=0.25f;
        const std::array banks{&f.bank}; isle_rng rng{1},oracle=rng; EffectRuntime r(banks,rng); ok(r.status());
        r.create_effect(0,0,{animation::InstanceHandle{1,1,2},0});
        for(unsigned k=0;k<3;++k) { isle_rng_next_float(&oracle); ok(r.update_effects(missing,nullptr)); }
        require(rng.seed==oracle.seed&&r.particle_count()==0,"nonemitting attachment resolved early or randomized rate draw changed");
    }
    {
        Fixture f; Program p; p.op(0xa5).u16(1).hold(); f.add(p); Program child; child.hold(); f.add(child);
        f.bank.scripts[0].effect_lifetime=1; f.bank.scripts[1].effect_lifetime=0;
        const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
        const auto e=r.create_effect(0,0); ok(r.update_effects());
        require(r.effect_count()==1&&r.particle_count()==1,"A5 head insertion lost when current emitter retired");
        ok(r.update_effects()); require(r.effect_count()==1&&r.particle_count()==2,"inherited emitter did not survive to next source update");
        require(particle(r).effect_id==e.effect_id&&r.create_effect(0,1).effect_id==3,"A5 inherited source ID versus allocation counter");
    }
}
void errors_and_packet_bounds() {
    Fixture f; Program p; p.hold(); f.add(p); const std::array banks{&f.bank}; isle_rng rng{1}; EffectRuntime r(banks,rng); ok(r.status());
    const auto missing_bank=r.create_particle(1,0),missing_script=r.create_effect(0,1);
    require(bool(missing_bank.status)&&!missing_bank.spawned&&bool(missing_script.status)&&!missing_script.spawned,"source missing bank/script must return successful null");
    require(!r.create_particle(128,0).status,"encoded list outside sixteen accepted");
    require(!r.create_particle(0,0,{std::numeric_limits<float>::infinity(),0,0},{}).status,"nonfinite creation accepted");
    require(rng.seed==1 && r.particle_count()==0 && r.effect_count()==0,"invalid creation mutated runtime"); spawn(r,0,0);
    std::array<std::uint32_t,10> frames{}; for(unsigned k=0;k<frames.size();++k) frames[k]=0x200000+512*k;
    const std::array<EffectImageBinding,1> images{{{frames,{}}}}; const std::array<EffectBankBinding,1> bound{{{0,images}}};
    const std::array<RamRange,1> reservations{{{0x200000,5120}}};
    EffectCamera camera{}; camera.projection[0][0]=camera.projection[1][1]=0.04f; camera.projection[2][2]=0.001f; camera.projection[3][3]=1;
    camera.viewport_scale={128,96,511}; camera.viewport_translate={128,96,511};
    std::array<std::uint32_t,EffectRuntime::max_render_words> words{}; words.fill(0xdeadbeef);
    auto result=r.render(camera,{std::span(words).first(2),bound,reservations}); require(result.status.error==EffectsError::extent && result.words>2,"short packet accepted");
    for(auto word:words) require(word==0xdeadbeef,"packet capacity failure wrote output");
    result=r.render(camera,{words,bound,{}}); require(result.status.error==EffectsError::extent,"unreserved frame accepted");
    for(auto word:words) require(word==0xdeadbeef,"asset preflight failure wrote packet");
    result=r.render(camera,{words,bound,reservations}); ok(result.status); require(result.particles==1,"authored visible particle packet");
    camera.particle_lists=0; result=r.render(camera,{words,bound,reservations}); ok(result.status); require(result.particles==0,"render list mask ignored");
    const std::array duplicate{&f.bank,&f.bank}; EffectRuntime bad(duplicate,rng); require(!bad.status(),"duplicate active resource slot accepted");
    Fixture invalid; Program q; q.op(0xe0); invalid.add(q); const std::array invalid_banks{&invalid.bank}; EffectRuntime unsupported(invalid_banks,rng); require(!unsupported.status(),"unsupported opcode admission");
    invalid.bank.scripts[0].instructions[0].size=2; EffectRuntime escaped(invalid_banks,rng); require(!escaped.status(),"instruction escaped bytecode");
}
}
int main() {
    try {
        masks_and_physics(); interpolation_and_waits(); control_and_retirement(); rotation_a9(); random_operations(); flags();
        children_and_lists(); child_emitters_and_sibling_order(); color_masks_and_extended_duration(); operand_bounds();
        regular_child_traversal(); source_id_wrap(); emitter_rng_and_source_order();
        target_local_services(); emitters_pools_and_retirement(); normalized_columns(); errors_and_packet_bounds();
        std::cout << "shared_effect_authored_cpu=passed Original_parity=unqualified\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
