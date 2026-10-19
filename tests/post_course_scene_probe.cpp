#include "post_course_scene_probe.hpp"
#include "post_course_scene.hpp"
#include "post_course_window.hpp"
#include "credits_worker_native.hpp"
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
    Clock::time_point started{};
    struct FrameTag {
        std::uint64_t ordinal=0,updates=0;
        std::int32_t argument=0;
        std::uint32_t images=0;
        bool full=false,next=false,menu=false,returned=false;
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
            const auto* s=scene.report_source();
            require(s,"actual Report typed Source missing");
            if(s->D_80230E14_A27784 && s->D_80230C1C_A2758C && menu_frames>=3 &&
               menu_ready(scene))pulse(b_button);
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
    for(std::size_t i=0;i<p.image_incarnations.size();++i)
        if(p.image_incarnations[i])image_mask|=std::uint32_t(1)<<i;
    bool full=false;
    if(const auto* gallery=scene.gallery_source())
        full=visible(scene.source(),gallery->D_801EA7FC_A084BC);
    else if(const auto* end=scene.end_level_8_source())
        full=visible(scene.source(),end->D_801DD240_A08E30);
    const bool menu=p.menu_ready(scene);
    *slot={ordinal,p.updates,argument,image_mask,full,p.next_requested,menu,
           menu && p.return_requested};
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
    if(p.argument==12 || p.argument==14) {
        require(p.menu_frames>=3,"Album/Report omitted usable unfaded Host presentations");
        require(result==13,"Album/Report Original return must be OaksLab2=13");
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
