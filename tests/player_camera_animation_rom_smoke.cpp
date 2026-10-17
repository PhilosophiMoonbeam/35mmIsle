// Opt-in private-ROM native-camera consumer; no CTest/default/game entrypoint.
// The sole currently admitted camera is the title clip, not course-camera
// grammar qualification or the complete production Player. No GPU/device work.
#include "player_camera_animation.hpp"
#include "scene_storage.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
namespace m = isle::menu;
namespace a = m::animation;
namespace c = isle::player::camera_animation;
constexpr std::uint16_t real_mask = 0x277; // XYZ eye, XYZ at, FOV; never roll/path.
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void native(IsleGameResult result) { require(result == ISLE_GAME_OK,"native camera operation rejected"); }
void accepted(c::Status status) {
    if (!status) {
        std::cerr << "animation_error=" << static_cast<unsigned>(status.animation.error)
                  << " address=" << std::hex << status.animation.address << std::dec
                  << " native_error=" << static_cast<unsigned>(status.native) << '\n';
        throw std::runtime_error("native camera Driver operation rejected");
    }
}
bool same(float x, float y) { return std::bit_cast<std::uint32_t>(x) == std::bit_cast<std::uint32_t>(y); }
bool same(const a::CameraState& x, const a::CameraState& y) {
    return same(x.eye.x,y.eye.x) && same(x.eye.y,y.eye.y) && same(x.eye.z,y.eye.z) &&
        same(x.at.x,y.at.x) && same(x.at.y,y.at.y) && same(x.at.z,y.at.z) &&
        same(x.roll,y.roll) && same(x.fov_y,y.fov_y) && x.view_dirty == y.view_dirty &&
        x.projection_dirty == y.projection_dirty && x.written_channels == y.written_channels &&
        x.clock.phase == y.clock.phase && same(x.clock.time_left,y.clock.time_left) &&
        same(x.clock.time_passed,y.clock.time_passed) && same(x.clock.speed,y.clock.speed);
}
struct NativeValue { IsleGameCameraParameters parameters; float roll; };
NativeValue read(IsleGame* game, IsleGameCamera* camera) {
    const IsleGameCameraParameters* borrowed = nullptr;
    native(isle_game_camera_parameters(game,camera,&borrowed));
    NativeValue result{*borrowed,0}; // Never retain a native parameter borrow.
    native(isle_game_camera_roll(game,camera,&result.roll));
    return result;
}
bool same(const NativeValue& x, const NativeValue& y) {
    const auto& p=x.parameters; const auto& q=y.parameters;
    for (unsigned i=0;i<3;++i)
        if (!same(p.look_at.eye[i],q.look_at.eye[i]) || !same(p.look_at.at[i],q.look_at.at[i]) ||
            !same(p.look_at.up[i],q.look_at.up[i])) return false;
    for (unsigned i=0;i<4;++i)
        if (p.viewport.scale[i]!=q.viewport.scale[i] || p.viewport.translate[i]!=q.viewport.translate[i]) return false;
    return same(p.perspective.fovy,q.perspective.fovy) && same(p.perspective.aspect,q.perspective.aspect) &&
        same(p.perspective.near_plane,q.perspective.near_plane) &&
        same(p.perspective.far_plane,q.perspective.far_plane) && same(p.perspective.scale,q.perspective.scale) &&
        same(x.roll,y.roll);
}
void preserve_nonwritten(const NativeValue& value, const NativeValue& external) {
    auto merged=value;
    for (unsigned i=0;i<3;++i) {
        merged.parameters.look_at.eye[i]=external.parameters.look_at.eye[i];
        merged.parameters.look_at.at[i]=external.parameters.look_at.at[i];
    }
    merged.parameters.perspective.fovy=external.parameters.perspective.fovy;
    require(same(merged,external),"animator overwrote nonwritten native roll/up/projection/viewport fields");
}
void install(IsleGame* game, IsleGameCamera* camera, const NativeValue& value) {
    native(isle_game_camera_set_look_at(game,camera,&value.parameters.look_at));
    native(isle_game_camera_set_perspective(game,camera,&value.parameters.perspective));
    native(isle_game_camera_set_viewport(game,camera,&value.parameters.viewport));
    native(isle_game_camera_set_roll(game,camera,value.roll));
}
std::uint32_t object_flags(IsleGame* game, IsleGameObject* object) {
    IsleGameObjectMetadata metadata{}; native(isle_game_object_metadata(game,object,&metadata));
    return metadata.flags;
}

// These are two closed-form samples of independently inspected actual ROM
// segments, NOT another command interpreter, Runtime or pose cache. All scalar
// operands come from the admitted Catalog; no private expected pose payload is
// embedded. Shape checks identify precisely the source segments used below.
struct Oracle {
    const m::Animation& program;
    const m::AnimationCommand& command(std::uint32_t address, unsigned opcode,
                                      unsigned mask, unsigned duration) const {
        for (const auto& q:program.commands) if (q.wire.guest_address==address) {
            require(q.opcode==opcode && q.mask==mask && q.duration==duration,
                    "real camera sample segment differs from independently inspected ROM metadata");
            return q;
        }
        throw std::runtime_error("actual camera segment address missing from admitted clip");
    }
    float operand(std::uint32_t address, unsigned opcode, unsigned mask, unsigned duration,
                  unsigned index=0) const {
        return std::bit_cast<float>(command(address,opcode,mask,duration).wire.u32(4+4*index));
    }
    static float cubic(float initial, float target, float initial_rate, float target_rate,
                       double duration, double time) {
        const double u=time/duration, u2=u*u, u3=u2*u;
        return static_cast<float>((2*u3-3*u2+1)*initial + (u3-2*u2+u)*duration*initial_rate +
                                  (-2*u3+3*u2)*target + (u3-u2)*duration*target_rate);
    }
    std::array<float,7> first(double time) const {
        require(time>=0 && time<345,"first-interval oracle used outside inspected real segment");
        return {operand(0x803a8114,3,1,0),operand(0x803a810c,3,2,0),operand(0x803a8104,3,4,0),
            cubic(operand(0x803a80d0,5,0x10,0),operand(0x803a80dc,6,0x10,346),
                  operand(0x803a80d0,5,0x10,0,1),operand(0x803a80dc,6,0x10,346,1),346,time),
            operand(0x803a80fc,3,0x20,0),
            cubic(operand(0x803a80f0,5,0x40,0),operand(0x803a8144,5,0x40,345),
                  operand(0x803a80f0,5,0x40,0,1),operand(0x803a8144,5,0x40,345,1),345,time),
            operand(0x803a80e8,10,0x200,0)};
    }
    std::array<float,7> terminal() const {
        return {
            cubic(operand(0x803a8710,6,1,241),operand(0x803a8750,6,1,109),
                  operand(0x803a8710,6,1,241,1),operand(0x803a8750,6,1,109,1),109,108),
            operand(0x803a8748,4,2,109),
            cubic(operand(0x803a86f8,6,4,241),operand(0x803a873c,6,4,109),
                  operand(0x803a86f8,6,4,241,1),operand(0x803a873c,6,4,109,1),109,108),
            operand(0x803a871c,4,0x10,350),
            cubic(operand(0x803a8724,5,0x20,241),operand(0x803a8730,6,0x20,109),
                  operand(0x803a8724,5,0x20,241,1),operand(0x803a8730,6,0x20,109,1),109,108),
            cubic(operand(0x803a8690,10,0x40,1),operand(0x803a86e4,9,0x40,495),
                  operand(0x803a8698,7,0x40,0),0,495,494),
            operand(0x803a86dc,11,0x200,386)};
    }
    void inspect() const {
        require(program.wire.guest_address==0x803a80d0,"wrong admitted real camera identity");
        unsigned mask=0, total=0;
        for (const auto& q:program.commands) {
            require(q.opcode==0 || q.opcode==2 || (q.opcode>=3 && q.opcode<=11),
                    "real selected clip requires additional camera grammar; cannot silently ignore it");
            if (q.opcode>=3 && q.opcode<=11) mask|=q.mask;
            if (q.opcode==2 || q.opcode==3 || q.opcode==5 || q.opcode==8 || q.opcode==10) total+=q.duration;
        }
        require(mask==real_mask && total==1860,"real camera program mask/terminal source duration changed");
        command(0x803a80dc,6,0x10,346);
        command(0x803a811c,4,0x20,345); command(0x803a8124,4,4,345);
        command(0x803a812c,4,2,345); command(0x803a8134,4,1,345);
        command(0x803a813c,11,0x200,346); command(0x803a875c,2,0,108);
        command(0x803a8760,0,0,0);
        (void)first(0); (void)terminal();
    }
};
void sample(const NativeValue& value, const std::array<float,7>& expected) {
    const auto& p=value.parameters;
    const std::array actual{p.look_at.eye[0],p.look_at.eye[1],p.look_at.eye[2],
                            p.look_at.at[0],p.look_at.at[1],p.look_at.at[2],p.perspective.fovy};
    for (unsigned i=0;i<actual.size();++i)
        require(std::isfinite(actual[i]) && std::abs(actual[i]-expected[i])<=0.001f,
                "actual native camera differs from independent real-segment analytic sample");
}
void matrices(IsleGame* game, IsleGameCamera* camera) {
    native(isle_game_camera_compute_all(game,camera));
    for (unsigned i=0;i<2;++i) {
        const IsleGameCameraMatrix* matrix=nullptr;
        native(isle_game_camera_matrix(game,camera,i,&matrix));
        require(matrix->kind==(i==0?ISLE_GAME_CAMERA_PERSPECTIVE_FAST:ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL) &&
                matrix->revision>0,"native camera did not compute authoritative matrices 3+14");
        for (const auto& row:matrix->floating) for (float x:row)
            require(std::isfinite(x),"real trajectory published nonfinite native matrix");
    }
    const IsleGameCameraReflection* reflection=nullptr;
    native(isle_game_camera_reflection(game,camera,1,&reflection));
    require(reflection->revision>0,"native kind14 reflection was not computed");
}
void record(std::ostream& report, const char* stage, std::size_t index, const a::CameraState& state,
            const NativeValue& value) {
    const auto& p=value.parameters;
    report << stage << '\t' << index << '\t' << static_cast<unsigned>(state.clock.phase) << '\t'
           << state.clock.time_passed << '\t' << state.clock.time_left << '\t' << state.clock.speed
           << '\t' << state.written_channels;
    for (float x:p.look_at.eye) report << '\t' << x;
    for (float x:p.look_at.at) report << '\t' << x;
    for (float x:p.look_at.up) report << '\t' << x;
    report << '\t' << value.roll << '\t' << p.perspective.fovy << '\t' << p.perspective.aspect
           << '\t' << p.perspective.near_plane << '\t' << p.perspective.far_plane << '\t' << p.perspective.scale;
    for (auto x:p.viewport.scale) report << '\t' << x;
    for (auto x:p.viewport.translate) report << '\t' << x;
    report << '\n'; require(bool(report),"native camera observation report write failed");
}
void rejection(c::NativeDriver& driver, IsleGame* game, IsleGameCamera* camera,
               const IslePlayerCameraAnimationClip* clip, float skip, a::Error error) {
    const auto before=driver.state(); const auto native_before=read(game,camera);
    const auto status=driver.set(clip,skip);
    require(status.animation.error==error && status.native==ISLE_GAME_OK &&
            same(before,driver.state()) && same(native_before,read(game,camera)),
            "typed clip rejection changed native values/track clocks or wrong error domain");
    native(isle_game_status(game)); // A recoverable diagnostic is not a native fault.
}
void run(const std::filesystem::path& rom, const std::filesystem::path& output) {
    require(output.is_absolute() && !std::filesystem::exists(output),
            "NEW_OUTPUT_DIRECTORY must be absolute and nonexistent");
    require(std::filesystem::is_directory(output.parent_path()),"output parent must already exist");
    const auto snapshot=isle::rom::read_snapshot(rom);
    auto hydrated=m::hydrate(snapshot); require(bool(hydrated),"actual admitted menu Catalog hydration rejected");
    const auto& catalog=*hydrated.catalog;
    const auto index=catalog.camera_animation();
    require(index<catalog.animations().size(),"admitted camera clip index escaped Catalog");
    const Oracle oracle{catalog.animations()[index]}; oracle.inspect();
    require(std::filesystem::create_directory(output),"fresh output creation failed");
    const auto profile=output/"profile";
    require(profile.is_absolute() && !std::filesystem::exists(profile) &&
            std::filesystem::create_directory(profile),"fresh private profile directory creation failed");
    std::ofstream report(output/"native-camera.tsv"); require(bool(report),"native camera report open failed");
    report << std::setprecision(std::numeric_limits<float>::max_digits10)
           << "# resource=menu_catalog camera_guest=0x803a80d0 clip_index=" << index
           << " program_mask=0x277 terminal_source_frame=1860 matrices=3,14\n"
           << "# replacement=real_NULL_same_precompiled_real_new_skip no_second_admitted_clip\n"
           << "# profile_reserved_empty_no_application_State native_camera_only_no_GPU_audio_device\n"
           << "stage\tclip_index\tphase\ttime_passed\ttime_left\tspeed\twritten_mask"
              "\teye_x\teye_y\teye_z\tat_x\tat_y\tat_z\tup_x\tup_y\tup_z\troll\tfovy\taspect"
              "\tnear\tfar\tprojection_scale\tviewport_s0\tviewport_s1\tviewport_s2\tviewport_s3"
              "\tviewport_t0\tviewport_t1\tviewport_t2\tviewport_t3\n";
    isle::scene::Storage storage; native(storage.create()); auto* game=storage.game();
    IsleGameObject* object=nullptr; native(isle_game_add_object(game,7,9,9,nullptr,nullptr,&object));
    IsleGameCamera* camera=nullptr; native(isle_game_attach_camera(game,object,&camera));
    native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_PERSPECTIVE_FAST));
    native(isle_game_camera_add_matrix(game,camera,ISLE_GAME_CAMERA_LOOK_AT_REFLECT_ROLL));
    NativeValue external{{{63,1.625f,19,43000,0.75f},
        {{720,310,900},{-100,70,50},{0.125f,1,0.0625f}},
        {{580,420,500,0},{612,452,515,0}}},0.375f};
    install(game,camera,external); matrices(game,camera);
    // Borrow the full immutable existing pool once. No replacement/rewritten
    // clip is manufactured; Runtime precompiles every program in this pool.
    const a::Resources resources{catalog.models(),catalog.materials(),catalog.animations(),m::no_index,{}};
    auto made=c::NativeDriver::create(game,camera,resources,{});
    if (!made) { accepted(made.status); throw std::runtime_error("Driver creation returned no owner"); }
    auto& driver=*made.driver;
    require(driver.state().clock.phase==a::Phase::disabled,"Driver construction implicitly selected/ticked a clip");
    IslePlayerCameraAnimationClip clip{}; accepted(driver.clip(index,clip));
    const auto initial=read(game,camera);
    accepted(driver.set(&clip,0));
    require(driver.state().clock.phase==a::Phase::changed && driver.state().clock.time_passed==0 &&
            same(initial,read(game,camera)),"source Set evaluated native camera or failed CHANGED/skip publication");
    accepted(driver.update(object_flags(game,object))); // Exactly one explicit source update.
    require(driver.state().clock.phase==a::Phase::running && driver.state().clock.time_passed==0 &&
            driver.state().clock.time_left==345 && driver.state().written_channels==real_mask,
            "source Set+oneUpdate advanced more than time0 or wrote the wrong real channel mask");
    sample(read(game,camera),oracle.first(0)); preserve_nonwritten(read(game,camera),external);
    record(report,"set_one_update",index,driver.state(),read(game,camera));
    const auto after_one=driver.state(); const auto native_one=read(game,camera);
    require(same(after_one,driver.state()) && same(native_one,read(game,camera)),"camera observation automatically ticks");
    accepted(driver.update(object_flags(game,object)));
    sample(read(game,camera),oracle.first(0.5));
    const auto before_speed=driver.state(); const auto native_speed=read(game,camera);
    accepted(driver.set_speed(1.25f));
    auto expected_speed=before_speed; expected_speed.clock.speed=1.25f;
    require(same(expected_speed,driver.state()) && same(native_speed,read(game,camera)),
            "set_speed reset track/clock/field state or evaluated a clip");
    accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.time_passed==1.75f && driver.state().clock.time_left==343.25f,
            "set_speed failed to preserve the running source cursor");
    sample(read(game,camera),oracle.first(1.75)); record(report,"speed_preserves_tracks",index,driver.state(),read(game,camera));

    // Intervening native changes intentionally alter written AND nonwritten
    // fields. A fresh merge must restore real track outputs, not a stale pose.
    external.parameters.look_at={{950,440,1100},{50,20,-100},{-0.25f,1,0.125f}};
    external.parameters.perspective={71,1.375f,23,39000,0.875f};
    external.parameters.viewport={{540,400,490,0},{610,450,505,0}}; external.roll=-0.625f;
    install(game,camera,external);
    native(isle_game_object_set_flags(game,object,2u|0x40u));
    accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.time_passed==3 && driver.state().written_channels==0 &&
            same(external,read(game,camera)),"actual native object flag2 did not advance tracks while suppressing all writes");
    record(report,"actual_object_flag2",index,driver.state(),read(game,camera));
    native(isle_game_object_set_flags(game,object,0x40u));
    accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.time_passed==4.25f && driver.state().written_channels==real_mask,
            "cleared actual native flag2 did not resume the advanced track state");
    sample(read(game,camera),oracle.first(4.25)); preserve_nonwritten(read(game,camera),external);
    matrices(game,camera); record(report,"fresh_native_merge",index,driver.state(),read(game,camera));

    IslePlayerCameraAnimationClip untouched{91,92};
    auto status=driver.clip(resources.animations.size(),untouched);
    require(status.animation.error==a::Error::handle && status.native==ISLE_GAME_OK &&
            untouched.owner==91 && untouched.index==92,"out-of-range clip admission published output");
    status=driver.clip(0,untouched);
    require(resources.animations[0].kind!=m::AnimationKind::camera && status.animation.error==a::Error::handle &&
            status.native==ISLE_GAME_OK && untouched.owner==91 && untouched.index==92,"non-camera index admitted as typed camera clip");
    auto invalid=clip; invalid.index=resources.animations.size();
    rejection(driver,game,camera,&invalid,0,a::Error::handle);
    invalid=clip; invalid.owner=0; rejection(driver,game,camera,&invalid,0,a::Error::handle);
    invalid=clip; invalid.index=0; rejection(driver,game,camera,&invalid,0,a::Error::handle);
    rejection(driver,game,camera,&clip,std::numeric_limits<float>::quiet_NaN(),a::Error::malformed);
    const auto finite_speed=driver.state();
    status=driver.set_speed(std::numeric_limits<float>::infinity());
    require(status.animation.error==a::Error::malformed && status.native==ISLE_GAME_OK &&
            same(finite_speed,driver.state()),"nonfinite speed rejection mutated source state");
    const auto owned_state=driver.state(); const auto owned_native=read(game,camera);
    c::Status wrong_thread;
    std::thread other([&] { wrong_thread=driver.update(0); }); other.join();
    require(wrong_thread.native==ISLE_GAME_WRONG_OWNER && wrong_thread.animation.error==a::Error::none &&
            same(owned_state,driver.state()) && same(owned_native,read(game,camera)),
            "native wrong-owner rejection advanced source tracks or conflated error domains");
    native(isle_game_status(game));

    const auto before_null=read(game,camera); accepted(driver.set(nullptr,7));
    require(driver.state().clock.phase==a::Phase::changed && driver.state().clock.time_passed==7 &&
            same(before_null,read(game,camera)),"source NULL Set did not wait for its explicit terminal Update");
    accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.phase==a::Phase::disabled && driver.state().clock.time_passed==-7 &&
            driver.state().written_channels==0 && driver.state().clock.speed==1.25f &&
            same(before_null,read(game,camera)),"NULL terminal semantics lost source skip/speed or leaked old tracks");
    auto null_state=driver.state();
    // Per-update dirty metadata clears even though the terminal tracks/clocks
    // and every native parameter must remain unchanged.
    null_state.view_dirty=null_state.projection_dirty=false;
    accepted(driver.update(object_flags(game,object)));
    require(same(null_state,driver.state()) && same(before_null,read(game,camera)),"NULL terminal ran twice");
    record(report,"NULL_terminal",index,driver.state(),read(game,camera));
    accepted(driver.set(&clip,172.5f));
    require(driver.state().clock.phase==a::Phase::changed && driver.state().clock.time_passed==172.5f &&
            driver.state().clock.speed==1.25f && same(before_null,read(game,camera)),
            "same precompiled clip replacement evaluated or lost new skip/retained speed");
    accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.time_passed==172.5f && driver.state().clock.time_left==172.5f &&
            driver.state().written_channels==real_mask,"replacement+oneUpdate applied skip more than once");
    sample(read(game,camera),oracle.first(172.5)); preserve_nonwritten(read(game,camera),external);
    record(report,"same_real_clip_new_skip",index,driver.state(),read(game,camera));

    accepted(driver.set_speed(0.5f)); accepted(driver.set(&clip,0));
    accepted(driver.update(object_flags(game,object))); sample(read(game,camera),oracle.first(0));
    for (unsigned update=1;update<=3720;++update) {
        accepted(driver.update(object_flags(game,object)));
        const auto observed=read(game,camera);
        require(driver.state().written_channels==real_mask &&
                driver.state().clock.phase==(update==3720?a::Phase::disabled:a::Phase::running),
                "finite actual ROM trajectory terminated early/late or lost its program channel mask");
        preserve_nonwritten(observed,external); matrices(game,camera);
        for (float x:observed.parameters.look_at.eye) require(std::isfinite(x),"nonfinite actual eye trajectory");
        for (float x:observed.parameters.look_at.at) require(std::isfinite(x),"nonfinite actual target trajectory");
        require(std::isfinite(observed.parameters.perspective.fovy),"nonfinite actual FOV trajectory");
        record(report,"finite_trajectory",index,driver.state(),observed);
    }
    sample(read(game,camera),oracle.terminal());
    require(driver.state().clock.time_passed==0 && driver.state().clock.time_left==0,
            "real END did not publish source terminal residual time");
    record(report,"real_END_analytic_sample",index,driver.state(),read(game,camera));
    const auto terminal=read(game,camera); accepted(driver.update(object_flags(game,object)));
    require(driver.state().clock.phase==a::Phase::disabled && driver.state().written_channels==0 &&
            same(terminal,read(game,camera)),"finite real END republished track writes");
    driver.retire();
    rejection(driver,game,camera,&clip,0,a::Error::retired);
    status=driver.clip(index,untouched);
    require(status.animation.error==a::Error::retired && status.native==ISLE_GAME_OK &&
            untouched.owner==91 && untouched.index==92,"retired typed admission published a handle");
    status=driver.update(object_flags(game,object));
    require(status.animation.error==a::Error::retired && status.native==ISLE_GAME_OK,"retired update touched native camera");
    status=driver.set_speed(1);
    require(status.animation.error==a::Error::retired && status.native==ISLE_GAME_OK,"retired speed setter accepted");
    made.driver.reset();

    // Sequential Driver owners on ONE live NativeGame. An actually issued old
    // owner/index token must never authorize the next precompiled owner.
    auto next=c::NativeDriver::create(game,camera,resources,{});
    if (!next) { accepted(next.status); throw std::runtime_error("sequential Driver creation returned no owner"); }
    IslePlayerCameraAnimationClip current{}; accepted(next.driver->clip(index,current));
    require(current.owner!=clip.owner && current.index==clip.index,"typed camera owner token reused");
    rejection(*next.driver,game,camera,&clip,0,a::Error::handle);
    accepted(next.driver->set(&current,37.5f)); accepted(next.driver->update(object_flags(game,object)));
    sample(read(game,camera),oracle.first(37.5)); preserve_nonwritten(read(game,camera),external);
    record(report,"sequential_owner_old_clip_rejected",index,next.driver->state(),read(game,camera));
    next.driver->retire(); next.driver.reset(); // Resource borrows end before Catalog/Snapshot.
    native(isle_game_delete_object(game,object)); // No callbacks, processes, GPU or identity holds were installed.
    IsleGameCounts counts{}; native(isle_game_counts(game,&counts));
    require(counts.objects==0 && counts.processes==0 && counts.cameras==0 && counts.matrices==0,
            "actual native camera/object retirement retained live parties");
    native(isle_game_status(game)); native(storage.destroy());
    require(std::filesystem::is_empty(profile),"camera-only consumer unexpectedly touched application profile");
    report << "# passed=source_set_one_update speed_tracks flag2 fresh_merge typed_errors NULL replacement finite_END sequential_owners joined_native_teardown\n"
              "# boundary=sole_admitted_title_clip_not_all_course_camera_grammar_not_complete_Player_not_fullgame\n";
    report.close(); require(bool(report),"native camera report close failed");
    std::cout << "native-camera-ROM consumer passed clip_index=" << index
              << " program_mask=0x277 source_END=1860 matrices=3,14 report=" << (output/"native-camera.tsv")
              << " boundary=sole_title_clip_not_all_course_grammar_not_complete_Player_not_fullgame\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc==3,"usage: player-camera-animation-rom-smoke PRIVATE_ROM NEW_OUTPUT_DIRECTORY");
        run(argv[1],argv[2]); return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "player-camera-animation-rom-smoke: " << error.what() << '\n'; return EXIT_FAILURE;
    }
}
