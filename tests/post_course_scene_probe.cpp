#include "post_course_scene_probe.hpp"
#include "post_course_scene.hpp"
#include "post_course_window.hpp"
#include "credits_worker_native.hpp"
#include "application_photos_source.h"
#include <cstddef>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace isle::private_post_course {
namespace {
using Scene=post_course::Scene;
using Status=post_course::SceneStatus;
using Clock=std::chrono::steady_clock;
constexpr std::array<std::int32_t,9> arguments{14,12,22,24,23,17,18,19,20};
constexpr std::uint16_t a_button=0x8000,b_button=0x4000,start_button=0x1000;
constexpr std::uint64_t update_bound=6000,ledger_bound=65536;
// Original18/19 cannot take17's early-skip process. Their five tables contain
// 129 rows at 18-pixel spacing, scrolling at0.5/0.44 pixels per update, plus
// five explicit tail intervals and entry/ending waits: allow the whole sequence.
constexpr std::uint64_t full_credits_update_bound=12000;
constexpr auto visit_deadline=std::chrono::seconds(360);
void require(bool yes,const char* message) {
    if(!yes)throw std::runtime_error(message);
}
std::size_t index(std::int32_t argument) {
    const auto at=std::find(arguments.begin(),arguments.end(),argument);
    require(at!=arguments.end(),"unsupported actual menu Scene argument");
    return static_cast<std::size_t>(at-arguments.begin());
}
bool visible(const IslePostCourseSceneSource& common,IsleGameObject* object) {
    if(!object || !common.game)return false;
    IsleGameObjectMetadata metadata{};
    require(isle_game_object_metadata(common.game,object,&metadata)==ISLE_GAME_OK,
            "actual menu camera object observation rejected");
    return !(metadata.flags&1);
}
bool unfaded(const IsleSprite* sprite) {
    return sprite && !(sprite->attr&ISLE_SP_HIDDEN) &&
           sprite->red==255 && sprite->green==255 && sprite->blue==255;
}
bool raster_varied(const IsleSprite& sprite) {
    if(!sprite.bitmap || sprite.nbitmaps<=0 || sprite.nbitmaps>32)return false;
    std::size_t bytes=0,changed=0;
    unsigned first=0;bool have=false;
    for(int i=0;i<sprite.nbitmaps;++i) {
        const auto& bitmap=sprite.bitmap[i];
        require(bitmap.byte_count<=1024*1024 && bytes<=1024*1024-bitmap.byte_count,
                "credits raster exceeds bounded readonly inspection");
        bytes+=bitmap.byte_count;
        if(!bitmap.buf)return false;
        for(std::size_t b=0;b+1<bitmap.byte_count;b+=2) {
            const unsigned pixel=(unsigned(bitmap.buf[b])<<8)|bitmap.buf[b+1];
            if(!have) {first=pixel;have=true;}
            else if(pixel!=first)++changed;
        }
    }
    return bytes>=1024 && changed>=128;
}
// Compare the real tiled/swizzled RGBA5551 raster with its completed RGBA8
// image, not merely a created destination or an offscreen varied thumbnail.
// Report children are root-relative; Credits C places each SObj absolutely.
std::uint64_t raster_digest(const IsleSprite& sprite) {
    std::uint64_t digest=1469598103934665603ull;
    if(!sprite.bitmap || sprite.nbitmaps<=0 || sprite.nbitmaps>32)return 0;
    std::size_t bytes=0;
    for(int i=0;i<sprite.nbitmaps;++i) {
        const auto& piece=sprite.bitmap[i];
        require(piece.byte_count<=1024*1024 && bytes<=1024*1024-piece.byte_count,
                "Report raster exceeds bounded readonly inspection");
        if(!piece.buf)return 0;
        bytes+=piece.byte_count;
        for(std::size_t j=0;j<piece.byte_count;++j) {
            digest^=piece.buf[j];digest*=1099511628211ull;
        }
    }
    return bytes?digest:0;
}
bool photo_image_matches(const IsleSprite& root,const IsleSprite& photo,
                         const scene::graphics::Completed& frame,bool relative_to_root=true) {
    if(photo.width!=140 || photo.height!=105 || photo.bmfmt!=0 || photo.bmsiz!=2 ||
       !unfaded(&root) || !unfaded(&photo) ||
       ((photo.attr&ISLE_SP_SCALE) && (photo.scalex!=1 || photo.scaley!=1)) ||
       !raster_varied(photo))return false;
    const int left=(relative_to_root?int(root.x):0)+int(photo.x);
    const int top=(relative_to_root?int(root.y):0)+int(photo.y);
    if(left<0 || top<0 || left+140>int(frame.width) || top+105>int(frame.height) ||
       frame.stride<frame.width*4 || frame.rgba.size()<std::size_t(frame.stride)*frame.height)
        return false;
    unsigned samples=0,matches=0,changed=0,changed_matches=0,first=0;
    for(unsigned y=4;y<101;y+=4)for(unsigned x=4;x<136;x+=4) {
        unsigned local_y=y;
        const IsleBitmap* piece=nullptr;
        for(int i=0;i<photo.nbitmaps;++i) {
            const auto& candidate=photo.bitmap[i];
            if(candidate.actualHeight<=0)return false;
            if(local_y<unsigned(candidate.actualHeight)) {piece=&candidate;break;}
            local_y-=unsigned(candidate.actualHeight);
        }
        if(!piece || piece->width_img<140)return false;
        const auto offset=(std::size_t(local_y)*std::size_t(piece->width_img)*2+x*2)^
                          ((local_y&1)?std::size_t{4}:std::size_t{0});
        if(offset+1>=piece->byte_count)return false;
        const unsigned pixel=(unsigned(piece->buf[offset])<<8)|piece->buf[offset+1];
        if(!samples)first=pixel;
        else if(pixel!=first)++changed;
        const auto at=std::size_t(top+int(y))*frame.stride+std::size_t(left+int(x))*4;
        const std::array<unsigned,3> expected{
            ((pixel>>11)&31)*255/31,((pixel>>6)&31)*255/31,((pixel>>1)&31)*255/31};
        bool match=true;
        for(unsigned channel=0;channel<3;++channel) {
            const auto actual=std::to_integer<unsigned>(frame.rgba[at+channel]);
            const auto value=expected[channel];
            if(actual>value+20 || value>actual+20)match=false;
        }
        ++samples;if(match)++matches;
        if(pixel!=first && match)++changed_matches;
    }
    return samples>=128 && changed>=32 && matches*4>=samples*3 && changed_matches*4>=changed*3;
}
}
struct Probe::Impl {
    Controller controller;
    std::ofstream ledger;
    std::array<bool,9> qualified{};
    const Scene* active=nullptr;
    std::int32_t argument=0;
    std::uint64_t rows=0,visits=0,updates=0,last_external=0,frames=0,varied_frames=0;
    std::uint64_t last_digest=0,first_digest=0,pulse_at=0,next_pulse=0;
    std::uint64_t full_frames=0,full_at=0,after_next_frames=0,menu_frames=0;
    std::array<std::uint64_t,5> image_incarnations{};
    std::size_t images=0;
    bool surface_ready=false,full_seen=false,gallery_returned=false,next_requested=false;
    bool next_image_seen=false,return_requested=false,full_ready=false,holding=false,releasing=false;
    unsigned stage=0;
    enum class ReportStep : unsigned {
        cover,list_menu,list_selected,detail,returned_list,reopened_detail,
        final_list,exit_list,returned_cover,exit_cover
    };
    enum class ReportView : unsigned { none,cover,list_menu,list_selected,detail };
    ReportStep report_step=ReportStep::cover;
    std::uint64_t report_step_frames=0,report_prototype_digest=0,report_detail_digest=0;
    std::uint64_t report_revision=0;
    std::int32_t report_species=0,report_slot=-1;
    bool report_detail_presented=false,report_reopened_presented=false;
    std::uint64_t report_cover_at=0;
    bool report_cover_observed=false;
    Clock::time_point started{};
    struct FrameTag {
        std::uint64_t ordinal=0,updates=0;
        std::int32_t argument=0;
        std::uint32_t images=0;
        bool full=false,next=false,menu=false,returned=false;
        ReportView report_view=ReportView::none;
        ReportStep report_step=ReportStep::cover;
        std::uint64_t report_raster=0;
    };
    std::array<FrameTag,2> frame_tags{};
    std::uint32_t presented_images=0;
    std::uint64_t next_baseline_digest=0;

    Impl(Controller supplied,const std::filesystem::path& root):controller(supplied) {
        require(controller.owner && controller.set,"actual owned software controller setter is required");
        std::filesystem::create_directories(root);
        ledger.exceptions(std::ios::badbit|std::ios::failbit);
        ledger.open(root/"menu-api-source.tsv",std::ios::out|std::ios::trunc);
        row("scope\tauthored-input actual RootManager/Runtime Scene API; not captured gameplay or N64 parity");
        row("bounds\t9 variants; 32 visits; 6000 updates (12000 for full Original Credits18/19), 360 wall seconds per visit; 65536 rows of at most 512 bytes; no images written");
        row("kind\targument\tupdates\tpresented\tvaried\tstage\tfull_frames\tauthored_images\tSource_status\tack_serial\tack_consumed\tprints\tfades\tlayouts\tresult_or_digest");
        neutral();
    }
    void neutral() noexcept {controller.set(controller.owner,0,0,0);}
    void row(const std::string& text) {
        require(rows<ledger_bound,"bounded menu API ledger exhausted");
        require(text.size()<512,"bounded menu API ledger row overflow");
        ledger<<text<<'\n';++rows;
    }
    void note(const char* kind,const Scene& scene,std::uint64_t value=0) {
        post_course::WindowObservation window{};
        if(scene.source().window)
            require(post_course::Window::observe_operations(scene.source().window,&window)==ISLE_GAME_OK,
                    "actual Window operation observation rejected");
        const auto& ack=scene.source().acknowledgment;
        std::ostringstream text;
        text<<kind<<'\t'<<argument<<'\t'<<updates<<'\t'<<frames<<'\t'<<varied_frames<<'\t'<<stage
            <<'\t'<<full_frames<<'\t'<<images<<'\t'<<int(scene.source().status)
            <<'\t'<<ack.serial<<'\t'<<ack.consumed_serial<<'\t'<<window.prints
            <<'\t'<<window.fades<<'\t'<<window.layouts<<'\t'<<value;
        row(text.str());
    }
    void enter(const Scene& scene,std::int32_t entry,std::uint64_t external) {
        (void)index(entry);
        require(!active,"new actual menu appeared before previous retirement receipt");
        require(++visits<=32,"actual menu visit bound exhausted");
        active=&scene;argument=entry;updates=0;last_external=external;
        frames=varied_frames=last_digest=first_digest=full_frames=full_at=after_next_frames=0;
        images=0;image_incarnations={};presented_images=0;surface_ready=full_seen=gallery_returned=false;
        menu_frames=0;return_requested=full_ready=false;next_baseline_digest=0;
        next_requested=next_image_seen=holding=releasing=false;stage=0;
        pulse_at=next_pulse=0;started=Clock::now();neutral();
        report_step=ReportStep::cover;report_step_frames=0;
        report_prototype_digest=report_detail_digest=report_revision=0;
        report_cover_at=0;report_cover_observed=false;
        report_species=0;report_slot=-1;report_detail_presented=report_reopened_presented=false;
        require(scene.source().scene==entry,"actual Source entry argument differs from Manager borrow");
        note("enter",scene);ledger.flush();
    }
    void observe(const Scene& scene,std::int32_t entry,std::uint64_t external) {
        if(!active)enter(scene,entry,external);
        require(active==&scene && argument==entry,"actual menu identity changed without joined retirement");
        require(external>=last_external,"actual Source update observation moved backwards");
        const auto delta=external-last_external;
        const auto limit=entry==18 || entry==19?full_credits_update_bound:update_bound;
        require(delta<=limit && updates<=limit-delta,"actual menu Source update bound exhausted");
        updates+=delta;last_external=external;
        require(Clock::now()-started<visit_deadline,"actual menu wall bound exhausted; no completion claimed");
        require(scene.error()==ISLE_GAME_OK,scene.message());
        require(scene.source().status==ISLE_GAME_OK,"actual menu Source failed or STOPPED; no normal receipt");
    }
    bool settled(const Scene& scene) {
        post_course::WindowObservation view{};
        require(scene.source().window &&
                post_course::Window::observe_operations(scene.source().window,&view)==ISLE_GAME_OK,
                "actual live Window observation rejected");
        return !view.printing && !view.prints && !view.fades && !view.layouts;
    }
    bool menu_ready(const Scene& scene) {
        if(!settled(scene))return false;
        const IsleSprite* sprite=nullptr;
        if(const auto* album=scene.album_source()) {
            // Original assigns PanelButtons only inside album_Update, AFTER
            // its sixty-step cover fade and completed initial panel transition.
            if(!album->album_PanelButtons || !album->album_BackgroundGObj)return false;
            require(post_course::Window::observe_sprite_root(scene.source().window,
                        album->album_BackgroundGObj,&sprite)==ISLE_GAME_OK,
                    "actual Album cover observation rejected");
        } else if(const auto* report=scene.report_source()) {
            // Report's custom Source RGB fade is not a Window fade operation.
            if(!report->D_80230E14_A27784 || !report->D_80230C1C_A2758C)return false;
            require(post_course::Window::observe_element(scene.source().window,
                        report->D_80230C1C_A2758C->handle,&sprite)==ISLE_GAME_OK,
                    "actual Report cover observation rejected");
            // Header/list and detail reuse this Source cell. Only the actual
            // cover geometry may authorize the initial/final cover command.
            if(!sprite || sprite->width!=182 || sprite->height!=97)return false;
        } else if(const auto* gallery=scene.gallery_source()) {
            // The selector is assigned only AFTER both initial acknowledgments.
            // The title is hidden throughout the real full-photo excursion.
            if(!gallery->D_801EA1F4_A07EB4 || !gallery->D_802308A0_A4E560 ||
               !gallery->D_8023087C_A4E53C ||
               visible(scene.source(),gallery->D_801EA7FC_A084BC))return false;
            require(post_course::Window::observe_element(scene.source().window,
                        gallery->D_8023087C_A4E53C->handle,&sprite)==ISLE_GAME_OK,
                    "actual Gallery grid title observation rejected");
        } else return false;
        return unfaded(sprite);
    }
    // Pulses span actual completed Source updates, not Host poll iterations.
    // Three admitted updates hold, three release; further commands are spaced
    // by at least 24 updates. No pulse completion is a scene success condition.
    bool pulse_ready() {
        if(holding && updates-pulse_at>=3) {
            neutral();holding=false;releasing=true;pulse_at=updates;
        }
        if(releasing && updates-pulse_at>=3)releasing=false;
        return !holding && !releasing && updates>=next_pulse;
    }
    void pulse(std::uint16_t buttons,std::int8_t y=0) {
        controller.set(controller.owner,buttons,0,y);
        holding=true;pulse_at=updates;next_pulse=updates+24;
    }
    int gallery_slot(const Scene& scene,int slot) {
        int id=0;
        require(slot>=0 && slot<8 &&
                post_course::Window::observe_layout_slot_id(scene.source().window,
                    static_cast<std::size_t>(slot),&id)==ISLE_GAME_OK,
                "actual Gallery Layout button slot observation rejected");
        return id;
    }
    bool report_element(const Scene& scene,const IsleReportElement* element,int width,int height,
                        bool full_color=true) {
        if(!element || !element->handle.owner || !element->handle.serial)return false;
        const IsleSprite* sprite=nullptr;
        require(post_course::Window::observe_element(scene.source().window,element->handle,&sprite)==ISLE_GAME_OK,
                "actual Report element observation rejected");
        return sprite && sprite->width==width && sprite->height==height &&
               (full_color?unfaded(sprite):(!(sprite->attr&ISLE_SP_HIDDEN) &&
                sprite->red>=229 && sprite->green>=229 && sprite->blue>=229));
    }
    bool report_root_visible(const Scene& scene) {
        const auto* s=scene.report_source();
        if(!s || !visible(scene.source(),s->D_80230C10_A27580))return false;
        const IsleSprite* root=nullptr;
        require(post_course::Window::observe_sprite_root(scene.source().window,s->D_80230C10_A27580,&root)==ISLE_GAME_OK,
                "actual Report root visibility observation rejected");
        // Report hides this hierarchy through the root Sprite, not GObj flags.
        return root && !(root->attr&ISLE_SP_HIDDEN);
    }
    ReportView report_view(const Scene& scene) {
        const auto* s=scene.report_source();
        if(!s || !settled(scene) || !s->D_80230E14_A27784)return ReportView::none;
        if(menu_ready(scene))return ReportView::cover;
        if(!report_element(scene,s->D_80230C34_A275A4,108,16))return ReportView::none;
        if(s->D_80203008_9F9978==2 && report_root_visible(scene) &&
           report_element(scene,s->D_80230C38_A275A8,84,12) &&
           report_element(scene,s->D_80230C30_A275A0,192,52)) {
            for(const auto* row:s->D_80230C80_A275F0)
                if(report_element(scene,row,184,8))return ReportView::none;
            return ReportView::detail;
        }
        if(!report_element(scene,s->D_80230C1C_A2758C,176,15,false) ||
           !report_element(scene,s->D_80230C80_A275F0[0],184,8) ||
           report_root_visible(scene))return ReportView::none;
        if(s->D_80203008_9F9978==1 && s->D_80230DB4_A27724==0 &&
           s->D_8020300C_9F997C==0)return ReportView::list_selected;
        if(s->D_80203008_9F9978==0 && gallery_slot(scene,0)==29)return ReportView::list_menu;
        return ReportView::none;
    }
    void report_record(const Scene& scene) {
        const auto* s=scene.report_source();
        IslePhotoRecord record{},first{};
        std::int32_t count=0,score=0;
        require(scene.source().photos &&
                isle_application_photos_report_count(scene.source().photos,&count)==ISLE_GAME_OK && count>0,
                "Report detail prerequisite missing: existing canonical ordinary Report photo; no fixture manufactured");
        const auto species=s->D_80230CB0_A27620[0];
        require(species>0 && species<=151 && s->D_80230DB4_A27724==0 &&
                isle_application_photos_report(scene.source().photos,species,&record)==ISLE_GAME_OK &&
                isle_application_photos_report_ordinal(scene.source().photos,0,&first)==ISLE_GAME_OK &&
                record.photo && record.owner==scene.source().photos && record.storage==ISLE_PHOTO_REPORT &&
                record.photo==first.photo && record.index==first.index && record.revision &&
                isle_application_photos_report_score(scene.source().photos,species,&score)==ISLE_GAME_OK && score>0,
                "Report detail prerequisite missing: positive first-species canonical Report record");
        if(!report_species) {
            report_species=species;report_slot=record.index;report_revision=record.revision;
            note("report_first_record",scene,static_cast<std::uint64_t>(species));
        } else require(report_species==species && report_slot==record.index && report_revision==record.revision,
                       "Report detail/reopen changed the existing canonical first-species identity");
    }
    void report_control(const Scene& scene) {
        const auto view=report_view(scene);
        if(report_step_frames<3)return;
        // Pinned GAME func_801E2EB0 enters func_801E1378 before polling:
        // even an already visible cover has an unconditional 21-update wait.
        if((report_step==ReportStep::cover || report_step==ReportStep::returned_cover) &&
           (!report_cover_observed || updates<report_cover_at || updates-report_cover_at<21))return;
        const auto command=[&](std::uint16_t button,ReportStep next,const char* kind) {
            pulse(button);report_step=next;stage=static_cast<unsigned>(next);
            report_step_frames=0;report_cover_observed=false;note(kind,scene);
        };
        switch(report_step) {
        case ReportStep::cover:
            if(view==ReportView::cover) {
                report_record(scene);
                require(gallery_slot(scene,scene.report_source()->D_80202EC0_9F9830)==2,
                        "actual Report cover did not select PKMN Report");
                command(a_button,ReportStep::list_menu,"report_pkmn_input");
            }
            break;
        case ReportStep::list_menu:
            if(view==ReportView::list_menu)command(a_button,ReportStep::list_selected,"report_view_input");
            break;
        case ReportStep::list_selected:
        case ReportStep::returned_list:
            if(view==ReportView::list_selected) {
                report_record(scene);
                require(report_prototype_digest,"Report omitted original pre-preview detail child observation");
                command(a_button,report_step==ReportStep::list_selected?ReportStep::detail:ReportStep::reopened_detail,
                        "report_first_species_input");
            }
            break;
        case ReportStep::detail:
            if(view==ReportView::detail && report_detail_presented)
                command(b_button,ReportStep::returned_list,"report_detail_back_input");
            break;
        case ReportStep::reopened_detail:
            if(view==ReportView::detail && report_reopened_presented)
                command(b_button,ReportStep::final_list,"report_reopened_back_input");
            break;
        case ReportStep::final_list:
            if(view==ReportView::list_selected)command(b_button,ReportStep::exit_list,"report_list_back_input");
            break;
        case ReportStep::exit_list:
            if(view==ReportView::list_menu)command(b_button,ReportStep::returned_cover,"report_cover_input");
            break;
        case ReportStep::returned_cover:
            if(view==ReportView::cover)command(b_button,ReportStep::exit_cover,"report_oak_input");
            break;
        case ReportStep::exit_cover:break;
        }
    }
    void control(const Scene& scene) {
        const bool ready=pulse_ready();
        if(scene.status()!=Status::running) {neutral();holding=releasing=false;return;}
        const auto& ack=scene.source().acknowledgment;
        if(ready && ack.active && ack.waiting && settled(scene)) {pulse(a_button);return;}
        if(!surface_ready || !ready || !settled(scene))return;
        if(argument==12) {
            const auto* s=scene.album_source();
            require(s,"actual Album typed Source missing");
            if(s->album_PanelButtons && s->album_BackgroundGObj && menu_frames>=3 &&
               menu_ready(scene))pulse(b_button);
        } else if(argument==14) {
            report_control(scene);
        } else if(argument==22 || argument==23) {
            const auto* s=scene.gallery_source();
            require(s,"actual Gallery typed Source missing");
            const bool full=visible(scene.source(),s->D_801EA7FC_A084BC);
            if(full) {
                if(!full_ready || updates-full_at<90 || full_frames<3)return;
                if(!next_requested) {
                    // Original full-view A selects the next canonical temp
                    // photo and joins actual display/Photo before redisplay.
                    pulse(a_button);next_requested=true;stage=2;
                    after_next_frames=frames;full_at=updates;next_baseline_digest=last_digest;
                    note("gallery_next_input",scene);
                } else if(next_image_seen && !return_requested && updates-full_at>=60) {
                    pulse(b_button);return_requested=true;note("gallery_grid_input",scene);
                }
                return;
            }
            if(!s->D_801EA1F4_A07EB4)return;
            if(!s->D_802308A0_A4E560) {
                // Gallery retains Original window/8486E0.c's plain C helper,
                // not common.acknowledgment. It disables text skipping while
                // waiting for A/B; the selector does not exist until BOTH
                // greetings have consumed real input (9FAC10.c:823-844).
                post_course::WindowObservation text{};
                require(post_course::Window::observe_operations(scene.source().window,&text)==ISLE_GAME_OK,
                        "actual Gallery introductory text observation rejected");
                if(!text.skip_enabled) {pulse(a_button);note("gallery_intro_input",scene);}
                return;
            }
            if(!menu_ready(scene) || menu_frames<3)return;
            if(!full_seen) {
                // Actual initialized menu id20 is the full-view command.
                // Move by genuine stick impulses, never overwrite selection.
                const int id=gallery_slot(scene,s->D_802308A4_A4E564);
                if(id==20) {pulse(a_button);stage=1;note("gallery_full_input",scene);}
                else pulse(0,-70);
            } else if(gallery_returned) {
                // Original B selects Save (slot7); A commits its real route.
                if(gallery_slot(scene,s->D_802308A4_A4E564)==4)pulse(a_button);
                else pulse(b_button);
            }
        } else if(argument==24) {
            const auto* s=scene.end_level_8_source();
            require(s,"actual End8 typed Source missing");
            if(full_seen && full_frames>=3 && updates-full_at>=90 &&
               s->D_801DD250_A08E40==0x10000000)pulse(b_button);
        } else {
            const auto* s=scene.credits_source();
            require(s && s->worker,"actual Credits worker missing");
            // All four authored report rasters must have been consumed by
            // Original's real independent worker and presented before skip.
            // The fifth NULL Source request legitimately leaves pixels alone.
            if(images>=4 && !s->worker->main_excluded() && updates>=300)pulse(start_button);
        }
    }
    void inspect_images(const Scene& scene) {
        const auto* credits=scene.credits_source();
        if(!credits || !credits->worker || credits->worker->main_excluded())return;
        require(credits->destination_count<=image_incarnations.size(),"Credits destination bound exceeded");
        for(std::size_t i=0;i<credits->destination_count;++i) {
            const auto& d=credits->destinations[i];
            if(!d.incarnation || image_incarnations[i]==d.incarnation)continue;
            const IsleSprite* sprite=nullptr;
            require(post_course::Window::observe_sprite(scene.source().window,d.sprite,&sprite)==ISLE_GAME_OK,
                    "actual Credits destination observation rejected");
            if(sprite && raster_varied(*sprite)) {
                image_incarnations[i]=d.incarnation;
                note("worker_image_observed",scene,d.incarnation);
            }
        }
    }
};
Probe::Probe(Controller controller,const std::filesystem::path& root)
    :p_(std::make_unique<Impl>(controller,root)) {}
Probe::~Probe() {p_->neutral();}
void Probe::update(const Scene& scene,std::int32_t argument,std::uint64_t updates) {
    auto& p=*p_;
    const auto before=p.updates;
    p.observe(scene,argument,updates);
    if(p.updates!=before)p.note("update",scene);
    p.control(scene);
}
void Probe::frame(const Scene& scene,std::int32_t argument,std::uint64_t ordinal) {
    auto& p=*p_;
    require(p.active==&scene && p.argument==argument,"completed Frame differs from actual borrowed menu");
    require(ordinal && !scene.graphics_busy() && scene.completed_frame() &&
            scene.completed_frame().ordinal==ordinal,"menu provenance lacks actual completed Frame");
    auto slot=std::find_if(p.frame_tags.begin(),p.frame_tags.end(),[](const auto& tag){return !tag.ordinal;});
    require(slot!=p.frame_tags.end(),"bounded copied menu presentation provenance exhausted");
    p.inspect_images(scene);
    std::uint32_t image_mask=0;
    if(const auto* credits=scene.credits_source();
       credits && credits->worker && !credits->worker->main_excluded()) {
        for(std::size_t i=0;i<credits->destination_count;++i) {
            const auto& destination=credits->destinations[i];
            // Source later removes these SObjs before releasing destination
            // storage. A completed Host image is evidence, not a live borrow.
            if(p.presented_images&(std::uint32_t(1)<<i))continue;
            if(!destination.incarnation || p.image_incarnations[i]!=destination.incarnation)continue;
            IsleGameObject* object=nullptr;
            require(isle_game_sprite_object(scene.source().game,destination.sprite,&object)==ISLE_GAME_OK,
                    "actual Credits image owner observation rejected");
            if(!visible(scene.source(),object))continue;
            const IsleSprite *photo=nullptr,*root=nullptr;
            require(post_course::Window::observe_sprite(scene.source().window,destination.sprite,&photo)==ISLE_GAME_OK &&
                    post_course::Window::observe_sprite_root(scene.source().window,object,&root)==ISLE_GAME_OK,
                    "actual Credits image geometry observation rejected");
            if(root && photo && photo_image_matches(*root,*photo,scene.completed_frame(),false))
                image_mask|=std::uint32_t(1)<<i;
        }
    }
    bool full=false;
    if(const auto* gallery=scene.gallery_source())
        full=visible(scene.source(),gallery->D_801EA7FC_A084BC);
    else if(const auto* end=scene.end_level_8_source())
        full=visible(scene.source(),end->D_801DD240_A08E30);
    const bool menu=p.menu_ready(scene);
    *slot={ordinal,p.updates,argument,image_mask,full,p.next_requested,menu,
           menu && p.return_requested};
    if(argument==14) {
        const auto* s=scene.report_source();
        require(s,"actual Report typed Source missing");
        slot->report_step=p.report_step;
        slot->report_view=p.report_view(scene);
        if(slot->report_view==Impl::ReportView::cover &&
           (p.report_step==Impl::ReportStep::cover || p.report_step==Impl::ReportStep::returned_cover) &&
           !p.report_cover_observed) {
            p.report_cover_at=p.updates;p.report_cover_observed=true;
        }
        const IsleSprite *photo=nullptr,*root=nullptr;
        if(s->D_80230C14_A27584) {
            require(post_course::Window::observe_sprite(scene.source().window,s->D_80230C14_A27584,&photo)==ISLE_GAME_OK,
                    "actual Report detail child observation rejected");
            if(photo && p.report_step==Impl::ReportStep::cover && !p.report_prototype_digest)
                p.report_prototype_digest=raster_digest(*photo);
        }
        if(slot->report_view==Impl::ReportView::detail) {
            p.report_record(scene);
            require(post_course::Window::observe_sprite_root(scene.source().window,s->D_80230C10_A27580,&root)==ISLE_GAME_OK,
                    "actual Report detail root observation rejected");
            const auto digest=photo?raster_digest(*photo):0;
            if(!root || !photo || !digest || digest==p.report_prototype_digest ||
               !photo_image_matches(*root,*photo,scene.completed_frame()))
                slot->report_view=Impl::ReportView::none;
            else slot->report_raster=digest;
        }
    }
}
void Probe::presented(std::int32_t argument,std::uint64_t ordinal,
                      std::uint64_t pixel_digest,bool visible_pixels) {
    auto& p=*p_;
    const auto slot=std::find_if(p.frame_tags.begin(),p.frame_tags.end(),[&](const auto& tag) {
        return tag.ordinal==ordinal && tag.argument==argument;
    });
    require(slot!=p.frame_tags.end(),"Host completion lacks its actual copied Frame provenance");
    const auto tag=*slot;*slot={};
    // Actual Root may retire the Source before the copied Host presentation.
    // An old terminal frame cannot contribute evidence to the next visit.
    if(p.argument!=argument)return;
    ++p.frames;
    if(!visible_pixels)return;
    ++p.varied_frames;p.last_digest=pixel_digest;
    if(!p.first_digest)p.first_digest=pixel_digest;
    const auto scalar_note=[&](const char* kind) {
        std::ostringstream text;
        text<<kind<<'\t'<<argument<<'\t'<<tag.updates<<'\t'<<p.frames<<'\t'
            <<p.varied_frames<<'\t'<<p.stage<<'\t'<<p.full_frames<<'\t'<<p.images
            <<"\t0\t0\t0\t0\t0\t0\t"<<pixel_digest;
        p.row(text.str());
    };
    if(!p.surface_ready) {
        p.surface_ready=true;scalar_note("surface_presented");p.ledger.flush();
        if(argument!=12 && argument!=14 && argument!=22 && argument!=23)
            std::cout<<"menu_api_surface_ready scene="<<argument<<" frames="<<p.frames<<std::endl;
    }
    if(tag.menu && ++p.menu_frames==1) {
        scalar_note("interactive_surface_presented");p.ledger.flush();
        std::cout<<"menu_api_surface_ready scene="<<argument<<" frames="<<p.frames<<" view=menu"<<std::endl;
    }
    if(argument==14 && tag.report_step==p.report_step) {
        using Step=Impl::ReportStep;using View=Impl::ReportView;
        const auto expected=p.report_step==Step::cover || p.report_step==Step::returned_cover?View::cover:
            p.report_step==Step::list_menu || p.report_step==Step::exit_list?View::list_menu:
            p.report_step==Step::detail || p.report_step==Step::reopened_detail?View::detail:
            p.report_step==Step::exit_cover?View::none:View::list_selected;
        if(expected!=View::none && tag.report_view==expected) {
            ++p.report_step_frames;
            if(expected==View::detail) {
                require(tag.report_raster,"Report detail Host completion lacks reconstructed native raster");
                if(p.report_step==Step::detail) {
                    if(!p.report_detail_digest)p.report_detail_digest=tag.report_raster;
                    require(p.report_detail_digest==tag.report_raster,"Report detail raster changed without input");
                    p.report_detail_presented=true;
                } else {
                    // Replays retain canonical record identity, not an
                    // incidental cross-lifetime renderer pixel digest.
                    require(p.report_detail_presented,
                            "Report reopen preceded the first actual detail presentation");
                    p.report_reopened_presented=true;
                }
            }
            if(p.report_step_frames==3) {
                scalar_note(expected==View::detail?
                    (p.report_step==Step::detail?"report_detail_image_presented":"report_reopened_image_presented"):
                    expected==View::cover?"report_cover_presented":
                    expected==View::list_menu?"report_list_menu_presented":"report_first_row_presented");
                p.ledger.flush();
            }
        }
    }
    if(tag.returned && !p.gallery_returned) {
        p.gallery_returned=true;p.stage=3;scalar_note("gallery_grid_returned");
    }
    if(tag.full) {
        if(!p.full_seen) {
            p.full_seen=true;p.full_at=tag.updates;p.stage=1;scalar_note("full_photo_presented");
        }
        if(!p.full_ready && p.full_seen && p.full_frames>=3 && tag.updates>=p.full_at+90) {
            p.full_ready=true;scalar_note("full_photo_ready_presented");
            p.ledger.flush();
            std::cout<<"menu_api_surface_ready scene="<<argument<<" view=full_photo frames="<<p.frames<<std::endl;
        }
        ++p.full_frames;
        if(tag.next && p.frames>p.after_next_frames+2 && tag.updates>=p.full_at+30 &&
           pixel_digest!=p.next_baseline_digest) {
            if(!p.next_image_seen) {
                scalar_note("next_photo_presented");
                std::cout<<"menu_api_surface_ready scene="<<argument<<" view=next_photo frames="<<p.frames<<std::endl;
            }
            p.next_image_seen=true;
        }
    }
    const auto old=p.presented_images;
    p.presented_images|=tag.images;
    p.images=static_cast<std::size_t>(std::popcount(p.presented_images));
    if(old!=p.presented_images)scalar_note("worker_images_presented");
    if(old!=p.presented_images && p.images>=4 && std::popcount(old)<4)
        std::cout<<"menu_api_surface_ready scene="<<argument<<" view=worker_images frames="<<p.frames<<std::endl;
}
void Probe::retired(const Scene& scene,std::int32_t result) {
    auto& p=*p_;
    require(p.active==&scene,"actual menu retired without borrowed probe observations");
    p.neutral();p.holding=p.releasing=false;
    const auto& common=scene.source();
    require(scene.status()==Status::retired && scene.error()==ISLE_GAME_OK &&
            common.status==ISLE_GAME_OK && !common.game && !common.window && !common.replay &&
            !common.domain && !common.input_object && !common.input_process &&
            !scene.graphics_busy() && !scene.completed_frame(),
            "actual menu retirement retains Source/native/Frame/Window ownership");
    require(scene.result() && *scene.result()==result,"actual Original menu result missing or mismatched");
    require(p.surface_ready && p.varied_frames>=3,"actual menu retired without visible canonical Host presentations");
    if(p.argument==12) {
        require(p.menu_frames>=3,"Album omitted usable unfaded Host presentations");
        require(result==13,"Album Original return must be OaksLab2=13");
    } else if(p.argument==14) {
        require(p.menu_frames>=6 && p.report_step==Impl::ReportStep::exit_cover &&
                p.report_species>0 && p.report_slot>=0 && p.report_revision &&
                p.report_detail_presented && p.report_reopened_presented && p.report_detail_digest,
                "Report omitted genuine cover/PKMN/View/first-detail/B/reopen/list/cover controls and image presentations");
        require(result==13,"Report Original return must be OaksLab2=13");
    }
    else if(p.argument==22 || p.argument==23) {
        const auto* gallery=scene.gallery_source();
        require(gallery && p.menu_frames>=3 && p.full_seen && p.full_ready &&
                p.full_frames>=3 && p.next_requested && p.next_image_seen &&
                p.return_requested && p.gallery_returned,
                "Gallery omitted actual full-view/photo/return controls");
        require(result==(gallery->D_801EA1FC_A07EBC?24:8),"Gallery Original route result mismatch");
    } else if(p.argument==24) {
        require(result==23 && p.full_seen && p.full_frames>=3,"End8 omitted actual full photo or Original Gallery2=23 return");
    } else {
        const auto* credits=scene.credits_source();
        require(result==8 && credits && credits->entry_complete && credits->source_result==8 &&
                !credits->worker && !credits->worker_active && !credits->destination_count && p.images>=4,
                "Credits omitted authored worker images or joined Original MainMenu=8 return");
    }
    p.qualified[index(p.argument)]=true;p.note("normal_retired",scene,static_cast<std::uint64_t>(result));
    p.ledger.flush();p.active=nullptr;
}
bool Probe::complete() const noexcept {
    return std::all_of(p_->qualified.begin(),p_->qualified.end(),[](bool yes){return yes;});
}
std::uint32_t Probe::qualified_mask() const noexcept {
    std::uint32_t mask=0;
    for(std::size_t i=0;i<p_->qualified.size();++i)if(p_->qualified[i])mask|=std::uint32_t(1)<<i;
    return mask;
}
std::int32_t Probe::next_argument() const noexcept {
    for(std::size_t i=0;i<arguments.size();++i)if(!p_->qualified[i])return arguments[i];
    return 0;
}
}
