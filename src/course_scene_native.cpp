#include "course_scene_native.hpp"
#include "environment_sound.hpp"
#include "player_ui_resources.hpp"
#include "item_resources.hpp"
#include <limits>
namespace isle::course {
bool SceneMechanics::receiver(IsleGameObject* object) const noexcept {
    if (!host || !object || !host->game() || !host->root() || !host->actors()) return false;
    IsleGameObjectMetadata metadata{};
    if (isle_game_object_metadata(host->game(),object,&metadata)!=ISLE_GAME_OK) return false;
    if (object==helper || object==player()->object || object==host->reference_owner() || host->owns_input(object)) return true;
    IsleGameObject* camera_object=nullptr;
    if (player()->camera && isle_game_camera_object(host->game(),player()->camera,&camera_object)==ISLE_GAME_OK && camera_object==object) return true;
    if (host->cart() && host->cart()->instance(object)) return true;
    if (host->items_owner() && host->items_owner()->instance(object)) return true;
    for (const auto& binding:host->actors()->render_bindings()) if (binding.model.object==object) return true;
    return false;
}
IsleGameResult SceneMechanics::joined() const noexcept {
    return receipt && receipt->joined() && !host && !helper ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult SceneMechanics::request(const void* actual,const void* expected) noexcept {
    if (!actual || actual!=expected || requested || host || !snapshot || !receipt) return ISLE_GAME_INVALID_STATE;
    requested=true; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::initialize_ui(IsleGameObject** output) noexcept {
    if (output) *output=nullptr;
    if (!setup() || ui_bound || !host->camera() || !player()) return ISLE_GAME_INVALID_STATE;
    IsleGameObject* object=nullptr;
    auto result=isle_player_root_invoke_init(player(),&object);
    if (result==ISLE_GAME_OK) result=host->root()->complete_initialization(object);
    if (result==ISLE_GAME_OK) { ui_bound=true; if (output) *output=object; }
    return result;
}
IsleGameResult SceneMechanics::background(std::uint8_t* r,std::uint8_t* g,std::uint8_t* b) noexcept {
    if (!r || !g || !b || !host || !host->world_owner()) return ISLE_GAME_INVALID_ARGUMENT;
    const auto& rgb=host->game_state().background_rgb; *r=rgb[0]; *g=rgb[1]; *b=rgb[2]; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::map(std::uint32_t identity,bool ceiling) noexcept {
    if (!setup()) return ISLE_GAME_INVALID_STATE;
    const auto& value=ceiling?host->world_resources().ceiling_map():host->world_resources().height_map();
    auto& bound=ceiling?ceiling_bound:height_bound;
    if (bound || value.wire.guest_address!=identity || value.wire.bytes.empty()) return ISLE_GAME_INVALID_ARGUMENT;
    bound=true; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::world(std::uint32_t identity,std::int32_t sky,std::int32_t first,std::int32_t last,std::int32_t link,std::int32_t draw) noexcept {
    if (!setup() || identity!=host->world_resources().setup().wire.guest_address || sky!=99 || first!=100 || last!=126 || link!=9 || draw!=3) return ISLE_GAME_INVALID_ARGUMENT;
    return host->publish_world();
}
IsleGameResult SceneMechanics::block_next(const world::Block* block,world::Block** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT;
    *output=nullptr;
    if (!host || !host->valid_block(block) || (block->next && !host->valid_block(block->next))) return ISLE_GAME_INVALID_ARGUMENT;
    *output=block->next; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::block_position(const world::Block* block,IsleGameVec3* output) noexcept {
    if (!output || !host || !host->valid_block(block) || !block->resource) return ISLE_GAME_INVALID_ARGUMENT;
    const auto value=block->resource->metadata.world_position; *output={value.x,value.y,value.z}; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::registry(std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d) noexcept {
    return setup() && host->actors()->configure_registry(a,b,c,d) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult SceneMechanics::pokemon_add(world::Block* block,world::Block* reference,std::uint32_t table,std::size_t count) noexcept {
    if (!host || !host->actors()) return ISLE_GAME_INVALID_STATE;
    if (!block) return ISLE_GAME_OK;
    if (!host->valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
    if (!block->resource || block->resource->spawns.empty()) return ISLE_GAME_OK;
    if (!host->valid_block(reference) || !reference->resource) return ISLE_GAME_INVALID_ARGUMENT;
    const auto values=host->actor_resources().definitions();
    for (const auto& row:block->resource->spawns) {
        std::size_t selected=actor::actor_no_index;
        for (std::size_t i=0;i<values.size();++i) if (values[i].wire.guest_address>=table && values[i].wire.guest_address<table+count*16 && values[i].actor_id==std::uint32_t(row.id)) {
            if (selected!=actor::actor_no_index) return ISLE_GAME_INVALID_STATE;
            selected=i;
        }
        if (selected==actor::actor_no_index) return ISLE_GAME_INVALID_ARGUMENT;
        const auto result=host->actors()->add_one(selected,block,reference,row);
        if (!result) return host->fail(ISLE_GAME_INVALID_STATE,result.status.detail);
    }
    return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::pokemon_change(world::Block* block,world::Block* reference,std::uint32_t table,std::size_t count,IsleCourseActorBase* source,IsleGameResult (*invoke)(IsleCourseActorBase*,std::uint32_t,IsleGameObject*,const IsleGameVec3*,const IsleGameVec3*)) noexcept {
    if (!host || !host->services() || !source || !invoke) return ISLE_GAME_INVALID_STATE;
    if (!host->services()->send_command_to_link(3,19,nullptr)) return ISLE_GAME_INVALID_STATE;
    if (!block || !reference) return ISLE_GAME_OK;
    if (!host->valid_block(block) || !host->valid_block(reference)) return ISLE_GAME_INVALID_ARGUMENT;
    if (!reference->resource || !reference->previous || !reference->previous->resource) return ISLE_GAME_OK;
    std::array<menu::animation::InstanceHandle,100> members{}; std::size_t written=0;
    if (!host->actors()->block_members(*block,members,written)) return ISLE_GAME_INVALID_STATE;
    const auto p=reference->previous->resource->metadata.world_position, c=reference->resource->metadata.world_position;
    const IsleGameVec3 previous{p.x,p.y,p.z}, current{c.x,c.y,c.z};
    for (std::size_t n=0;n<written;++n) {
        const auto* actor=host->actors()->source(members[n]); if (!actor) return ISLE_GAME_INVALID_STATE;
        for (const auto& row:host->actor_resources().definitions()) if (row.wire.guest_address>=table && row.wire.guest_address<table+count*16 && row.actor_id==std::uint32_t(actor->id)) {
            if (row.change_block_identity) {
                const auto result=invoke(source,row.change_block_identity,host->actors()->object(members[n]),&previous,&current);
                if (result!=ISLE_GAME_OK) return result;
            }
            break;
        }
    }
    return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::pokemon_remove(world::Block* block,std::uint32_t table,std::size_t count,IsleCourseActorBase* source,IsleGameResult (*invoke)(IsleCourseActorBase*,std::uint32_t,IsleGameObject*)) noexcept {
    if (!host || !host->actors() || !source || !invoke) return ISLE_GAME_INVALID_STATE;
    if (!block) return ISLE_GAME_OK;
    if (!host->valid_block(block)) return ISLE_GAME_INVALID_ARGUMENT;
    if (!block->resource) return ISLE_GAME_OK;
    std::array<menu::animation::InstanceHandle,100> members{}; std::size_t written=0;
    if (!host->actors()->block_members(*block,members,written)) return ISLE_GAME_INVALID_STATE;
    for (std::size_t n=0;n<written;++n) {
        const auto* actor=host->actors()->source(members[n]); if (!actor) return ISLE_GAME_INVALID_STATE;
        for (const auto& row:host->actor_resources().definitions()) if (row.wire.guest_address>=table && row.wire.guest_address<table+count*16 && row.actor_id==std::uint32_t(actor->id)) {
            if (row.remove_identity==0x80364280u) {
                if (!host->actors()->request_cleanup(members[n])) return ISLE_GAME_INVALID_STATE;
            } else if (row.remove_identity) {
                const auto result=invoke(source,row.remove_identity,host->actors()->object(members[n]));
                if (result!=ISLE_GAME_OK) return result;
            }
            break;
        }
    }
    return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::asset(std::size_t index,std::uint32_t first,std::uint32_t end,SceneAsset** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!setup() || index>=assets.size() || first>=end || end>snapshot->canonical().size()) return ISLE_GAME_INVALID_ARGUMENT;
    auto& value=assets[index];
    if (value.first!=first || value.end!=end) return ISLE_GAME_INVALID_ARGUMENT;
    value.loaded=true; *output=&value; return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::bank(std::int32_t id,const SceneAsset* script,const SceneAsset* images,menu::EffectGroup group) noexcept {
    if (!setup() || !script || !images || !script->loaded || !images->loaded) return ISLE_GAME_INVALID_STATE;
    if (id==3 && script==&assets[0] && images==&assets[1]) return host->install_effect_bank(menu::EffectGroup::common);
    if (id==0 && script==&assets[2] && images==&assets[3]) return host->install_effect_bank(group);
    return ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult SceneMechanics::fx_camera(std::int32_t link,std::int32_t count,IsleGameCamera* camera,IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    return setup() && player() && player()->camera==camera && camera && link==4 && count==100 ? host->publish_particle_camera(output) : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult SceneMechanics::fx_pool(std::int32_t count,IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    return setup() && count==10 ? host->publish_effect_emitters(output) : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult SceneMechanics::fx_publish(std::uint32_t identity,IsleGameObject* object) noexcept {
    if (!setup() || !player() || !object) return ISLE_GAME_INVALID_STATE;
    if (identity==0x80382d10u) return object==host->actors()->particle_object() && player()->D_80382D10_523120==object ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    if (identity==0x80382d14u) return object==host->actors()->effect_object() && player()->D_80382D14_523124==object ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
    return ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult SceneMechanics::dither(std::uint32_t color,std::uint32_t alpha) noexcept {
    if (!host || !host->actors() || color>255 || alpha>255) return ISLE_GAME_INVALID_ARGUMENT;
    host->actors()->effects().set_dither(std::uint8_t(color),std::uint8_t(alpha)); return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::effect(std::int32_t bank,std::int32_t script,SceneEffect** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!host || !host->actors() || bank<0 || bank>UINT16_MAX || script<0) return ISLE_GAME_INVALID_ARGUMENT;
    const auto made=host->actors()->effects().create_effect(std::uint16_t(bank),std::size_t(script));
    if (!made.status) return ISLE_GAME_INVALID_STATE;
    if (made.spawned) { if (made.handle.slot>=effects.size()) return ISLE_GAME_INVALID_STATE; auto& value=effects[made.handle.slot]; value.handle=made.handle; *output=&value; }
    return ISLE_GAME_OK;
}
IsleGameResult SceneMechanics::attach(SceneEffect* effect,const IslePlayerCartNode* node) noexcept {
    if (!host || !host->cart() || !host->actors() || !effect || !node) return ISLE_GAME_INVALID_ARGUMENT;
    bool found=false; for (auto& value:effects) if (&value==effect) found=true;
    if (!found) return ISLE_GAME_INVALID_ARGUMENT;
    auto attachment=host->cart()->retain_effect_node(node->binding,node->node);
    if (!attachment) return ISLE_GAME_INVALID_STATE;
    return host->actors()->effects().set_effect_attachment(effect->handle,std::move(attachment)) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult SceneMechanics::reference(IsleGameObject** field,IsleGameObject* value) noexcept { return host ? host->reference(field,value) : ISLE_GAME_INVALID_STATE; }
IsleGameResult SceneMechanics::helper_create(std::int32_t id,IsleGameSourceCallback callback,std::int32_t link,std::uint32_t priority,IsleGameObject** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!setup() || helper || id!=0 || link!=0 || priority!=0x80000000u || !callback) return ISLE_GAME_INVALID_ARGUMENT;
    const auto result=host->create_helper(0,callback,0,0x80000000u,output); if (result==ISLE_GAME_OK) helper=*output; return result;
}
IsleGameResult SceneMechanics::process(IsleGameObject* object,IsleGameSourceCallback callback,std::int32_t kind,std::int32_t priority,IsleGameProcess** output) noexcept {
    if (!output) return ISLE_GAME_INVALID_ARGUMENT; *output=nullptr;
    if (!receiver(object) || !callback || (kind!=0 && kind!=1) || priority<0) return ISLE_GAME_INVALID_ARGUMENT;
    return isle_game_add_source_process(host->game(),object,callback,kind==0?ISLE_GAME_STACKFUL:ISLE_GAME_SYNCHRONOUS,std::uint32_t(priority),output);
}
IsleGameResult SceneMechanics::camera_component(IsleGameCamera* camera,std::uint32_t component,float* output,bool store,float value) noexcept {
    if (!host || !player() || camera!=player()->camera || !camera || component>5 || (!store && !output)) return ISLE_GAME_INVALID_ARGUMENT;
    const IsleGameCameraParameters* parameters=nullptr;
    auto result=isle_game_camera_parameters(host->game(),camera,&parameters); if (result!=ISLE_GAME_OK) return result;
    if (!store) {
        *output=component<3 ? parameters->look_at.eye[component] : parameters->look_at.at[component-3];
        return ISLE_GAME_OK;
    }
    auto look=parameters->look_at;
    if (component<3) look.eye[component]=value; else look.at[component-3]=value;
    return isle_game_camera_set_look_at(host->game(),camera,&look);
}
IsleGameResult SceneMechanics::cart_retire(IsleGameObject* object) noexcept {
    return host && host->cart() && object && host->cart()->instance(object) && host->cart()->request_retire(object) ? ISLE_GAME_OK : ISLE_GAME_INVALID_ARGUMENT;
}
IsleGameResult SceneMechanics::release_references() noexcept {
    if (!host || !host->services()) return ISLE_GAME_OK;
    if (isle_game_require_idle(host->game())!=ISLE_GAME_OK || host->graphics_busy()) return ISLE_GAME_INVALID_STATE;
    if (host->actors() && host->actors()->effects().effect_count()) return ISLE_GAME_INVALID_STATE;
    if (host->renderer()) { const auto result=host->renderer()->service_main_removal(); if (result!=ISLE_GAME_OK) return result; }
    return !player() || (!player()->D_80382D10_523120 && !player()->D_80382D14_523124) ? ISLE_GAME_OK : ISLE_GAME_INVALID_STATE;
}
IsleGameResult SceneMechanics::environment(const void* values,std::size_t count,std::size_t expected,std::uint16_t (*sound_at)(const void*,std::size_t),std::uint8_t (*pitch_at)(const void*,std::size_t),std::uint8_t (*range_at)(const void*,std::size_t)) noexcept {
    if (!setup() || !values || count!=expected || !sound_at || !pitch_at || !range_at) return ISLE_GAME_INVALID_ARGUMENT;
    try {
        environment_sound::Resources resources(*snapshot,host->actors()->course_context().course_id());
        const auto descriptors=resources.descriptors(); if (descriptors.size()!=count) return ISLE_GAME_INVALID_STATE;
        for (std::size_t i=0;i<count;++i) if (sound_at(values,i)!=descriptors[i].sound || pitch_at(values,i)!=descriptors[i].pitch_modifier || range_at(values,i)!=descriptors[i].hearing_range) return ISLE_GAME_INVALID_ARGUMENT;
        return host->environment_initialize();
    } catch (...) { return ISLE_GAME_INVALID_STATE; }
}
IsleGameResult SceneMechanics::sound(std::int32_t id,std::int32_t volume,std::int32_t pan,float pitch,std::int32_t reverb,std::int32_t* output) noexcept {
    if (!output || !audio) return ISLE_GAME_INVALID_ARGUMENT; *output=-1;
    int slot=-1; const auto result=audio->play_sound(std::uint32_t(id),{std::uint16_t(volume),std::uint8_t(pan),std::uint8_t(reverb),pitch},slot); *output=slot; return audio_result(result);
}
void SceneMechanics::prepare_capacity(std::int32_t id,std::size_t field_count) {
    if (!snapshot || !model_blocks.empty() || capacity.arena_bytes) throw std::logic_error("native capacity preparation is once-only");
    world::Resources world(*snapshot,id);
    actor::Resources actors(*snapshot,id,{world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max});
    items::Resources items(*snapshot,world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max);
    player::CartResources carts(*snapshot,id,{world.setup().fog_rgb,world.setup().fog_min,world.setup().fog_max});
    envelope.reference_fields=field_count;
    for (const auto& block:world.blocks()) { envelope.initialized_spawns+=block.spawns.size(); envelope.world_objects+=1+(block.road_model!=world::no_index)+block.decorations.size(); }
    // World publishes id99 and its timer even when its sky display/model is absent.
    envelope.world_objects+=1;
    // Reserve all initialized World publications simultaneously, rather than
    // assuming only two visible blocks. Native retained cohorts include a second
    // complete logical registry and every entry-owned semantic global target.
    capacity.actors=200+field_count; capacity.items=64; capacity.carts=4;
    capacity.helpers=32; capacity.render_order=envelope.world_objects+capacity.actors+capacity.items+capacity.carts+64;
    capacity.sprite_draws=512; capacity.command_nodes=capacity.render_order*64;
    capacity.pools={capacity.render_order+128,capacity.actors*16+capacity.items*8+capacity.carts*8+512,512,32,capacity.render_order*4};
    capacity.arena_bytes=128u*1024u*1024u;
    std::size_t native_nodes=16;
    auto add=[&](std::span<const menu::Model> models,std::size_t instances) {
        std::size_t maximum=0;
        for (const auto& model:models) maximum=std::max(maximum,model.nodes.size());
        if (maximum && instances) { model_blocks.push_back({maximum,instances}); native_nodes+=maximum*instances; }
    };
    add(world.models(),envelope.world_objects); add(actors.models(),capacity.actors); add(items.models(),capacity.items); add(carts.animation_resources().models,capacity.carts);
    model_blocks.push_back({1,16}); capacity.model_blocks=model_blocks;
    capacity.pools.matrices=native_nodes*3+capacity.pools.cameras*8;
    auto execution=[](std::span<const menu::Animation> animations,std::span<const menu::Model> models) {
        std::size_t commands=0,receivers=1;
        for (const auto& animation:animations) commands+=animation.commands.size();
        for (const auto& model:models) { const auto slot=menu::animation::slot_capacity(model); receivers=std::max(receivers,slot.nodes+slot.material_attachments); }
        // Operational cyclic-step envelope: four complete decoded instruction
        // closures per receiver. This is explicit, not a finite-only certificate.
        const auto bound=std::max<std::size_t>(1,commands)*receivers*4;
        return menu::animation::ExecutionCapacity{bound,bound};
    };
    capacity.world_execution=execution(world.animations(),world.models()); capacity.actor_execution=execution(actors.animations(),actors.models());
    capacity.item_execution=execution(items.animations(),items.models());
    const auto cart=carts.animation_resources(); capacity.cart_execution=execution(cart.animations,cart.models); capacity.camera_execution=execution(cart.animations,{});
}
}
