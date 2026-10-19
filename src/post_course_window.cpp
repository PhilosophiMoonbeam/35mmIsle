// Own-GAME authority: ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c,
// src/window/{element,text,frame,frame_fade,focus_mark,layout,button,845160}.c;
// reached calls: camera_check/{87A330,87D1A0}.c and
// photo_check/{98C330,993C50}.c. This veneer uses their existing native owners.
#include "post_course_window.hpp"
#include "credits_window_extension.h"
#include "scene_graphics.hpp"
#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

struct IslePostCourseWindow { void* owner; };
namespace {
using namespace isle;
using Handle=IslePostCourseWindowHandle;
using Step=IslePostCourseWindowStep;
using namespace isle::window;
Color color(IslePostCourseColor c) noexcept { return {c.r,c.g,c.b,c.a}; }
IslePostCourseColor color(Color c) noexcept { return {c.r,c.g,c.b,c.a}; }
std::string_view bytes(IslePostCourseBytes b) {
    if (!b.data && b.size) throw std::invalid_argument("window text lifetime");
    return b.data?std::string_view(b.data,b.size):std::string_view{};
}
Step step_value(StepResult r,const TextEvent* event=nullptr) noexcept {
    Step s{};s.status=static_cast<IslePostCourseStepStatus>(r.status);
    s.error=static_cast<IslePostCourseStepError>(r.error);s.updates=r.updates;
    if (event && r.status==StepStatus::event)
        s.event={static_cast<IslePostCourseEventKind>(event->kind),event->character,
                 event->x,event->y,event->scale,event->button,event->offset,event->size};
    return s;
}
Step error_step(StepError e=StepError::invalid_operation) noexcept {return step_value({StepStatus::error,e});}
struct PhotoPrototype {
    const IsleSprite* sprite=nullptr;
    const std::uint8_t* pixels=nullptr;
    std::size_t source_bytes=0,plane_bytes=0;
};
PhotoPrototype photo_prototype(const post_course::Resources& resources,std::uint32_t identity) {
    const auto kind=resources.kind();
    const bool camera=kind==post_course::Resources::Kind::camera_check;
    const bool photo=kind==post_course::Resources::Kind::photo_check;
    const bool album=kind==post_course::Resources::Kind::album;
    const bool report=kind==post_course::Resources::Kind::report;
    const bool gallery=kind==post_course::Resources::Kind::gallery;
    constexpr std::array<std::uint32_t,7> report_roots{
        0x801f67e8u,0x801f7820u,0x801f8858u,0x801f9890u,0x801fa8c8u,0x801fb900u,0x80202ce8u};
    if ((camera && identity!=0x80201520u && identity!=0x80208900u) ||
        (photo && identity!=0x801f3b10u) ||
        (album && identity!=0x80201728u && identity!=0x80208b10u) ||
        (report && std::find(report_roots.begin(),report_roots.end(),identity)==report_roots.end()) ||
        (gallery && identity!=0x801ea1b0u) ||
        (!camera && !photo && !album && !report && !gallery))
        throw std::invalid_argument("unreached post-course Photo prototype");
    const auto& view=*resources.roots()[resources.sprite_index(identity)];
    if(view.bmfmt!=0 || view.bmsiz!=2 || view.width<=0 || view.height<=0 ||
       view.nbitmaps<=0 || view.nbitmaps>4096 || !view.bitmap || view.LUT || view.LUT_bytes || view.nTLUT)
        throw std::invalid_argument("post-course Photo prototype topology");
    const auto& first=view.bitmap[0];
    if(!first.buf || first.width_img<view.width || first.actualHeight<=0)
        throw std::invalid_argument("post-course Photo first bitmap");
    const auto row=std::size_t(first.width_img)*2,height=std::size_t(view.height);
    const auto tile=std::size_t(first.actualHeight),needed=(height+tile-1)/tile;
    const auto first_address=reinterpret_cast<std::uintptr_t>(first.buf);
    if(needed>std::size_t(view.nbitmaps) || height>2*1024*1024/row ||
       row*height>std::numeric_limits<std::uintptr_t>::max()-first_address)
        throw std::invalid_argument("post-course Photo source extent");
    // The original clear path is contiguous from the FIRST bitmap. The
    // reduction path follows every bitmap's actual buffer pointer instead.
    auto begin=first_address,end=first_address+row*height;
    for(std::size_t i=0;i<std::size_t(view.nbitmaps);++i) {
        const auto& bitmap=view.bitmap[i];
        if(!bitmap.byte_count || !bitmap.width) {
            if(i<needed)throw std::invalid_argument("post-course Photo missing reached piece");
            continue;
        }
        const auto address=reinterpret_cast<std::uintptr_t>(bitmap.buf);
        if(!bitmap.buf || bitmap.byte_count>2*1024*1024 ||
           bitmap.byte_count>std::numeric_limits<std::uintptr_t>::max()-address)
            throw std::invalid_argument("post-course Photo bitmap extent");
        begin=std::min(begin,address);end=std::max(end,address+bitmap.byte_count);
        if(i>=needed)continue;
        const auto rows=std::min(tile,height-i*tile);
        if(rows>bitmap.byte_count/row)
            throw std::invalid_argument("post-course Photo reached rows exceed piece");
        // XOR-4 is applied to the actual destination address. Preflight its
        // final eight-byte lane, including non-eight-byte row strides, without
        // assuming any guessed preview tiling or width.
        for(std::size_t y=0;y<rows;++y)
            for(std::size_t tail=0;tail<std::min(std::size_t(view.width),std::size_t{4});++tail) {
                const auto offset=(y*row+(std::size_t(view.width)-1-tail)*2)^((y&1)?4u:0u);
                if(offset>bitmap.byte_count || bitmap.byte_count-offset<2)
                    throw std::invalid_argument("post-course Photo XOR lane exceeds piece");
            }
    }
    if(end-begin>2*1024*1024-7)
        throw std::invalid_argument("post-course Photo allocation extent");
    for(std::size_t i=0;i<std::size_t(view.nbitmaps);++i) {
        const auto& bitmap=view.bitmap[i];
        if(bitmap.byte_count && bitmap.width &&
           ((reinterpret_cast<std::uintptr_t>(bitmap.buf)-begin)&7u))
            throw std::invalid_argument("post-course Photo piece alignment");
    }
    for(const auto& bank:resources.banks()) {
        if(!bank.owner)continue;
        const auto base=reinterpret_cast<std::uintptr_t>(bank.owner->data());
        if(begin>=base && begin-base<=bank.owner->size() &&
           end-begin<=bank.owner->size()-(begin-base))
            return {&view,bank.owner->data()+std::size_t(begin-base),std::size_t(end-begin),
                    (std::size_t(end-begin)+7u)&~std::size_t{7}};
    }
    throw std::invalid_argument("post-course Photo pixels escape admitted initialized bank");
}
// Disjoint opaque kinds let observation recognize every issued Element even
// after its fixed raster slot has been erased/reused, without retaining history.
constexpr std::uint64_t element_kind=std::uint64_t{1}<<63;
struct State {
    struct ElementSlot {
        post_course::WindowRasterSlot memory;
        std::shared_ptr<Element> element;
        sprite::RasterBinding binding{};
        std::uint64_t serial=0;
    };
    struct Thumbnail {
        post_course::WindowRasterSlot memory;
        std::shared_ptr<sprite::Raster> raster;
        sprite::RasterBinding binding{};
        IsleGameObject* object=nullptr;
        IsleGameSprite* sprite=nullptr;
        std::uint32_t prototype=0;
        std::uint64_t serial=0;
        std::uint32_t credits_holds=0;
        bool credits=false;
    };
    struct ThumbnailAlias {
        IsleGameObject* object=nullptr;
        IsleGameSprite* sprite=nullptr;
        Thumbnail* thumbnail=nullptr;
    };
    struct ThumbnailStorage {
        std::shared_ptr<Heap> heap;
        void* block=nullptr;
        ~ThumbnailStorage(){if(block)heap->deallocate(block);}
    };
    struct Table {
        std::uint64_t serial=0;
        const IslePostCourseButton* source=nullptr;
        std::size_t count=0;
        std::unique_ptr<Button[]> converted;
        std::span<const Button> view{};
    };
    enum class Kind { none,print,fade,layout };
    struct Operation {
        std::uint64_t serial=0;
        Kind kind=Kind::none;
        ElementSlot* element=nullptr;
        Element::PrintOperation print;
        Frame::FadeOperation fade;
        Layout::Operation layout;
        StepResult result{};
    };
    IslePostCourseWindow bridge{this};
    scene::Services& services;
    Text& text;Frame& frame;Focus& focus;Layout& layout;Buttons& buttons;
    const post_course::Resources& resources;
    std::shared_ptr<Heap> heap;
    sprite::Producer& producer;
    ElementList list;
    ElementDefaults defaults{};
    IslePostCourseTextCallbacks callbacks{};
    post_course::WindowCapacity capacity;
    std::unique_ptr<ElementSlot[]> elements;
    std::unique_ptr<Operation[]> operations;
    std::unique_ptr<Table[]> tables;
    std::unique_ptr<Thumbnail[]> thumbnails;
    std::unique_ptr<ThumbnailAlias[]> thumbnail_aliases;
    scene::graphics::Frame* graphics=nullptr;
    std::size_t thumbnail_count=0;
    std::unique_ptr<IsleGameObject*[]> adopted;
    std::unique_ptr<bool[]> owned;
    std::uint64_t next_serial=0,next_element_serial=0,frame_serial=0;
    IsleGameResult failure=ISLE_GAME_OK;
    bool text_live=false,focus_live=false,layout_live=false;
    State(scene::Services& s,Text& t,Frame& f,Focus& m,Layout& l,Buttons& b,
          const post_course::Resources& r,std::shared_ptr<Heap> h,sprite::Producer& p,
          post_course::WindowCapacity c)
      :services(s),text(t),frame(f),focus(m),layout(l),buttons(b),resources(r),heap(std::move(h)),
       producer(p),capacity(c),elements(std::make_unique<ElementSlot[]>(c.rasters.size())),
       operations(std::make_unique<Operation[]>(c.operations)),tables(std::make_unique<Table[]>(c.button_tables)),
       thumbnails(std::make_unique<Thumbnail[]>(c.thumbnails.size())),
       thumbnail_aliases(std::make_unique<ThumbnailAlias[]>(c.adopted_objects)),thumbnail_count(c.thumbnails.size()),
       adopted(std::make_unique<IsleGameObject*[]>(c.adopted_objects)),
       owned(std::make_unique<bool[]>(c.adopted_objects)) {
        if (!heap || c.rasters.empty() || !c.operations || !c.buttons_per_table)
            throw std::invalid_argument("post-course window capacity");
        for (std::size_t i=0;i<c.rasters.size();++i) {
            const auto slot=c.rasters[i];
            if (!slot.plane_capacity || slot.plane_capacity>0x20000000u)
                throw std::invalid_argument("post-course window plane capacity");
            for (unsigned plane=0;plane<2;++plane) {
                if (plane && !slot.physical[plane]) continue;
                const std::uint64_t start=slot.physical[plane],end=start+slot.plane_capacity;
                if ((start&7u) || end>0x20000000ull || end<=start)
                    throw std::invalid_argument("post-course window physical range");
                for (std::size_t j=0;j<=i;++j) for (unsigned k=0;k<2;++k) {
                    if (j==i && k>=plane) break;
                    const auto& other=c.rasters[j];
                    if (k && !other.physical[k]) continue;
                    const std::uint64_t a=other.physical[k],z=a+other.plane_capacity;
                    if (start<z && a<end) throw std::invalid_argument("post-course overlapping window ranges");
                }
            }
            elements[i].memory=slot;
        }
        const bool credits=resources.kind()==post_course::Resources::Kind::credits;
        if (credits && (c.rasters.size()<14 || c.thumbnails.size()>5))
            throw std::invalid_argument("Credits requires fourteen Elements and at most five image slots");
        const unsigned thumbnail_planes=credits?1u:2u;
        for(std::size_t i=0;i<c.thumbnails.size();++i) {
            const auto slot=c.thumbnails[i];
            if(!slot.plane_capacity || slot.plane_capacity>0x20000000u ||
               (credits && (slot.plane_capacity<8u*3920u || slot.physical[1])))
                throw std::invalid_argument("post-course thumbnail plane capacity");
            for(unsigned plane=0;plane<thumbnail_planes;++plane) {
                const std::uint64_t start=slot.physical[plane],end=start+slot.plane_capacity;
                if((start&7u) || end>0x20000000ull || end<=start)
                    throw std::invalid_argument("post-course thumbnail physical range");
                for(const auto& other:c.rasters)for(unsigned k=0;k<2;++k) {
                    if(k && !other.physical[k])continue;
                    const std::uint64_t a=other.physical[k],z=a+other.plane_capacity;
                    if(start<z && a<end)throw std::invalid_argument("thumbnail overlaps Element reservation");
                }
                for(std::size_t j=0;j<=i;++j)for(unsigned k=0;k<thumbnail_planes;++k) {
                    if(j==i && k>=plane)break;
                    const auto& other=c.thumbnails[j];
                    const std::uint64_t a=other.physical[k],z=a+other.plane_capacity;
                    if(start<z && a<end)throw std::invalid_argument("overlapping thumbnail reservations");
                }
            }
            thumbnails[i].memory=slot;
        }
        for (std::size_t i=0;i<c.button_tables;++i)
            tables[i].converted=std::make_unique<Button[]>(c.buttons_per_table);
        capacity.rasters={}; // ElementSlot owns the caller configuration, not its span.
        capacity.thumbnails={};
    }
    IsleGameResult fail(IsleGameResult r) noexcept {
        if (r!=ISLE_GAME_OK && failure==ISLE_GAME_OK) failure=r;
        return r;
    }
    IsleGameResult status() noexcept {
        if (services.failed()) fail(ISLE_GAME_INVALID_STATE);
        return failure;
    }
    IsleGameResult ok(bool success=true) noexcept {
        if (!success) return fail(ISLE_GAME_INVALID_STATE);
        return status();
    }
    std::uint64_t serial() {
        if (next_serial==element_kind-1) throw std::overflow_error("window handle incarnations");
        return ++next_serial;
    }
    std::uint64_t element_serial() {
        if (next_element_serial==element_kind-1) throw std::overflow_error("window Element incarnations");
        return element_kind|++next_element_serial;
    }
    Handle handle(std::uint64_t n) const noexcept {return {this,n};}
    bool owns(Handle h) const noexcept {return h.owner==this && h.serial;}
    ElementSlot* element(Handle h) noexcept {
        if (owns(h)) for(std::size_t i=0;i<element_count();++i)
            if(elements[i].serial==h.serial && elements[i].element)return &elements[i];
        return nullptr;
    }
    std::size_t element_count() const noexcept {return raster_count;}
    std::size_t raster_count=0;
    Operation* operation(Handle h) noexcept {
        if(owns(h))for(std::size_t i=0;i<capacity.operations;++i)
            if(operations[i].serial==h.serial)return &operations[i];
        return nullptr;
    }
    Table* table(Handle h) noexcept {
        if(owns(h))for(std::size_t i=0;i<capacity.button_tables;++i)
            if(tables[i].serial==h.serial)return &tables[i];
        return nullptr;
    }
    Thumbnail* thumbnail(Handle h) noexcept {
        if(owns(h))for(std::size_t i=0;i<thumbnail_count;++i)
            if(thumbnails[i].serial==h.serial)return &thumbnails[i];
        return nullptr;
    }
    bool thumbnail_sprite(IsleGameSprite* sprite) const noexcept {
        for(std::size_t i=0;i<thumbnail_count;++i)
            if(thumbnails[i].serial && thumbnails[i].sprite==sprite)return true;
        return false;
    }
    IsleSprite* thumbnail_view(Thumbnail& t) noexcept {
        if(!t.raster || !t.binding.generation || !owns_sprite(t.sprite))return nullptr;
        auto* view=services.view(t.sprite);
        const auto& source=t.raster->view(sprite::RasterSource::primary);
        if(!view || view->bitmap!=source.bitmap || view->nbitmaps!=source.nbitmaps ||
           view->width!=source.width || view->height!=source.height ||
           view->bmfmt!=0 || view->bmsiz!=2)return nullptr;
        return view;
    }
    bool frame_handle(Handle h) const noexcept {return owns(h) && h.serial==frame_serial;}
    bool owns_object(IsleGameObject* object) const noexcept {
        if(!object)return false;
        for(std::size_t i=0;i<element_count();++i)
            if(elements[i].element && elements[i].element->attachment().object==object)return true;
        for(std::size_t i=0;i<capacity.adopted_objects;++i)if(adopted[i]==object)return true;
        return false;
    }
    static void object_joined(IsleGameObject* object,void* user) noexcept {
        auto& p=*static_cast<State*>(user);
        for(std::size_t i=0;i<p.capacity.adopted_objects;++i)if(p.adopted[i]==object) {
            p.adopted[i]=nullptr;p.owned[i]=false;
        }
        for(std::size_t i=0;i<p.capacity.adopted_objects;++i)if(p.thumbnail_aliases[i].object==object) {
            // A native join invalidates identities, not outstanding GPU backing.
            p.thumbnail_aliases[i].object=nullptr;p.thumbnail_aliases[i].sprite=nullptr;
        }
        // Real join invalidates native aliases, but graphics registration and
        // pixel storage retire only later, outside this native callback.
        for(std::size_t i=0;i<p.thumbnail_count;++i)if(p.thumbnails[i].object==object) {
            p.thumbnails[i].object=nullptr;p.thumbnails[i].sprite=nullptr;
        }
    }
    bool owns_sprite(IsleGameSprite* sprite) noexcept {
        scene::SpriteHandle owner;IsleGameSprite *parent,*child,*next,*previous;
        return services.sprite_links(sprite,owner,parent,child,next,previous) && owns_object(owner.object);
    }
    void refresh(Table& t) {
        if(!t.source)return;
        for(std::size_t i=0;i<t.count;++i)t.converted[i]={t.source[i].id,bytes(t.source[i].text)};
    }
    Operation* acquire(Kind kind,Handle* output) {
        if(!output)return nullptr;
        *output={};
        for(std::size_t i=0;i<capacity.operations;++i)if(!operations[i].serial) {
            auto& op=operations[i];op.serial=serial();op.kind=kind;op.element=nullptr;
            op.result={StepStatus::more};*output=handle(op.serial);return &op;
        }
        fail(ISLE_GAME_EXHAUSTED);return nullptr;
    }
    bool terminal(const Operation& op) const noexcept {return op.result.status==StepStatus::done || op.result.status==StepStatus::error;}
    void cancel(Operation& op) noexcept {
        if(terminal(op))return;
        switch(op.kind) {
        case Kind::print:if(op.element && op.element->element)op.element->element->cancel_print(op.print);break;
        case Kind::fade:frame.cancel_fade(op.fade);break;
        case Kind::layout:layout.cancel(op.layout);break;
        case Kind::none:break;
        }
        op.result={StepStatus::error,StepError::cancelled};
    }
    void cancel_all() noexcept {for(std::size_t i=0;i<capacity.operations;++i)if(operations[i].serial)cancel(operations[i]);}
    Step finish(Operation& op,StepResult r) noexcept {
        // A rejected acknowledgement can leave the underlying operation live.
        // Release that claim before publishing the terminal veneer error.
        if(r.status==StepStatus::error)cancel(op);
        op.result=r;
        if(r.status==StepStatus::error && r.error!=StepError::cancelled)fail(ISLE_GAME_INVALID_STATE);
        return step_value(r,op.kind==Kind::print?&op.print.event():nullptr);
    }
    IsleGameResult erase(ElementSlot& slot) noexcept {
        for(std::size_t i=0;i<capacity.operations;++i)if(operations[i].serial && operations[i].element==&slot)cancel(operations[i]);
        if(slot.element)slot.element->detach();
        try {if(slot.binding.generation)producer.unregister_raster(slot.binding);}
        catch(...) {return fail(ISLE_GAME_INVALID_STATE);}
        slot.binding={};slot.element.reset();slot.serial=0;
        return services.failed()?fail(ISLE_GAME_INVALID_STATE):ISLE_GAME_OK;
    }
    IsleGameResult release_thumbnail(Thumbnail& t) noexcept {
        if(t.credits_holds)return ISLE_GAME_INVALID_STATE;
        bool aliased=false;
        for(std::size_t i=0;i<capacity.adopted_objects;++i)if(thumbnail_aliases[i].thumbnail==&t) {
            if(thumbnail_aliases[i].object)return fail(ISLE_GAME_INVALID_STATE);
            aliased=true;
        }
        if(aliased && (!graphics || !graphics->quiescent()))return fail(ISLE_GAME_INVALID_STATE);
        scene::SpriteHandle owner;IsleGameSprite *parent,*child,*next,*previous;
        const bool live=t.sprite && services.sprite_links(t.sprite,owner,parent,child,next,previous);
        if(live && owner.object!=t.object)return fail(ISLE_GAME_INVALID_STATE);
        IsleSprite* view=live?services.view(t.sprite):nullptr;
        if(live && !view)return fail(ISLE_GAME_INVALID_STATE);
        try {
            // Producer rejects outstanding actual graphics borrows BEFORE
            // changing registration or the canonical native descriptor.
            if(t.binding.generation){producer.unregister_raster(t.binding);t.binding={};}
            // A cached Report destination may currently select a different
            // immutable root. Its descriptor/resource binding stays untouched.
            if(view && view->bitmap==t.raster->view(sprite::RasterSource::primary).bitmap) {
                const auto index=resources.sprite_index(t.prototype);
                auto replacement=*view;
                replacement.bitmap=resources.roots()[index]->bitmap;
                const bool restored=services.failed() ?
                    services.restore_sprite_after_failure(t.sprite,index,replacement) :
                    services.replace_sprite(t.sprite,index,replacement);
                if(!restored)return fail(ISLE_GAME_INVALID_STATE);
            }
        }catch(...){return fail(ISLE_GAME_INVALID_STATE);}
        for(std::size_t i=0;i<capacity.adopted_objects;++i)
            if(thumbnail_aliases[i].thumbnail==&t)thumbnail_aliases[i]={};
        t.raster.reset();t.object=nullptr;t.sprite=nullptr;t.prototype=0;t.serial=0;t.credits=false;
        return ISLE_GAME_OK;
    }
    IsleGameResult retire() noexcept {
        for(std::size_t i=0;i<thumbnail_count;++i)if(thumbnails[i].credits_holds)return ISLE_GAME_INVALID_STATE;
        cancel_all();
        // These dependencies retire only the objects initialized by this veneer.
        if(layout_live){layout.retire();layout_live=false;}
        if(focus_live){focus.retire();focus_live=false;}
        if(frame_serial){frame.retire();frame_serial=0;}
        if(text_live){text.detach();text_live=false;}
        buttons.clear();
        bool released=true;
        for(std::size_t i=0;i<element_count();++i)if(elements[i].element || elements[i].binding.generation)
            if(erase(elements[i])!=ISLE_GAME_OK && elements[i].binding.generation)released=false;
        for(std::size_t i=0;i<capacity.adopted_objects;++i)if(auto* object=adopted[i]) {
            if(owned[i]) {
                services.erase(object);
                if(adopted[i])released=false; // Real join must run object_joined.
            }
        }
        for(std::size_t i=0;i<thumbnail_count;++i)if(thumbnails[i].serial)
            if(release_thumbnail(thumbnails[i])!=ISLE_GAME_OK)released=false;
        for(std::size_t i=0;i<capacity.adopted_objects;++i)if(adopted[i] && !owned[i]) {
            bool retained=false;
            for(std::size_t j=0;j<thumbnail_count;++j)
                if(thumbnails[j].serial && thumbnails[j].object==adopted[i])retained=true;
            if(retained){released=false;continue;}
            if(services.unobserve_lifecycle(adopted[i],this))adopted[i]=nullptr;
            else released=false;
        }
        return released?ISLE_GAME_OK:ISLE_GAME_INVALID_STATE;
    }
};
State* state(IslePostCourseWindow* s) noexcept {
    auto* p=s?static_cast<State*>(s->owner):nullptr;
    return p && &p->bridge==s?p:nullptr;
}
const State* state(const IslePostCourseWindow* s) noexcept {
    const auto* p=s?static_cast<const State*>(s->owner):nullptr;
    return p && &p->bridge==s?p:nullptr;
}
IsleGameResult observation_status(const State& p) noexcept {
    if(p.failure!=ISLE_GAME_OK)return p.failure;
    if(p.services.failed())return ISLE_GAME_INVALID_STATE;
    return isle_game_status(p.services.game);
}
IsleGameResult observe_native_sprite(const State& p,IsleGameSprite* sprite,
    IsleGameObject* expected,const IsleSprite** output) noexcept {
    if(!sprite)return ISLE_GAME_OK;
    IsleGameObject* object=nullptr;
    auto result=isle_game_sprite_object(p.services.game,sprite,&object);
    if(result!=ISLE_GAME_OK)return result;
    if(expected && object!=expected)return ISLE_GAME_INVALID_STATE;
    if(!p.owns_object(object))return ISLE_GAME_INVALID_ARGUMENT;
    IsleSprite* view=nullptr;
    result=isle_game_sprite_view(p.services.game,sprite,&view);
    if(result==ISLE_GAME_OK)*output=view;
    return result;
}
template<class F> IsleGameResult invoke(IslePostCourseWindow* s,F&& f) noexcept {
    auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;
    if(p->status()!=ISLE_GAME_OK)return p->status();
    try {return p->fail(f(*p));}
    catch(const std::bad_alloc&){return p->fail(ISLE_GAME_EXHAUSTED);}
    catch(const std::invalid_argument&){return p->fail(ISLE_GAME_INVALID_ARGUMENT);}
    catch(...){return p->fail(ISLE_GAME_INVALID_STATE);}
}
template<class F> IsleGameResult element_call(IslePostCourseWindow* s,Handle h,F&& f) noexcept {
    return invoke(s,[&](State& p){auto* e=p.element(h);if(!e)return ISLE_GAME_INVALID_ARGUMENT;
        f(p,*e->element);return p.ok();});
}
template<class F> IsleGameResult frame_call(IslePostCourseWindow* s,Handle h,F&& f) noexcept {
    return invoke(s,[&](State& p){if(!p.frame_handle(h))return ISLE_GAME_INVALID_ARGUMENT;f(p.frame);return p.ok();});
}
template<class F> IsleGameResult focus_call(IslePostCourseWindow* s,F&& f) noexcept {
    return invoke(s,[&](State& p){if(!p.focus_live)return ISLE_GAME_INVALID_STATE;f(p.focus);return p.ok();});
}
template<class F> IsleGameResult layout_call(IslePostCourseWindow* s,F&& f) noexcept {
    return invoke(s,[&](State& p){if(!p.layout_live)return ISLE_GAME_INVALID_STATE;f(p.layout);return p.ok();});
}
template<class F> Step begin(IslePostCourseWindow* s,State::Kind kind,Handle* out,F&& f) noexcept {
    if(out)*out={};auto* p=state(s);if(!p)return error_step();
    if(!out){p->fail(ISLE_GAME_INVALID_ARGUMENT);return error_step();}
    if(p->status()!=ISLE_GAME_OK)return error_step(StepError::native_failure);
    State::Operation* op=nullptr;
    try {
        op=p->acquire(kind,out);
        if(!op)return error_step(StepError::native_failure);
        return p->finish(*op,f(*p,*op));
    }catch(...) {
        if(op){p->cancel(*op);op->result={StepStatus::error,StepError::native_failure};}
        p->fail(ISLE_GAME_INVALID_STATE);return error_step(StepError::native_failure);
    }
}
}
namespace isle::post_course {
struct Window::Impl:State {using State::State;};
Window::Window(scene::Services& s,window::Text& t,window::Frame& f,window::Focus& m,
               window::Layout& l,window::Buttons& b,const Resources& r,
               std::shared_ptr<window::Heap> h,sprite::Producer& p,WindowCapacity c)
  :p_(std::make_unique<Impl>(s,t,f,m,l,b,r,std::move(h),p,c)){p_->raster_count=c.rasters.size();}
Window::~Window(){if(retire()!=ISLE_GAME_OK)std::terminate();}
IslePostCourseWindow* Window::source() noexcept {return &p_->bridge;}
IsleGameResult Window::retire() noexcept {return p_->retire();}
IsleGameResult Window::bind_graphics_frame(scene::graphics::Frame& frame) noexcept {
    if(p_->graphics || !frame.quiescent())return p_->fail(ISLE_GAME_INVALID_STATE);
    p_->graphics=&frame;return p_->status();
}
std::size_t Window::thumbnail_plane_bytes(const Resources& resources,std::uint32_t identity) {
    return photo_prototype(resources,identity).plane_bytes;
}
IsleGameResult Window::observe_element(const IslePostCourseWindow* s,
    IslePostCourseElement h,const IsleSprite** output) noexcept {
    if(output)*output=nullptr;
    const auto* p=state(s);
    if(!p || !output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto status=observation_status(*p);
    if(status!=ISLE_GAME_OK)return status;
    if(!h.owner && !h.serial)return ISLE_GAME_OK;
    const auto incarnation=h.serial&~element_kind;
    if(h.owner!=p || !(h.serial&element_kind) || !incarnation ||
       incarnation>p->next_element_serial)return ISLE_GAME_INVALID_ARGUMENT;
    for(std::size_t i=0;i<p->element_count();++i) {
        const auto& slot=p->elements[i];
        if(slot.serial!=h.serial || !slot.element)continue;
        const auto attachment=slot.element->attachment();
        if(!attachment.object && !attachment.sprite)return ISLE_GAME_OK;
        if(!attachment.object || !attachment.sprite)return ISLE_GAME_INVALID_STATE;
        const auto result=observe_native_sprite(*p,attachment.sprite,attachment.object,output);
        // A genuine owned Element can outlive its native attachment. Its old
        // incarnation must never resolve to a later raster-slot replacement.
        return result==ISLE_GAME_INVALID_ARGUMENT?ISLE_GAME_OK:result;
    }
    return ISLE_GAME_OK;
}
IsleGameResult Window::observe_sprite(const IslePostCourseWindow* s,
    IsleGameSprite* sprite,const IsleSprite** output) noexcept {
    if(output)*output=nullptr;
    const auto* p=state(s);
    if(!p || !output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto status=observation_status(*p);
    return status==ISLE_GAME_OK?observe_native_sprite(*p,sprite,nullptr,output):status;
}
IsleGameResult Window::observe_sprite_root(const IslePostCourseWindow* s,
    IsleGameObject* object,const IsleSprite** output) noexcept {
    if(output)*output=nullptr;
    const auto* p=state(s);
    if(!p || !output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto status=observation_status(*p);
    if(status!=ISLE_GAME_OK)return status;
    if(!object)return ISLE_GAME_OK;
    if(!p->owns_object(object))return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameSprite* root=nullptr;
    const auto result=isle_game_object_sprite(p->services.game,object,&root);
    if(result==ISLE_GAME_INVALID_ARGUMENT)return ISLE_GAME_OK;
    if(result!=ISLE_GAME_OK)return result;
    IsleSprite* view=nullptr;
    const auto borrowed=isle_game_sprite_view(p->services.game,root,&view);
    if(borrowed==ISLE_GAME_OK)*output=view;
    return borrowed;
}
IsleGameResult Window::observe_layout_slot_id(const IslePostCourseWindow* s,
    std::size_t slot,int* output) noexcept {
    if(output)*output=35;
    const auto* p=state(s);
    if(!p || !output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto status=observation_status(*p);
    if(status!=ISLE_GAME_OK)return status;
    if(!p->layout_live)return ISLE_GAME_INVALID_STATE;
    const auto live=p->layout.buttons();
    if(slot>=live.size())return ISLE_GAME_INVALID_ARGUMENT;
    *output=live[slot].id;
    return ISLE_GAME_OK;
}
IsleGameResult Window::observe_operations(const IslePostCourseWindow* s,
    WindowObservation* output) noexcept {
    if(output)*output={};
    const auto* p=state(s);
    if(!p || !output)return ISLE_GAME_INVALID_ARGUMENT;
    const auto status=observation_status(*p);
    if(status!=ISLE_GAME_OK)return status;
    for(std::size_t i=0;i<p->capacity.operations;++i) {
        const auto& op=p->operations[i];
        if(!op.serial || p->terminal(op))continue;
        switch(op.kind) {
        case State::Kind::print:++output->prints;break;
        case State::Kind::fade:++output->fades;break;
        case State::Kind::layout:++output->layouts;break;
        case State::Kind::none:break;
        }
    }
    output->printing=p->text.printing();
    output->skip_enabled=p->text.skip_enabled();
    return ISLE_GAME_OK;
}
}
extern "C" {
IsleGameResult isle_post_window_status(IslePostCourseWindow* s){auto* p=state(s);return p?p->status():ISLE_GAME_INVALID_ARGUMENT;}
IsleGame* isle_post_window_game(IslePostCourseWindow* s){auto* p=state(s);return p?p->services.game:nullptr;}
IsleGameResult isle_post_window_fail(IslePostCourseWindow* s,IsleGameResult r){auto* p=state(s);return p?p->fail(r):ISLE_GAME_INVALID_ARGUMENT;}
IsleGameResult isle_post_window_text_initialize(IslePostCourseWindow* s){return invoke(s,[](State& p){if(p.text_live)return ISLE_GAME_INVALID_STATE;p.text_live=p.text.initialize(p.services);if(p.text_live)p.callbacks={};return p.ok(p.text_live);});}
IsleGameResult isle_post_window_text_font(IslePostCourseWindow* s,int v){return invoke(s,[&](State& p){p.text.set_font(v);return p.ok();});}
IsleGameResult isle_post_window_text_spacing(IslePostCourseWindow* s,int x,int y){return invoke(s,[&](State& p){p.text.set_spacing(x,y);return p.ok();});}
IsleGameResult isle_post_window_text_shadow_offset(IslePostCourseWindow* s,int v){return invoke(s,[&](State& p){p.text.set_shadow_offset(v);return p.ok();});}
IsleGameResult isle_post_window_text_shadow_color(IslePostCourseWindow* s,IslePostCourseColor c){return invoke(s,[&](State& p){p.text.set_shadow_color(color(c));return p.ok();});}
IsleGameResult isle_post_window_text_callbacks(IslePostCourseWindow* s,const IslePostCourseTextCallbacks* c){return invoke(s,[&](State& p){p.callbacks=c?*c:IslePostCourseTextCallbacks{};return p.ok();});}
IsleGameResult isle_post_window_text_get_callbacks(IslePostCourseWindow* s,IslePostCourseTextCallbacks* out){if(out)*out={};return invoke(s,[&](State& p){if(!out)return ISLE_GAME_INVALID_ARGUMENT;*out=p.callbacks;return p.ok();});}
IsleGameResult isle_post_window_text_delay(IslePostCourseWindow* s,int v){return invoke(s,[&](State& p){p.text.set_print_delay(v);return p.ok();});}
IsleGameResult isle_post_window_text_proportional(IslePostCourseWindow* s,int v){return invoke(s,[&](State& p){p.text.set_proportional(v!=0);return p.ok();});}
IsleGameResult isle_post_window_text_skip(IslePostCourseWindow* s,int v){return invoke(s,[&](State& p){p.text.set_skip_enabled(v!=0);return p.ok();});}
IsleGameResult isle_post_window_text_input(IslePostCourseWindow* s,uint32_t v){return invoke(s,[&](State& p){p.text.input(v);return p.ok();});}
IsleGameResult isle_post_window_text_width(IslePostCourseWindow* s,IslePostCourseBytes v,int* out){if(out)*out=0;return invoke(s,[&](State& p){if(!out)return ISLE_GAME_INVALID_ARGUMENT;*out=p.text.string_width(bytes(v));return p.ok();});}
IsleGameResult isle_post_window_element_create(IslePostCourseWindow* s,int x,int y,int w,int h,uint32_t flags,Handle* out){
    if(out)*out={};return invoke(s,[&](State& p){
        if(!out || w<=0 || w>320 || h<=0 || h>240)return ISLE_GAME_INVALID_ARGUMENT;
        const int border=flags&window::border?4:flags&window::thick_border?8:0;
        const int bpp=flags&bits8?1:flags&bits32?4:2;
        const auto stride=(32+w+2*border+(bpp>>2)*16+15)&~15;
        const auto tile=4096/bpp/stride;
        const auto plane=std::size_t(stride*tile*((h+2*border+tile-1)/tile)*bpp);
        State::ElementSlot* slot=nullptr;
        for(std::size_t i=0;i<p.element_count();++i) {
            auto& candidate=p.elements[i];
            if(candidate.element || candidate.binding.generation ||
               candidate.memory.plane_capacity<plane ||
               ((flags&window::use_alt_image) && !candidate.memory.physical[1]))continue;
            if(!slot || candidate.memory.plane_capacity<slot->memory.plane_capacity)slot=&candidate;
        }
        if(!slot)return ISLE_GAME_EXHAUSTED;
        slot->element=Element::create(p.heap,p.list,x,y,w,h,flags,slot->memory.physical,p.defaults);
        if(!slot->element)return ISLE_GAME_EXHAUSTED;
        slot->serial=p.element_serial();
        try {slot->binding=p.producer.register_raster(slot->element->raster());}
        catch(...){slot->element.reset();slot->serial=0;throw;}
        slot->element->set_buttons(&p.buttons);
        if(!slot->element->attach(p.services,slot->binding)){p.erase(*slot);return ISLE_GAME_INVALID_STATE;}
        *out=p.handle(slot->serial);return p.ok();
    });
}
IsleGameResult isle_post_window_element_delete(IslePostCourseWindow* s,Handle h){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;auto* e=p->element(h);return e?p->erase(*e):p->fail(ISLE_GAME_INVALID_ARGUMENT);}
IsleGameResult isle_post_window_element_position(IslePostCourseWindow* s,Handle h,int x,int y){return element_call(s,h,[&](State&,Element& e){e.set_pos(x,y);});}
IsleGameResult isle_post_window_element_alt_position(IslePostCourseWindow* s,Handle h,int x,int y){return element_call(s,h,[&](State&,Element& e){e.set_alt_pos(x,y);});}
IsleGameResult isle_post_window_element_visible(IslePostCourseWindow* s,Handle h,int v){return element_call(s,h,[&](State&,Element& e){e.set_visible(v!=0);});}
IsleGameResult isle_post_window_element_scale(IslePostCourseWindow* s,Handle h,float x,float y){return element_call(s,h,[&](State&,Element& e){e.set_scale(x,y);});}
IsleGameResult isle_post_window_element_sprite_color(IslePostCourseWindow* s,Handle h,IslePostCourseColor c){return element_call(s,h,[&](State&,Element& e){e.set_sprite_color(color(c));});}
IsleGameResult isle_post_window_element_color(IslePostCourseWindow* s,Handle h,int fg,IslePostCourseColor c){
    return invoke(s,[&](State& p){
        auto* slot=p.element(h);
        if(!slot && (h.owner || h.serial))return ISLE_GAME_INVALID_ARGUMENT;
        if(fg)p.defaults.foreground=color(c);else p.defaults.background=color(c);
        if(slot) {
            auto& e=*slot->element;e.set_color(fg!=0,color(c));
            p.text.palettes(e.flags()&grayscale?Color{255,255,255,p.defaults.foreground.a}:p.defaults.foreground,
                            e.flags()&grayscale?Color{255,255,255,p.defaults.background.a}:p.defaults.background);
        }
        return p.ok();
    });
}
IsleGameResult isle_post_window_element_font(IslePostCourseWindow* s,Handle h,int v){
    return invoke(s,[&](State& p){
        auto* e=p.element(h);if(!e && (h.owner || h.serial))return ISLE_GAME_INVALID_ARGUMENT;
        p.text.set_font(v);if(e)e->element->set_font(v);else p.defaults.font=v;
        return p.ok();
    });
}
IsleGameResult isle_post_window_element_text_position(IslePostCourseWindow* s,Handle h,int x,int y){return element_call(s,h,[&](State&,Element& e){e.set_text_pos(x,y);});}
IsleGameResult isle_post_window_element_fill(IslePostCourseWindow* s,Handle h,int x,int y,int z,int w,IslePostCourseColor c){return element_call(s,h,[&](State&,Element& e){e.fill(x,y,z,w,color(c));});}
IsleGameResult isle_post_window_element_fill_default(IslePostCourseWindow* s,Handle h,int x,int y,int z,int w){return element_call(s,h,[&](State&,Element& e){e.fill(x,y,z,w,e.background());});}
IsleGameResult isle_post_window_element_clear(IslePostCourseWindow* s,Handle h){return element_call(s,h,[](State&,Element& e){e.clear();});}
IsleGameResult isle_post_window_element_background(IslePostCourseWindow* s,Handle h){return element_call(s,h,[](State&,Element& e){e.draw_background();});}
IsleGameResult isle_post_window_element_scroll(IslePostCourseWindow* s,Handle h,int v,unsigned d){return element_call(s,h,[&](State&,Element& e){e.scroll(v,d);});}
IsleGameResult isle_post_window_element_vertical_scroll(IslePostCourseWindow* s,Handle h,int v,unsigned d,int x,int y,int z,int w){return element_call(s,h,[&](State&,Element& e){e.vertical_scroll(v,d,x,y,z,w);});}
IsleGameResult isle_post_window_element_image(IslePostCourseWindow* s,Handle h,int x,int y,int w,int z,const uint8_t* data,size_t n){if(!data && n)return isle_post_window_fail(s,ISLE_GAME_INVALID_ARGUMENT);return element_call(s,h,[&](State&,Element& e){e.draw_image(x,y,w,z,{data,n});});}
IsleGameResult isle_post_window_element_swap(IslePostCourseWindow* s,Handle h){return element_call(s,h,[](State&,Element& e){e.swap_images();});}
IsleGameResult isle_post_window_element_alt_sprite(IslePostCourseWindow* s,Handle h,uint32_t id){return element_call(s,h,[&](State& p,Element& e){const auto i=p.resources.sprite_index(id);e.set_alt_sprite(i,*p.resources.roots()[i]);});}
IsleGameResult isle_post_window_element_show_alt(IslePostCourseWindow* s,Handle h,int v){return element_call(s,h,[&](State&,Element& e){e.show_alt_sprite(v!=0);});}
IsleGameResult isle_post_window_element_move_head(IslePostCourseWindow* s,Handle h){return element_call(s,h,[](State& p,Element& e){p.list.move_head(e);});}
IsleGameResult isle_post_window_element_move_tail(IslePostCourseWindow* s,Handle h){return element_call(s,h,[](State& p,Element& e){p.list.move_tail(e);});}
IsleGameResult isle_post_window_element_attachment(IslePostCourseWindow* s,Handle h,IsleGameObject** o,IsleGameSprite** v){if(o)*o=nullptr;if(v)*v=nullptr;return invoke(s,[&](State& p){auto* e=p.element(h);if(!e || !o || !v)return ISLE_GAME_INVALID_ARGUMENT;auto a=e->element->attachment();*o=a.object;*v=a.sprite;return p.ok(a.object && a.sprite);});}
IsleGameResult isle_post_window_adopt_object(IslePostCourseWindow* s,IsleGameObject* object){
    return invoke(s,[&](State& p){
        IsleGameSprite* root=nullptr;
        if(!p.services.sprite_root(object,root))return ISLE_GAME_INVALID_ARGUMENT;
        if(p.owns_object(object))return p.ok();
        for(std::size_t i=0;i<p.capacity.adopted_objects;++i)if(!p.adopted[i]) {
            if(!p.services.observe_lifecycle(object,{nullptr,State::object_joined,&p}))return p.ok(false);
            p.adopted[i]=object;p.owned[i]=false;return p.ok();
        }
        return ISLE_GAME_EXHAUSTED;
    });
}
IsleGameResult isle_post_window_release_object(IslePostCourseWindow* s,IsleGameObject* object){
    auto* p=state(s);if(!p || !object)return ISLE_GAME_INVALID_ARGUMENT;
    for(std::size_t i=0;i<p->capacity.adopted_objects;++i)if(p->adopted[i]==object) {
        if(p->owned[i])return p->fail(ISLE_GAME_INVALID_STATE);
        for(std::size_t j=0;j<p->thumbnail_count;++j)
            if(p->thumbnails[j].serial && p->thumbnails[j].object==object)
                return p->fail(ISLE_GAME_INVALID_STATE);
        if(!p->services.unobserve_lifecycle(object,p))return p->fail(ISLE_GAME_INVALID_STATE);
        p->adopted[i]=nullptr;return ISLE_GAME_OK;
    }
    return p->fail(ISLE_GAME_INVALID_ARGUMENT);
}
IsleGameResult isle_post_window_sprite_object_create(IslePostCourseWindow* s,int link,uint32_t identity,
                                                    IsleGameObject** object,IsleGameSprite** sprite){
    if(object)*object=nullptr;if(sprite)*sprite=nullptr;
    return invoke(s,[&](State& p){
        if(!object || !sprite || link<0 || link>=32)return ISLE_GAME_INVALID_ARGUMENT;
        std::size_t slot=0;
        while(slot<p.capacity.adopted_objects && p.adopted[slot])++slot;
        if(slot==p.capacity.adopted_objects)return ISLE_GAME_EXHAUSTED;
        auto* o=p.services.object(1u<<link,static_cast<std::uint8_t>(link),static_cast<std::uint32_t>(link));
        if(!o)return p.ok(false);
        if(!p.services.observe_lifecycle(o,{nullptr,State::object_joined,&p})){
            p.services.erase(o);return p.ok(false);
        }
        p.adopted[slot]=o;p.owned[slot]=true;
        auto* v=p.services.append_sprite(o,identity);
        if(!v || !p.services.set_render(o,{30,static_cast<std::uint32_t>(link),0x40000000u})){
            p.services.erase(o);return p.ok(false);
        }
        *object=o;*sprite=v;return p.ok();
    });
}
IsleGameResult isle_post_window_sprite_object_delete(IslePostCourseWindow* s,IsleGameObject* object){
    auto* p=state(s);if(!p || !object)return ISLE_GAME_INVALID_ARGUMENT;
    for(std::size_t i=0;i<p->capacity.adopted_objects;++i)if(p->adopted[i]==object) {
        if(!p->owned[i])return p->fail(ISLE_GAME_INVALID_ARGUMENT);
        p->services.erase(object);return p->status();
    }
    return p->fail(ISLE_GAME_INVALID_ARGUMENT);
}
IsleGameResult isle_post_window_sprite_root(IslePostCourseWindow* s,IsleGameObject* o,IsleGameSprite** out){if(out)*out=nullptr;return invoke(s,[&](State& p){if(!out || !p.owns_object(o))return ISLE_GAME_INVALID_ARGUMENT;return p.ok(p.services.sprite_root(o,*out));});}
IsleGameResult isle_post_window_sprite_view(IslePostCourseWindow* s,IsleGameSprite* v,IsleSprite** out){if(out)*out=nullptr;return invoke(s,[&](State& p){if(!out || !p.owns_sprite(v))return ISLE_GAME_INVALID_ARGUMENT;*out=p.services.view(v);return p.ok(*out!=nullptr);});}
IsleGameResult isle_post_window_sprite_links(IslePostCourseWindow* s,IsleGameSprite* v,IsleGameObject** o,IsleGameSprite** parent,IsleGameSprite** child,IsleGameSprite** next,IsleGameSprite** previous){
    if(o)*o=nullptr;if(parent)*parent=nullptr;if(child)*child=nullptr;if(next)*next=nullptr;if(previous)*previous=nullptr;
    return invoke(s,[&](State& p){if(!o || !parent || !child || !next || !previous)return ISLE_GAME_INVALID_ARGUMENT;
        scene::SpriteHandle owner;IsleGameSprite *a,*b,*c,*d;
        if(!p.services.sprite_links(v,owner,a,b,c,d) || !p.owns_object(owner.object))return ISLE_GAME_INVALID_ARGUMENT;
        *o=owner.object;*parent=a;*child=b;*next=c;*previous=d;return p.ok();});
}
IsleGameResult isle_post_window_sprite_sibling(IslePostCourseWindow* s,IsleGameSprite* v,uint32_t id,IsleGameSprite** out){if(out)*out=nullptr;return invoke(s,[&](State& p){if(!out || !p.owns_sprite(v))return ISLE_GAME_INVALID_ARGUMENT;*out=p.services.add_sprite_sibling(v,id);return p.ok(*out!=nullptr);});}
IsleGameResult isle_post_window_sprite_child(IslePostCourseWindow* s,IsleGameSprite* v,uint32_t id,IsleGameSprite** out){if(out)*out=nullptr;return invoke(s,[&](State& p){if(!out || !p.owns_sprite(v))return ISLE_GAME_INVALID_ARGUMENT;*out=p.services.append_sprite_child(v,id);return p.ok(*out!=nullptr);});}
IsleGameResult isle_post_window_sprite_rebind(IslePostCourseWindow* s,IsleGameSprite* v,uint32_t id){return invoke(s,[&](State& p){if(!p.owns_sprite(v) || p.thumbnail_sprite(v))return ISLE_GAME_INVALID_ARGUMENT;return p.ok(p.services.rebind_sprite(v,id));});}
IsleGameResult isle_post_window_sprite_assign_thumbnail(IslePostCourseWindow* s,
    IsleGameSprite* destination,IsleGameSprite* origin){
    return invoke(s,[&](State& p){
        if(!p.graphics || !p.graphics->quiescent())return ISLE_GAME_INVALID_STATE;
        if(p.resources.kind()!=post_course::Resources::Kind::album || destination==origin ||
           !p.owns_sprite(destination) || !p.owns_sprite(origin) || p.thumbnail_sprite(destination))
            return ISLE_GAME_INVALID_ARGUMENT;
        State::Thumbnail* thumbnail=nullptr;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(p.thumbnails[i].serial && p.thumbnails[i].sprite==origin)thumbnail=&p.thumbnails[i];
        if(!thumbnail || thumbnail->prototype!=0x80208b10u)return ISLE_GAME_INVALID_ARGUMENT;
        const auto* view=p.thumbnail_view(*thumbnail);if(!view)return ISLE_GAME_INVALID_STATE;
        const auto* previous=p.services.view(destination);
        const auto& prototype=*p.resources.roots()[p.resources.sprite_index(0x80208b10u)];
        if(!previous || previous->bitmap!=prototype.bitmap || previous->nbitmaps!=prototype.nbitmaps)
            return ISLE_GAME_INVALID_STATE;
        State::ThumbnailAlias* alias=nullptr;
        for(std::size_t i=0;i<p.capacity.adopted_objects;++i){
            auto& candidate=p.thumbnail_aliases[i];
            if(candidate.sprite==destination)return ISLE_GAME_INVALID_STATE;
            // Joined aliases are reusable only under this actual GPU fence.
            if(!candidate.object)candidate={};
            if(!candidate.thumbnail && !alias)alias=&candidate;
        }
        if(!alias)return ISLE_GAME_EXHAUSTED;
        scene::SpriteHandle owner;IsleGameSprite *parent,*child,*next,*previous_link;
        if(!p.services.sprite_links(destination,owner,parent,child,next,previous_link))
            return ISLE_GAME_INVALID_ARGUMENT;
        if(!p.services.replace_sprite(destination,thumbnail->binding.resources[0],*view,
            thumbnail->binding.generation))return ISLE_GAME_INVALID_STATE;
        *alias={owner.object,destination,thumbnail};return p.ok();
    });
}
IsleGameResult isle_post_window_sprite_clear_root_links(IslePostCourseWindow* s,IsleGameSprite* sprite){
    return invoke(s,[&](State& p){
        if(!p.graphics || !p.graphics->quiescent())return ISLE_GAME_INVALID_STATE;
        if(!p.owns_sprite(sprite))return ISLE_GAME_INVALID_ARGUMENT;
        return p.ok(p.services.clear_sprite_root_links(sprite));
    });
}
IsleGameResult isle_post_window_sprite_add(IslePostCourseWindow* s,IsleGameObject* object,
    uint32_t identity,IsleGameSprite** out){
    if(out)*out=nullptr;
    return invoke(s,[&](State& p){
        if(!out || !p.owns_object(object))return ISLE_GAME_INVALID_ARGUMENT;
        auto* sprite=p.services.append_sprite(object,identity);
        if(!sprite)return p.ok(false);
        *out=sprite;return p.ok();
    });
}
IsleGameResult isle_post_window_sprite_remove(IslePostCourseWindow* s,IsleGameObject* object){
    return invoke(s,[&](State& p){
        if(!p.owns_object(object))return ISLE_GAME_INVALID_ARGUMENT;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(p.thumbnails[i].serial && p.thumbnails[i].object==object)return ISLE_GAME_INVALID_STATE;
        for(std::size_t i=0;i<p.capacity.adopted_objects;++i)
            if(p.thumbnail_aliases[i].object==object)return ISLE_GAME_INVALID_STATE;
        return p.ok(p.services.remove_sprites(object));
    });
}
IsleGameResult isle_post_window_sprite_clear_link(IslePostCourseWindow* s,IsleGameSprite* sprite,int which){
    return invoke(s,[&](State& p){
        if((which!=0 && which!=1) || !p.owns_sprite(sprite))return ISLE_GAME_INVALID_ARGUMENT;
        return p.ok(p.services.clear_sprite_link(sprite,which==1));
    });
}
IsleGameResult isle_post_window_sprite_aux_get(IslePostCourseWindow* s,IsleGameSprite* sprite,int32_t* out){
    if(out)*out=0;
    return invoke(s,[&](State& p){
        if(!out || !p.owns_sprite(sprite))return ISLE_GAME_INVALID_ARGUMENT;
        return p.fail(isle_game_sprite_aux_get(p.services.game,sprite,out));
    });
}
IsleGameResult isle_post_window_sprite_aux_set(IslePostCourseWindow* s,IsleGameSprite* sprite,int32_t value){
    return invoke(s,[&](State& p){
        if(!p.owns_sprite(sprite))return ISLE_GAME_INVALID_ARGUMENT;
        return p.fail(isle_game_sprite_aux_set(p.services.game,sprite,value));
    });
}
IsleGameResult isle_post_window_thumbnail_create(IslePostCourseWindow* s,IsleGameSprite* native,
                                                uint32_t identity,Handle* out){
    if(out)*out={};
    return invoke(s,[&](State& p){
        if(!out || !p.owns_sprite(native) || p.thumbnail_sprite(native))
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto layout=photo_prototype(p.resources,identity);
        const auto& prototype=*layout.sprite;
        auto* current=p.services.view(native);
        if(!current || current->bitmap!=prototype.bitmap || current->nbitmaps!=prototype.nbitmaps ||
           current->bmfmt!=prototype.bmfmt || current->bmsiz!=prototype.bmsiz ||
           current->width!=prototype.width || current->height!=prototype.height)
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto count=layout.plane_bytes;
        State::Thumbnail* slot=nullptr;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(!p.thumbnails[i].serial && p.thumbnails[i].memory.plane_capacity>=count){slot=&p.thumbnails[i];break;}
        if(!slot)return ISLE_GAME_EXHAUSTED;
        scene::SpriteHandle owner;IsleGameSprite *parent,*child,*next,*previous;
        if(!p.services.sprite_links(native,owner,parent,child,next,previous) || !parent)
            return ISLE_GAME_INVALID_ARGUMENT;
        const auto incarnation=p.serial();
        auto allocation=std::make_shared<State::ThumbnailStorage>();allocation->heap=p.heap;
        allocation->block=p.heap->allocate(count*2);
        if(!allocation->block)return ISLE_GAME_EXHAUSTED;
        auto* pixels=static_cast<std::uint8_t*>(allocation->block);
        std::memcpy(pixels,layout.pixels,layout.source_bytes);
        if(count>layout.source_bytes)std::memset(pixels+layout.source_bytes,0,count-layout.source_bytes);
        std::memcpy(pixels+count,pixels,count);
        std::vector<sprite::RasterPiece> pieces;
        pieces.reserve(std::size_t(prototype.nbitmaps));
        for(std::size_t i=0;i<std::size_t(prototype.nbitmaps);++i) {
            const auto& bitmap=prototype.bitmap[i];
            const auto offset=bitmap.byte_count && bitmap.width?
                std::size_t(reinterpret_cast<std::uintptr_t>(bitmap.buf)-
                            reinterpret_cast<std::uintptr_t>(layout.pixels)):0;
            pieces.push_back({bitmap,offset});
        }
        auto raster=std::make_shared<sprite::Raster>(prototype,pieces,
            std::array<sprite::RasterStorage,2>{{
                {allocation,{pixels,count},slot->memory.physical[0]},
                {allocation,{pixels+count,count},slot->memory.physical[1]}}});
        auto binding=p.producer.register_raster(raster);
        // Publish ownership before touching the native attachment so even a
        // terminal native error retains the registration/storage for teardown.
        slot->raster=std::move(raster);slot->binding=binding;slot->serial=incarnation;
        slot->object=owner.object;slot->sprite=native;slot->prototype=identity;
        auto replacement=*current;replacement.bitmap=slot->raster->view(sprite::RasterSource::primary).bitmap;
        if(!p.services.replace_sprite(native,binding.resources[0],replacement,binding.generation))
            return ISLE_GAME_INVALID_STATE;
        *out=p.handle(incarnation);return p.ok();
    });
}
#include "credits_window_extension.inc"

IsleGameResult isle_post_window_thumbnail_write_photo(IslePostCourseWindow* s,Handle h,
                                                      const uint8_t* image,size_t count,size_t stride){
    return invoke(s,[&](State& p){
        auto* t=p.thumbnail(h);if(!t || !image)return ISLE_GAME_INVALID_ARGUMENT;
        const auto* native=p.thumbnail_view(*t);if(!native)return ISLE_GAME_INVALID_STATE;
        const auto width=std::size_t(native->width),height=std::size_t(native->height);
        const auto source_row=width*4,source_rows=height*2;
        if(stride<source_row || count<source_row || source_rows-1>(count-source_row)/stride)
            return ISLE_GAME_INVALID_ARGUMENT;
        auto pixels=t->raster->pixels(sprite::RasterSource::primary);
        const auto input=reinterpret_cast<std::uintptr_t>(image),output=reinterpret_cast<std::uintptr_t>(pixels.data());
        if(count>std::numeric_limits<std::uintptr_t>::max()-input ||
           (input<output+pixels.size() && output<input+count))return ISLE_GAME_INVALID_ARGUMENT;
        const auto row=std::size_t(native->bitmap->width_img)*2;
        const auto read_pixel=[](const std::uint8_t* p){return (unsigned(p[0])<<8)|p[1];};
        // Exact GAME window/847B60.c114–175: first-bitmap stride/tile height
        // stay constant, but destination buffers and row parity are bitmap-local.
        // Pixels remain canonical BE bytes, independent of host endianness.
        const auto tile=std::size_t(native->bitmap[0].actualHeight);
        std::size_t source_y=0;
        for(std::size_t piece=0;source_y<height;++piece) {
            const auto& bitmap=native->bitmap[piece];
            const auto base=std::size_t(bitmap.buf-pixels.data());
            const auto rows=std::min(tile,height-source_y);
            for(std::size_t local_y=0;local_y<rows;++local_y)for(std::size_t x=0;x<width;++x) {
                const auto* source=image+((source_y+local_y)*2)*stride+x*4;
                const auto a=read_pixel(source),b=read_pixel(source+2);
                const auto c=read_pixel(source+stride),d=read_pixel(source+stride+2);
                const auto value=((((a&0xf800)+(b&0xf800)+(c&0xf800)+(d&0xf800))/4)&0xf800) |
                                 ((((a&0x07c0)+(b&0x07c0)+(c&0x07c0)+(d&0x07c0))/4)&0x07c0) |
                                 ((((a&0x003e)+(b&0x003e)+(c&0x003e)+(d&0x003e))/4)&0x003e) | 1u;
                const auto offset=base+((local_y*row+x*2)^((local_y&1)?std::size_t{4}:std::size_t{0}));
                pixels[offset]=static_cast<std::uint8_t>(value>>8);
                pixels[offset+1]=static_cast<std::uint8_t>(value);
            }
            const auto published=std::min(bitmap.byte_count,(rows*row+7u)&~std::size_t{7});
            t->raster->publish(sprite::RasterSource::primary,base,published);
            source_y+=rows;
        }
        return p.ok();
    });
}
IsleGameResult isle_post_window_thumbnail_clear(IslePostCourseWindow* s,Handle h){
    return invoke(s,[&](State& p){
        auto* t=p.thumbnail(h);if(!t)return ISLE_GAME_INVALID_ARGUMENT;
        const auto* native=p.thumbnail_view(*t);if(!native)return ISLE_GAME_INVALID_STATE;
        auto pixels=t->raster->pixels(sprite::RasterSource::primary);
        const auto offset=std::size_t(native->bitmap[0].buf-pixels.data());
        const auto count=std::size_t(native->bitmap[0].width_img)*std::size_t(native->height)*2;
        std::memset(pixels.data()+offset,0,count);
        t->raster->publish(sprite::RasterSource::primary,offset,count);return p.ok();
    });
}
IsleGameResult isle_post_window_thumbnail_pixels(IslePostCourseWindow* s,Handle h,uint8_t** out,size_t* size){
    if(out)*out=nullptr;if(size)*size=0;
    return invoke(s,[&](State& p){
        if(!out || !size)return ISLE_GAME_INVALID_ARGUMENT;
        auto* thumbnail=p.thumbnail(h);
        if(!thumbnail || !p.thumbnail_view(*thumbnail))return ISLE_GAME_INVALID_ARGUMENT;
        auto pixels=thumbnail->raster->pixels(sprite::RasterSource::primary);
        *out=pixels.data();*size=pixels.size();return p.ok();
    });
}
IsleGameResult isle_post_window_thumbnail_publish(IslePostCourseWindow* s,Handle h,size_t offset,size_t count){
    return invoke(s,[&](State& p){
        auto* thumbnail=p.thumbnail(h);
        if(!thumbnail || !p.thumbnail_view(*thumbnail))return ISLE_GAME_INVALID_ARGUMENT;
        thumbnail->raster->publish(sprite::RasterSource::primary,offset,count);return p.ok();
    });
}
IsleGameResult isle_post_window_thumbnail_select(IslePostCourseWindow* s,Handle h,uint32_t identity){
    return invoke(s,[&](State& p){
        auto* thumbnail=p.thumbnail(h);
        if(!thumbnail || !thumbnail->raster || !thumbnail->binding.generation ||
           !p.owns_sprite(thumbnail->sprite) || thumbnail->credits)return ISLE_GAME_INVALID_ARGUMENT;
        const auto resource=p.resources.sprite_index(identity);
        auto replacement=*p.resources.roots()[resource];
        // This check precedes descriptor/resource mutation, including a
        // temporary immutable selection of the same retained raster slot.
        (void)thumbnail->raster->pixels(sprite::RasterSource::primary);
        if(identity==thumbnail->prototype){
            replacement.bitmap=thumbnail->raster->view(sprite::RasterSource::primary).bitmap;
            return p.ok(p.services.replace_sprite(thumbnail->sprite,
                thumbnail->binding.resources[0],replacement,thumbnail->binding.generation));
        }
        return p.ok(p.services.replace_sprite(thumbnail->sprite,resource,replacement));
    });
}
IsleGameResult isle_post_window_thumbnail_release(IslePostCourseWindow* s,Handle h){
    auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;
    auto* t=p->thumbnail(h);if(!t)return p->fail(ISLE_GAME_INVALID_ARGUMENT);
    return p->release_thumbnail(*t);
}
IsleGameResult isle_post_window_frame_create(IslePostCourseWindow* s,Handle* out){if(out)*out={};return invoke(s,[&](State& p){if(!out || p.frame_serial)return ISLE_GAME_INVALID_ARGUMENT;const auto serial=p.serial();if(!p.frame.create())return p.ok(false);p.frame_serial=serial;*out=p.handle(p.frame_serial);return p.ok();});}
IsleGameResult isle_post_window_frame_delete(IslePostCourseWindow* s,Handle h){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;if(!p->frame_handle(h))return p->fail(ISLE_GAME_INVALID_ARGUMENT);for(std::size_t i=0;i<p->capacity.operations;++i)if(p->operations[i].serial && p->operations[i].kind==State::Kind::fade)p->cancel(p->operations[i]);p->frame.retire();p->frame_serial=0;return p->status();}
IsleGameResult isle_post_window_frame_retire_hidden(IslePostCourseWindow* s,Handle h){
    return invoke(s,[&](State& p){
        if(!p.frame_handle(h))return ISLE_GAME_INVALID_ARGUMENT;
        if(!p.graphics || !p.graphics->quiescent() || !p.frame.hidden())return ISLE_GAME_INVALID_STATE;
        for(std::size_t i=0;i<p.capacity.operations;++i)
            if(p.operations[i].serial && p.operations[i].kind==State::Kind::fade)
                return ISLE_GAME_INVALID_STATE;
        p.frame.retire();if(p.services.failed())return ISLE_GAME_INVALID_STATE;
        p.frame_serial=0;return p.status();
    });
}
IsleGameResult isle_post_window_frame_style(IslePostCourseWindow* s,Handle h,int v){return frame_call(s,h,[&](Frame& f){f.set_style(v);});}
IsleGameResult isle_post_window_frame_position(IslePostCourseWindow* s,Handle h,int x,int y){return frame_call(s,h,[&](Frame& f){f.set_pos(x,y);});}
IsleGameResult isle_post_window_frame_size(IslePostCourseWindow* s,Handle h,int x,int y){return frame_call(s,h,[&](Frame& f){f.set_size(x,y);});}
IsleGameResult isle_post_window_frame_visible(IslePostCourseWindow* s,Handle h,int v){return frame_call(s,h,[&](Frame& f){f.show(v!=0);});}
IsleGameResult isle_post_window_frame_background(IslePostCourseWindow* s,Handle h,int v){return frame_call(s,h,[&](Frame& f){f.show_background(v!=0);});}
IsleGameResult isle_post_window_frame_opacity(IslePostCourseWindow* s,Handle h,int v){return frame_call(s,h,[&](Frame& f){f.set_opacity(v);});}
IsleGameResult isle_post_window_frame_color(IslePostCourseWindow* s,Handle h,IslePostCourseColor* out){if(out)*out={};return invoke(s,[&](State& p){if(!out || !p.frame_handle(h))return ISLE_GAME_INVALID_ARGUMENT;*out=color(p.frame.background_color());return p.ok();});}
IsleGameResult isle_post_window_focus_create(IslePostCourseWindow* s){return invoke(s,[](State& p){if(p.focus_live)return ISLE_GAME_INVALID_STATE;p.focus_live=p.focus.create();return p.ok(p.focus_live);});}
IsleGameResult isle_post_window_focus_delete(IslePostCourseWindow* s){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;p->focus.retire();p->focus_live=false;return p->status();}
IsleGameResult isle_post_window_focus_visible(IslePostCourseWindow* s,int v){return focus_call(s,[&](Focus& f){f.show(v!=0);});}
IsleGameResult isle_post_window_focus_front(IslePostCourseWindow* s){return focus_call(s,[](Focus& f){f.move_front();});}
IsleGameResult isle_post_window_focus_position(IslePostCourseWindow* s,int x,int y){return focus_call(s,[&](Focus& f){f.set_pos(x,y);});}
IsleGameResult isle_post_window_focus_target_position(IslePostCourseWindow* s,int x,int y){return focus_call(s,[&](Focus& f){f.set_target_pos(x,y);});}
IsleGameResult isle_post_window_focus_size(IslePostCourseWindow* s,int x,int y){return focus_call(s,[&](Focus& f){f.set_size(x,y);});}
IsleGameResult isle_post_window_focus_target_size(IslePostCourseWindow* s,int x,int y){return focus_call(s,[&](Focus& f){f.set_target_size(x,y);});}
IsleGameResult isle_post_window_focus_stop(IslePostCourseWindow* s,int v){return focus_call(s,[&](Focus& f){f.stop_animation(v!=0);});}
IsleGameResult isle_post_window_layout_initialize(IslePostCourseWindow* s){return invoke(s,[](State& p){if(p.layout_live)return ISLE_GAME_INVALID_STATE;p.layout_live=p.layout.initialize();return p.ok(p.layout_live);});}
IsleGameResult isle_post_window_layout_instant(IslePostCourseWindow* s,int v){return layout_call(s,[&](Layout& l){l.set_instant_transition(v!=0);});}
IsleGameResult isle_post_window_layout_direction(IslePostCourseWindow* s){return layout_call(s,[](Layout& l){l.toggle_animation_direction();});}
IsleGameResult isle_post_window_layout_speed(IslePostCourseWindow* s){return layout_call(s,[](Layout& l){l.toggle_animation_speed();});}
IsleGameResult isle_post_window_layout_disable(IslePostCourseWindow* s,int v,int id){return layout_call(s,[&](Layout& l){l.disable_button(v!=0,id);});}
IsleGameResult isle_post_window_layout_disable_except(IslePostCourseWindow* s,int v,int id){return layout_call(s,[&](Layout& l){l.disable_all_except(v!=0,id);});}
IsleGameResult isle_post_window_layout_show_button(IslePostCourseWindow* s,int v,int id){return layout_call(s,[&](Layout& l){l.show_button(v!=0,id);});}
IsleGameResult isle_post_window_layout_scale(IslePostCourseWindow* s,float v,int id){return layout_call(s,[&](Layout& l){l.set_button_scale(v,id);});}
IsleGameResult isle_post_window_layout_header_flags(IslePostCourseWindow* s,int v){return layout_call(s,[&](Layout& l){l.set_header_flags(v);});}
IsleGameResult isle_post_window_layout_show_header(IslePostCourseWindow* s,int v){return layout_call(s,[&](Layout& l){l.show_header_element(v);});}
IsleGameResult isle_post_window_layout_hide_header(IslePostCourseWindow* s,int v){return layout_call(s,[&](Layout& l){l.hide_header_element(v);});}
IsleGameResult isle_post_window_layout_panel(IslePostCourseWindow* s,int v){return layout_call(s,[&](Layout& l){l.show_panel(v!=0);});}
IsleGameResult isle_post_window_layout_slot_id(IslePostCourseWindow* s,size_t slot,int* id){
    if(id)*id=35;
    return invoke(s,[&](State& p){
        if(!id || !p.layout_live)return ISLE_GAME_INVALID_ARGUMENT;
        const auto live=p.layout.buttons();
        if(slot>=live.size())return ISLE_GAME_INVALID_ARGUMENT;
        *id=live[slot].id;return p.ok();
    });
}
IsleGameResult isle_post_window_table_borrow(IslePostCourseWindow* s,const IslePostCourseButton* source,size_t count,Handle* out){if(out)*out={};return invoke(s,[&](State& p){if(!out || !source || !count || count>p.capacity.buttons_per_table)return ISLE_GAME_INVALID_ARGUMENT;for(std::size_t i=0;i<p.capacity.button_tables;++i){auto& t=p.tables[i];if(t.serial && t.source==source && t.count==count){p.refresh(t);*out=p.handle(t.serial);return p.ok();}}for(std::size_t i=0;i<p.capacity.button_tables;++i){auto& t=p.tables[i];if(t.serial)continue;t.source=source;t.count=count;p.refresh(t);t.serial=p.serial();t.view={t.converted.get(),count};*out=p.handle(t.serial);return p.ok();}return ISLE_GAME_EXHAUSTED;});}
IsleGameResult isle_post_window_table_resource(IslePostCourseWindow* s,uint32_t id,Handle* out){if(out)*out={};return invoke(s,[&](State& p){if(!out)return ISLE_GAME_INVALID_ARGUMENT;const auto list=p.resources.buttons(id);for(std::size_t i=0;i<p.capacity.button_tables;++i){auto& t=p.tables[i];if(t.serial && !t.source && t.view.data()==list.data()){*out=p.handle(t.serial);return p.ok();}}for(std::size_t i=0;i<p.capacity.button_tables;++i){auto& t=p.tables[i];if(t.serial)continue;t.serial=p.serial();t.view=list;*out=p.handle(t.serial);return p.ok();}return ISLE_GAME_EXHAUSTED;});}
IsleGameResult isle_post_window_table_text(IslePostCourseWindow* s,Handle h,int id,IslePostCourseBytes* out){if(out)*out={};return invoke(s,[&](State& p){auto* t=p.table(h);if(!out || !t)return ISLE_GAME_INVALID_ARGUMENT;p.refresh(*t);const auto v=Layout::button_text(t->view,id);*out={v.data(),v.size()};return p.ok();});}
IsleGameResult isle_post_window_buttons_create(IslePostCourseWindow* s,int x,int y,char name,int scale){return invoke(s,[&](State& p){p.buttons.create(x,y,name,scale);return p.ok();});}
IsleGameResult isle_post_window_buttons_clear(IslePostCourseWindow* s){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;p->buttons.clear();return p->status();}
IsleGameResult isle_post_window_buttons_erase(IslePostCourseWindow* s,int x,int y,int z,int w){return invoke(s,[&](State& p){p.buttons.erase_rect(x,y,z,w);return p.ok();});}
IsleGameResult isle_post_window_buttons_show(IslePostCourseWindow* s,int v,int x,int y,int z,int w){return invoke(s,[&](State& p){p.buttons.show_rect(v!=0,x,y,z,w);return p.ok();});}
IsleGameResult isle_post_window_buttons_interval(IslePostCourseWindow* s,char name,float seconds){return invoke(s,[&](State& p){p.buttons.set_animation_interval(name,seconds);return p.ok();});}
Step isle_post_window_print_begin(IslePostCourseWindow* s,Handle h,IslePostCourseBytes value,int ascii,const IslePostCourseTextCallbacks* callbacks,Handle* out){
    return begin(s,State::Kind::print,out,[&](State& p,State::Operation& op){
        auto* e=p.element(h);if(!e)return StepResult{StepStatus::error,StepError::invalid_operation};
        if(!value.data && !value.size)return StepResult{StepStatus::done};
        op.element=e;
        const auto& c=callbacks?*callbacks:p.callbacks;
        // Source's registered character callback is live across waits. Even a
        // currently NULL callback may be installed before the next character.
        TextEvents events{!ascii && (!callbacks || c.character!=nullptr),
                          !ascii && c.button!=nullptr,c.changed!=nullptr};
        return e->element->begin_print(op.print,p.text,bytes(value),events,
                                      ascii?TextEncoding::ascii:TextEncoding::source);
    });
}
Step isle_post_window_fade_in_begin(IslePostCourseWindow* s,Handle h,int style,Handle* out){return begin(s,State::Kind::fade,out,[&](State& p,State::Operation& op){return p.frame_handle(h)?p.frame.begin_fade_in(op.fade,style):StepResult{StepStatus::error,StepError::invalid_operation};});}
Step isle_post_window_fade_out_begin(IslePostCourseWindow* s,Handle h,Handle* out){return begin(s,State::Kind::fade,out,[&](State& p,State::Operation& op){return p.frame_handle(h)?p.frame.begin_fade_out(op.fade):StepResult{StepStatus::error,StepError::invalid_operation};});}
Step isle_post_window_layout_wait_begin(IslePostCourseWindow* s,Handle* out){return begin(s,State::Kind::layout,out,[](State& p,State::Operation& op){return p.layout_live?p.layout.begin_wait_transition(op.layout):StepResult{StepStatus::error,StepError::invalid_operation};});}
Step isle_post_window_layout_hide_begin(IslePostCourseWindow* s,Handle* out){return begin(s,State::Kind::layout,out,[](State& p,State::Operation& op){return p.layout_live?p.layout.begin_hide_buttons(op.layout):StepResult{StepStatus::error,StepError::invalid_operation};});}
Step isle_post_window_layout_create_begin(IslePostCourseWindow* s,Handle h,Handle* out){return begin(s,State::Kind::layout,out,[&](State& p,State::Operation& op){auto* t=p.table(h);if(!t || !p.layout_live)return StepResult{StepStatus::error,StepError::invalid_operation};p.refresh(*t);return p.layout.begin_create_buttons(op.layout,t->view);});}
Step isle_post_window_step(IslePostCourseWindow* s,Handle h,int resume){
    auto* p=state(s);if(!p)return error_step();auto* op=p->operation(h);
    if(!op){p->fail(ISLE_GAME_INVALID_ARGUMENT);return error_step();}
    if(p->terminal(*op))return step_value(op->result);
    if(p->status()!=ISLE_GAME_OK){p->cancel(*op);return p->finish(*op,{StepStatus::error,StepError::native_failure});}
    if(!resume && (op->result.status==StepStatus::event || op->result.status==StepStatus::needs_source_wait))
        return step_value(op->result,op->kind==State::Kind::print?&op->print.event():nullptr);
    try {
        switch(op->kind){
        case State::Kind::print:if(!op->element || !op->element->element)break;return p->finish(*op,op->element->element->step_print(op->print,resume!=0));
        case State::Kind::fade:return p->finish(*op,p->frame.step_fade(op->fade,resume!=0));
        case State::Kind::layout:return p->finish(*op,p->layout.step(op->layout,resume!=0));
        case State::Kind::none:break;
        }
    }catch(...){p->cancel(*op);return p->finish(*op,{StepStatus::error,StepError::native_failure});}
    p->cancel(*op);return p->finish(*op,{StepStatus::error,StepError::invalid_operation});
}
Step isle_post_window_cancel(IslePostCourseWindow* s,Handle h){
    auto* p=state(s);if(!p)return error_step();auto* op=p->operation(h);
    if(!op){p->fail(ISLE_GAME_INVALID_ARGUMENT);return error_step();}
    p->cancel(*op);return step_value(op->result);
}
IsleGameResult isle_post_window_operation_release(IslePostCourseWindow* s,Handle h){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;auto* op=p->operation(h);if(!op || !p->terminal(*op))return p->fail(ISLE_GAME_INVALID_STATE);op->serial=0;op->kind=State::Kind::none;op->element=nullptr;return ISLE_GAME_OK;}
IsleGameResult isle_post_window_cancel_all(IslePostCourseWindow* s){auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;p->cancel_all();return ISLE_GAME_OK;}
}
