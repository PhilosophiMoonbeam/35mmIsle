// Explicit private-ROM consumer of the actual application Runtime. Never a
// ROM-free CTest, installed executable, Source-text test or alternate game loop.
// Original Manager results alone select entries; owned input never injects routes.
// The diagnostic-disabled Host supplies NO controller; SoftwareController below
// is a separate, explicitly owned acquisition through the real Input Service.
// CLI modes: ingress, course, native0..native6, menus, credits17..credits19.
// Course continues through original Beach/CameraCheck9/PhotoCheck15 and the
// constructed Manager Oak13 owner, not a rendered/interactable return Oak.
// NativeN authors an unlocked profile; real Oak selects N and actual Source
// natural course result9 must join before actual Manager CameraCheck9 admission.
// Exit0 requires the entire selected scenario; exit1 is any failure.
// Each run predeclares bounded artifacts in its new owned proof root.
#include "application_runtime.hpp"
#include "camera_check_source.h"
#include "game_host.hpp"
#include "host_progress.hpp"
#include "new_game_scene.hpp"
#include "oaks_lab_scene.hpp"
#include "player_source.h"
#include "post_course_scene.hpp"
#include "post_course_window.hpp"
#include "player_photo_source.h"
#include "player_photo.hpp"
#include "player_ui_source.h"
#include "startup_runtime.hpp"
#include "post_course_probe_inputs.hpp"
#include "post_course_scene_probe.hpp"
#include "native_course_probe.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace app = isle::application;
namespace g = isle::game_host;
namespace h = isle::host;
namespace in = isle::game_input;
namespace startup = isle::startup;
using Time = std::chrono::steady_clock;
constexpr std::uint16_t button_a = 0x8000, button_b = 0x4000, button_z = 0x2000, button_start = 0x1000;
enum class Mode { ingress, course, native_course, menus, credits17, credits18, credits19 };
bool menu_mode(Mode mode) noexcept { return mode>=Mode::menus; }
std::int32_t credit_argument(Mode mode) noexcept {
    return mode==Mode::credits17?17:mode==Mode::credits18?18:mode==Mode::credits19?19:0;
}
std::uint32_t menu_target(Mode mode) noexcept {
    const auto credit=credit_argument(mode);
    // End24/Gallery23 and Credits20 have no discovered producer in the pinned
    // Original Root; their separate APIs are not fabricated menu selections.
    return credit?std::uint32_t{1}<<(credit-12):0x7u;
}
constexpr std::array<const char*,3> course_captures{{
    "actual-beach-tutorial-window.ppm", "actual-beach-course-window.ppm", "actual-beach-outro-window.ppm"}};
constexpr std::array<const char*,4> post_captures{{
    "actual-camera-check-window.ppm","actual-camera-marked-window.ppm",
    "actual-photo-assessment-window.ppm","actual-photo-report-window.ppm"}};
constexpr std::size_t scene_count = 5;
constexpr std::array<const char*,scene_count> scene_names{{"Boot","Title","Name","Oak","Beach"}};
constexpr std::array<const char*,6> captures{{
    "actual-logo-window.ppm", "actual-title-window.ppm", "actual-menu-window.ppm",
    "actual-name-window.ppm", "actual-oak-window.ppm", "actual-beach-window.ppm"}};
constexpr std::array<IsleApplicationEntryOperation,scene_count> operations{{
    ISLE_APPLICATION_ENTRY_BOOT, ISLE_APPLICATION_ENTRY_TITLE, ISLE_APPLICATION_ENTRY_NAME,
    ISLE_APPLICATION_ENTRY_OAKS_LAB, ISLE_APPLICATION_ENTRY_BEACH}};
void require(bool value,const char* reason) { if (!value) throw std::runtime_error(reason); }
struct SoftwareController {
    std::uint16_t buttons = 0;
    std::int8_t stick_x = 0,stick_y = 0;
    in::HostFrame sample() const noexcept {
        in::HostFrame result{};
        result[0] = {in::Connection::present,
            static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this)),0,buttons,stick_x,stick_y};
        return result;
    }
};
struct CourseEvidence {
    std::array<bool,course_captures.size()> captured{};
    std::array<std::uint64_t,5> phases{}; // Actual intro/tutorial/travel/outro/retirement observations.
    std::uint64_t source_rows=0,shutter_presses=0,movement_changes=0,stick_updates=0;
    int photos=0,qualifying_photos=0;
    std::uint64_t drive_calls=0,gate_running=0,gate_intro=0,gate_controls=0,gate_outro=0;
    std::uint64_t focus_present=0,focus_subject=0,focus_qualified=0,zoom_ready=0,shutter_ready=0;
    int last_direction=0,last_zoom_target=0,last_cooldown=0,last_input_disabled=0,last_controls=0,last_focus_id=0;
    std::uint32_t last_source_buttons=0;
    bool tutorial_seen=false,tutorial_input_released=false;
    bool tutorial_followup_seen=false,tutorial_followup_cleared=false,tutorial_stick_after_message=false;
    bool intro_seen=false,intro_finished=false,outro_seen=false,retiring_seen=false;
    bool ending_cart_wait_seen=false,ending_cart_terminal_seen=false,ending_sound_seen=false;
    bool post_scene_verified=false,qualified=false;
    std::uint64_t next_serial=0;
    IsleApplicationEntryOperation next_operation=ISLE_APPLICATION_ENTRY_BOOT;
    std::uint32_t next_argument=0;
    std::uint64_t retirement_callbacks=0,retired_entry_serial=0;
    std::int32_t retired_input_scene=0,retired_output_scene=0,retired_result=0,retired_end_reason=0;
    bool retired_player_detached=false,retired_music_detached=false,retired_graphics_closed=false;
    bool retired_source_domain_closed=false,retired_ending_closed=false;
};
struct PostEvidence {
    std::array<bool,4> captured{};
    std::array<std::uint64_t,2> serial{},updates{},frames{},presentations{},retirements{};
    std::array<std::int32_t,2> results{};
    std::array<bool,2> joined{};
    std::uint64_t rows=0,oak_serial=0;
    int marked=0,scored=0;
    std::uint32_t assessment_mask=0;
    int report_before=0,report_after=0;
    std::int64_t score_before=0,score_after=0;
    int stage=0,camera_e4=-1,camera_e8=-1,camera_ec=-1,camera_f0=0,slot=-1,live_marked=0;
    int photo_cursor=-1,marked_roll=-1,marked_species=0,selected_roll=-1,selected_species=0,report_slot=-1;
    std::uint64_t marked_revision=0,selected_revision=0,marked_digest=0,selected_digest=0,report_digest=0;
    std::uint32_t selected_score=0;
    unsigned assessment_progress=0;
    int phase=4,skip=0,scene_status=0,source_status=0,source_current=-1,source_pressed=-1;
    int summary_width=0,summary_height=0;
    int photo_x=0,photo_y=0,photo_attr=-1;
    std::uint64_t summary_serial=0,initial_element_serial=0,last_retrace=0;
    std::array<std::uint64_t,4> ready_update{},ready_retrace{},frame_floor{},capture_frame{},capture_presentation{};
    std::array<std::uint64_t,4> ready_ack_serial{},ready_ack_polls{};
    std::array<int,4> ready_ack_line{};
    std::uint64_t intro_ack_serial=0,save_ack_serial=0,final_ack_serial=0;
    bool save_ack_requested=false,save_ack_consumed=false,save_ack_released=false;
    bool intro_prompt_seen=false,intro_presented=false,intro_ack_requested=false,intro_ack_consumed=false;
    bool retired_element_seen=false;
    bool filter_seen=false,filter_unwound=false,menu_reset=false,menu_refreshed=false,confirmation_seen=false;
    bool selected_identity=false,report_identity=false,summary_seen=false,summary_visible=false;
    std::uint16_t requested_buttons=0,published_buttons=0,published_pad=0,published_released=0;
    std::int8_t requested_y=0,published_y=0;
    unsigned pulse=0;
    bool returned=false;
};
struct Evidence {
    Mode mode=Mode::ingress;
    CourseEvidence course;
    PostEvidence post;
    isle::private_post_course::Probe* menu_probe=nullptr;
    std::array<char,512> menu_failure{};
    std::uint64_t menu_entries=0,menu_updates=0,menu_frames=0,menu_presentations=0;
    std::uint32_t menu_qualified=0;
    bool authored_menu_inputs=false;
    isle::private_course::Probe* course_probe=nullptr;
    std::int32_t requested_course=0;
    std::uint64_t course_updates=0,course_frames=0,course_presentations=0;
    bool authored_course_profile=false,course_api_completed=false;
    std::array<std::uint64_t,scene_count> updates{},frames{},presentations{};
    std::array<bool,6> captured{};
    std::uint64_t entries = 0,read_requests = 0,read_completions = 0,read_skips = 0,publication_acknowledgements = 0;
    std::uint64_t released_frames = 0,declined_frames = 0,coalesced_ticks = 0,dropped_retraces = 0;
    std::uint64_t input_hold_ticks = 0,input_hold_audio = 0,frame_hold_ticks = 0,frame_hold_audio = 0;
    std::uint64_t beach_camera_changes = 0,submissions = 0,terminals = 0,accepted_pcm = 0;
    bool disabled_metadata = false,cold_stop = false,blocked_claim = false,resumed_claim = false;
    bool boot_wait = false,boot_complete = false,menu_result = false,name_confirming = false,name_confirmed = false;
    bool oak_result = false,frame_hold = false,stop_while_held = false,native_retired = false,input_stopped = false;
    bool audio_drained = false,host_released = false,main_music = false;
    bool native_retirement_deadline = false;
};
// Synchronous borrowed inspection after real native/Photo retirement and the
// original post-omSetupScene return. Never retain pointers or reenter Runtime.
void observe_retirement(void* user,app::SceneRetirementView view) noexcept {
    auto& evidence=*static_cast<Evidence*>(user);
    if (evidence.mode==Mode::native_course && evidence.course_probe) {
        try { evidence.course_probe->retired(view); }
        catch (const std::exception& error) {
            if (!evidence.menu_failure[0])
                std::snprintf(evidence.menu_failure.data(),evidence.menu_failure.size(),"%s",error.what());
        } catch (...) {
            if (!evidence.menu_failure[0])
                std::snprintf(evidence.menu_failure.data(),evidence.menu_failure.size(),"unknown borrowed course retirement failure");
        }
        return;
    }
    if (menu_mode(evidence.mode) && view.post && evidence.menu_probe) {
        try {
            evidence.menu_probe->retired(*view.post,view.source_result);
            evidence.menu_qualified=evidence.menu_probe->qualified_mask();
        } catch (const std::exception& error) {
            if (!evidence.menu_failure[0])
                std::snprintf(evidence.menu_failure.data(),evidence.menu_failure.size(),"%s",error.what());
        } catch (...) {
            if (!evidence.menu_failure[0])
                std::snprintf(evidence.menu_failure.data(),evidence.menu_failure.size(),"unknown borrowed menu retirement failure");
        }
        return;
    }
    if (evidence.mode==Mode::course && view.post) {
        const auto index=view.post->camera_source()?0u:1u;
        auto& p=evidence.post;
        ++p.retirements[index];
        const auto result=view.post->result();
        p.results[index]=view.source_result;
        p.joined[index]=view.entry && view.entry->serial==p.serial[index] &&
            view.entry->argument==(index==0?9u:15u) && result && *result==view.source_result &&
            view.post->status()==isle::post_course::SceneStatus::retired &&
            view.post->error()==ISLE_GAME_OK && !view.post->graphics_busy() &&
            !view.post->completed_frame() && !view.post->setup_pending() &&
            view.post->source().domain==nullptr && view.post->source().game==nullptr &&
            view.post->source().window==nullptr && view.post->source().replay==nullptr;
        return;
    }
    if (evidence.mode!=Mode::course || !view.beach) return;
    auto& c=evidence.course;
    ++c.retirement_callbacks;
    const auto& source=view.beach->source();
    const auto result=view.beach->result();
    c.retired_entry_serial=view.entry?view.entry->serial:0;
    c.retired_input_scene=source.input_scene;
    c.retired_output_scene=source.output_scene;
    c.retired_result=view.source_result;
    c.retired_end_reason=source.D_beach_802CC018;
    c.retired_player_detached=source.player==nullptr;
    c.retired_music_detached=source.music==nullptr;
    c.retired_graphics_closed=!view.beach->graphics_busy() && !view.beach->completed_frame() && !view.beach->pending_setup();
    c.retired_source_domain_closed=source.domain==nullptr && source.D_beach_802CC0E4==nullptr;
    c.retired_ending_closed=view.beach->ending_source()==nullptr;
    c.post_scene_verified=view.entry && view.entry->operation==ISLE_APPLICATION_ENTRY_BEACH && view.entry->argument==0 &&
        view.beach->status()==isle::beach::LifecycleStatus::retired && view.beach->error()==ISLE_GAME_OK &&
        source.status==ISLE_GAME_OK && source.completed && !source.running && source.scene_requested &&
        source.scene_completion_ready && source.scene_completion_status==ISLE_GAME_OK &&
        c.retired_player_detached && c.retired_music_detached && c.retired_graphics_closed &&
        c.retired_source_domain_closed && c.retired_ending_closed &&
        result && *result==view.source_result && source.output_scene==view.source_result;
}
void write_course_contract(const std::filesystem::path& root) {
    std::ofstream out(root/"proof"/"course-contract.txt");
    out << "invocation=PRIVATE_ROM NEW_OWNED_ROOT course\n"
        << "default_and_explicit_ingress=PRIVATE_ROM NEW_OWNED_ROOT [ingress]\n"
        << "exit0=qualified_selected_scenario_including_course_CameraCheck9_PhotoCheck15_constructed_Manager_Oak13_admission\n"
        << "exit1=any_failure_including_missing_Source_UI_report_or_join_evidence\n"
        << "next_owner=actual_typed_CameraCheck9_then_PhotoCheck15_result13_then_constructed_Manager_Oak_argument13_ONLY\n"
        << "full_application=bounded_Beach_post_course_Oak_admission_NOT_return_Oak_presentation_all_courses_or_hardware_parity\n"
        << "source_authority=ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c\n"
        << "gameplay=original_Runtime_Controller_Input_Audio_GTL_GPU_Photo_only\n"
        << "driving=readonly_original_Source_observations_to_owned_buttons_and_sticks_only\n"
        << "frame_hold=real_ingress_hold_released_without_cancelling_course\n"
        << "phase_indices=0_intro_1_tutorial_2_travel_3_outro_4_retirement\n"
        << "tutorial_evidence=actual_roll_qualifying3_native_input_release_original_followup_message_clear_actual_stick6_then_real_scene_process_join\n"
        << "IsTutorialEnabled=original_course_initialization_choice_NOT_a_tutorial_completion_marker\n"
        << "camera_driver=actual_mark_filtered_F0_positive_then0_outer_grid_B_until_E4_reset_minus1_refreshed_menu_slot22_actual_confirmation_A\n"
        << "post_input=owned_pulses_separated_by_observed_released_published_samples_Photo_A_ONLY_skip0\n"
        << "post_stage_indices=0_choose_1_marked_presentation_2_filter_unwind_3_outer_grid_4_menu_refresh_5_select_Oak22_6_await_confirmation_7_confirm_A_8_Photo_intro_9_assessment_hold_10_assess_A_11_summary_hold_12_Photo_finish_A_13_Oak13_admitted\n"
        << "post_pulse_indices=0_ready_1_await_actual_pressed_publication_2_await_actual_neutral_publication\n"
        << "post_identity=actual_owner_roll_pointer_slot_exact_canonical_nonflag_bytes_to_selected_scored_record_and_Report_species_score_flags_may_revise_record\n"
        << "photo_assessment=selected_marked_roll_identity_scored_then_phase0_visible_photo_completed_presentation_then_later_phase1_or2\n"
        << "photo_report=actual_Source_identity_and_new_216x72_summary_then_save_ack2197_real_A_consumed_released_final_values_guidance_ack2302_completed\n"
        << "post_screenshots=completed_actual_Host_presentations_of_later_canonical_frames_after_exact_completed_ack_waits_Camera1039_Photo2302_visual_inspection_required\n"
        << "photo_assessment_screenshot=truthful_early_phase0_assessment_NOT_completed_evaluation\n"
        << "post_input_diagnostics=published_Service_samples_and_readonly_actual_Photo_and_Camera_consumed_caches_NO_accessor_calls\n"
        << "outro_evidence=actual_ending_Source_cart_publication_native_animation_terminal0_to1_then_original_scene_return\n"
        << "post_scene_evidence=synchronous_borrow_after_native_Photo_GPU_scene_and_continuation_retirement_before_Manager_acknowledgement\n"
        << "predeclared_artifacts=application-runtime-report.txt,course-contract.txt,course-source.tsv,course-retirement.txt,course-next-entry.txt,post-course-source.tsv\n"
        << "post_ledger_bound=20000_rows_including_header_512_bytes_per_row_maximum\n"
        << "post_capture_bound=4_PPM_files_each_at_most_50331680_bytes_4096x4096_RGB_plus_header\n";
    for (const auto* name:captures) out << "original_ingress_capture=" << name << '\n';
    for (const auto* name:course_captures) out << "course_capture=" << name << '\n';
    for (const auto* name:post_captures) out << "post_course_capture=" << name << '\n';
    require(bool(out),"private course artifact/expected-boundary contract could not be predeclared");
}
void write_report(const std::filesystem::path& root,const Evidence& e,const std::string& failure) {
    std::ofstream out(root/"proof"/"application-runtime-report.txt");
    out << "actual_application_runtime=" << (!failure.empty()?"FAILED":"passed") << '\n'
        << "input=explicit_owned_software_NOT_Host_LUS_or_physical\n"
        << "audio=actual_native_RSP_checked_SDL_dummy_NOT_hardware_playback\n"
        << "graphics=actual_CPU_Vulkan_canonical_RGBA_copied_LUS_OpenGL\n"
        << "scene_selection=original_Manager_results_only\n"
        << "hardware_numerical_pixel_parity_full_game_Windows_runtime=not_claimed\n";
    if (!failure.empty()) out << "failure=" << failure << '\n';
    for (std::size_t n=0;n<scene_count;++n)
        out << scene_names[n] << " updates=" << e.updates[n] << " canonical_frames=" << e.frames[n]
            << " actual_LUS_presentations=" << e.presentations[n] << '\n';
    for (std::size_t n=0;n<captures.size();++n) out << captures[n] << " captured=" << e.captured[n] << '\n';
    out << "diagnostic_disabled_metadata=" << e.disabled_metadata << " immediate_stop_before_first_pump=" << e.cold_stop
        << " original_entries=" << e.entries << '\n'
        << "input_read_requests=" << e.read_requests << " real_read_completions=" << e.read_completions
        << " pending_read_retrace_skips=" << e.read_skips << " publication_acknowledgements=" << e.publication_acknowledgements << '\n'
        << "coalesced_actual_ticks=" << e.coalesced_ticks << " GTL_dropped_retraces=" << e.dropped_retraces
        << " actual_update_claim_blocked=" << e.blocked_claim << " actual_update_claim_resumed=" << e.resumed_claim << '\n'
        << "withheld_input_actual_retraces=" << e.input_hold_ticks << " accepted_PCM_during_input_hold=" << e.input_hold_audio << '\n'
        << "held_completed_frame=" << e.frame_hold << " held_frame_actual_retraces=" << e.frame_hold_ticks
        << " accepted_PCM_during_frame_hold=" << e.frame_hold_audio << " stop_while_frame_held=" << e.stop_while_held << '\n'
        << "released_actual_frames=" << e.released_frames << " optional_present_declines=" << e.declined_frames
        << " admitted_submissions=" << e.submissions << " retired_terminals=" << e.terminals << '\n'
        << "source_Boot_first_wait=" << e.boot_wait << " source_Boot_completed=" << e.boot_complete
        << " source_new_game_result0_notice1=" << e.menu_result << " source_Name_confirming=" << e.name_confirming
        << " source_Name_confirmed=" << e.name_confirmed
        << (menu_mode(e.mode)?" source_Oak_original_authored_menu_result=":" source_Oak_confirmed_course0=") << e.oak_result << '\n'
        << "native_Beach_camera_changes=" << e.beach_camera_changes << " original_main_music=" << e.main_music << '\n'
        << "native_GPU_Photo_Manager_retired=" << e.native_retired << " Input_Service_stopped=" << e.input_stopped
        << " SDL_audio_stream_drained=" << e.audio_drained << " Host_owners_released=" << e.host_released
        << " accepted_dummy_PCM_frames=" << e.accepted_pcm << '\n';
    out << "native_retirement_deadline=" << e.native_retirement_deadline << '\n';
    if (menu_mode(e.mode)) {
        out << "mode=" << (credit_argument(e.mode)?"credits":"menus")
            << " credits_argument=" << credit_argument(e.mode) << '\n'
            << "canonical_photo_scores_comments_profile_flags_and_notices=AUTHORED_API_inputs_NOT_captured_gameplay\n"
            << "actual_Manager_menu_entries=" << e.menu_entries << " actual_menu_updates=" << e.menu_updates
            << " actual_menu_canonical_frames=" << e.menu_frames << " actual_menu_LUS_presentations=" << e.menu_presentations << '\n'
            << "qualified_menu_mask=" << e.menu_qualified << " requested_Root_reachable_mask=" << menu_target(e.mode) << '\n'
            << "Root_reachability_End24_Gallery23_Credits20=NOT_claimed_no_discovered_Original_producer\n";
        if (e.menu_failure[0]) out << "actual_borrowed_menu_failure=" << e.menu_failure.data() << '\n';
    }
    if (e.mode==Mode::native_course) {
        out<<"mode=native_course requested_course="<<e.requested_course
            <<"\ncanonical_unlocked_profile=AUTHORED_API_input_NOT_natural_progression"
            <<"\nactual_Source_natural_course_result9_and_Manager_CameraCheck9="<<e.course_api_completed
            <<"\nactual_course_updates="<<e.course_updates<<" canonical_course_frames="<<e.course_frames
            <<" actual_course_LUS_presentations="<<e.course_presentations<<'\n';
        if(e.menu_failure[0])out<<"actual_borrowed_course_failure="<<e.menu_failure.data()<<'\n';
    }
    if (e.mode==Mode::course) {
        const auto& c=e.course;
        out << "mode=course\n"
            << "qualification=" << (c.qualified && failure.empty()?"completed_Beach_CameraCheck_PhotoCheck_constructed_Manager_Oak13_admission":"FAILED") << '\n'
            << "full_application=bounded_course_post_course_Oak13_admission_only_NOT_return_Oak_rendering\n"
            << "course_exit_contract=0_only_for_complete_original_course_post_course_and_teardown_other_failures_1\n"
            << "source_rows=" << c.source_rows << " shutter_button_presses=" << c.shutter_presses
            << " source_roll_photos=" << c.photos << " source_qualifying_photos=" << c.qualifying_photos
            << " native_movement_changes=" << c.movement_changes << " actual_stick_updates=" << c.stick_updates << '\n'
            << "source_intro_seen=" << c.intro_seen << " source_intro_finished=" << c.intro_finished
            << " source_tutorial_seen=" << c.tutorial_seen << " source_tutorial_input_released=" << c.tutorial_input_released
            << " source_tutorial_followup_message=" << c.tutorial_followup_seen
            << " source_tutorial_followup_message_cleared=" << c.tutorial_followup_cleared
            << " source_tutorial_stick_after_message=" << c.tutorial_stick_after_message << " source_outro_seen=" << c.outro_seen
            << " source_retiring_seen=" << c.retiring_seen << " original_post_scene_verified=" << c.post_scene_verified << '\n'
            << "original_ending_sound_observed=" << c.ending_sound_seen
            << " original_outro_cart_wait_observed=" << c.ending_cart_wait_seen
            << " original_outro_animation_terminal_observed=" << c.ending_cart_terminal_seen << '\n'
            << "actual_next_entry_serial=" << c.next_serial << " actual_next_entry_operation=" << int(c.next_operation)
            << " actual_next_entry_argument=" << c.next_argument
            << " post_course_return=" << e.post.returned << '\n'
            << "course_frame_hold_released_without_stop=" << (e.frame_hold && !e.stop_while_held) << '\n';
        for (std::size_t n=0;n<c.phases.size();++n) out << "course_source_phase_" << n << "_observations=" << c.phases[n] << '\n';
        for (std::size_t n=0;n<course_captures.size();++n) out << course_captures[n] << " captured=" << c.captured[n] << '\n';
    }
    if (e.mode==Mode::course) {
        const auto& c=e.course;
        std::ofstream retired(root/"proof"/"course-retirement.txt");
        retired << "actual_completed_Beach_observer_calls=" << c.retirement_callbacks
            << " retired_entry_serial=" << c.retired_entry_serial << '\n'
            << "original_input_scene=" << c.retired_input_scene << " original_output_scene=" << c.retired_output_scene
            << " actual_original_result=" << c.retired_result << " original_end_reason=" << c.retired_end_reason << '\n'
            << "actual_Player_detached=" << c.retired_player_detached << " actual_Music_detached=" << c.retired_music_detached
            << " actual_GPU_Photo_frame_closed=" << c.retired_graphics_closed
            << " actual_original_source_domain_closed=" << c.retired_source_domain_closed
            << " actual_ending_owner_closed=" << c.retired_ending_closed
            << " original_post_scene_verified=" << c.post_scene_verified << '\n'
            << "Runtime_native_Manager_retired=" << e.native_retired << " Input_stopped=" << e.input_stopped
            << " Audio_drained=" << e.audio_drained << " Host_released=" << e.host_released << '\n';
        std::ofstream next(root/"proof"/"course-next-entry.txt");
        next << "actual_original_Manager_next_serial=" << c.next_serial << '\n'
            << "actual_typed_operation=" << int(c.next_operation) << " actual_argument=" << c.next_argument << '\n'
            << "actual_typed_owner=" << (c.next_operation==ISLE_APPLICATION_ENTRY_CAMERA_CHECK?"CameraCheck":
                c.next_operation==ISLE_APPLICATION_ENTRY_PHOTO_CHECK?"PhotoCheck":"unexpected") << '\n'
            << "qualified_completed_course_post_course=" << (c.qualified && failure.empty()) << '\n'
            << "full_application=bounded_course_post_course_Oak13_admission_only_NOT_return_Oak_rendering\n";
        require(bool(retired) && bool(next),"private actual retirement/next-entry artifacts could not be retained");
    }
    if (e.mode==Mode::course) {
        const auto& p=e.post;
        out << "post_source_rows=" << p.rows << " source_marked=" << p.marked << " source_scored=" << p.scored
            << " assessment_mask=" << p.assessment_mask << " ordered_assessment_progress=" << p.assessment_progress
            << " report_before=" << p.report_before << " report_after=" << p.report_after
            << " score_before=" << p.score_before << " score_after=" << p.score_after
            << " constructed_return_Oak13_serial=" << p.oak_serial << " return_Oak_rendering=not_claimed\n"
            << "camera_filter_seen=" << p.filter_seen << " camera_filter_unwound=" << p.filter_unwound
            << " camera_menu_reset=" << p.menu_reset << " camera_menu_refreshed=" << p.menu_refreshed
            << " camera_actual_confirmation=" << p.confirmation_seen << '\n'
            << "marked_roll=" << p.marked_roll << " marked_species=" << p.marked_species
            << " marked_revision=" << p.marked_revision << " marked_digest=" << p.marked_digest
            << " selected_roll=" << p.selected_roll << " selected_species=" << p.selected_species
            << " selected_revision=" << p.selected_revision << " selected_score=" << p.selected_score
            << " selected_digest=" << p.selected_digest << " selected_identity=" << p.selected_identity
            << " report_slot=" << p.report_slot << " report_digest=" << p.report_digest
            << " report_identity=" << p.report_identity << '\n'
            << "summary_element_serial=" << p.summary_serial << " initial_element_serial=" << p.initial_element_serial
            << " summary_width=" << p.summary_width << " summary_height=" << p.summary_height
            << " summary_seen=" << p.summary_seen << " last_summary_visible=" << p.summary_visible
            << " screenshot_visual_inspection=required summary_text_completion_at_final_ack2302=" << bool(p.final_ack_serial) << '\n';
        out << "camera_intro_prompt=" << p.intro_prompt_seen << " presented=" << p.intro_presented
            << " actual_A_requested=" << p.intro_ack_requested << " Source_A_consumed=" << p.intro_ack_consumed
            << " retired_Photo_element_observed_without_fault=" << p.retired_element_seen << '\n';
        out << "photo_save_ack_serial=" << p.save_ack_serial << " actual_A_requested=" << p.save_ack_requested
            << " Source_A_consumed=" << p.save_ack_consumed << " actual_release_observed=" << p.save_ack_released
            << " final_ack_serial=" << p.final_ack_serial << " camera_final_intro_ack_serial=" << p.intro_ack_serial << '\n'
            << "photo_assessment_capture=early_phase0_NOT_completed_evaluation\n";
        for (std::size_t i=0;i<post_captures.size();++i)
            out << post_captures[i] << " ready_Source_update=" << p.ready_update[i]
                << " ready_retrace=" << p.ready_retrace[i] << " excluded_frame_ordinal=" << p.frame_floor[i]
                << " captured_canonical_frame_ordinal=" << p.capture_frame[i]
                << " completed_Host_presentation=" << p.capture_presentation[i]
                << " ready_ack_serial=" << p.ready_ack_serial[i] << " ready_pinned_ack_line=" << p.ready_ack_line[i]
                << " actual_helper_polls=" << p.ready_ack_polls[i] << '\n';
        for (std::size_t i=0;i<2;++i)
            out << (i==0?"CameraCheck9":"PhotoCheck15") << " serial=" << p.serial[i]
                << " updates=" << p.updates[i] << " frames=" << p.frames[i] << " presentations=" << p.presentations[i]
                << " retirements=" << p.retirements[i] << " original_result=" << p.results[i]
                << " actual_native_GPU_Source_join=" << p.joined[i] << '\n';
        for (std::size_t i=0;i<post_captures.size();++i)
            out << post_captures[i] << " captured=" << p.captured[i] << '\n';
    }
    require(bool(out),"private Runtime report could not be retained");
}
// Actual current default-framebuffer pixels AFTER successful copied LUS
// presentation and its GL completion. Canonical RDRAM is never labeled a window.
void capture_window(const std::filesystem::path& path) {
    auto* window = SDL_GL_GetCurrentWindow();
    require(window != nullptr,"presented GL window is unavailable");
    int width=0,height=0;
    require(SDL_GetWindowSizeInPixels(window,&width,&height) && width>0 && height>0 && width<=4096 && height<=4096,
        "presented window has invalid pixel extent");
    using ReadBuffer = void (APIENTRY*)(GLenum);
    using PixelStore = void (APIENTRY*)(GLenum,GLint);
    using ReadPixels = void (APIENTRY*)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
    using GetError = GLenum (APIENTRY*)();
    using BindFramebuffer = void (APIENTRY*)(GLenum,GLuint);
    using GetInteger = void (APIENTRY*)(GLenum,GLint*);
    const auto read_buffer = reinterpret_cast<ReadBuffer>(SDL_GL_GetProcAddress("glReadBuffer"));
    const auto pixel_store = reinterpret_cast<PixelStore>(SDL_GL_GetProcAddress("glPixelStorei"));
    const auto read_pixels = reinterpret_cast<ReadPixels>(SDL_GL_GetProcAddress("glReadPixels"));
    const auto error = reinterpret_cast<GetError>(SDL_GL_GetProcAddress("glGetError"));
    const auto bind_framebuffer = reinterpret_cast<BindFramebuffer>(SDL_GL_GetProcAddress("glBindFramebuffer"));
    const auto get_integer = reinterpret_cast<GetInteger>(SDL_GL_GetProcAddress("glGetIntegerv"));
    require(read_buffer && pixel_store && read_pixels && error && bind_framebuffer && get_integer,
        "actual GL surface readback API missing");
    require(error()==GL_NO_ERROR,"presentation left an actual GL error before readback");
    GLint old_framebuffer=0,old_read_buffer=0,old_alignment=0;
    get_integer(GL_READ_FRAMEBUFFER_BINDING,&old_framebuffer);
    get_integer(GL_READ_BUFFER,&old_read_buffer);
    get_integer(GL_PACK_ALIGNMENT,&old_alignment);
    bind_framebuffer(GL_READ_FRAMEBUFFER,0);
    read_buffer(GL_FRONT);
    pixel_store(GL_PACK_ALIGNMENT,1);
    std::vector<std::byte> pixels(static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*3);
    read_pixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    const auto read_error=error();
    pixel_store(GL_PACK_ALIGNMENT,old_alignment);
    bind_framebuffer(GL_READ_FRAMEBUFFER,static_cast<GLuint>(old_framebuffer));
    read_buffer(static_cast<GLenum>(old_read_buffer));
    require(read_error==GL_NO_ERROR && error()==GL_NO_ERROR,"actual window readback/state restoration failed");
    std::ofstream out(path,std::ios::binary);
    out << "P6\n" << width << ' ' << height << "\n255\n";
    const auto row=static_cast<std::size_t>(width)*3;
    for (int y=height;y-- > 0;)
        out.write(reinterpret_cast<const char*>(pixels.data()+static_cast<std::size_t>(y)*row),static_cast<std::streamsize>(row));
    require(bool(out),"actual presented-window capture could not be retained");
}
bool varied(std::span<const std::byte> rgba,int left,int top,int right,int bottom) {
    const auto first=static_cast<std::size_t>(top*320+left)*4;
    for (int y=top;y<bottom;++y) for (int x=left;x<right;++x) {
        const auto at=static_cast<std::size_t>(y*320+x)*4;
        if (!std::equal(rgba.begin()+first,rgba.begin()+first+3,rgba.begin()+at)) return true;
    }
    return false;
}
std::uint64_t digest(std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash=14695981039346656037ull;
    for (const auto value:bytes) { hash^=std::to_integer<unsigned char>(value);hash*=1099511628211ull; }
    return hash;
}
using Tick = h::ProgressTick;
struct Timeline {
    g::Host& host;
    h::Progress progress;
    app::Runtime* runtime=nullptr;
    void* display_owner=nullptr;
    IsleGameResult (*display_service)(void*) noexcept=nullptr;
    Timeline(g::Host& actual,in::Service& input,SoftwareController& controller)
        : host(actual),progress(actual,input,{&controller,[](void* owner) noexcept {
            g::Input acquired;
            acquired.ports=static_cast<SoftwareController*>(owner)->sample();
            acquired.completed=true;
            return acquired;
        }}) {}
    void bind(h::GtlClock& clock,app::Runtime& application) {
        progress.bind(clock,{&application,[](void* owner,const in::EventResult& completed) noexcept {
            return static_cast<app::Runtime*>(owner)->acknowledge_input(completed)==ISLE_GAME_OK;
        }});
        runtime=&application;
        require(application.bind_post_course_progress(progress,{this,[](void* owner) noexcept {
            auto& timeline=*static_cast<Timeline*>(owner);
            if (timeline.display_service) return timeline.display_service(timeline.display_owner);
            if (timeline.runtime->completed_frame()) return timeline.runtime->release_completed();
            return ISLE_GAME_OK;
        }})==ISLE_GAME_OK,application.message());
    }
    Tick advance(bool strict) {
        const auto tick=progress.poll();
        if (strict) require(progress.status()==h::ProgressStatus::running,
            progress.message()[0]?progress.message():"actual host quit before scenario completion");
        return tick;
    }
    static std::uint32_t raw(const Tick& tick) noexcept { return h::Progress::raw(tick); }
};
void remember_failure(std::string& failure,const char* message) {
    if (failure.empty()) {
        failure=message && message[0]?message:"unspecified actual Runtime consumer failure";
        std::cerr<<"first actual Runtime consumer failure: "<<failure<<'\n';
    }
}
// Every ordinary assertion is caught BEFORE the Runtime unique_ptr unwinds.
// Retire the actual native/GPU/Photo owners even after a sticky Source failure.
// A deadline is failure, NEVER a GPU/native terminal or a fabricated completion.
void close_application(std::unique_ptr<app::Runtime>& application,h::GtlClock& clock,
        in::Service& input,SoftwareController& controller,Timeline& timeline,
        const std::filesystem::path& root,Evidence& evidence,std::string& failure) {
    evidence.native_retired=false; // A prior cold-stop scope cannot certify this Runtime.
    if (!application) return;
    application->request_stop();
    controller.buttons=0;
    controller.stick_x=controller.stick_y=0;
    timeline.progress.set_policy({false,false,false,false,true});
    timeline.progress.set_deadline(Time::now()+std::chrono::seconds(90),
        "actual Runtime shutdown deadline: native/GPU/Photo owners remain live");
    while (!application->stopped()) {
        if (timeline.progress.deadline_expired()) {
            evidence.native_retirement_deadline=true;
            remember_failure(failure,"actual Runtime shutdown deadline: native/GPU/Photo owners remain live");
            evidence.submissions+=clock.stats().submissions;
            evidence.terminals+=clock.stats().terminals;
            evidence.dropped_retraces+=clock.stats().dropped_retraces;
            std::cerr << "actual Runtime shutdown deadline: native/GPU/Photo owners remain live; first failure: "
                      << failure << "; Runtime: " << application->message() << '\n';
            try { write_report(root,evidence,failure); } catch (...) {}
            // There is no legal Runtime destructor before stopped(). A broken
            // retirement must fail visibly, not terminate during unwinding or
            // destroy borrowed Audio/SDL underneath a live native/GPU owner.
            std::cerr.flush();std::cout.flush();std::_Exit(EXIT_FAILURE);
        }
        const auto tick=timeline.advance(false);
        if (input.pending_read()) {
            const auto completed=timeline.progress.complete_input();
            if (!completed.completed || !completed.completion_accepted)
                remember_failure(failure,"real input completion failed during Runtime cancellation");
            else ++evidence.read_completions;
        }
        (void)application->pump(Timeline::raw(tick));
        if (application->completed_frame()) {
            if (application->release_completed()!=ISLE_GAME_OK)
                remember_failure(failure,"actual completed frame release failed during Runtime cancellation");
            else ++evidence.released_frames;
        }
        (void)timeline.progress.idle();
    }
    if (application->error()!=ISLE_GAME_OK) remember_failure(failure,application->message());
    const auto view=application->view();
    const bool native_retired=!application->scene_active() && !application->completed_frame() &&
        !view.entry && !view.startup && !view.name && !view.oak && !view.beach && !view.post && !view.rainbow &&
        !view.tunnel && !view.volcano && !view.river && !view.cave && !view.valley &&
        clock.stopped() && clock.stats().submissions==clock.stats().terminals;
    evidence.native_retired=native_retired;
    if (!native_retired)
        remember_failure(failure,"stopped Runtime retained a scene, Photo/frame, Manager entry or actual GTL terminal");
    if (clock.failed()) remember_failure(failure,"actual GTL fault remains after stopped Runtime retirement");
    evidence.submissions+=clock.stats().submissions;
    evidence.terminals+=clock.stats().terminals;
    evidence.dropped_retraces+=clock.stats().dropped_retraces;
    timeline.progress.unbind();
    timeline.progress.clear_deadline();
    application.reset(); // Only after actual stopped()/native/graphics retirement.
}
void exercise_cold_stop(const isle::rom::Snapshot& snapshot,const isle::menu::Catalog& catalog,
        isle::game::State& state,in::Service& input,SoftwareController& controller,
        Timeline& timeline,const std::filesystem::path& root,Evidence& evidence) {
    h::GtlClock clock({1,1,2,true,0});
    std::unique_ptr<app::Runtime> application;
    std::string failure;
    try {
        auto made=app::Runtime::create(snapshot,catalog,state,input,*timeline.host.audio(),clock,{true});
        require(bool(made),made.message.data());
        application=std::move(made.runtime);
        timeline.bind(clock,*application);
        timeline.progress.set_policy({false,false,false,false,true});
        // create() has already consumed the clock's one-shot stopped action.
        // Stop must consult actual stopped state, not wait for a second action.
        require(clock.stopped() && !application->scene_active() && !application->view().entry,
            "cold Runtime created an unsolicited scene");
        application->request_stop();
        require(application->pump(Timeline::raw(timeline.advance(true))),application->message());
        require(application->stopped() && !state.initialization_attempted() && !state.initialized() &&
            !application->scene_active() && !application->completed_frame() && !application->view().entry &&
            clock.stats().updates==0 && clock.stats().submissions==0 && clock.stats().terminals==0,
            "immediate stop before first pump lost the one-shot stopped action or entered Boot");
        require(application->pump(Timeline::raw(timeline.advance(true))) && application->stopped(),
            "closed Runtime pump reopened the original Manager");
        evidence.cold_stop=true;
    } catch (const std::exception& error) { remember_failure(failure,error.what()); }
      catch (...) { remember_failure(failure,"unknown immediate-stop consumer failure"); }
    close_application(application,clock,input,controller,timeline,root,evidence,failure);
    if (!failure.empty()) throw std::runtime_error(failure);
}
class Scenario {
public:
    Scenario(app::Runtime& runtime,h::GtlClock& clock,in::Service& input,SoftwareController& controller,
            isle::game::State& state,Timeline& timeline,const std::filesystem::path& root,Evidence& evidence,
            const isle::rom::Snapshot& snapshot)
        : application_(runtime),clock_(clock),input_(input),controller_(controller),state_(state),timeline_(timeline),root_(root),e_(evidence),
          snapshot_(snapshot) {
        if (menu_mode(e_.mode)) {
            menu_probe_=std::make_unique<isle::private_post_course::Probe>(
                isle::private_post_course::Controller{&controller_,[](void* owner,std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
                    auto& control=*static_cast<SoftwareController*>(owner);
                    control.buttons=buttons;control.stick_x=x;control.stick_y=y;
                }},root_/"proof");
            e_.menu_probe=menu_probe_.get();
        }
        if (e_.mode==Mode::native_course) {
            course_probe_=std::make_unique<isle::private_course::Probe>(e_.requested_course,
                isle::private_post_course::Controller{&controller_,[](void* owner,std::uint16_t buttons,std::int8_t x,std::int8_t y) noexcept {
                    auto& control=*static_cast<SoftwareController*>(owner);
                    control.buttons=buttons;control.stick_x=x;control.stick_y=y;
                }},root_/"proof");
            e_.course_probe=course_probe_.get();
        }
        timeline_.bind(clock_,application_);
        timeline_.display_owner=this;
        timeline_.display_service=[](void* owner) noexcept {
            auto& scenario=*static_cast<Scenario*>(owner);
            try { scenario.consume_frame({});scenario.present();return ISLE_GAME_OK; }
            catch (const std::exception& error) {
                std::snprintf(scenario.display_failure_.data(),scenario.display_failure_.size(),"%s",error.what());
                return ISLE_GAME_INVALID_STATE;
            } catch (...) {
                std::snprintf(scenario.display_failure_.data(),scenario.display_failure_.size(),"unknown actual display failure");
                return ISLE_GAME_INVALID_STATE;
            }
        };
    }
    ~Scenario() {e_.menu_probe=nullptr;e_.course_probe=nullptr;timeline_.display_service=nullptr;timeline_.display_owner=nullptr;}
    void open_course_log() {
        if (e_.mode!=Mode::course) return;
        course_log_.open(root_/"proof"/"course-source.tsv");
        course_log_ << "beach_update\tactual_retrace\tlifecycle\tsource_running\tscene_ready\toriginal_scene_end_reason\toriginal_intro_animation_terminal"
            "\tinput_disabled\ttutorial_enabled\tsource_finish_flag\tdirection\tzoom_target\tphoto_cooldown"
            "\troll_photos\tqualifying_photos\tmove_time\tmove_speed\tposition_x\tposition_y\tposition_z"
            "\tplayer_flags\tmessage_printed\tmessage_interactive\tsource_buttons\tsource_pressed\tsource_stick_x\tsource_stick_y"
            "\tending_sound\toutro_cart_present\toutro_terminal\tnext_owned_buttons\tnext_owned_stick_x\tnext_owned_stick_y\n";
        require(bool(course_log_),"predeclared actual course source ledger could not be opened");
        post_log_.open(root_/"proof"/"post-course-source.tsv");
        constexpr std::string_view header=
            "kind\tserial\tupdate\tretrace\tstage\tE4\tE8\tEC\tF0\tslot\tphoto_cursor\tmarked\tlive_marked"
            "\tmark_roll\tspecies\tselected_roll\tselected_species\tselected_revision\tscored\tscore\tphase\tskip\tassessment\tcaptures\treports\treport_score"
            "\treport_slot\tidentity\tstatus\tsource_status\trequested_buttons\trequested_y\tpublished_buttons\tpublished_pad"
            "\tpublished_released\tpublished_y\tconsumed_current\tconsumed_pressed\tpulse\tsummary_serial\twidth\theight\tvisible"
            "\tphoto_x\tphoto_y\tphoto_attr\tobservations\n";
        static_assert(header.size()<=512);
        post_log_ << header;e_.post.rows=1;
        require(bool(post_log_),"predeclared post-course ledger could not be opened");
    }
    void run() try {
        open_course_log();
        timeline_.progress.set_deadline(Time::now()+std::chrono::seconds(e_.mode==Mode::course?7200:1800),
            "actual application wall deadline expired; no scene completion claimed");
        while (!stop_requested_) {
            require(!timeline_.progress.deadline_expired(),"actual application wall deadline expired; no scene completion claimed");
            timeline_.progress.set_policy({true,backpressure_==Backpressure::normal,
                application_.scene_active(),backpressure_!=Backpressure::accumulating,true});
            const auto tick=timeline_.advance(true);
            acquire(tick);
            if (application_.scene_active() && !clock_.stopping() && !clock_.stopped()) {
                if (backpressure_==Backpressure::accumulating) {
                    batch_+=tick.count;
                    if (batch_>=8) {
                        offer(batch_);e_.coalesced_ticks=batch_;backpressure_=Backpressure::offered;
                    }
                }
            }
            const auto before=clock_.stats();
            const bool pumped=application_.pump(Timeline::raw(tick));
            require(pumped && application_.error()==ISLE_GAME_OK,application_.message());
            require(!e_.menu_failure[0],e_.menu_failure.data());
            inspect_entry();
            const auto after=clock_.stats();
            require(after.updates>=before.updates && after.updates-before.updates<=1,
                "one Runtime pump manufactured catch-up Source updates");
            if (after.updates!=before.updates) observe_update();
            if (e_.mode==Mode::course) observe_course();
            inspect_backpressure(tick);
            consume_frame(tick);
            present();
            e_.main_music|=timeline_.host.audio()->bgm_playing(0);
            require(!application_.stopped(),"Runtime stopped before real Beach ingress/consumer shutdown request");
            (void)timeline_.progress.idle();
        }
        if (e_.mode==Mode::native_course) {
            require(e_.authored_course_profile && e_.course_api_completed && course_probe_->completed() &&
                e_.entries==6 && e_.cold_stop && e_.blocked_claim && e_.resumed_claim && e_.boot_wait && e_.boot_complete &&
                e_.menu_result && e_.name_confirming && e_.name_confirmed && e_.oak_result &&
                e_.input_hold_audio>0 && e_.main_music && e_.course_frames>0 && e_.course_presentations>0 &&
                std::all_of(e_.captured.begin(),e_.captured.begin()+5,[](bool value){return value;}),
                "actual natural course/Original result9/CameraCheck9 admission proof incomplete");
            for(std::size_t n=0;n<4;++n)
                require(e_.updates[n]>0 && e_.frames[n]>0 && e_.presentations[n]>0,
                    "native course qualification lost genuine original Boot/Title/Name/Oak ingress");
            return;
        }
        if (menu_mode(e_.mode)) {
            require(e_.authored_menu_inputs && (e_.menu_qualified&menu_target(e_.mode))==menu_target(e_.mode) &&
                e_.cold_stop && e_.blocked_claim && e_.resumed_claim && e_.boot_wait && e_.boot_complete &&
                e_.menu_result && e_.name_confirming && e_.name_confirmed && e_.oak_result &&
                e_.input_hold_audio>0 && e_.main_music && e_.menu_frames>0 && e_.menu_presentations>0 &&
                std::all_of(e_.captured.begin(),e_.captured.begin()+5,[](bool value){return value;}),
                "authored-save original Manager menu/credits route lacks real Source/UI/return/presentation evidence");
            for (std::size_t n=0;n<4;++n)
                require(e_.updates[n]>0 && e_.frames[n]>0 && e_.presentations[n]>0,
                    "menu qualification lost genuine original Boot/Title/Name/Oak ingress");
            return;
        }
        require(e_.entries==(e_.mode==Mode::course?8u:scene_count) && e_.cold_stop && e_.blocked_claim && e_.resumed_claim && e_.boot_wait && e_.boot_complete &&
            e_.menu_result && e_.name_confirming && e_.name_confirmed && e_.oak_result && e_.frame_hold &&
            (e_.mode==Mode::course || e_.stop_while_held) &&
            e_.input_hold_audio>0 && e_.frame_hold_audio>0 && e_.beach_camera_changes>0 && e_.main_music &&
            std::all_of(e_.captured.begin(),e_.captured.end(),[](bool value){return value;}),
            "actual application source/UI/backpressure/window/audio acceptance evidence is incomplete");
        for (std::size_t n=0;n<scene_count;++n)
            require(e_.updates[n]>0 && e_.frames[n]>0 && e_.presentations[n]>0,
                "an original Manager scene lacked genuine Source updates, canonical draws or copied LUS presentation");
        if (e_.mode==Mode::course) {
            const auto& c=e_.course;
            const bool complete=c.intro_seen && c.intro_finished && c.tutorial_seen && c.tutorial_input_released &&
                c.tutorial_followup_seen && c.tutorial_followup_cleared && c.tutorial_stick_after_message &&
                c.photos==4 && c.qualifying_photos>=3 && c.shutter_presses==4 && c.movement_changes>0 && c.stick_updates>0 &&
                c.outro_seen && c.ending_sound_seen && c.ending_cart_wait_seen && c.ending_cart_terminal_seen &&
                c.retiring_seen && c.post_scene_verified && c.retirement_callbacks==1 &&
                c.next_serial>c.retired_entry_serial && e_.post.returned && c.source_rows>0 &&
                std::all_of(c.phases.begin(),c.phases.end(),[](std::uint64_t count){return count>0;}) &&
                std::all_of(c.captured.begin(),c.captured.end(),[](bool value){return value;});
            if (!complete) {
                std::cerr << "incomplete original course evidence: intro=" << c.intro_seen << '/' << c.intro_finished
                    << " tutorial=" << c.tutorial_seen << '/' << c.tutorial_input_released << '/'
                    << c.tutorial_followup_seen << '/' << c.tutorial_followup_cleared << '/' << c.tutorial_stick_after_message
                    << " photos=" << c.photos << " qualifying=" << c.qualifying_photos << " shutters=" << c.shutter_presses
                    << " movement=" << c.movement_changes << " sticks=" << c.stick_updates
                    << " outro=" << c.outro_seen << " ending_sound_wait_terminal=" << c.ending_sound_seen << '/'
                    << c.ending_cart_wait_seen << '/' << c.ending_cart_terminal_seen
                    << " retiring=" << c.retiring_seen << " verified=" << c.post_scene_verified
                    << " retirement_callbacks=" << c.retirement_callbacks
                    << " next=" << c.next_serial << '/' << static_cast<int>(c.next_operation) << '/' << c.next_argument
                    << " retired=" << c.retired_entry_serial << '/' << c.retired_result << '/' << c.retired_output_scene
                    << " end_reason=" << c.retired_end_reason
                    << " post_return=" << e_.post.returned
                    << " source_rows=" << c.source_rows << " phases=";
                for (auto count:c.phases) std::cerr << count << ',';
                std::cerr << " captures=";
                for (auto captured:c.captured) std::cerr << captured << ',';
                std::cerr << " focus=" << c.focus_present << '/' << c.focus_subject << '/' << c.focus_qualified
                    << " zoom_ready=" << c.zoom_ready << " cooldown_ready=" << c.shutter_ready << '\n';
                throw std::runtime_error("complete original Beach/tutorial/photos/travel/ending/post-scene retirement evidence is incomplete");
            }
            require(e_.post.marked>0 && e_.post.scored>0 && e_.post.report_before==0 &&
                e_.post.intro_presented && e_.post.intro_ack_requested && e_.post.intro_ack_consumed &&
                e_.post.retired_element_seen &&
                e_.post.report_after>0 && e_.post.score_after>e_.post.score_before &&
                e_.post.filter_seen && e_.post.filter_unwound && e_.post.menu_reset && e_.post.menu_refreshed &&
                e_.post.confirmation_seen && e_.post.selected_identity && e_.post.report_identity &&
                e_.post.assessment_progress>=3 && e_.post.summary_seen &&
                e_.post.save_ack_requested && e_.post.save_ack_consumed && e_.post.save_ack_released &&
                e_.post.final_ack_serial>e_.post.save_ack_serial &&
                e_.post.capture_frame[0]>e_.post.frame_floor[0] &&
                e_.post.capture_frame[3]>e_.post.frame_floor[3] &&
                e_.post.results[1]==13 &&
                std::all_of(e_.post.joined.begin(),e_.post.joined.end(),[](bool v){return v;}) &&
                std::all_of(e_.post.captured.begin(),e_.post.captured.end(),[](bool v){return v;}),
                "actual post-course Source selection/ordered assessment/visible summary/identity/join proof incomplete");
            for (std::size_t i=0;i<2;++i)
                require(e_.post.updates[i]>0 && e_.post.frames[i]>0 && e_.post.presentations[i]>0 &&
                    e_.post.retirements[i]==1,"actual post-course owner lacked updates/render/presentation/retirement");
            post_log_.flush();
            require(bool(post_log_),"actual post-course source ledger write failed");
            course_log_.flush();
            require(bool(course_log_),"actual course source ledger write failed");
        }
    }
    catch (const std::exception& error) {
        throw std::runtime_error(std::string(error.what())+"; "+post_diagnostic()+
            (display_failure_[0]?std::string("; display failure=")+display_failure_.data():std::string{}));
    }
private:
    enum class Backpressure { accumulating,offered,claimed,normal };
    struct Display {
        std::size_t scene=0;
        int capture=-1,course_capture=-1,post_capture=-1;
        std::uint64_t frame=0;
        std::int32_t menu_argument=0;
        std::uint64_t menu_digest=0;
        bool menu_varied=false;
        std::int32_t native_course=-1;
    };
    app::Runtime& application_;
    h::GtlClock& clock_;
    in::Service& input_;
    SoftwareController& controller_;
    isle::game::State& state_;
    Timeline& timeline_;
    const std::filesystem::path& root_;
    Evidence& e_;
    const isle::rom::Snapshot& snapshot_;
    std::unique_ptr<isle::private_course::Probe> course_probe_;
    std::unique_ptr<isle::private_post_course::Probe> menu_probe_;
    std::uint64_t menu_visit_updates_=0;
    unsigned gallery_title_stage_=0;
    std::uint64_t gallery_title_stable_=0;
    int menu_oak_phase_=-1,menu_oak_slot_=-1;
    std::uint64_t oak_input_updates_=0,oak_hold_until_=0,oak_next_command_=0;
    Backpressure backpressure_=Backpressure::accumulating;
    in::ReadToken held_read_{};
    std::uint64_t last_read_serial_=0,batch_=0,claim_ordinal_=0,claim_audio_=0;
    std::uint64_t last_entry_=0,last_frame_entry_=0,last_frame_ordinal_=0,stable_update_=0;
    std::size_t scene_=0;
    unsigned menu_stage_=0,name_stage_=0,oak_stage_=0;
    isle::save::GameSession* persistent_save_=nullptr;
    const isle::save::GameOpenResult* initialization_=nullptr;
    std::array<std::uint8_t,16> confirmed_name_{};
    bool saved_name_=false,stop_requested_=false,holding_frame_=false,have_camera_=false;
    std::array<float,6> last_camera_{};
    std::uint64_t held_frame_ordinal_=0,held_digest_=0,held_ordinal_=0,held_audio_=0;
    h::ClockStats held_stats_{};
    std::optional<Display> display_;
    std::ofstream course_log_;
    std::ofstream post_log_;
    enum class PostStage {
        choose,marked_presentation,filter_unwind,outer_grid,menu_refresh,select_oak,
        await_confirmation,confirm,photo_intro,assessment_hold,assess,summary_hold,photo_finish,oak_admitted
    };
    enum class PostPulse { ready,pressed,releasing };
    PostStage post_stage_=PostStage::choose;
    PostPulse post_pulse_=PostPulse::ready;
    IslePhotoRecord marked_photo_{};
    std::array<std::byte,isle::save::game_photo_bytes> marked_photo_bytes_{};
    bool assessment_visible_=false,summary_ready_=false,intro_ready_=false;
    std::array<char,256> display_failure_{};
    static const char* post_stage_name(PostStage stage) noexcept {
        constexpr std::array<const char*,14> names{{"choose","marked-presentation","filter-unwind","outer-grid",
            "menu-refresh","select-oak22","await-confirmation","confirm-A","photo-intro",
            "assessment-hold","assess-A","summary-hold","photo-finish-A","Oak13-admitted"}};
        return names[static_cast<std::size_t>(stage)];
    }
    std::string post_diagnostic() const {
        const auto& p=e_.post;
        std::array<char,1536> text{};
        std::snprintf(text.data(),text.size(),
            "last_post stage=%s serial=%llu/%llu updates=%llu/%llu retrace=%llu E4/E8/EC/F0=%d/%d/%d/%d slot=%d"
            " photo_cursor=%d mark=%d/%d roll/species=%d/%d selected_roll/species=%d/%d score=%u identity=%d/%d"
            " phase/skip/progress=%d/%d/%u captures=%d%d%d%d report=%d/%lld report_slot=%d status=%d/%d"
            " requested=%u,%d published=%u,%u,%u,%d consumed=%d,%d pulse=%u summary=%llu,%dx%d,%d photo=%d,%d,attr%d",
            post_stage_name(post_stage_),static_cast<unsigned long long>(p.serial[0]),
            static_cast<unsigned long long>(p.serial[1]),static_cast<unsigned long long>(p.updates[0]),
            static_cast<unsigned long long>(p.updates[1]),static_cast<unsigned long long>(p.last_retrace),
            p.camera_e4,p.camera_e8,p.camera_ec,p.camera_f0,p.slot,p.photo_cursor,p.marked,p.live_marked,
            p.marked_roll,p.marked_species,p.selected_roll,p.selected_species,p.selected_score,
            int(p.selected_identity),int(p.report_identity),p.phase,p.skip,p.assessment_progress,
            int(p.captured[0]),int(p.captured[1]),int(p.captured[2]),int(p.captured[3]),
            p.report_after,static_cast<long long>(p.score_after),p.report_slot,p.scene_status,p.source_status,
            unsigned(p.requested_buttons),int(p.requested_y),unsigned(p.published_buttons),unsigned(p.published_pad),
            unsigned(p.published_released),int(p.published_y),p.source_current,p.source_pressed,p.pulse,
            static_cast<unsigned long long>(p.summary_serial),p.summary_width,p.summary_height,int(p.summary_visible),
            p.photo_x,p.photo_y,p.photo_attr);
        return text.data();
    }
    static std::uint64_t photo_identity(const IslePlayerPhotoData& photo,
            std::array<std::byte,isle::save::game_photo_bytes>& bytes) noexcept {
        isle::player::encode_photo(photo,bytes);
        // Scorer-owned Photo flags are not the identity of a course photograph.
        bytes[1]=std::byte{0};
        return digest(bytes);
    }
    static bool on_screen(const IsleSprite& sprite) noexcept {
        return !(sprite.attr&ISLE_SP_HIDDEN) && sprite.alpha && sprite.scalex>0 && sprite.scaley>0 &&
            sprite.x<320 && sprite.y<240 &&
            sprite.x+sprite.width*sprite.scalex>0 && sprite.y+sprite.height*sprite.scaley>0;
    }
    static const IsleSprite* sprite_view(IslePostCourseWindow* window,IsleGameSprite* sprite) {
        if (!sprite) return nullptr;
        const IsleSprite* view=nullptr;
        const auto result=isle::post_course::Window::observe_sprite(window,sprite,&view);
        if (result==ISLE_GAME_INVALID_ARGUMENT) return nullptr;
        require(result==ISLE_GAME_OK,"actual post-course attached sprite observation failed");
        return view;
    }
    template<class Element>
    static const IsleSprite* element_view(IslePostCourseWindow* window,const Element* element) {
        if (!element) return nullptr;
        const IsleSprite* view=nullptr;
        require(isle::post_course::Window::observe_element(window,element->handle,&view)==ISLE_GAME_OK,
            "actual post-course element observation failed");
        return view;
    }
    static bool acknowledgment_ready(const IslePostCourseSceneSource& source,
            IslePostCourseElement element,int line) {
        const auto& ack=source.acknowledgment;
        isle::post_course::WindowObservation operations{};
        require(isle::post_course::Window::observe_operations(source.window,&operations)==ISLE_GAME_OK,
            "actual post-course operation observation failed");
        // The helper has actually read neutral input and is suspended inside
        // its poll loop, not its initial wait or its post-consumption wait.
        return ack.active && ack.waiting && ack.polls && ack.caller_line==line &&
            ack.element.owner==element.owner && ack.element.serial==element.serial &&
            !ack.current_buttons && !ack.pressed_buttons &&
            !operations.printing && !operations.skip_enabled &&
            !operations.prints && !operations.fades && !operations.layouts;
    }
    void observe_capture_readiness(std::size_t capture,std::size_t source) {
        auto& p=e_.post;
        if (p.ready_update[capture]) return;
        p.ready_update[capture]=p.updates[source];
        p.ready_retrace[capture]=timeline_.progress.ordinal();
        const auto view=application_.view();
        if (view.post && (capture==0 || capture==3)) {
            const auto& ack=view.post->source().acknowledgment;
            p.ready_ack_serial[capture]=ack.serial;p.ready_ack_line[capture]=ack.caller_line;
            p.ready_ack_polls[capture]=ack.polls;
        }
    }
    void sample_post_input() {
        auto& p=e_.post;
        const auto& snapshot=input_.snapshot();
        p.requested_buttons=controller_.buttons;p.requested_y=controller_.stick_y;
        p.published_buttons=snapshot.ports[0].buttons;p.published_y=snapshot.ports[0].stick_y;
        p.published_pad=snapshot.primary().buttons;p.published_released=snapshot.primary().released;
        p.pulse=static_cast<unsigned>(post_pulse_);p.stage=static_cast<int>(post_stage_);
        p.last_retrace=timeline_.progress.ordinal();
    }
    bool release_post_pulse() {
        const auto& sample=input_.snapshot();
        const auto& published=sample.ports[0];
        const auto& pad=sample.primary();
        const bool neutral=published.error==0 && !published.buttons && !published.stick_x && !published.stick_y &&
            !pad.buttons && !pad.stick_x && !pad.stick_y;
        if (post_pulse_==PostPulse::pressed) {
            if (published.error || published.buttons!=controller_.buttons ||
                published.stick_x!=controller_.stick_x || published.stick_y!=controller_.stick_y ||
                pad.buttons!=controller_.buttons || pad.stick_x!=controller_.stick_x || pad.stick_y!=controller_.stick_y)
                return false;
            controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
            post_pulse_=PostPulse::releasing;
            return false;
        }
        controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
        if (post_pulse_==PostPulse::releasing) {
            if (neutral) post_pulse_=PostPulse::ready;
            // Retain an entire observed Source update at the real release.
            return false;
        }
        return neutral;
    }
    void request_post_pulse(std::uint16_t buttons,std::int8_t y=0) {
        controller_.buttons=buttons;controller_.stick_y=y;post_pulse_=PostPulse::pressed;
    }
    void hold_post_input() {
        controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
        if (post_pulse_==PostPulse::pressed) post_pulse_=PostPulse::releasing;
    }
    void write_post_row(const char* kind,std::uint64_t serial,std::uint64_t update) {
        auto& p=e_.post;
        sample_post_input();
        require(p.rows<20000,"bounded post-course observation ledger exhausted");
        std::array<char,512> row{};
        std::size_t used=std::char_traits<char>::length(kind);
        require(used<row.size(),"bounded post-course row kind overflow");
        std::copy_n(kind,used,row.data());
        const auto field=[&](auto value) {
            require(used+1<row.size(),"bounded post-course observation row exhausted");
            row[used++]='\t';
            const auto written=std::to_chars(row.data()+used,row.data()+row.size()-1,value);
            require(written.ec==std::errc{},"bounded post-course observation field exhausted");
            used=static_cast<std::size_t>(written.ptr-row.data());
        };
        field(serial);field(update);field(p.last_retrace);field(p.stage);
        field(p.camera_e4);field(p.camera_e8);field(p.camera_ec);field(p.camera_f0);field(p.slot);
        field(p.photo_cursor);field(p.marked);field(p.live_marked);field(p.marked_roll);field(p.marked_species);
        field(p.selected_roll);field(p.selected_species);field(p.selected_revision);field(p.scored);
        field(p.selected_score);field(p.phase);field(p.skip);
        field(p.assessment_progress);
        field(unsigned(p.captured[0])|unsigned(p.captured[1])<<1|unsigned(p.captured[2])<<2|unsigned(p.captured[3])<<3);
        field(p.report_after);field(p.score_after);field(p.report_slot);field(unsigned(p.selected_identity)|unsigned(p.report_identity)<<1);
        field(p.scene_status);field(p.source_status);field(p.requested_buttons);field(int(p.requested_y));
        field(p.published_buttons);field(p.published_pad);field(p.published_released);field(int(p.published_y));
        field(p.source_current);field(p.source_pressed);field(p.pulse);field(p.summary_serial);
        field(p.summary_width);field(p.summary_height);field(int(p.summary_visible));
        field(p.photo_x);field(p.photo_y);field(p.photo_attr);
        field(unsigned(p.intro_prompt_seen)|unsigned(p.intro_presented)<<1|unsigned(p.intro_ack_requested)<<2|
            unsigned(p.intro_ack_consumed)<<3|unsigned(p.retired_element_seen)<<4);
        row[used++]='\n';
        post_log_.write(row.data(),static_cast<std::streamsize>(used));
        ++p.rows;require(bool(post_log_),"bounded post-course observation ledger failed");
    }
    void observe_photo_identity(const IslePhotoCheckSource& photo) {
        auto& p=e_.post;
        int32_t count=0;
        require(isle_application_photos_selected_count(photo.common.photos,&count)==ISLE_GAME_OK && count>=0 && count<=60,
            "actual Photo selected count unavailable or outside original capacity");
        for (int32_t i=0;i<count;++i) {
            const IsleSelectedPhoto* selected=nullptr;
            require(isle_application_photos_selected(photo.common.photos,i,&selected)==ISLE_GAME_OK && selected,
                "actual Photo selected record unavailable");
            if (!selected->for_oak || selected->photo!=marked_photo_.photo) continue;
            const auto& scored=photo.D_802291A0_9D8C10[static_cast<std::size_t>(i)];
            if (scored.unk_0!=selected->photo || !scored.unk_1A_14 || scored.pkmnID<=0 || !scored.totalScore) continue;
            IslePhotoRecord identity{};
            require(isle_application_photos_identify(photo.common.photos,scored.unk_0,&identity)==ISLE_GAME_OK,
                "actual selected/scored photo identity unavailable");
            p.selected_roll=identity.index;p.selected_revision=identity.revision;
            p.selected_species=scored.pkmnID;p.selected_score=scored.totalScore;p.scored=1;
            if (!p.selected_identity) {
                std::array<std::byte,isle::save::game_photo_bytes> bytes{};
                p.selected_digest=photo_identity(*identity.photo,bytes);
                // Native scorer flag writes legitimately revise this same roll
                // record. Preserve owner/pointer/slot and exact non-flag bytes.
                p.selected_identity=identity.owner==marked_photo_.owner && identity.photo==marked_photo_.photo &&
                    identity.storage==ISLE_PHOTO_ROLL && identity.index==p.marked_roll &&
                    identity.revision>=p.marked_revision && scored.pkmnID==p.marked_species && bytes==marked_photo_bytes_;
                require(p.selected_identity,"actual marked -> selected/scored photo identity changed");
            }
            if (!p.report_identity) {
                IslePhotoRecord report{};
                int32_t points=0;
                require(isle_application_photos_report(photo.common.photos,p.selected_species,&report)==ISLE_GAME_OK &&
                    isle_application_photos_report_score(photo.common.photos,p.selected_species,&points)==ISLE_GAME_OK,
                    "actual scored species Report record unavailable");
                if (report.photo && points>0) {
                    std::array<std::byte,isle::save::game_photo_bytes> bytes{};
                    p.report_slot=report.index;p.report_digest=photo_identity(*report.photo,bytes);
                    p.report_identity=report.owner==identity.owner && report.storage==ISLE_PHOTO_REPORT &&
                        bytes==marked_photo_bytes_ && static_cast<std::uint32_t>(points)==p.selected_score;
                    require(p.report_identity,"actual selected/scored -> Report photo/score identity changed");
                }
            }
            break;
        }
    }
    void observe_photo_display(const IslePhotoCheckSource& photo) {
        auto& p=e_.post;
        p.phase=photo.D_801F3E30_9A38A0;p.skip=photo.D_801F3E34_9A38A4;
        p.source_current=static_cast<int>(photo.input.currentButtons);
        p.source_pressed=static_cast<int>(photo.input.pressedButtons);
        require(p.skip==0,"original assessment was skipped instead of driven through dialogue");
        if (p.phase>=0 && p.phase<32) p.assessment_mask|=1u<<p.phase;
        const auto* element=element_view(photo.common.window,photo.D_802290E0_9D8B50);
        p.retired_element_seen|=photo.D_802290E0_9D8B50 && photo.D_802290E0_9D8B50->handle.serial && !element;
        if (element && element->width==204 && element->height==120)
            p.initial_element_serial=photo.D_802290E0_9D8B50->handle.serial;
        p.summary_width=element?element->width:0;p.summary_height=element?element->height:0;
        const bool summary=element && element->width==216 && element->height==72 &&
            p.initial_element_serial && photo.D_802290E0_9D8B50->handle.serial>p.initial_element_serial && on_screen(*element);
        if (summary) p.summary_serial=photo.D_802290E0_9D8B50->handle.serial;
        p.summary_visible=summary;p.summary_seen|=summary;
        const auto& pair=photo.D_802290A0_9D8B10[0];
        const IsleSprite* displayed=nullptr;
        if (pair.gobj) {
            const auto result=isle::post_course::Window::observe_sprite_root(photo.common.window,pair.gobj,&displayed);
            require(result==ISLE_GAME_OK || result==ISLE_GAME_INVALID_ARGUMENT,
                "actual selected-photo sprite root observation failed");
        }
        const auto* image=sprite_view(photo.common.window,pair.sobj);
        p.photo_x=displayed?displayed->x:0;p.photo_y=displayed?displayed->y:0;p.photo_attr=displayed?displayed->attr:-1;
        // 98C330: the marked selected Photo is replayed into pair0's child
        // before evaluation sets phase0. Its moving/hidden parent, not the
        // child's relative (2,2), establishes actual on-screen placement.
        assessment_visible_=p.selected_identity && p.phase==0 && displayed && on_screen(*displayed) &&
            image && !(image->attr&ISLE_SP_HIDDEN) && image->alpha && image->bitmap && image->bitmap->buf;
        if (assessment_visible_ && p.assessment_progress==0) {
            p.assessment_progress=1;
            post_stage_=PostStage::assessment_hold;
        }
        if (p.assessment_progress==2 && (p.phase==1 || p.phase==2)) p.assessment_progress=3;
        const auto& ack=photo.common.acknowledgment;
        if (p.save_ack_requested && ack.consumed_serial==p.save_ack_serial &&
            (ack.consumed_buttons&button_a)) p.save_ack_consumed=true;
        if (p.save_ack_consumed && post_pulse_==PostPulse::ready &&
            !p.published_buttons && !p.published_pad) p.save_ack_released=true;
        const bool final_wait=photo.D_802290DC_9D8B4C &&
            acknowledgment_ready(photo.common,photo.D_802290DC_9D8B4C->handle,2302);
        summary_ready_=summary && p.assessment_progress>=3 && p.report_identity &&
            p.report_after>p.report_before && p.score_after>p.score_before &&
            p.save_ack_consumed && p.save_ack_released && final_wait && ack.serial>p.save_ack_serial;
        if (summary_ready_) {
            p.final_ack_serial=ack.serial;observe_capture_readiness(3,1);
            if (!p.captured[3]) post_stage_=PostStage::summary_hold;
        }
        if ((post_stage_==PostStage::assessment_hold && !p.captured[2]) ||
            (summary && !p.captured[3] && !(p.save_ack_requested && post_pulse_==PostPulse::pressed)))
            hold_post_input();
    }
    void drive_post(const isle::post_course::Scene& post) {
        auto& p=e_.post;
        const auto* camera=post.camera_source();
        const auto* photo=post.photo_source();
        const auto i=camera?0u:1u;
        ++p.updates[i];p.scene_status=static_cast<int>(post.status());p.source_status=post.source().status;
        sample_post_input();
        require(post.error()==ISLE_GAME_OK && post.source().status==ISLE_GAME_OK,post.message());
        const bool ready=release_post_pulse();
        if (camera) {
            p.camera_e4=camera->D_camera_check_802089E4;p.camera_e8=camera->D_camera_check_802089E8;
            p.camera_ec=camera->D_camera_check_802089EC;p.camera_f0=camera->D_camera_check_802089F0;
            p.photo_cursor=camera->D_camera_check_80208960;p.slot=-1;p.source_current=p.source_pressed=-1;
            if (const auto* consumed=post.camera_input_observation(0)) {
                p.source_current=static_cast<int>(consumed->currentButtons);p.source_pressed=static_cast<int>(consumed->pressedButtons);
                const auto& ack=camera->common.acknowledgment;
                p.intro_ack_consumed|=p.intro_ack_requested && ack.consumed_serial==p.intro_ack_serial &&
                    (ack.consumed_buttons&button_a);
            }
            const auto* dialogue=element_view(camera->common.window,camera->D_camera_check_80249914);
            const bool intro_visible=dialogue && on_screen(*dialogue);
            intro_ready_=p.camera_e4<0 && intro_visible && camera->D_camera_check_80249914 &&
                acknowledgment_ready(camera->common,camera->D_camera_check_80249914->handle,1039);
            if (intro_ready_) {
                p.intro_prompt_seen=true;p.intro_ack_serial=camera->common.acknowledgment.serial;
                observe_capture_readiness(0,0);
            }
            if (p.camera_e4>=0)
                require(isle::post_course::Window::observe_layout_slot_id(camera->common.window,static_cast<std::size_t>(p.camera_e4),
                    &p.slot)==ISLE_GAME_OK,"actual Camera menu slot unavailable");
            p.live_marked=0;
            for (const auto& item:camera->D_camera_check_80249B30) {
                if (!item.unk_00 || !item.unk_18_0x20000000) continue;
                ++p.live_marked;
                if (!marked_photo_.photo) {
                    require(isle_application_photos_identify(camera->common.photos,item.unk_00,&marked_photo_)==ISLE_GAME_OK &&
                        marked_photo_.storage==ISLE_PHOTO_ROLL,"actual Camera marked roll identity unavailable");
                    p.marked_roll=marked_photo_.index;p.marked_revision=marked_photo_.revision;
                    p.marked_species=item.unk_08;p.marked_digest=photo_identity(*item.unk_00,marked_photo_bytes_);
                }
            }
            p.marked=std::max(p.marked,p.live_marked);
            p.filter_seen|=p.live_marked>0 && p.camera_f0>0;
            p.filter_unwound|=p.marked && p.filter_seen && p.camera_f0==0;
            if (p.marked && post_stage_==PostStage::choose) post_stage_=PostStage::marked_presentation;
            if (post_stage_==PostStage::marked_presentation && p.captured[1]) post_stage_=PostStage::filter_unwind;
            if (post_stage_==PostStage::filter_unwind && p.filter_unwound) post_stage_=PostStage::outer_grid;
            if (post_stage_==PostStage::outer_grid && p.camera_e4==-1) {
                p.menu_reset=true;post_stage_=PostStage::menu_refresh;
            }
            if (post_stage_==PostStage::menu_refresh && p.camera_e4>=0) {
                p.menu_refreshed=true;post_stage_=PostStage::select_oak;
            }
            if (post_stage_==PostStage::await_confirmation && p.camera_f0>0) {
                p.confirmation_seen=true;post_stage_=PostStage::confirm;
            }
            if (post_stage_==PostStage::marked_presentation || post_stage_==PostStage::filter_unwind)
                hold_post_input();
            // df8815e 87D1A0: successful mark owns its wait/unwind. E4 is
            // cached throughout the nested grid: only the later -1 reset and
            // refreshed live slot authorize down; F0 then proves confirmation.
            if (ready) {
                // Optional original intro acknowledgments precede final1039.
                // Only that final completed wait must hold for its capture.
                const auto& ack=camera->common.acknowledgment;
                const bool earlier_intro=camera->D_camera_check_80249914 && p.camera_e4<0 &&
                    (ack.caller_line==1025 || ack.caller_line==1031) &&
                    acknowledgment_ready(camera->common,camera->D_camera_check_80249914->handle,ack.caller_line);
                if (post_stage_==PostStage::choose && (earlier_intro ||
                    (p.captured[0] && (p.camera_e4>=0 || (intro_ready_ && p.intro_presented))))) {
                    if (p.camera_e4<0 && !earlier_intro) p.intro_ack_requested=true;
                    request_post_pulse(button_a);
                }
                else if (post_stage_==PostStage::outer_grid) request_post_pulse(button_b);
                else if (post_stage_==PostStage::select_oak) {
                    if (p.slot!=22) request_post_pulse(0,-64);
                    else {request_post_pulse(button_a);post_stage_=PostStage::await_confirmation;}
                } else if (post_stage_==PostStage::await_confirmation || post_stage_==PostStage::confirm)
                    request_post_pulse(button_a);
            }
        } else {
            require(photo,"actual post-course owner has no typed Source context");
            observe_photo_identity(*photo);read_report(p.report_after,p.score_after);observe_photo_display(*photo);
            if (post_stage_==PostStage::assessment_hold && p.captured[2]) post_stage_=PostStage::assess;
            if (post_stage_==PostStage::summary_hold && p.captured[3]) post_stage_=PostStage::photo_finish;
            const auto* dialogue=element_view(photo->common.window,photo->D_802290DC_9D8B4C);
            const auto& ack=photo->common.acknowledgment;
            const bool save_wait=p.summary_visible && photo->D_802290DC_9D8B4C &&
                acknowledgment_ready(photo->common,photo->D_802290DC_9D8B4C->handle,2197);
            if (ready && save_wait && !p.save_ack_requested) {
                p.save_ack_serial=ack.serial;p.save_ack_requested=true;request_post_pulse(button_a);
            } else if (ready && dialogue && on_screen(*dialogue) &&
                ((!p.summary_visible && (post_stage_==PostStage::photo_intro || post_stage_==PostStage::assess)) ||
                 (post_stage_==PostStage::photo_finish && summary_ready_ && p.captured[3])))
                request_post_pulse(button_a);
        }
        write_post_row(camera?"CameraCheck9":"PhotoCheck15",p.serial[i],p.updates[i]);
    }
    std::uint64_t logged_course_update_=0;
    int logged_course_status_=-1;
    std::optional<IsleGameVec3> last_position_;
    void read_report(int& count,std::int64_t& score) {
        const auto* save=state_.save();
        require(save!=nullptr,"actual post-course profile unavailable");
        count=0;score=0;
        for (std::size_t i=0;i<isle::save::game_report_count;++i) {
            const auto photo=save->photo(isle::save::PhotoSet::report,i);
            const auto points=save->report_score(i);
            require(bool(photo) && bool(points) && photo.value.size()==isle::save::game_photo_bytes,
                "actual report read failed");
            // Same canonical empty-slot word as GameSnapshot::photo_present.
            // Borrow only: no session sealing, allocation or image copy.
            const auto empty=std::all_of(photo.value.begin()+4,photo.value.begin()+8,
                [](std::byte b){return b==std::byte{0xff};});
            count+=empty?0:1;score+=points.value;
        }
    }
    void observe_course() {
        const auto view=application_.view();
        if (!view.beach) return;
        const auto status=view.beach->status();
        if (logged_course_update_==e_.updates[4] && logged_course_status_==int(status)) return;
        logged_course_update_=e_.updates[4];logged_course_status_=int(status);
        auto& c=e_.course;
        const auto& source=view.beach->source();
        require(source.status==ISLE_GAME_OK && view.beach->error()==ISLE_GAME_OK,view.beach->message());
        c.intro_seen|=source.D_beach_802CC0E4!=nullptr && source.D_beach_802CC0E0==0;
        int ending_sound=-1,outro_cart_present=0,outro_terminal=0;
        if (const auto* ending=view.beach->ending_source()) {
            ending_sound=ending->D_beach_802CD9D0;
            outro_cart_present=ending->D_beach_80347658!=nullptr;outro_terminal=ending->D_beach_802CD9D4;
            if (ending->D_beach_80347658) {
                c.outro_seen=true;
                c.ending_sound_seen|=ending->D_beach_802CD9D0>=0;
                c.ending_cart_wait_seen|=ending->D_beach_802CD9D4==0;
                c.ending_cart_terminal_seen|=c.ending_cart_wait_seen && ending->D_beach_802CD9D4==1;
            }
        }
        if (status==isle::beach::LifecycleStatus::retiring) {
            c.retiring_seen=true;++c.phases[4];
        }
        const auto* player=source.player;
        course_log_ << e_.updates[4] << '\t' << timeline_.progress.ordinal() << '\t' << int(status) << '\t' << int(source.running)
            << '\t' << int(source.scene_completion_ready) << '\t' << source.D_beach_802CC018 << '\t' << source.D_beach_802CC0E0;
        if (player) {
            require(player->status==ISLE_GAME_OK && player->photo && player->photo->status==ISLE_GAME_OK,
                "actual course Root/application Photo Source failed");
            const auto& photo=*player->photo;
            c.photos=photo.gPhotoCount;c.qualifying_photos=photo.D_800AE27C;
            c.last_direction=player->gDirectionIndex;c.last_zoom_target=player->TargetDirectionZoomedIn;
            c.last_cooldown=player->sTimerAfterPhotoTaken;c.last_input_disabled=player->IsInputDisabled;
            c.last_controls=player->D_80382D04_523114;c.last_source_buttons=player->input.current_buttons;
            require(c.photos>=c.qualifying_photos && c.qualifying_photos>=0 && c.photos<=60,
                "actual native Photo roll/qualifying counters are inconsistent");
            c.intro_finished|=source.D_beach_802CC0E0==1 && player->D_80382D04_523114!=0;
            c.tutorial_seen|=player->IsTutorialEnabled!=0;
            c.tutorial_input_released|=c.qualifying_photos>=3 && !player->IsInputDisabled;
            const auto& position=player->movement.position;
            if (last_position_ && (last_position_->x!=position.x || last_position_->y!=position.y || last_position_->z!=position.z))
                ++c.movement_changes;
            last_position_=position;
            if (player->input.stick_x!=0 || player->input.stick_y!=0) ++c.stick_updates;
            const auto* ui=player->ui;
            const bool followup_message=tutorial_followup_message(ui);
            if (c.tutorial_input_released && followup_message) c.tutorial_followup_seen=true;
            c.tutorial_followup_cleared|=c.tutorial_followup_seen && !followup_message;
            c.tutorial_stick_after_message|=c.tutorial_followup_cleared &&
                (player->input.stick_x>=6 || player->input.stick_x<=-6 || player->input.stick_y>=6 || player->input.stick_y<=-6);
            if (status!=isle::beach::LifecycleStatus::retiring)
                ++c.phases[c.outro_seen?3:!c.intro_finished?0:
                    (!c.tutorial_input_released || followup_message)?1:2];
            course_log_ << '\t' << player->IsInputDisabled << '\t' << int(player->IsTutorialEnabled)
                << '\t' << int(player->D_803AE516_54E926) << '\t' << player->gDirectionIndex
                << '\t' << player->TargetDirectionZoomedIn << '\t' << player->sTimerAfterPhotoTaken
                << '\t' << photo.gPhotoCount << '\t' << photo.D_800AE27C
                << '\t' << player->movement.move_time << '\t' << player->movement.move_speed
                << '\t' << position.x << '\t' << position.y << '\t' << position.z << '\t' << player->ProgressFlags
                << '\t' << (ui?ui->Msg_IsPrinted:0) << '\t' << (ui?ui->Msg_IsInteractive:0)
                << '\t' << player->input.current_buttons << '\t' << player->input.pressed_buttons
                << '\t' << player->input.stick_x << '\t' << player->input.stick_y;
        } else course_log_ << "\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA\tNA";
        course_log_ << '\t' << ending_sound << '\t' << outro_cart_present << '\t' << outro_terminal
            << '\t' << controller_.buttons << '\t' << int(controller_.stick_x) << '\t' << int(controller_.stick_y) << '\n';
        ++c.source_rows;
        require(bool(course_log_),"actual course Source observation ledger failed");
    }
    static bool tutorial_message_displayed(const IslePlayerUiSource* ui) noexcept {
        if (!ui || ui->Msg_MessageLength<=0 ||
            static_cast<std::size_t>(ui->Msg_MessageLength)>=sizeof(ui->Msg_PrintedText)) return false;
        const auto length=static_cast<std::size_t>(ui->Msg_MessageLength);
        // Original msg.c329-375: zero-delay tutorials copy complete display
        // bytes without setting the delayed printer's Msg_IsPrinted flag.
        if (ui->Msg_PrintDelay!=0 || ui->Msg_MessageText[length]!=0 || ui->Msg_PrintedText[length]!=0 ||
            !std::equal(ui->Msg_MessageText,ui->Msg_MessageText+length,ui->Msg_PrintedText)) return false;
        const std::string_view message(reinterpret_cast<const char*>(ui->Msg_MessageText),length);
        return message=="Press \\z to aim." || message=="Press \\a to shoot." ||
            message.starts_with("Try to take a lot of Pok") || message=="Please use the Control Stick.";
    }
    static bool tutorial_followup_message(const IslePlayerUiSource* ui) noexcept {
        if (!tutorial_message_displayed(ui)) return false;
        // Verified original player.c:1875–1879. Prefix avoids depending on the
        // native font encoding of the one non-ASCII character in "Pokemon".
        constexpr std::string_view pictures="Try to take a lot of Pok",stick="Please use the Control Stick.";
        return std::equal(pictures.begin(),pictures.end(),ui->Msg_MessageText) ||
            std::equal(stick.begin(),stick.end(),ui->Msg_MessageText);
    }
    void drive_course(const IslePlayerSource& player) {
        const auto view=application_.view();
        require(view.beach && player.photo,"course controller lost its actual source observation");
        const auto prior=controller_.buttons;
        controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
        // A/Start can skip the original intro. Keep the pad released through
        // its actual animation terminal and original show-controls readiness.
        const auto& beach=view.beach->source();
        auto& evidence=e_.course;
        ++evidence.drive_calls;
        if (view.beach->status()!=isle::beach::LifecycleStatus::running) {++evidence.gate_running;return;}
        if (beach.D_beach_802CC0E0!=1) {++evidence.gate_intro;return;}
        if (!player.D_80382D04_523114) {++evidence.gate_controls;return;}
        if (evidence.outro_seen) {++evidence.gate_outro;return;}
        if (player.IsTutorialEnabled && player.photo->D_800AE27C<3) {
            // Like a real player, observe the actual presented tutorial before
            // supplying its Z/A response. A ready Source string is not proof
            // that an asynchronous GPU/window has shown that message yet.
            if (!evidence.captured[0]) return;
            controller_.buttons=button_z;
            IslePlayerPhotoFocus focus{};
            require(isle_player_photo_published_focus(player.photo,&focus)==ISLE_GAME_OK,
                "actual course published Photo focus observation failed");
            evidence.last_focus_id=focus.pokemon_id;
            if (focus.present) ++evidence.focus_present;
            if (focus.present && focus.object && focus.pokemon_id>0 && focus.pokemon_id<=151) ++evidence.focus_subject;
            if (focus.present && focus.object && focus.pokemon_id>0 && focus.pokemon_id<=151 && (focus.flags&4u))
                ++evidence.focus_qualified;
            if (player.gDirectionIndex==-1 && player.TargetDirectionZoomedIn==0) ++evidence.zoom_ready;
            if (player.sTimerAfterPhotoTaken<0) ++evidence.shutter_ready;
            if (!(prior&button_a) && focus.present && focus.object && focus.pokemon_id>0 && focus.pokemon_id<=151 &&
                (focus.flags&4u) && player.gDirectionIndex==-1 && player.TargetDirectionZoomedIn==0 &&
                player.sTimerAfterPhotoTaken<0) {
                controller_.buttons|=button_a;++e_.course.shutter_presses;
            }
            return;
        }
        if (player.photo->gPhotoCount<4 && !player.IsInputDisabled) controller_.buttons=button_z;
        if (player.photo->gPhotoCount<4 && !player.IsInputDisabled && !(prior&button_a) &&
            player.sTimerAfterPhotoTaken<0 && player.gDirectionIndex==-1 && player.TargetDirectionZoomedIn==0) {
            controller_.buttons=button_z|button_a;++evidence.shutter_presses;return;
        }
        if (player.IsTutorialEnabled && !player.D_803AE516_54E926) {
            // IsTutorialEnabled is a course-long initialization choice, not a
            // completion flag. Original C owns its real90/600 waits and joins.
            // Supply actual raw stick8 across the native follow-up message's
            // clear/unpause boundary; no consumer update counter drives it.
            if (e_.course.tutorial_followup_seen && !e_.course.tutorial_stick_after_message)
                controller_.stick_x=8;
            if (!(prior&button_a) && tutorial_followup_message(player.ui))
                controller_.buttons=button_a;
        }
        // Otherwise leave movement to original Player/World course speed and
        // ending1001. No dash, quit, end callback or progress/movement writes.
    }
    void offer(std::uint64_t ticks) {
        if (!ticks) return;
        const auto result=timeline_.progress.offer_retraces(ticks);
        require(result==h::ClockError::none || result==h::ClockError::full,"real retrace GTL admission failed");
    }
    void record_completion(const Tick& tick) {
        require(tick.completed && tick.completion_accepted && tick.completion.error==in::Error::none && !input_.pending_read(),
            "actual owned software read completion rejected");
        require(timeline_.progress.status()==h::ProgressStatus::running,timeline_.progress.message());
        ++e_.read_completions;
        if (tick.completion.acknowledged) ++e_.publication_acknowledgements;
    }
    void complete_read() {
        require(bool(input_.pending_read()),"owned software acquisition lost its actual read token");
        record_completion(timeline_.progress.complete_input());
    }
    void acquire(const Tick& tick) {
        if (tick.requested) {
            require(tick.requested.owner==&input_ && tick.requested.serial>last_read_serial_,
                "actual Input retrace read request rejected or reused");
            last_read_serial_=tick.requested.serial;++e_.read_requests;
            if (!held_read_) held_read_=tick.requested;
        }
        if (const auto pending=input_.pending_read())
            require(pending.owner==&input_ && pending.serial==last_read_serial_,
                "in-flight acquisition identity changed without completion");
        e_.read_skips+=tick.read_skips;
        if (tick.completed) record_completion(tick);
    }
    void inspect_backpressure(const Tick& tick) {
        if (backpressure_==Backpressure::offered) {
            const auto view=application_.view();
            require(scene_==0 && view.startup && view.startup->updates()==0 && clock_.stats().updates==0 &&
                clock_.buffered_retraces()==0 && clock_.stats().submissions==0 && !input_.pending_update() &&
                input_.pending_read()==held_read_ && e_.read_requests==1 && clock_.stats().dropped_retraces>=4,
                "Runtime did not retain its genuinely claimed GTL update behind the withheld Input read");
            claim_ordinal_=timeline_.progress.ordinal();claim_audio_=timeline_.host.audio()->accepted_frames();
            e_.blocked_claim=true;backpressure_=Backpressure::claimed;
        } else if (backpressure_==Backpressure::claimed) {
            const auto view=application_.view();
            require(view.startup && view.startup->updates()==0 && clock_.stats().updates==0 &&
                clock_.stats().submissions==0 && input_.pending_read()==held_read_ && e_.read_requests==1 &&
                clock_.buffered_retraces()<=4 && !input_.pending_update(),
                "withheld acquisition advanced Source/GTL, duplicated a read or lost its update claim");
            if (timeline_.progress.ordinal()-claim_ordinal_>=8 && timeline_.host.audio()->accepted_frames()>claim_audio_) {
                e_.input_hold_ticks=timeline_.progress.ordinal()-claim_ordinal_;
                e_.input_hold_audio=timeline_.host.audio()->accepted_frames()-claim_audio_;
                require(e_.read_skips>=7,"coalesced actual retraces did not exercise single-read-in-flight admission");
                complete_read();
                require(input_.needs_publication(),"real held read did not retain its unpublished sample");
                require(application_.pump(Timeline::raw(tick)),application_.message());
                const auto resumed=application_.view();
                require(resumed.startup && resumed.startup->updates()==1 && clock_.stats().updates==1 &&
                    !input_.pending_read() && !input_.pending_update() && !input_.needs_publication(),
                    "actual read completion failed to resume exactly the retained Source/GTL update");
                observe_update();e_.resumed_claim=true;backpressure_=Backpressure::normal;
            }
        }
    }
    void flag16(bool expected) {
        const auto snapshot=state_.save()->snapshot();
        require(bool(snapshot),"actual profile snapshot unavailable");
        const auto flag=snapshot.value.player_flag(16);
        require(flag && flag.value==expected,"PFID16 crossed the wrong actual Name retirement boundary");
        require(snapshot.value.generation()==0,"new-game/Name/Oak implicitly committed a durable profile");
    }
    void inspect_entry() {
        const auto view=application_.view();
        if (!view.entry) return;
        const auto entry=*view.entry; // Value only; RuntimeView pointers expire at the next pump.
        if (entry.serial==last_entry_) return;
        if (e_.mode==Mode::native_course && e_.entries>=4) {
            constexpr std::array course_operations{ISLE_APPLICATION_ENTRY_BEACH,ISLE_APPLICATION_ENTRY_TUNNEL,
                ISLE_APPLICATION_ENTRY_VOLCANO,ISLE_APPLICATION_ENTRY_RIVER,ISLE_APPLICATION_ENTRY_CAVE,
                ISLE_APPLICATION_ENTRY_VALLEY,ISLE_APPLICATION_ENTRY_RAINBOW};
            require(entry.serial>last_entry_ && state_.initialized() && state_.save()==persistent_save_ &&
                clock_.stats().submissions==clock_.stats().terminals,
                "actual course Manager handoff changed canonical profile or bypassed GPU retirement");
            if(e_.entries==4) {
                require(entry.argument==std::uint32_t(e_.requested_course) &&
                    entry.operation==course_operations[std::size_t(e_.requested_course)] && e_.oak_result,
                    "actual Oak course selection did not reach requested Original course owner");
                flag16(true);
            } else require(e_.entries==5 && view.post && entry.operation==ISLE_APPLICATION_ENTRY_CAMERA_CHECK && entry.argument==9,
                "natural Source course result did not admit actual Manager CameraCheck9");
            ++e_.entries;scene_=scene_count+2;last_entry_=entry.serial;
            controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
            course_probe_->update(view,e_.course_updates);
            if(e_.entries==6) {
                require(course_probe_->completed(),"CameraCheck9 admission lacks actual joined natural Source course receipt");
                e_.course_api_completed=true;stop_requested_=true;
            }
            std::cout<<"original_course_entry serial="<<entry.serial<<" operation="<<int(entry.operation)
                <<" argument="<<entry.argument<<" requested_course="<<e_.requested_course
                <<" naturally_completed="<<e_.course_api_completed<<std::endl;
            return;
        }
        if (menu_mode(e_.mode) && e_.entries>=4) {
            require(entry.serial>last_entry_ && e_.menu_entries<32 &&
                state_.initialized() && state_.save()==persistent_save_ &&
                clock_.stats().submissions==clock_.stats().terminals,
                "original menu Manager entry regressed, exceeded its bound or bypassed canonical/GPU ownership");
            const bool title=entry.operation==ISLE_APPLICATION_ENTRY_TITLE && entry.argument==8 && view.startup;
            const bool oak=entry.operation==ISLE_APPLICATION_ENTRY_OAKS_LAB && entry.argument==13 && view.oak;
            const bool menu=view.post && (entry.argument==12 || entry.argument==14 || entry.argument==22 ||
                entry.argument==23 || entry.argument==24 || (entry.argument>=17 && entry.argument<=20));
            require(title || oak || menu,"original Manager selected an unexpected authored-save menu route");
            ++e_.entries;++e_.menu_entries;scene_=scene_count+2;last_entry_=entry.serial;menu_visit_updates_=0;
            gallery_title_stage_=0;gallery_title_stable_=0;
            controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
            if ((e_.menu_qualified&menu_target(e_.mode))==menu_target(e_.mode)) stop_requested_=true;
            std::cout<<"original_menu_entry serial="<<entry.serial<<" operation="<<int(entry.operation)
                <<" argument="<<entry.argument<<" qualified_mask="<<e_.menu_qualified<<std::endl;
            return;
        }
        if (e_.entries>=scene_count) {
            const auto index=static_cast<std::size_t>(e_.entries-scene_count);
            require(e_.mode==Mode::course && index<3 && entry.serial>last_entry_,
                "unexpected original Manager post-course entry");
            require(index==0?(entry.operation==ISLE_APPLICATION_ENTRY_CAMERA_CHECK && entry.argument==9 && view.post &&
                e_.course.post_scene_verified && e_.course.retired_result==9):
                index==1?(entry.operation==ISLE_APPLICATION_ENTRY_PHOTO_CHECK && entry.argument==15 && view.post &&
                    e_.post.joined[0] && e_.post.results[0]==15):
                    (entry.operation==ISLE_APPLICATION_ENTRY_OAKS_LAB && entry.argument==13 && view.oak &&
                        e_.post.joined[1] && e_.post.results[1]==13),
                "original post-course result/typed owner/retirement/Manager handoff mismatch");
            ++e_.entries;scene_=scene_count+index;last_entry_=entry.serial;
            post_pulse_=PostPulse::releasing;
            post_stage_=index==0?PostStage::choose:index==1?PostStage::photo_intro:PostStage::oak_admitted;
            controller_.buttons=0;controller_.stick_x=controller_.stick_y=0;
            require(clock_.stats().submissions==clock_.stats().terminals,"post-course Manager bypassed actual GPU terminal");
            if (index<2) {
                e_.post.serial[index]=entry.serial;
                if (index==0) {
                    e_.course.next_serial=entry.serial;e_.course.next_operation=entry.operation;e_.course.next_argument=entry.argument;
                    read_report(e_.post.report_before,e_.post.score_before);
                }
                read_report(e_.post.report_after,e_.post.score_after);
            } else {
                e_.post.oak_serial=entry.serial;
                read_report(e_.post.report_after,e_.post.score_after);
                e_.post.returned=true;stop_requested_=true;
            }
            write_post_row("ManagerEntry",entry.serial,0);
            return;
        }
        require(e_.entries<scene_count && entry.serial>last_entry_ && entry.operation==operations[e_.entries],
            ("original Manager entry sequence was skipped, replayed or substituted: actual serial="+
             std::to_string(entry.serial)+" operation="+std::to_string(int(entry.operation))+
             " argument="+std::to_string(entry.argument)+" accepted_entries="+std::to_string(e_.entries)+
             " retired_Beach_result="+std::to_string(e_.course.retired_result)+
             " retired_Beach_output="+std::to_string(e_.course.retired_output_scene)+
             " verified_retirement="+std::to_string(e_.course.post_scene_verified)+
             " end_reason="+std::to_string(e_.course.retired_end_reason)+
             " photos="+std::to_string(e_.course.photos)+" qualifying="+std::to_string(e_.course.qualifying_photos)+
             " shutter_presses="+std::to_string(e_.course.shutter_presses)+
             " drive="+std::to_string(e_.course.drive_calls)+" gate_intro="+std::to_string(e_.course.gate_intro)+
             " gate_controls="+std::to_string(e_.course.gate_controls)+" gate_outro="+std::to_string(e_.course.gate_outro)+
             " focus_present="+std::to_string(e_.course.focus_present)+" focus_subject="+std::to_string(e_.course.focus_subject)+
             " focus_qualified="+std::to_string(e_.course.focus_qualified)+" zoom_ready="+std::to_string(e_.course.zoom_ready)+
             " cooldown_ready="+std::to_string(e_.course.shutter_ready)+
             " last_direction="+std::to_string(e_.course.last_direction)+" last_zoom="+std::to_string(e_.course.last_zoom_target)+
             " last_cooldown="+std::to_string(e_.course.last_cooldown)+" last_disabled="+std::to_string(e_.course.last_input_disabled)+
             " last_controls="+std::to_string(e_.course.last_controls)+" last_focus="+std::to_string(e_.course.last_focus_id)+
             " last_buttons="+std::to_string(e_.course.last_source_buttons)).c_str());
        scene_=static_cast<std::size_t>(e_.entries++);last_entry_=entry.serial;stable_update_=0;controller_.buttons=0;
        controller_.stick_x=controller_.stick_y=0;
        require(unsigned(bool(view.startup))+unsigned(bool(view.name))+unsigned(bool(view.oak))+unsigned(bool(view.beach))==1,
            "Runtime exposed overlapping or missing actual scene owners");
        if (scene_==0) {
            require(view.startup && view.startup->phase()==startup::Phase::logo && !state_.initialization_attempted(),
                "original Boot did not precede profile initialization");
        } else {
            require(state_.initialized() && state_.save() && state_.save()==persistent_save_ &&
                state_.save_initialization()==initialization_ && clock_.stats().submissions==clock_.stats().terminals,
                "original scene handoff replaced the profile or bypassed actual prior graphics retirement");
            if (scene_==1) {
                require(entry.argument==8 && view.startup && e_.boot_complete,"original Boot did not dispatch actual Title8");flag16(false);
            } else if (scene_==2) {
                require(entry.argument==0 && view.name && e_.menu_result && (state_.scene_notices()&1u),
                    "original new-game menu result0/notice1 did not dispatch Name0");flag16(false);
            } else if (scene_==3) {
                require(entry.argument==11 && view.oak && saved_name_ && (state_.scene_notices()&1u),
                    "actual Name result11 did not dispatch first-meet Oak");flag16(true);
                const auto snapshot=state_.save()->snapshot();
                require(snapshot && std::equal(snapshot.value.player_name().begin(),snapshot.value.player_name().end(),
                    reinterpret_cast<const std::byte*>(confirmed_name_.data())),"Name/Oak handoff lost actual full16-byte source name");
            } else {
                require(entry.argument==0 && view.beach && e_.oak_result && !(state_.scene_notices()&1u),
                    "confirmed original Oak course0 did not enter actual Beach0");flag16(true);
                const auto& source=view.beach->source();
                require(view.beach->status()==isle::beach::LifecycleStatus::running && source.status==ISLE_GAME_OK &&
                    source.running && source.scene_requested && !source.completed && !source.scene_completion_ready &&
                    source.player && source.player->game && source.player->object && source.player->camera && source.player->photo &&
                    source.screen.width==320 && source.screen.height==240 && source.scene_setup.contexts==2 && !view.beach->result(),
                    "Beach ingress lacks genuine original C setup, Root/camera/Photo owners or a running native scene");
            }
        }
    }
    void observe_update() {
        if (scene_>=scene_count) {
            const auto view=application_.view();
            if (e_.mode==Mode::native_course) {
                ++e_.course_updates;
                course_probe_->update(view,e_.course_updates);
                return;
            }
            if (menu_mode(e_.mode)) {
                ++e_.menu_updates;++menu_visit_updates_;
                require(view.entry,"actual menu update lost its original Manager entry");
                if (view.post) menu_probe_->update(*view.post,static_cast<std::int32_t>(view.entry->argument),menu_visit_updates_);
                else if (view.oak) drive_oak(*view.oak);
                else if (view.startup) drive_gallery_title(*view.startup);
                return;
            }
            if (view.post) drive_post(*view.post);
            return;
        }
        ++e_.updates[scene_];
        const auto view=application_.view();
        if (scene_==0) {
            require(view.startup,"Boot update lost actual startup owner");
            if (view.startup->updates()==1) {
                require(!state_.initialization_attempted() && view.startup->save_initialization()==nullptr,
                    "actual Boot initialized the profile before its first source wait");e_.boot_wait=true;
            }
            if (view.startup->updates()==2) {
                require(state_.initialized() && state_.save_initialization() && *state_.save_initialization(),
                    "actual Boot did not initialize its missing-only profile after the first wait");
                persistent_save_=state_.save();initialization_=state_.save_initialization();
            }
            if (view.startup->phase()==startup::Phase::boot_complete) e_.boot_complete=true;
        } else if (scene_==1) drive_menu(*view.startup);
        else if (scene_==2) drive_name(*view.name);
        else if (scene_==3) drive_oak(*view.oak);
        else {
            require(view.beach && (view.beach->status()==isle::beach::LifecycleStatus::running ||
                (e_.mode==Mode::course && view.beach->status()==isle::beach::LifecycleStatus::retiring)),
                "Beach unexpectedly completed or failed instead of running original native C updates");
            const auto* player=view.beach->source().player;
            require(player && player->status==ISLE_GAME_OK,"actual Beach Root Source failed");
            const IsleGameCameraParameters* parameters=nullptr;
            require(isle_game_camera_parameters(player->game,player->camera,&parameters)==ISLE_GAME_OK && parameters,
                "actual Beach camera observation missing");
            std::array<float,6> camera{};
            std::copy_n(parameters->look_at.eye,3,camera.begin());
            std::copy_n(parameters->look_at.at,3,camera.begin()+3);
            if (have_camera_ && camera!=last_camera_) ++e_.beach_camera_changes;
            last_camera_=camera;have_camera_=true;
            if (e_.mode==Mode::course) drive_course(*player);
        }
    }
    void drive_menu(const startup::Runtime& runtime) {
        const auto* ui=runtime.menu_ui();
        require(ui,"actual Title update lost source UI");
        const auto state=ui->source_state();
        const auto updates=e_.updates[1];
        const auto press=[&](std::uint16_t buttons,unsigned next) {controller_.buttons=buttons;menu_stage_=next;stable_update_=updates;};
        if (menu_stage_==0 && e_.captured[1]) press(button_start,1);
        else if (menu_stage_==1 && state==2) press(0,2);
        else if (menu_stage_==2 && updates-stable_update_>=30) press(button_a,3);
        else if (menu_stage_==3 && state==3) press(0,4);
        else if (menu_stage_==4 && updates-stable_update_>=30 && e_.captured[2]) press(button_b,5);
        else if (menu_stage_==5 && state==2) press(0,6);
        else if (menu_stage_==6 && updates-stable_update_>=30) press(button_start,7);
        else if (menu_stage_==7 && state==3) press(0,8);
        else if (menu_stage_==8 && updates-stable_update_>=30) press(button_a,9);
        if (runtime.phase()==startup::Phase::scene_exit) {
            require(menu_stage_==9 && runtime.exit().scene==isle::menu::scene::SourceScene::beach &&
                runtime.exit().entry_flags==1 && (state_.scene_notices()&1u),
                "actual fresh new-game menu did not emit Source result0/notice1");
            controller_.buttons=0;e_.menu_result=true;
        }
    }
    void drive_name(const isle::new_game::Scene& name) {
        const auto view=name.observation();
        flag16(false);
        if (name_stage_==0 && view.controls_ready && e_.captured[3]) {controller_.buttons=button_b;name_stage_=1;}
        else if (name_stage_==1 && view.row==19 && view.column==2) {controller_.buttons=0;name_stage_=2;stable_update_=e_.updates[2];}
        else if (name_stage_==2 && e_.updates[2]-stable_update_>=2) {controller_.buttons=button_a;name_stage_=3;}
        if (view.confirming && !view.confirmed) {
            require(name_stage_==3,"actual empty-name End confirmation bypassed UI navigation");
            controller_.buttons=0;e_.name_confirming=true;
        }
        if (view.confirmed) {
            require(e_.name_confirming && view.name[0]!=0 && view.name[8]==0,
                "actual Name skipped its one-update confirmation/default-name rule");
            confirmed_name_=view.name;saved_name_=true;e_.name_confirmed=true;controller_.buttons=0;
            if(e_.mode==Mode::native_course && !e_.authored_course_profile) {
                isle::private_course::seed_progress(state_,e_.requested_course);
                e_.authored_course_profile=true;
            }
            if (menu_mode(e_.mode) && !e_.authored_menu_inputs) {
                // Explicit authored canonical saved-profile inputs, installed
                // outside Source after real Name confirmation, before Oak's
                // owner exists. Manager entries and SDK output remain original.
                app::Photos photos(snapshot_,*state_.save());
                isle::private_post_course::seed(photos,state_);
                require(!state_.save()->set_player_flag(7,true),"authored Report-unlocked flag rejected");
                const auto credit=credit_argument(e_.mode);
                if (credit==17) {
                    require(!state_.save()->set_player_flag(14,true),"authored completed-profile flag rejected");
                    state_.add_scene_notices(0x80000);
                } else if (credit==18) state_.add_scene_notices(0x40000);
                else if (credit==19) state_.add_scene_notices(0x40);
                e_.authored_menu_inputs=true;
            }
        }
    }
    void drive_gallery_title(const startup::Runtime& runtime) {
        const auto* ui=runtime.menu_ui();
        require(ui,"actual saved-profile Title lost its original menu UI");
        const auto source=ui->source_state();
        const auto press=[&](std::uint16_t buttons,unsigned stage) {
            controller_.buttons=buttons;gallery_title_stage_=stage;gallery_title_stable_=menu_visit_updates_;
        };
        if (gallery_title_stage_==0 && runtime.updates()>=100) press(button_start,1);
        else if (gallery_title_stage_==1 && source==2) press(0,2);
        else if (gallery_title_stage_==2 && menu_visit_updates_-gallery_title_stable_>=30) press(button_a,3);
        else if (gallery_title_stage_==3 && source==3) press(0,4);
        else if (gallery_title_stage_==4 && menu_visit_updates_-gallery_title_stable_>=30) {
            controller_.stick_y=-60;gallery_title_stage_=5;gallery_title_stable_=menu_visit_updates_;
        } else if (gallery_title_stage_==5) {
            controller_.stick_y=0;gallery_title_stage_=6;gallery_title_stable_=menu_visit_updates_;
        } else if (gallery_title_stage_==6 && menu_visit_updates_-gallery_title_stable_>=30) press(button_a,7);
        if (runtime.phase()==startup::Phase::scene_exit) {
            require(gallery_title_stage_==7 && runtime.exit().scene==isle::menu::scene::SourceScene::gallery,
                "actual saved-profile Main Menu did not select original Gallery22");
            controller_.buttons=0;controller_.stick_y=0;
        }
    }
    void drive_oak(const isle::oaks_lab::Scene& oak) {
        using Phase=isle::oaks_lab::Phase;
        const auto view=oak.view();
        if (menu_mode(e_.mode) || e_.mode==Mode::native_course) {
            ++oak_input_updates_;
            if (menu_oak_phase_!=int(view.phase) || menu_oak_slot_!=view.selected_slot) {
                menu_oak_phase_=int(view.phase);menu_oak_slot_=view.selected_slot;
                std::cout<<"menu_root_oak phase="<<menu_oak_phase_<<" slot="<<menu_oak_slot_
                    <<" commit_attempts="<<view.commit_attempts<<" result="<<view.source_result
                    <<" requested_buttons="<<controller_.buttons<<" requested_y="<<int(controller_.stick_y)<<std::endl;
            }
            if (view.exit_requested) {
                const auto expected=credit_argument(e_.mode);
                require(view.phase==Phase::complete &&
                    (e_.mode==Mode::native_course?view.source_result==e_.requested_course:
                     expected?view.source_result==expected:
                        view.source_result==12 || view.source_result==14 || view.source_result==8),
                    "actual Oak emitted an unexpected authored-save menu/credits result");
                controller_.buttons=0;controller_.stick_y=0;e_.oak_result=true;return;
            }
            if (oak_hold_until_ && oak_input_updates_>=oak_hold_until_) {
                controller_.buttons=0;controller_.stick_y=0;oak_hold_until_=0;
            }
            if (oak_hold_until_ || oak_input_updates_<oak_next_command_) return;
            const auto pulse=[&](std::uint16_t buttons,std::int8_t y=0) {
                controller_.buttons=buttons;controller_.stick_y=y;
                oak_hold_until_=oak_input_updates_+3;oak_next_command_=oak_input_updates_+24;
            };
            // A real held edge followed by released updates cannot leak the
            // last dialogue acknowledgment into a newly entered Lab menu.
            if (view.phase==Phase::page_wait || view.phase==Phase::save_prompt) pulse(button_a);
            else if(e_.mode==Mode::native_course && view.phase==Phase::courses) {
                if(view.selected_slot!=e_.requested_course)pulse(0,-60);
                else pulse(button_a);
            } else if(e_.mode==Mode::native_course && view.phase==Phase::confirmation)pulse(button_a);
            else if (view.phase==Phase::repeat_question || view.phase==Phase::keep_going ||
                     view.phase==Phase::courses || view.phase==Phase::confirmation) pulse(button_b);
            else if (view.phase==Phase::lab && e_.captured[4]) {
                require(!credit_argument(e_.mode),"authored completion notices did not select actual Oak credits route");
                const int target=e_.mode==Mode::native_course?0:!(e_.menu_qualified&1u)?1:!(e_.menu_qualified&2u)?6:7;
                if (view.selected_slot!=target) pulse(0,-60);
                else pulse(button_a);
            }
            return;
        }
        require(view.commit_attempts==0,"actual first-meet/course navigation implicitly saved");
        if (view.exit_requested) {
            require(oak_stage_==7 && view.phase==Phase::complete && view.source_result==0 &&
                view.first_meet_consumed && !(state_.scene_notices()&1u),"actual Oak emitted an unconfirmed/non-Beach source result");
            controller_.buttons=0;e_.oak_result=true;return;
        }
        // Every button press is separated by a genuine native update publishing
        // a released pad. No Scene method, Source record or result is modified.
        if (controller_.buttons) {controller_.buttons=0;return;}
        if (oak_stage_==0) {
            if (view.phase==Phase::dialogue || view.phase==Phase::page_wait) controller_.buttons=button_a;
            else if (view.phase==Phase::repeat_question) controller_.buttons=button_b;
            else if (view.phase==Phase::lab && e_.captured[4]) {controller_.buttons=button_a;oak_stage_=1;}
        } else if (oak_stage_==1 && view.phase==Phase::courses) {controller_.buttons=button_b;oak_stage_=2;}
        else if (oak_stage_==2 && view.phase==Phase::lab) {controller_.buttons=button_a;oak_stage_=3;}
        else if (oak_stage_==3 && view.phase==Phase::courses) {
            require(view.selected_slot==0,"fresh source profile selected a non-Beach course");controller_.buttons=button_a;oak_stage_=4;
        } else if (oak_stage_==4 && view.phase==Phase::confirmation) {controller_.buttons=button_b;oak_stage_=5;}
        else if (oak_stage_==5 && view.phase==Phase::courses) {controller_.buttons=button_a;oak_stage_=6;}
        else if (oak_stage_==6 && view.phase==Phase::confirmation) {controller_.buttons=button_a;oak_stage_=7;}
    }
    int capture_candidate(std::span<const std::byte> rgba) const {
        const auto view=application_.view();
        if (scene_==0 && !e_.captured[0] && view.startup && view.startup->logo_counter()>=10 && view.startup->logo_counter()<60 &&
            varied(rgba,0,0,320,240)) return 0;
        if (scene_==1 && view.startup && view.startup->menu_ui()) {
            const auto state=view.startup->menu_ui()->source_state();
            if (!e_.captured[1] && state==0 && view.startup->updates()>=100 && varied(rgba,0,0,320,240)) return 1;
            if (!e_.captured[2] && state==3 && e_.updates[1]-stable_update_>=30 && varied(rgba,0,0,320,240)) return 2;
        }
        if (scene_==2 && !e_.captured[3] && view.name && view.name->observation().controls_ready &&
            varied(rgba,23,20,83,220) && varied(rgba,96,21,280,120)) return 3;
        if (scene_==3 && !e_.captured[4] && view.oak && view.oak->view().printed_characters>=4 &&
            varied(rgba,100,34,192,120) && varied(rgba,104,173,296,220)) return 4;
        if (scene_==4 && !e_.captured[5] && e_.updates[4]>=16 && e_.beach_camera_changes && varied(rgba,0,0,320,240)) return 5;
        return -1;
    }
    int course_capture_candidate(std::span<const std::byte> rgba) const {
        if (e_.mode!=Mode::course || scene_!=4 || !varied(rgba,0,0,320,240)) return -1;
        const auto view=application_.view();
        if (!view.beach || !view.beach->source().player) return -1;
        const auto& player=*view.beach->source().player;
        if (!e_.course.captured[0] && e_.course.intro_finished && player.IsTutorialEnabled &&
            tutorial_message_displayed(player.ui)) return 0;
        if (!e_.course.captured[1] && e_.course.tutorial_input_released && e_.course.tutorial_stick_after_message &&
            !e_.course.outro_seen && e_.course.movement_changes>0) return 1;
        if (!e_.course.captured[2] && e_.course.outro_seen) return 2;
        return -1;
    }
    void consume_frame(const Tick& tick) {
        const auto frame=application_.completed_frame();
        if (!frame) return;
        require(frame.width==320 && frame.height==240 && frame.stride==1280 && frame.rgba.size()==320u*240u*4u && frame.ordinal,
            "actual Runtime completion lacks a complete new canonical RGBA frame");
        const bool fresh=last_frame_entry_!=last_entry_ || last_frame_ordinal_!=frame.ordinal;
        if (fresh) {
            require(!holding_frame_,"Runtime replaced the caller's unreleased completed frame");
            if (last_frame_entry_==last_entry_) require(frame.ordinal>last_frame_ordinal_,"actual canonical frame ordinal regressed");
            last_frame_entry_=last_entry_;last_frame_ordinal_=frame.ordinal;
            if (scene_<scene_count) ++e_.frames[scene_];
            else if (scene_<scene_count+2) ++e_.post.frames[scene_-scene_count];
            if (menu_mode(e_.mode) && scene_>=scene_count) ++e_.menu_frames;
            if(course_probe_ && scene_>=scene_count)++e_.course_frames;
            if (!timeline_.host.presentation_pending()) {
                const int capture=capture_candidate(frame.rgba);
                const auto menu_view=application_.view();
                if (menu_probe_ && menu_view.post)
                    menu_probe_->frame(*menu_view.post,static_cast<std::int32_t>(menu_view.entry->argument),frame.ordinal);
                if(course_probe_ && scene_>=scene_count && !menu_view.post)
                    course_probe_->frame(menu_view,frame.ordinal);
                require(timeline_.host.queue_present(frame.rgba,frame.screen_offsets),timeline_.host.message());
                int post_capture=-1;
                const auto view=application_.view();
                if (view.post) {
                    if (view.post->camera_source()) {
                        if (e_.post.marked>0) post_capture=1;
                        else if (intro_ready_ && !e_.post.captured[0]) {
                            if (!e_.post.frame_floor[0]) e_.post.frame_floor[0]=frame.ordinal;
                            if (frame.ordinal>e_.post.frame_floor[0]) post_capture=0;
                        }
                    }
                    else if (const auto* photo=view.post->photo_source()) {
                        observe_photo_identity(*photo);read_report(e_.post.report_after,e_.post.score_after);
                        observe_photo_display(*photo);
                        // The first completed generation at readiness can
                        // predate the held Source state. Only a subsequent
                        // genuine canonical frame may carry its capture tag.
                        if (assessment_visible_) {
                            observe_capture_readiness(2,1);
                            if (!e_.post.frame_floor[2]) e_.post.frame_floor[2]=frame.ordinal;
                        }
                        if (summary_ready_ && !e_.post.frame_floor[3]) e_.post.frame_floor[3]=frame.ordinal;
                        if (!e_.post.captured[2] && assessment_visible_ && frame.ordinal>e_.post.frame_floor[2]) post_capture=2;
                        else if (!e_.post.captured[3] && summary_ready_ && frame.ordinal>e_.post.frame_floor[3]) post_capture=3;
                    }
                }
                display_=Display{scene_,capture,course_capture_candidate(frame.rgba),post_capture,frame.ordinal};
                if (menu_probe_ && menu_view.post) {
                    display_->menu_argument=static_cast<std::int32_t>(menu_view.entry->argument);
                    display_->menu_digest=digest(frame.rgba);
                    display_->menu_varied=varied(frame.rgba,0,0,320,240);
                }
                if(course_probe_ && scene_>=scene_count && !menu_view.post) {
                    display_->native_course=e_.requested_course;
                    display_->menu_digest=digest(frame.rgba);
                    display_->menu_varied=varied(frame.rgba,0,0,320,240);
                }
            } else ++e_.declined_frames;
            if (scene_==4 && !e_.frame_hold && e_.updates[4]>=16 && e_.beach_camera_changes &&
                (e_.captured[5] || (display_ && display_->capture==5)) && varied(frame.rgba,0,0,320,240)) {
                holding_frame_=true;held_frame_ordinal_=frame.ordinal;held_digest_=digest(frame.rgba);
                held_ordinal_=timeline_.progress.ordinal();held_audio_=timeline_.host.audio()->accepted_frames();held_stats_=clock_.stats();
            }
        }
        if (holding_frame_) {
            require(frame.ordinal==held_frame_ordinal_ && clock_.stats().updates==held_stats_.updates &&
                clock_.stats().submissions==held_stats_.submissions && clock_.stats().terminals==held_stats_.terminals &&
                clock_.buffered_retraces()<=4 && application_.view().beach && !application_.stopped(),
                "unreleased genuine Beach frame allowed Source/GTL progress or premature native/Photo retirement");
            if (timeline_.progress.ordinal()-held_ordinal_<8 || timeline_.host.audio()->accepted_frames()==held_audio_ || !e_.captured[5]) return;
            require(digest(frame.rgba)==held_digest_,"actual held completed pixels changed before caller release");
            e_.frame_hold=true;e_.frame_hold_ticks=timeline_.progress.ordinal()-held_ordinal_;
            e_.frame_hold_audio=timeline_.host.audio()->accepted_frames()-held_audio_;
            if (e_.mode==Mode::ingress) {
                application_.request_stop();
                require(application_.pump(Timeline::raw(tick)) && !application_.stopped() && application_.scene_active() &&
                    application_.completed_frame().ordinal==held_frame_ordinal_,
                    "actual stop bypassed the caller's retained completed GPU/Photo frame");
                e_.stop_while_held=true;stop_requested_=true;
            }
            holding_frame_=false;
        }
        require(application_.release_completed()==ISLE_GAME_OK,application_.message());
        ++e_.released_frames;
        require(!application_.completed_frame(),"Runtime retained completed frame access after actual caller release");
    }
    void present() {
        if (!timeline_.host.presentation_pending()) return;
        require(bool(display_),"copied presentation lost its original scene/capture tag");
        const auto result=timeline_.host.try_present();
        require(result!=g::Present::failed,timeline_.host.message());
        if (result==g::Present::completed) {
            if (display_->scene<scene_count) ++e_.presentations[display_->scene];
            else if (display_->scene<scene_count+2) ++e_.post.presentations[display_->scene-scene_count];
            if (menu_mode(e_.mode) && display_->scene>=scene_count) ++e_.menu_presentations;
            if (menu_probe_ && display_->menu_argument)
                menu_probe_->presented(display_->menu_argument,display_->frame,display_->menu_digest,display_->menu_varied);
            if(course_probe_ && display_->native_course>=0) {
                ++e_.course_presentations;
                course_probe_->presented(display_->native_course,display_->frame,display_->menu_digest,display_->menu_varied);
            }
            if (display_->capture>=0) {
                const auto capture=static_cast<std::size_t>(display_->capture);
                if (!e_.captured[capture]) {capture_window(root_/"proof"/captures[capture]);e_.captured[capture]=true;}
            }
            if (display_->course_capture>=0) {
                const auto capture=static_cast<std::size_t>(display_->course_capture);
                if (!e_.course.captured[capture]) {
                    capture_window(root_/"proof"/course_captures[capture]);e_.course.captured[capture]=true;
                }
            }
            if (display_->post_capture>=0) {
                const auto capture=static_cast<std::size_t>(display_->post_capture);
                if (!e_.post.captured[capture]) {
                    capture_window(root_/"proof"/post_captures[capture]);e_.post.captured[capture]=true;
                    const auto source=capture<2?0u:1u;
                    e_.post.capture_frame[capture]=display_->frame;
                    e_.post.capture_presentation[capture]=e_.post.presentations[source];
                    if (capture==0) e_.post.intro_presented=true;
                    if (capture==2) {
                        require(e_.post.assessment_progress==1,
                            "assessment presentation lost its ordered phase0 selected-photo provenance");
                        e_.post.assessment_progress=2;
                    }
                }
            }
            display_.reset();
        }
    }
};
int run(const std::filesystem::path& rom,const std::filesystem::path& root,Mode mode,std::int32_t requestedCourse) {
    for (const auto* name:{"SDL_AUDIO_DRIVER","SDL_AUDIODRIVER"}) {
        const char* value=std::getenv(name);
        require(value && std::string_view(value)=="dummy","private consumer requires both explicit SDL dummy audio variables");
    }
    require(root.is_absolute() && std::filesystem::is_directory(root) && !std::filesystem::is_symlink(root) && std::filesystem::is_empty(root),
        "NEW_OWNED_ROOT must be an existing empty absolute nonsymlink private directory");
    require(std::filesystem::create_directory(root/"host") && std::filesystem::create_directory(root/"profile") &&
        std::filesystem::create_directory(root/"proof"),"private Runtime child directories must be newly owned, never reused");
    Evidence evidence;
    evidence.mode=mode;
    evidence.requested_course=requestedCourse;
    if (mode==Mode::course) write_course_contract(root);
    std::string failure;
    const auto snapshot=isle::rom::read_snapshot(rom);
    auto hydrated=isle::menu::hydrate(snapshot);
    require(bool(hydrated),"actual complete startup catalog hydration failed");
    // Check real persisted-config precedence, as in startup_runtime_rom_smoke.
    {
        std::ofstream config(root/"host"/"35mmIsle.json");
        config << "{\"Window\":{\"Backend\":{\"Id\":123456},\"Width\":640,\"Height\":480}}\n";
        require(bool(config),"owned Host renderer-precedence config write failed");
    }
    g::Config options;options.data_directory=root/"host";options.input=g::InputMode::diagnostic_disabled;
    auto platform=g::Host::create(snapshot,std::move(options));
    require(bool(platform),platform.message.data());
    auto& host=*platform.host;
    SoftwareController controller;
    in::Service input;
    Timeline timeline{host,input,controller};
    isle::game::State state(root/"profile");
    try {
        require(SDL_GL_GetCurrentContext()!=nullptr,"actual required OpenGL override was lost during CVar reload");
        require(std::string_view(host.audio_info().driver.data())=="dummy" && host.audio_info().source.freq==32006 &&
            host.audio_info().source.channels==2 && host.audio_info().source.format==SDL_AUDIO_S16,
            "actual checked SDL output is not explicit dummy native S16 stereo at the source rate");
        const auto disabled=host.acquire_input();
        require(disabled.completed && disabled.controller_bits==0 &&
            std::all_of(disabled.ports.begin(),disabled.ports.end(),[](const auto& port) {
                return port.connection==in::Connection::absent && port.identity==0 && port.error==0 &&
                    port.buttons==0 && port.stick_x==0 && port.stick_y==0;
            }),"diagnostic-disabled Host invented physical/controller/accessory metadata");
        evidence.disabled_metadata=true;
        require(input.start(controller.sample())==in::Error::none,"explicitly owned software Input Service start failed");
        require(input.snapshot().controller_count==1 && input.snapshot().ports[0].identity==controller.sample()[0].identity &&
            input.snapshot().indices[0]==0,"owned software identity did not enter actual Input Service metadata");
        exercise_cold_stop(snapshot,*hydrated.catalog,state,input,controller,timeline,root,evidence);
        h::GtlClock clock({1,1,2,true,0});
        std::unique_ptr<app::Runtime> application;
        try {
            auto made=app::Runtime::create(snapshot,*hydrated.catalog,state,input,*host.audio(),clock,
                {true,&evidence,observe_retirement});
            require(bool(made),made.message.data());
            application=std::move(made.runtime);
            Scenario scenario(*application,clock,input,controller,state,timeline,root,evidence,snapshot);
            scenario.run();
        } catch (const std::exception& error) { remember_failure(failure,error.what()); }
          catch (...) { remember_failure(failure,"unknown actual Runtime scenario failure"); }
        close_application(application,clock,input,controller,timeline,root,evidence,failure);
        require(!application,"closed actual Runtime still owns its Manager or native scenes");
    } catch (const std::exception& error) { remember_failure(failure,error.what()); }
      catch (...) { remember_failure(failure,"unknown actual Runtime setup failure"); }
    // No Runtime, Manager, native scene, graphics or Photo owner remains before
    // the real Input cancellation and independent Audio stop/stream drain.
    if (input.running()) {
        const auto stopped=input.stop();
        evidence.input_stopped=stopped.error==in::Error::none && !input.running() && !input.pending_read() && !input.pending_update();
        if (!evidence.input_stopped) remember_failure(failure,"actual Input Service shutdown failed");
        if (std::any_of(stopped.cancelled.begin(),stopped.cancelled.end(),[](const auto& token){return bool(token);}))
            remember_failure(failure,"actual application shutdown left an unacknowledged Input publication");
    }
    if (!host.cancel_present()) remember_failure(failure,host.message());
    if (host.error()==g::Error::none && host.stop_audio()) {
        timeline.progress.set_deadline(Time::now()+std::chrono::seconds(90),
            "actual SDL/native Audio stop/drain deadline or failure");
        while (!timeline.progress.deadline_expired()) {
            (void)timeline.advance(false);
            if (host.error()!=g::Error::none) break;
            const auto drain=host.poll_audio_drain();
            if (drain==g::Drain::failed) break;
            if (drain==g::Drain::drained) {evidence.audio_drained=true;break;}
            (void)timeline.progress.idle();
        }
    }
    if (!evidence.audio_drained) remember_failure(failure,host.message()[0]?host.message():"actual SDL/native Audio stop/drain deadline or failure");
    evidence.accepted_pcm=host.audio()->accepted_frames();
    const auto closed=host.shutdown();
    evidence.host_released=!closed.failed && closed.owners_released &&
        closed.audio==isle::audio::SdlAudioOutput::CloseDisposition::stream_drained;
    if (!evidence.host_released) remember_failure(failure,"actual Host/Audio/GUI/worker owners did not retire after observed stream drain");
    if (failure.empty() && (!evidence.input_stopped || evidence.accepted_pcm==0 || evidence.submissions!=evidence.terminals ||
        evidence.released_frames!=evidence.submissions || evidence.read_requests!=evidence.read_completions))
        remember_failure(failure,"actual final frame/input/Audio ownership ledger did not balance");
    if (mode==Mode::course && failure.empty()) {
        if (!evidence.post.returned || !evidence.course.post_scene_verified || !evidence.native_retired ||
            !evidence.audio_drained || !evidence.host_released)
            remember_failure(failure,"completed course requires original Camera9 Photo15 Oak return and real teardown");
        else evidence.course.qualified=true;
    }
    write_report(root,evidence,failure);
    if (!failure.empty()) {std::cerr << "private actual application integration: " << failure << '\n';return 1;}
    if(mode==Mode::native_course) {
        std::cout<<"actual_original_Source_natural_course_and_Manager_CameraCheck9=passed course="<<evidence.requested_course
            <<" actual_updates="<<evidence.course_updates<<" course_frames="<<evidence.course_frames
            <<" course_LUS_presentations="<<evidence.course_presentations
            <<" canonical_submissions="<<evidence.submissions<<" actual_terminals="<<evidence.terminals
            <<" accepted_dummy_PCM_frames="<<evidence.accepted_pcm<<std::endl;
        return 0;
    }
    if (menu_mode(mode)) {
        std::cout << "actual_original_Manager_authored_save_menu_routes=passed"
            << " qualified_mask=" << evidence.menu_qualified << " target_mask=" << menu_target(mode)
            << " canonical_submissions=" << evidence.submissions << " actual_terminals=" << evidence.terminals
            << " API_inputs=AUTHORED_NOT_gameplay_captures full_game_Windows_parity=not_claimed\n";
        return 0;
    }
    if (mode==Mode::course) {
        std::cout << "completed_original_Beach_CameraCheck9_PhotoCheck15_Manager_Oak=passed"
            << " private_report=proof/application-runtime-report.txt full_game_hardware_parity=not_claimed\n";
        return 0;
    }
    std::cout << "actual_application_runtime=passed original_Manager_Boot_Title_Name_Oak_Beach=passed"
        << " cold_stop_before_first_pump=passed real_Input_GTL_backpressure=passed real_completed_frame_hold_stop=passed"
        << " canonical_submissions=" << evidence.submissions << " actual_terminals=" << evidence.terminals
        << " accepted_dummy_PCM_frames=" << evidence.accepted_pcm
        << " private_report=proof/application-runtime-report.txt full_game_hardware_parity=not_claimed\n";
    return 0;
}
} // namespace
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    try {
        require(argc==3 || argc==4,
            "expected PRIVATE_ROM NEW_OWNED_ROOT [ingress|course|native0..native6|menus|credits17|credits18|credits19] (existing empty absolute nonsymlink private directory)");
        Mode mode=Mode::ingress;
        std::int32_t requestedCourse=0;
        if (argc==4) {
            const auto name=std::filesystem::path(argv[3]).generic_string();
            const bool native=name.size()==7 && name.compare(0,6,"native")==0 && name[6]>='0' && name[6]<='6';
            require(native || name=="ingress" || name=="course" || name=="menus" || name=="credits17" ||
                name=="credits18" || name=="credits19","unknown actual Runtime qualification mode");
            if(native){mode=Mode::native_course;requestedCourse=name[6]-'0';}
            else if (name=="course") mode=Mode::course;
            else if (name=="menus") mode=Mode::menus;
            else if (name=="credits17") mode=Mode::credits17;
            else if (name=="credits18") mode=Mode::credits18;
            else if (name=="credits19") mode=Mode::credits19;
        }
        return run(std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),mode,requestedCourse);
    } catch (const std::exception& error) {
        std::cerr << "private actual application integration: " << error.what() << '\n';return 1;
    } catch (...) {
        std::cerr << "private actual application integration: unknown setup/artifact failure\n";return 1;
    }
}
