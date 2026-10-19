/* Included only by the concrete post-course Window owner. */
IsleGameResult isle_post_window_credits_image_create(IslePostCourseWindow* s,IsleGameSprite* native,
    uint32_t identity,size_t piece_count,size_t piece_bytes,Handle* out){
    if(out)*out={};
    return invoke(s,[&](State& p){
        if(!out || !p.owns_sprite(native) || p.thumbnail_sprite(native) ||
           static_cast<unsigned>(p.resources.kind())!=6u || identity!=0x801e6058u ||
           piece_count!=8 || piece_bytes!=14u*140u*2u)return ISLE_GAME_INVALID_ARGUMENT;
        const auto& prototype=*p.resources.roots()[p.resources.sprite_index(identity)];
        auto* current=p.services.view(native);
        if(!current || current->bitmap!=prototype.bitmap || prototype.nbitmaps!=8 ||
           current->nbitmaps!=prototype.nbitmaps || current->width!=prototype.width ||
           current->height!=prototype.height || current->bmfmt!=0 || current->bmsiz!=2 ||
           prototype.bmfmt!=0 || prototype.bmsiz!=2 || prototype.width!=140 || prototype.height!=105 ||
           prototype.x!=0 || prototype.y!=0 || prototype.scalex!=1.0f || prototype.scaley!=1.0f ||
           prototype.attr!=ISLE_SP_TEXSHUF || prototype.bmheight!=14 || prototype.bmHreal!=14 ||
           !prototype.bitmap || prototype.LUT || prototype.LUT_bytes || prototype.nTLUT)
            return ISLE_GAME_INVALID_ARGUMENT;
        /* Actual qualified-640c native Sprite decoder receipt: first seven
         * pieces are140x14/3920 bytes, final piece140x7/1960 bytes. Source
         * still allocates3920 for all eight destination buffers. */
        for(std::size_t i=0;i<8;++i){
            const auto& bitmap=prototype.bitmap[i];
            if(bitmap.width!=140 || bitmap.width_img!=140 || bitmap.s!=0 || bitmap.t!=0 ||
               bitmap.actualHeight!=(i==7?7:14) || bitmap.LUToffset!=0 || !bitmap.buf ||
               bitmap.byte_count!=(i==7?1960u:3920u))return ISLE_GAME_INVALID_ARGUMENT;
        }
        const auto& first=prototype.bitmap[0];
        if(first.width_img<prototype.width || first.actualHeight<=0)return ISLE_GAME_INVALID_ARGUMENT;
        const auto row=std::size_t(first.width_img)*2,tile=std::size_t(first.actualHeight);
        const auto needed=(std::size_t(prototype.height)+tile-1)/tile;
        if(needed>piece_count || tile>piece_bytes/row)return ISLE_GAME_INVALID_ARGUMENT;
        for(std::size_t y=0;y<tile;++y)for(std::size_t x=0;x<std::size_t(prototype.width);++x){
            const auto offset=(y*row+x*2)^((y&1u)?std::size_t{4}:std::size_t{0});
            if(offset>piece_bytes-2)return ISLE_GAME_INVALID_ARGUMENT;
        }
        const auto count=piece_count*piece_bytes;
        State::Thumbnail* slot=nullptr;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(!p.thumbnails[i].serial && p.thumbnails[i].memory.plane_capacity>=count){slot=&p.thumbnails[i];break;}
        if(!slot)return ISLE_GAME_EXHAUSTED;
        scene::SpriteHandle owner;IsleGameSprite *parent,*child,*next,*previous;
        if(!p.services.sprite_links(native,owner,parent,child,next,previous))return ISLE_GAME_INVALID_ARGUMENT;
        const auto incarnation=p.serial();
        auto allocation=std::make_shared<State::ThumbnailStorage>();allocation->heap=p.heap;
        allocation->block=p.heap->allocate(count);
        if(!allocation->block)return ISLE_GAME_EXHAUSTED;
        auto* pixels=static_cast<std::uint8_t*>(allocation->block);
        std::array<sprite::RasterPiece,8> pieces{};
        for(std::size_t i=0;i<piece_count;++i){
            // Preserve the canonical short final piece; allocation stride is
            // still 3920 bytes for every Source destination buffer. Original
            // copies metadata, not stock pixels; fresh actual heap payload
            // remains untouched until its genuine non-NULL preview.
            pieces[i]={prototype.bitmap[i],i*piece_bytes};
        }
        auto raster=std::make_shared<sprite::Raster>(prototype,pieces,
            std::array<sprite::RasterStorage,2>{{{allocation,{pixels,count},slot->memory.physical[0]}, {}}});
        auto binding=p.producer.register_raster(raster);
        slot->raster=std::move(raster);slot->binding=binding;slot->serial=incarnation;
        slot->object=owner.object;slot->sprite=native;slot->prototype=identity;slot->credits=true;
        auto replacement=*current;replacement.bitmap=slot->raster->view(sprite::RasterSource::primary).bitmap;
        if(!p.services.replace_sprite(native,binding.resources[0],replacement,binding.generation))return ISLE_GAME_INVALID_STATE;
        *out=p.handle(incarnation);return p.ok();
    });
}
IsleGameResult isle_post_window_credits_image_retain(IslePostCourseWindow* s,Handle h){
    return invoke(s,[&](State& p){
        auto* t=p.thumbnail(h);if(!t || !t->credits || !p.thumbnail_view(*t))return ISLE_GAME_INVALID_ARGUMENT;
        if(t->credits_holds>=11)return ISLE_GAME_EXHAUSTED;
        ++t->credits_holds;return ISLE_GAME_OK;
    });
}
IsleGameResult isle_post_window_credits_image_resolve(IslePostCourseWindow* s,Handle h,
    IsleGameSprite** out,int* width,int* height){
    if(out)*out=nullptr;if(width)*width=0;if(height)*height=0;
    return invoke(s,[&](State& p){
        auto* t=p.thumbnail(h);if(!out || !width || !height || !t || !t->credits)return ISLE_GAME_INVALID_ARGUMENT;
        const auto* view=p.thumbnail_view(*t);if(!view)return ISLE_GAME_INVALID_STATE;
        *out=t->sprite;*width=view->width;*height=view->height;return ISLE_GAME_OK;
    });
}
IsleGameResult isle_post_window_credits_image_release_hold(IslePostCourseWindow* s,Handle h){
    auto* p=state(s);if(!p)return ISLE_GAME_INVALID_ARGUMENT;
    IsleGameCounts counts{};const auto access=isle_game_counts(p->services.game,&counts);
    if(access!=ISLE_GAME_OK)return access;
    auto* t=p->thumbnail(h);if(!t || !t->credits || !t->credits_holds)return ISLE_GAME_INVALID_ARGUMENT;
    --t->credits_holds;return ISLE_GAME_OK;
}
IsleGameResult isle_post_window_credits_image_detach_object(IslePostCourseWindow* s,IsleGameObject* object){
    return invoke(s,[&](State& p){
        if(!object || !p.owns_object(object))return ISLE_GAME_INVALID_ARGUMENT;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(p.thumbnails[i].serial && p.thumbnails[i].credits && p.thumbnails[i].object==object &&
               p.thumbnails[i].credits_holds)return ISLE_GAME_INVALID_STATE;
        for(std::size_t i=0;i<p.thumbnail_count;++i)
            if(p.thumbnails[i].serial && p.thumbnails[i].credits && p.thumbnails[i].object==object)
                p.thumbnails[i].sprite=nullptr;
        return ISLE_GAME_OK;
    });
}
