#include "window_element.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace isle::window {
namespace {
constexpr std::string_view button_names = "abcdfrzlsmu";
unsigned source_index(sprite::RasterSource s) { return static_cast<unsigned>(s); }
sprite::RasterSource other(sprite::RasterSource s) { return source_index(s) ? sprite::RasterSource::primary : sprite::RasterSource::alternate; }
struct Allocation {
    std::shared_ptr<Heap> heap;
    void* block = nullptr;
    ~Allocation() { if (block) heap->deallocate(block); }
};
void store(std::uint8_t* p, int bpp, std::uint32_t v) {
    for (int i = bpp - 1; i >= 0; --i) { p[i] = static_cast<std::uint8_t>(v); v >>= 8; }
}
std::uint32_t fill_value(std::uint32_t flags, Color c) {
    if (flags & bits8) return (c.r & 0xf0u) | (c.a >> 4);
    if (flags & bits32) return (std::uint32_t(c.r)<<24) | (std::uint32_t(c.g)<<16) | (std::uint32_t(c.b)<<8) | c.a;
    if (flags & grayscale) return flags & alpha_plane ? 0xff00u | c.a : (std::uint32_t(c.r)<<8) | 0xffu;
    return ((std::uint32_t(c.r)<<8)&0xf800u) | ((std::uint32_t(c.g)<<3)&0x7c0u) | ((c.b>>2)&0x3eu) | (c.a&1u);
}
}
void ElementList::unlink(Element& e) noexcept {
    if (e.previous_) e.previous_->next_ = e.next_; else head_ = e.next_;
    if (e.next_) e.next_->previous_ = e.previous_; else tail_ = e.previous_;
    e.previous_ = e.next_ = nullptr;
}
void ElementList::move_head(Element& e) {
    if (e.list_ != this) throw std::invalid_argument("foreign window list");
    if (head_ == &e) return;
    unlink(e); e.next_ = head_;
    if (head_) head_->previous_ = &e; else tail_ = &e;
    head_ = &e;
    if (e.services_) e.services_->move_to_tail(e.attachment_.object);
}
void ElementList::move_tail(Element& e) {
    if (e.list_ != this) throw std::invalid_argument("foreign window list");
    if (head_ == &e) return; // The pinned MoveToTail explicitly excludes the list head.
    unlink(e); e.previous_ = tail_;
    if (tail_) tail_->next_ = &e; else head_ = &e;
    tail_ = &e;
    if (e.services_) e.services_->move_to_head(e.attachment_.object);
}
std::shared_ptr<Element> Element::create(std::shared_ptr<Heap> heap, ElementList& list, int x, int y,
    int width, int height, std::uint32_t flags, std::array<std::uint32_t,2> physical, const ElementDefaults& defaults) {
    if (!heap || width <= 0 || width > 320 || height <= 0 || height > 240 ||
        ((flags & bits8) && !(flags & grayscale)) || ((flags & bits32) && (flags & grayscale)))
        throw std::invalid_argument("window element dimensions/format");
    const int edge = flags & border ? 4 : flags & thick_border ? 8 : 0;
    const int bpp = flags & bits8 ? 1 : flags & bits32 ? 4 : 2;
    const int total_width = width + edge * 2, total_height = height + edge * 2;
    const int stride = (32 + total_width + (bpp >> 2) * 16 + 15) & ~15;
    const int tile_height = 4096 / bpp / stride;
    const int tiles = (total_height + tile_height - 1) / tile_height;
    const auto plane_bytes = static_cast<std::size_t>(stride * tile_height * tiles * bpp);
    const auto image_bytes = plane_bytes * (flags & use_alt_image ? 2u : 1u);
    const auto element_bytes = (sizeof(Element) + 7u) & ~std::size_t{7};
    auto allocation = std::make_shared<Allocation>(); allocation->heap = std::move(heap);
    allocation->block = allocation->heap->allocate(element_bytes + image_bytes);
    if (!allocation->block) return {};
    auto* e = ::new (allocation->block) Element;
    std::shared_ptr<Element> result(e, [allocation](Element* value) { value->~Element(); });
    e->x_=x; e->y_=y; e->width_=width; e->height_=height; e->border_=edge; e->stride_=stride;
    e->bpp_=bpp; e->flags_=flags; e->foreground_=defaults.foreground; e->background_=defaults.background;
    e->font_=defaults.font; e->text_mode_=defaults.mode; e->alt_x_=x;
    e->id_=list.next_id_++;
    auto* pixels = reinterpret_cast<std::uint8_t*>(allocation->block) + element_bytes;
    std::memset(pixels,0,image_bytes);
    IsleSprite prototype{};
    prototype.x=static_cast<std::int16_t>(x-edge); prototype.y=static_cast<std::int16_t>(y-edge);
    // Native bitmap placement wraps at visible width; padding is storage-only.
    prototype.width=static_cast<std::int16_t>(total_width); prototype.height=static_cast<std::int16_t>(total_height);
    prototype.scalex=prototype.scaley=1; prototype.attr=ISLE_SP_HIDDEN|ISLE_SP_TRANSPARENT;
    if (flags & scalable) prototype.attr|=ISLE_SP_SCALE;
    prototype.zdepth=0x100; prototype.red=prototype.green=prototype.blue=prototype.alpha=255;
    prototype.istep=1; prototype.nbitmaps=static_cast<std::int16_t>(tiles);
    prototype.ndisplist=static_cast<std::int16_t>(tiles*12+24);
    prototype.bmheight=prototype.bmHreal=static_cast<std::int16_t>(tile_height);
    prototype.bmfmt=flags & grayscale ? 3 : 0; prototype.bmsiz=bpp==1?1:bpp==4?3:2;
    std::vector<sprite::RasterPiece> pieces;
    pieces.reserve(static_cast<std::size_t>(tiles));
    for (int i=0;i<tiles;++i) {
        const int actual=std::min(tile_height,total_height-i*tile_height);
        IsleBitmap bitmap{}; bitmap.width=static_cast<std::int16_t>(total_width); bitmap.width_img=static_cast<std::int16_t>(stride);
        bitmap.actualHeight=static_cast<std::int16_t>(actual); bitmap.byte_count=static_cast<std::size_t>(stride*actual*bpp);
        pieces.push_back({bitmap,static_cast<std::size_t>(i*stride*tile_height*bpp)});
    }
    std::array<sprite::RasterStorage,2> sources{{{allocation,{pixels,plane_bytes},physical[0]}, {}}};
    if (flags & use_alt_image) sources[1]={allocation,{pixels+plane_bytes,plane_bytes},physical[1]};
    e->raster_=std::make_shared<sprite::Raster>(prototype,pieces,std::move(sources));
    e->primary_=e->raster_->view(sprite::RasterSource::primary);
    if (flags & use_alt_image) e->display_source_=sprite::RasterSource::alternate;
    e->primary_.bitmap=e->raster_->view(e->display_source_).bitmap;
    e->displayed_=e->primary_;
    e->alternate_.x=static_cast<std::int16_t>(x); e->alternate_.y=static_cast<std::int16_t>(y);
    e->list_=&list; e->next_=list.head_;
    if (list.head_) list.head_->previous_=e; else list.tail_=e;
    list.head_=e;
    return result;
}
Element::~Element() { detach(); if (list_) list_->unlink(*this); }
std::span<std::uint8_t> Element::image() { return raster_->pixels(write_source_); }
std::span<const std::uint8_t> Element::image() const { return raster_->pixels(write_source_); }
void Element::synchronize() {
    displayed_ = flags_ & show_alt ? alternate_ : primary_;
    if (services_) services_->replace_sprite(attachment_.sprite,
        flags_ & show_alt ? alt_resource_ : binding_.resources[source_index(display_source_)], displayed_,
        flags_ & show_alt ? alt_generation_ : binding_.generation);
}
void Element::fill_storage(int x1,int y1,int x2,int y2,Color color) {
    if (x2<x1) std::swap(x1,x2);
    if (y2<y1) std::swap(y1,y2);
    const auto data=image(); const int rows=static_cast<int>(data.size()/static_cast<std::size_t>(stride_*bpp_));
    if (x1<0 || y1<0 || x2>=stride_ || y2>=rows) throw std::out_of_range("window fill storage");
    const auto value=fill_value(flags_,color);
    const auto pixel_bytes=static_cast<std::size_t>(bpp_);
    for (int row=y1;row<=y2;++row) for (int col=x1;col<=x2;++col)
        store(data.data()+static_cast<std::size_t>(row*stride_+col)*pixel_bytes,bpp_,value);
    for (int row=y1;row<=y2;++row)
        raster_->publish(write_source_,static_cast<std::size_t>(row*stride_+x1)*pixel_bytes,static_cast<std::size_t>(x2-x1+1)*pixel_bytes);
}
void Element::fill(int x1,int y1,int x2,int y2,Color color) {
    x1=std::clamp(x1,0,width_-1); x2=std::clamp(x2,0,width_-1);
    y1=std::clamp(y1,0,height_-1); y2=std::clamp(y2,0,height_-1);
    fill_storage(x1+border_,y1+border_,x2+border_,y2+border_,color);
}
void Element::fill_with_border(int x1,int y1,int x2,int y2,Color color) { fill_storage(x1,y1,x2,y2,color); }
void Element::clear() {
    fill(0,0,width_,height_,background_); text_x_=text_y_=text_state_=0;
    if (buttons_) buttons_->erase_rect(x_,y_,x_+width_,y_+height_);
}
void Element::draw_background() {
    source_state_.fill(0);
    if (flags_ & border) {
        const int last_row=static_cast<int>(image().size()/static_cast<std::size_t>(stride_*bpp_))-1;
        fill_with_border(0,0,width_+border_*2,std::min(height_+border_*2,last_row),background_);
    } else fill(0,0,width_-1,height_-1,background_);
    primary_.attr &= ~ISLE_SP_HIDDEN; text_state_=0; swap_images();
}
void Element::set_color(bool fg, Color color) {
    if (fg) foreground_=color; else background_=color;
    if (flags_ & grayscale) { primary_.red=foreground_.r;primary_.green=foreground_.g;primary_.blue=foreground_.b; }
}
void Element::set_text_pos(int x,int y) noexcept { text_x_=std::clamp(x,0,width_-1);text_y_=std::min(y,height_-1);text_state_=0; }
void Element::copy_pixel(std::size_t dst,std::size_t src) {
    const auto pixel_bytes=static_cast<std::size_t>(bpp_);
    const auto data=image(); const auto count=data.size()/pixel_bytes;
    if (dst>=count || src>=count) throw std::out_of_range("window scroll allocation boundary");
    std::memmove(data.data()+dst*pixel_bytes,data.data()+src*pixel_bytes,pixel_bytes);
}
void Element::scroll(int amount,unsigned direction) {
    if (!amount) return;
    if (direction>3 || amount<0 || amount>(direction<2?height_:width_)) throw std::invalid_argument("window scroll amount/direction");
    const int origin=border_*stride_+border_;
    const auto pixel_bytes=static_cast<std::size_t>(bpp_);
    if (direction==0 || direction==1) {
        for (int i=0;i<height_-amount;++i) {
            const int row=direction==0?i:height_-1-i;
            const int from=direction==0?row+amount:row-amount;
            for (int x=0;x<width_;++x) copy_pixel(static_cast<std::size_t>(origin+row*stride_+x),static_cast<std::size_t>(origin+from*stride_+x));
        }
        for (int row=0;row<height_;++row)
            raster_->publish(write_source_,static_cast<std::size_t>(origin+row*stride_)*pixel_bytes,static_cast<std::size_t>(width_)*pixel_bytes);
        fill(0,direction==0?height_-amount:0,width_,direction==0?height_:amount,background_);
    } else {
        // The pin's 8/32-bit left-scroll loop advances by an extra two row
        // widths; the 16-bit path advances normally. Preserve that distinction.
        const int row_step=direction==3 && bpp_!=2?stride_+2*(width_-amount):stride_;
        const auto last=static_cast<std::size_t>(origin+(height_-1)*row_step+width_-1);
        if (last>=image().size()/static_cast<std::size_t>(bpp_)) throw std::out_of_range("source horizontal scroll allocation boundary");
        for (int y=0;y<height_;++y) for (int i=0;i<width_-amount;++i) {
            const int x=direction==2?width_-1-i:i;
            const int from=direction==2?x-amount:x+amount;
            copy_pixel(static_cast<std::size_t>(origin+y*row_step+x),static_cast<std::size_t>(origin+y*row_step+from));
        }
        for (int row=0;row<height_;++row)
            raster_->publish(write_source_,static_cast<std::size_t>(origin+row*row_step)*pixel_bytes,static_cast<std::size_t>(width_)*pixel_bytes);
        fill(direction==2?0:width_-amount,0,direction==2?amount-1:width_-1,height_,background_);
    }
}
void Element::vertical_scroll(int amount,unsigned direction,int x1,int y1,int x2,int y2) {
    x1=std::clamp(x1,0,width_);x2=std::clamp(x2,0,width_);y1=std::clamp(y1,0,height_);y2=std::clamp(y2,0,height_);
    if (amount<0 || direction>3) throw std::invalid_argument("window vertical scroll");
    if (direction>=2) return; // Explicitly empty source switch arms.
    if (direction==1) { scroll(amount,1);return; }
    const int rows=y2-y1-amount+1;
    const int count=bpp_==1?width_:x2-x1+1;
    const int start_x=bpp_==4?0:x1;
    if (rows>0 && count>0) {
        const int origin=border_*stride_+border_+start_x;
        const auto pixel_bytes=static_cast<std::size_t>(bpp_);
        const auto last=static_cast<std::size_t>(origin+(rows-1+amount)*stride_+count-1);
        if (last>=image().size()/static_cast<std::size_t>(bpp_)) throw std::out_of_range("source vertical scroll allocation boundary");
        for (int row=0;row<rows;++row) for (int col=0;col<count;++col)
            copy_pixel(static_cast<std::size_t>(origin+row*stride_+col),static_cast<std::size_t>(origin+(row+amount)*stride_+col));
        for (int row=0;row<rows;++row)
            raster_->publish(write_source_,static_cast<std::size_t>(origin+row*stride_)*pixel_bytes,static_cast<std::size_t>(count)*pixel_bytes);
    }
    fill(x1,y2-amount,x2,y2,background_);
}
void Element::draw_image(int x,int y,int width,int height,std::span<const std::uint8_t> src) {
    if (width<=0 || height<=0) return;
    x=std::clamp(x,0,width_-1);y=std::clamp(y,0,height_-1);
    const auto pixel_bytes=static_cast<std::size_t>(bpp_);
    const auto row_stride=static_cast<std::size_t>(stride_)*pixel_bytes;
    const auto row_bytes=static_cast<std::size_t>(width)*pixel_bytes;
    const auto data=image();const auto start=static_cast<std::size_t>((border_+y)*stride_+border_+x)*pixel_bytes;
    const auto final_row=static_cast<std::size_t>(height-1)*row_stride;
    if (row_bytes>src.size() || static_cast<std::size_t>(height)>src.size()/row_bytes ||
        start>data.size() || final_row>data.size()-start ||
        row_bytes>data.size()-start-final_row)
        throw std::out_of_range("window image allocation boundary");
    for (int row=0;row<height;++row) std::memmove(data.data()+start+static_cast<std::size_t>(row)*row_stride,src.data()+static_cast<std::size_t>(row)*row_bytes,row_bytes);
    for (int row=0;row<height;++row)
        raster_->publish(write_source_,start+static_cast<std::size_t>(row)*row_stride,row_bytes);
}
TextSurface Element::surface() {
    const auto offset=static_cast<std::size_t>(stride_*border_+border_)*static_cast<std::size_t>(bpp_);
    return {image().subspan(offset),stride_,height_,bpp_,x_+border_,y_+border_};
}
StepResult Element::begin_print(PrintOperation& op,Text& text,std::string_view bytes,
                                TextEvents events,TextEncoding encoding) noexcept {
    if (active_print_ || (op.owner_ && op.result_.status!=StepStatus::done && op.result_.status!=StepStatus::error))
        return {StepStatus::error,StepError::busy};
    try {
        const auto result=text.begin_print(op.text_operation_,surface(),text_x_,text_y_,bytes,
            {events.character,events.button || buttons_!=nullptr,true},encoding);
        if (result.status==StepStatus::error) return result;
        op.owner_=this;op.text_=&text;op.events_=events;op.result_=result;op.internal_event_=false;
        op.source_=write_source_;
        op.origin_=static_cast<std::size_t>(stride_*border_+border_)*static_cast<std::size_t>(bpp_);
        active_print_=&op;
        text.set_font(font_);
        text.palettes(flags_&grayscale?Color{255,255,255,foreground_.a}:foreground_,
                      flags_&grayscale?Color{255,255,255,background_.a}:background_);
        return op.result_;
    } catch (...) { return {StepStatus::error,StepError::native_failure}; }
}
StepResult Element::finish_print(PrintOperation& op,StepError error) noexcept {
    const auto text_result=op.text_operation_.result();
    if (text_result.status!=StepStatus::done && text_result.status!=StepStatus::error)
        op.text_->cancel_print(op.text_operation_);
    op.result_={error==StepError::none?StepStatus::done:StepStatus::error,error};
    op.text_=nullptr;
    if (active_print_==&op) active_print_=nullptr;
    return op.result_;
}
void Element::cancel_print(PrintOperation& op) noexcept {
    if (op.owner_==this && active_print_==&op) finish_print(op,StepError::cancelled);
}
StepResult Element::step_print(PrintOperation& op,bool resume) noexcept {
    if (op.owner_!=this) return {StepStatus::error,StepError::invalid_operation};
    if (op.result_.status==StepStatus::done || op.result_.status==StepStatus::error) return op.result_;
    if (active_print_!=&op) return {StepStatus::error,StepError::invalid_operation};
    const bool pending=op.result_.status==StepStatus::event || op.result_.status==StepStatus::needs_source_wait;
    // Text::detach/retire may have cancelled the underlying operation.
    const auto text_result=op.text_operation_.result();
    if (text_result.status==StepStatus::error) return finish_print(op,text_result.error);
    if (pending && !resume) return op.result_;
    if (!pending && resume) return {StepStatus::error,StepError::invalid_operation};
    if (write_source_!=op.source_) return finish_print(op,StepError::invalid_operation);
    try {
        op.result_=op.text_->step_print(op.text_operation_,resume || op.internal_event_);
        op.internal_event_=false;
        if (op.result_.status==StepStatus::event) {
            const auto& value=op.event();
            switch (value.kind) {
            case TextEvent::Kind::changed:
                raster_->publish(op.source_,op.origin_+value.offset,value.size);
                op.internal_event_=!op.events_.changed;
                break;
            case TextEvent::Kind::button:
                if (!op.events_.button) {
                    if (!buttons_) return finish_print(op,StepError::missing_button);
                    buttons_->create(value.x,value.y,value.button,value.scale);
                    op.internal_event_=true;
                }
                break;
            case TextEvent::Kind::character: case TextEvent::Kind::none: break;
            }
            if (op.internal_event_) op.result_={StepStatus::more};
        }
        if (op.result_.status==StepStatus::done || op.result_.status==StepStatus::error)
            return finish_print(op,op.result_.error);
        return op.result_;
    } catch (...) { return finish_print(op,StepError::native_failure); }
}
bool Element::run_print(Text& text,std::string_view bytes,TextHooks hooks,TextEncoding encoding) {
    PrintOperation op;
    auto result=begin_print(op,text,bytes,
        {hooks.character!=nullptr,hooks.button!=nullptr,hooks.changed!=nullptr},encoding);
    try {
        for (;;) {
            switch (result.status) {
            case StepStatus::more: result=step_print(op);break;
            case StepStatus::event: {
                const auto value=op.event();
                switch (value.kind) {
                case TextEvent::Kind::character: hooks.character(hooks.userdata,value.character);break;
                case TextEvent::Kind::button: hooks.button(hooks.userdata,value.x,value.y,value.button,value.scale);break;
                case TextEvent::Kind::changed: hooks.changed(hooks.userdata,value.offset,value.size);break;
                case TextEvent::Kind::none: break;
                }
                result=step_print(op,true);break;
            }
            case StepStatus::needs_source_wait:
                if (hooks.wait) {
                    if (!hooks.wait(hooks.userdata,result.updates)) {cancel_print(op);return false;}
                } else if (services_) {
                    if (!services_->wait(result.updates)) {cancel_print(op);return false;}
                } else throw std::logic_error("window typewriter requires actual native continuation");
                result=step_print(op,true);break;
            case StepStatus::done: return true;
            case StepStatus::error:
                switch (result.error) {
                case StepError::cancelled: return false;
                case StepError::invalid_text: throw std::invalid_argument("truncated source text character");
                case StepError::invalid_surface: throw std::invalid_argument("window text surface");
                case StepError::surface_boundary: throw std::out_of_range("source glyph/shadow allocation boundary");
                case StepError::missing_button: throw std::logic_error("window button escape requires native Buttons owner");
                default: throw std::runtime_error("window element text operation failed");
                }
            }
        }
    } catch (...) {cancel_print(op);throw;}
}
bool Element::print(Text& text,std::string_view bytes,TextHooks hooks) {
    return run_print(text,bytes,hooks,TextEncoding::source);
}
void Element::print_ascii(Text& text,std::string_view bytes,TextHooks hooks) {
    run_print(text,bytes,{hooks.userdata,nullptr,nullptr,nullptr,hooks.changed},TextEncoding::ascii);
}
bool Element::print_format(Text& text, TextHooks hooks, const char* format, ...) {
    if (active_print_) throw std::logic_error("window text operation already active");
    if (!format) throw std::invalid_argument("window text format");
    va_list arguments;
    va_start(arguments,format);
    const int size=std::vsnprintf(format_buffer_.data(),format_buffer_.size(),format,arguments);
    va_end(arguments);
    if (size<0 || static_cast<std::size_t>(size)>=format_buffer_.size())
        throw std::length_error("source 400-byte formatted text buffer");
    return print(text,{format_buffer_.data(),static_cast<std::size_t>(size)},hooks);
}
void Element::swap_images() {
    if (active_print_) throw std::logic_error("cannot swap images during window text operation");
    if (flags_&use_alt_image) {
        const auto from=image();const auto to=raster_->pixels(other(write_source_));
        std::copy(from.begin(),from.end(),to.begin());
        raster_->publish(other(write_source_),0,to.size());
        display_source_=write_source_;write_source_=other(write_source_);
    } else display_source_=write_source_;
    primary_.bitmap=raster_->view(display_source_).bitmap;
    displayed_=primary_; // Source's no-alt branch also restores primary, without copying or switching planes.
    if (services_) services_->replace_sprite(attachment_.sprite,binding_.resources[source_index(display_source_)],displayed_,binding_.generation);
}
void Element::set_pos(int x,int y) {
    x_=x;y_=y;primary_.x=static_cast<std::int16_t>(x-border_);primary_.y=static_cast<std::int16_t>(y-border_);
    if (!(flags_&show_alt)) {
        displayed_.x=primary_.x;displayed_.y=primary_.y;
        if (services_) if(auto* view=services_->view(attachment_.sprite)){view->x=primary_.x;view->y=primary_.y;}
    }
}
void Element::set_alt_pos(int x,int y) {
    alt_x_=x;alt_y_=y;alternate_.x=static_cast<std::int16_t>(x);alternate_.y=static_cast<std::int16_t>(y);
    if (flags_&show_alt) {
        displayed_.x=alternate_.x;displayed_.y=alternate_.y;
        if (services_) if(auto* view=services_->view(attachment_.sprite)){view->x=alternate_.x;view->y=alternate_.y;}
    }
}
void Element::set_alt_sprite(std::size_t resource,const IsleSprite& sprite,std::uint64_t generation) {
    alternate_=sprite;alt_resource_=resource;alt_generation_=generation;alt_width_=sprite.width;alt_height_=sprite.height;
    alternate_.x=static_cast<std::int16_t>(alt_x_);alternate_.y=static_cast<std::int16_t>(alt_y_);
}
void Element::show_alt_sprite(bool show) { if(show)flags_|=show_alt;else flags_&=~show_alt;synchronize(); }
void Element::set_visible(bool visible) {
    if(visible)displayed_.attr&=~ISLE_SP_HIDDEN;else displayed_.attr|=ISLE_SP_HIDDEN;
    if (services_) if(auto* view=services_->view(attachment_.sprite)) view->attr=displayed_.attr;
    if (buttons_) buttons_->show_rect(visible,x_,y_,x_+width_,y_+height_);
}
void Element::set_scale(float x,float y) {
    if (!(flags_&scalable)) return;
    if (!std::isfinite(x)||!std::isfinite(y)) throw std::invalid_argument("window scale");
    primary_.scalex=x;primary_.scaley=y;displayed_.scalex=x;displayed_.scaley=y;
    if (services_) if(auto* view=services_->view(attachment_.sprite)){view->scalex=x;view->scaley=y;}
}
void Element::set_sprite_color(Color c) {
    displayed_.red=c.r;displayed_.green=c.g;displayed_.blue=c.b;displayed_.alpha=c.a;
    if (services_) if (auto* view=services_->view(attachment_.sprite)) {
        view->red=c.r;view->green=c.g;view->blue=c.b;view->alpha=c.a;
    }
}
bool Element::attach(scene::Services& services,sprite::RasterBinding binding) {
    if (services_ || !binding.generation) return false;
    auto* object=services.object(1u<<6,6,6);
    if (!object) return false;
    if (!services.set_render(object,{2,6,1u<<2})) {services.erase(object);return false;}
    auto* native=services.append_sprite(object,binding.resources[source_index(display_source_)],displayed_,binding.generation);
    if (!native) {services.erase(object);return false;}
    services_=&services;binding_=binding;attachment_={object,native};return true;
}
void Element::detach() noexcept {
    if (active_print_) cancel_print(*active_print_);
    if (services_) services_->erase(attachment_.object);
    services_=nullptr;attachment_={};
}

struct Buttons::Entry {
    Buttons* owner;
    Entry* next;
    Entry* previous;
    scene::SpriteHandle sprite;
    unsigned index, phase;
};
Buttons::Buttons(std::shared_ptr<const Resources> resources,std::shared_ptr<Heap> heap,scene::Services& services)
    : resources_(std::move(resources)),heap_(std::move(heap)),services_(services) {
    if(!resources_||!heap_)throw std::invalid_argument("native button owners");
    for(unsigned i=0;i<intervals_.size();++i)intervals_[i]=resources_->button_interval(button_names[i]);
}
Buttons::~Buttons(){clear();}
void Buttons::update(IsleGameObject*,void* data) {
    auto* e=static_cast<Entry*>(data);
    while(!e->owner->services_.failed()) {
        e->phase=!e->phase;
        auto saved=*e->owner->services_.view(e->sprite.sprite);
        saved.bitmap=e->owner->resources_->button(button_names[e->index],e->phase).sprite.view()->bitmap;
        if(!e->owner->services_.rebind_sprite(e->sprite.sprite,e->owner->resources_->button(button_names[e->index],e->phase).identity))return;
        *e->owner->services_.view(e->sprite.sprite)=saved;
        if(!e->owner->services_.wait(static_cast<std::uint32_t>(e->owner->intervals_[e->index])))return;
    }
}
void Buttons::create(int x,int y,char name,int scale) {
    const auto index=button_names.find(name);
    if(index==std::string_view::npos)return; // Source strchr miss creates nothing.
    auto* memory=heap_->allocate(sizeof(Entry));if(!memory)throw std::bad_alloc();
    auto* e=::new(memory)Entry{this,head_,nullptr,{},static_cast<unsigned>(index),0};
    e->sprite.object=services_.object(1u<<6,6,6);
    if(e->sprite.object) {
        e->sprite.sprite=services_.append_sprite(e->sprite.object,resources_->button(name,0).identity);
        if (!e->sprite.sprite || !services_.set_render(e->sprite.object,{2,6,1u<<2})) {
            services_.erase(e->sprite.object);e->sprite={};
        }
    }
    if(!e->sprite.object){e->~Entry();heap_->deallocate(e);throw std::runtime_error("native button creation");}
    auto* view=services_.view(e->sprite.sprite);view->x=static_cast<std::int16_t>(x);view->y=static_cast<std::int16_t>(y);
    if(scale!=24){view->scalex=view->scaley=static_cast<float>(scale)/12;view->attr|=ISLE_SP_SCALE;}
    if(head_)head_->previous=e;
    head_=e;
    if(!services_.process(e->sprite.object,update,e,ISLE_GAME_STACKFUL,0)){erase(e);throw std::runtime_error("native button process");}
}
void Buttons::erase(Entry* e) noexcept {
    services_.erase(e->sprite.object);
    if(e->previous)e->previous->next=e->next;else head_=e->next;
    if(e->next)e->next->previous=e->previous;
    e->~Entry();heap_->deallocate(e);
}
void Buttons::clear() noexcept {while(head_)erase(head_);}
void Buttons::erase_rect(int x1,int y1,int x2,int y2) noexcept {
    for(auto* e=head_;e;){auto* next=e->next;const auto* v=services_.view(e->sprite.sprite);
        if(v&&v->x>=x1&&v->x<=x2&&v->y>=y1&&v->y<=y2)erase(e);
        e=next;
    }
}
void Buttons::show_rect(bool visible,int x1,int y1,int x2,int y2) noexcept {
    for(auto* e=head_;e;e=e->next)if(auto* v=services_.view(e->sprite.sprite))
        if(v->x>=x1&&v->x<=x2&&v->y>=y1&&v->y<=y2){if(visible)v->attr&=~ISLE_SP_HIDDEN;else v->attr|=ISLE_SP_HIDDEN;}
}
void Buttons::set_animation_interval(char name,float seconds) {
    const auto i=button_names.find(name);if(i==std::string_view::npos)return;
    if(!std::isfinite(seconds)||seconds>static_cast<float>(std::numeric_limits<int>::max()/60))throw std::invalid_argument("button interval");
    intervals_[i]=seconds<=0?1:std::max(1,static_cast<int>(seconds*60));
}
} // namespace isle::window
