#!/usr/bin/env python3
"""Admit complete pinned own-GAME Msg/Icons/film definitions privately."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
VERSION = 'player-ui-complete-v1'
FILES = ('src/app_level/msg.c', 'src/app_level/icons.c', 'src/app_level/4FE780.c',
         'src/app_level/font.c', 'src/app_level/player.c', 'src/app_level/app_level.h', 'include/enums.h', 'tools/symbol_addrs.txt', 'splat.yaml')
DEFINITIONS = (
 ('msg.c','Msg_UpdatePrinter',48,83),('msg.c','Msg_GetGlyphWidth',85,93),
 ('msg.c','Msg_GetCharParams',95,209),('msg.c','Msg_DrawChar',211,238),
 ('msg.c','Msg_DrawMessage',240,315),('msg.c','Msg_InitCamera',317,327),
 ('msg.c','Msg_ShowMessage',329,375),('msg.c','Msg_Reset',377,380),
 ('msg.c','Msg_SetSpecialHandler',382,384),('msg.c','Msg_SetNonInteractive',386,388),
 ('msg.c','Msg_SetInteractive',390,392),('msg.c','Msg_IsMessagePrinted',394,396),
 ('icons.c','Icons_UpdateDefault',88,89),('icons.c','func_8035CBB8_4FCFC8',91,96),
 ('icons.c','Icons_ShakeIcon',98,112),('icons.c','Icons_UpdateDashEngineIcon',114,117),
 ('icons.c','Icons_UpdatePhotoIcon',119,143),('icons.c','func_8035CE74_4FD284',145,146),
 ('icons.c','Icons_ShrinkAndRestore',148,163),('icons.c','Icons_UpdateAppleIcon',165,168),
 ('icons.c','Icons_UpdatePesterBallIcon',170,173),('icons.c','Icons_UpdateFluteIcon',175,205),
 ('icons.c','Icons_Init',207,318),('icons.c','Icons_SetDashEngineEnabled',320,322),
 ('icons.c','Icons_MoveIcon',324,328),('icons.c','Icons_ProcessZoomIn',330,365),
 ('icons.c','Icons_FinishZoomOut',367,407),('icons.c','Icons_ProcessZoomOut',409,444),
 ('icons.c','Icons_FinishZoomIn',446,486),('icons.c','Icons_ProcessZoom',488,498),
 ('icons.c','Icons_Show',500,503),('icons.c','Icons_Hide',505,508),
 ('icons.c','Icons_ProcessButtonPress',510,523),('icons.c','Icons_ProcessTakePhotoPressed',525,527),
 ('icons.c','Icons_Freeze',529,531),('icons.c','Icons_UnFreeze',533,535),
 ('4FE780.c','func_8035E370_4FE780',38,40),('4FE780.c','func_8035E37C_4FE78C',44,72),
 ('4FE780.c','func_8035E4D0_4FE8E0',74,79),('4FE780.c','func_8035E508_4FE918',81,83),
 ('4FE780.c','func_8035E52C_4FE93C',85,116),('4FE780.c','func_8035E754_4FEB64',118,120),
 ('4FE780.c','func_8035E764_4FEB74',122,124))
ORIGINS = (
 ('src/app_level/font.c','complete_font_pixels_descriptors_maps_widths_defaults',1,1250),
 ('src/app_level/icons.c','icon_storage_and_choreography_definitions',47,84),
 ('src/app_level/player.c','reticle_pause_sprite_identities',38,56),
 ('src/app_level/app_level.h','item_ids',25,29),
 ('include/enums.h','progress_flags',57,74),
 ('include/enums.h','object_ids',283,299),
 ('include/enums.h','message_alignment',405,409),
 ('tools/symbol_addrs.txt','message_initialized_addresses',960,987),
 ('splat.yaml','app_level_initialized_mapping',5292,5338),
 ('splat.yaml','font_final_mapping',5625,5636),
)
CALLBACKS = {'Msg_UpdatePrinter','Icons_UpdateDefault','Icons_UpdateDashEngineIcon','Icons_UpdatePhotoIcon',
 'Icons_UpdateAppleIcon','Icons_UpdatePesterBallIcon','Icons_UpdateFluteIcon','Icons_ProcessZoomIn',
 'Icons_FinishZoomOut','Icons_ProcessZoomOut','Icons_FinishZoomIn'}
PREAMBLE = '''/* Complete pinned own-GAME bodies. Native semantic types, no SDK headers. */
#include "engine_internal.h"
#include "player_ui_source.h"
#include "player_photo_source.h"
#include <string.h>
#define SP_HIDDEN ISLE_SP_HIDDEN
#define A_BUTTON 0x8000u
#define B_BUTTON 0x4000u
#define START_BUTTON 0x1000u
#define PF_HAS_APPLE 1u
#define PF_HAS_PESTER_BALL 2u
#define PF_HAS_FLUTE 4u
#define PF_HAS_DASH_ENGINE 0x20u
#define PF_ZOOM_SWITCH 0x1000u
#define ITEM_ID_POKEFLUTE 161
#define ITEM_ID_PESTER_BALL 162
#define ITEM_ID_APPLE 163
#define MSG_ALIGN_LEFT 0
#define MSG_ALIGN_RIGHT 1
#define CHAR_TYPE_REGULAR 0
#define CHAR_TYPE_IMAGE 1
#define ICON_ID_APPLE 0
#define ICON_ID_PESTER_BALL 1
#define ICON_ID_FLUTE 2
#define ICON_ID_ZOOM 3
#define ICON_ID_DASH 4
#define ICON_ID_DASH_ZOOMED 5
#define ICON_ID_TAKE_PHOTO 6
#define ICON_ID_ZOOM_OFF 7
#define OBJID_28 28
#define LINK_0 0
#define DL_LINK_1 1
#define GOBJ_FLAG_HIDDEN 1u
#define SET_SPRITE_POS_PTR(s,xv,yv) ((s)->x=(xv),(s)->y=(yv))
typedef float f32;
typedef int16_t s16;
typedef IslePlayerUiIcon SpriteStruct;
typedef IslePlayerUiIconDef SpriteDefStruct;
static void ui_move(IsleSprite* s, int32_t x, int32_t y) {s->x=x;s->y=y;}
static void ui_scale(IsleSprite* s,float x,float y) {s->scalex=x;s->scaley=y;s->attr|=ISLE_SP_SCALE;}
static void ui_color(IsleSprite* s,uint8_t r,uint8_t g,uint8_t b,uint8_t a) {s->red=r;s->green=g;s->blue=b;s->alpha=a;}
#define spMove ui_move
#define spScale ui_scale
#define spColor ui_color
#define spSetAttribute(s,a) ((s)->attr|=(a))
#define spClearAttribute(s,a) ((s)->attr&=~(a))
static void ui_callback_check(IslePlayerUiSource* ui) {
 if(ui->status!=ISLE_GAME_OK) isle_native_execution_fault(ui->player->game,"Player UI native leaf failed");
}
'''

def transform(symbol, original, globals_):
    text = original
    changes = []
    def replace(old, new):
        nonlocal text
        count = text.count(old)
        if not count: raise ValueError('missing reviewed UI rewrite '+symbol+': '+old)
        changes.append(dict(find_sha256=hashlib.sha256(old.encode()).hexdigest(),
                            replace_sha256=hashlib.sha256(new.encode()).hexdigest(),count=count))
        text = text.replace(old,new)
    def regex(pattern,fn):
        nonlocal text
        counts={}
        def apply(match):
            old=match.group();new=fn(match)
            counts[(old,new)]=counts.get((old,new),0)+1
            return new
        text=re.sub(pattern,apply,text,flags=re.MULTILINE)
        for (old,new),count in counts.items():
            changes.append(dict(find_sha256=hashlib.sha256(old.encode()).hexdigest(),
                                replace_sha256=hashlib.sha256(new.encode()).hexdigest(),
                                pattern_sha256=hashlib.sha256(pattern.encode()).hexdigest(),count=count))
    if symbol == 'Msg_DrawChar':
        begin=text.index('    if (Msg_CharType')
        replace(text[begin:text.rindex('}')],'''    if (Msg_ShadowEnabled) {
        ui->draw_red=ui->draw_green=ui->draw_blue=0; ui->draw_alpha=255;
        isle_player_ui_source_record(ui,isle_player_ui_native_glyph(ui,timg,ulx+1,uly+1,lrx+1,lry+1,0));
        ui->draw_red=Msg_TextColorR; ui->draw_green=Msg_TextColorG;
        ui->draw_blue=Msg_TextColorB; ui->draw_alpha=Msg_TextColorA;
    }
    if (Msg_BlinkEnabled) {
        ui->draw_red=Msg_TextColorR*Msg_BlinkAlpha;
        ui->draw_green=Msg_TextColorG*Msg_BlinkAlpha;
        ui->draw_blue=Msg_TextColorB*Msg_BlinkAlpha; ui->draw_alpha=Msg_TextColorA;
    }
    isle_player_ui_source_record(ui,isle_player_ui_native_glyph(ui,timg,ulx,uly,lrx,lry,Msg_ShadowEnabled ? 0 : Msg_CharType));
''')
    if symbol == 'Msg_DrawMessage':
        first=text.index('    gDPPipeSync('); last=text.index("    if (Msg_PrintedText")
        replace(text[first:last],'''    ui->draw_red=Msg_TextColorR; ui->draw_green=Msg_TextColorG;
    ui->draw_blue=Msg_TextColorB; ui->draw_alpha=Msg_TextColorA;

''')
        replace('    gDPPipeSync(gMainGfxPos[0]++);\n','')
    if symbol == 'Msg_InitCamera':
        replace('    if (ohFindById(OBJID_MSG_CAMERA) != NULL) {','    if (isle_player_ui_source_find_id(ui,29) != NULL) {')
        replace('    Msg_CameraObject = cam = ohCreateCamera(OBJID_MSG_CAMERA, ohUpdateDefault, LINK_0, 0x80000000, Msg_DrawMessage, 3, 0, 0, false, 0, NULL, 0, 0);',
                '    cam = NULL;\n    isle_player_ui_source_record(ui,isle_player_ui_native_message_camera(ui,&cam));\n    Msg_CameraObject = cam;')
    if symbol == 'func_8035CBB8_4FCFC8':
        replace('    arg0->rsp_dl = arg1->rsp_dl;','    isle_player_ui_source_record(ui,isle_player_ui_native_copy_bitmap(ui,arg0,arg1));')
    if symbol == 'func_8035E52C_4FE93C':
        for target,index,digit in [('D_80388F58_529368',0,'ones'),('D_803890B8_5294C8',1,'tens'),('D_80389218_529628',2,'hundreds')]:
            replace(target+' = D_8038A034_52A444['+digit+'];',
                    'isle_player_ui_source_record(ui,isle_player_ui_native_digit(ui,'+str(index)+','+digit+'));')
    regex(r'omAddGObj\(([^,]+), (Icons_UpdateDefault|ohUpdateDefault), (?:0|LINK_0), 0x80000000\)',
          lambda m:'isle_player_ui_source_object(ui, '+m[1]+', '+('NULL' if m[2]=='ohUpdateDefault' else 'isle_player_ui_'+m[2])+')')
    regex(r'omLinkGObjDL\(([^,]+), &?renDrawSprite, (1|DL_LINK_1), 0x80000000, -1\)',
          lambda m:'isle_player_ui_source_record(ui,isle_player_ui_native_render(ui,'+m[1]+',1,0x80000000))')
    regex(r'omCreateProcess\(([^,]+), (\w+), ([01]), (\d+)\)',lambda m:
          'isle_player_ui_source_process(ui, '+m[1]+', isle_player_ui_'+m[2]+', '+m[3]+', '+m[4]+')')
    if 'omGObjAddSprite(' in text: replace('omGObjAddSprite(', 'isle_player_ui_source_sprite(ui, ')
    for root in ('D_80388E00_529210','D_80388F60_529370','D_803890C0_5294D0','D_80389220_529630'):
        if '&'+root in text: replace('&'+root,root)
    if symbol=='Icons_Init':
        replace('    gobj = isle_player_ui_source_object(ui, 26, isle_player_ui_Icons_UpdateDefault);',
                '    gobj = isle_player_ui_source_object(ui, 26, isle_player_ui_Icons_UpdateDefault);\n    if (gobj == NULL) return ui->status;')
        regex(r'(^[ ]*spr->spriteObj = [^\n]*isle_player_ui_source_sprite\([^\n]+;)',lambda m:m[0]+'\n        if (spr->spriteObj == NULL) return ui->status;')
    if symbol=='func_8035E37C_4FE78C':
        replace('    obj = isle_player_ui_source_object(ui, OBJID_28, NULL);',
                '    obj = isle_player_ui_source_object(ui, OBJID_28, NULL);\n    if (obj == NULL) return ui->status;')
        regex(r'(sobj = isle_player_ui_source_sprite\([^\n]+;)',lambda m:m[0]+'\n    if (sobj == NULL) return ui->status;')
    if symbol=='func_8035E4D0_4FE8E0':
        replace('omDeleteGObj(D_803B09D8_550DE8);','isle_player_ui_source_record(ui,isle_player_ui_native_delete(ui,D_803B09D8_550DE8));')
    for old,new in [('getProgressFlags()','ui->player->ProgressFlags'),('gContInputPressedButtons','ui->player->input.pressed_buttons'),
                    ('LastItemId','ui->player->LastItemId'),('randRange(5)','isle_rng_range(ui->player->rng,5)'),
                    ('func_8009BC68()','isle_player_photo_func_8009BC68(ui->player->photo)'),
                    ('ohPauseObjectProcesses(', 'isle_player_ui_source_pause(ui, '),
                    ('ohResumeObjectProcesses(', 'isle_player_ui_source_pause(ui, ')]:
        if old in text: replace(old,new)
    if symbol=='Icons_Freeze': replace('Icons_MainObject);','Icons_MainObject,1);')
    if symbol=='Icons_UnFreeze': replace('Icons_MainObject);','Icons_MainObject,0);')
    # Const read-only message/bitmap API; original byte arithmetic is unchanged.
    if symbol in {'Msg_GetCharParams','Msg_DrawChar','Msg_ShowMessage','Msg_DrawMessage'}:
        for old,new in [('u8* ptr','const u8* ptr'),('u8* img','const u8* img'),('u8* timg','const u8* timg'),('u8* message','const u8* message'),('s16* test','const s16* test')]:
            if old in text: replace(old,new)
        if symbol=='Msg_GetCharParams': replace('u8* Msg_GetCharParams','const u8* Msg_GetCharParams')
    if symbol=='func_8035CBB8_4FCFC8': replace('Sprite* arg1','const Sprite* arg1')
    # All mutable globals resolve to the sole module context. Longest names first.
    for token in sorted(globals_,key=len,reverse=True):
        regex(r'(?<![\w>])'+re.escape(token)+r'\b',lambda m:'ui->'+m[0])
    for name in sorted((d[1] for d in DEFINITIONS),key=len,reverse=True):
        if name in CALLBACKS: continue
        regex(r'\b'+name+r'\(',lambda m:'isle_player_ui_'+name+'(ui, ')
        text_old='(ui, )'
        if text_old in text: replace(text_old,'(ui)')
    # Rewrite the definition signature only after callsites; function pointer args
    # and callbacks keep their actual original one-argument callable identity.
    opening=text[:text.index('{')].rstrip()
    if symbol in CALLBACKS:
        arg='obj' if symbol=='Msg_UpdatePrinter' else 'arg0'
        native=opening.replace(symbol,'isle_player_ui_'+symbol)
        replace(opening+' {',native+' {\n    IslePlayerUiSource* ui = isle_player_ui_source_current('+arg+');\n    ui_callback_check(ui);')
    else:
        native=opening.replace('(ui, void)','(IslePlayerUiSource* ui)').replace('(ui, ','(IslePlayerUiSource* ui, ').replace('(ui)','(IslePlayerUiSource* ui)')
        if opening.startswith('bool '): native=native.replace('bool ','int ',1)
        if opening.startswith('void '): native=native.replace('void ','IsleGameResult ',1)
        replace(opening,native)
        if opening.startswith('void '):
            if 'return;' in text: replace('return;','return ui->status;')
            pos=text.rindex('}')
            tail=text[pos:]
            returned='    return ui->status;\n'+tail
            changes.append(dict(find_sha256=hashlib.sha256(tail.encode()).hexdigest(),
                                replace_sha256=hashlib.sha256(returned.encode()).hexdigest(),
                                count=1,offset=pos,scope='function_suffix'))
            text=text[:pos]+returned
    if symbol in CALLBACKS or symbol=='Icons_ShrinkAndRestore':
        if 'ohWait(' in text: replace('ohWait(', 'ui_callback_check(ui); ohWait(')
        if 'omEndProcess(' in text: replace('omEndProcess(', 'ui_callback_check(ui); omEndProcess(')
    return text,changes

def prepare(repository,manifest_path,output_path,evidence_path):
    raw=manifest_path.read_bytes(); manifest=json.loads(raw)
    if manifest['revision']!=REVISION or manifest['transformation_version']!=VERSION: raise ValueError('UI authority mismatch')
    if set(manifest['source_files'])!=set(FILES): raise ValueError('UI source allowlist mismatch')
    sources={}
    for path,identity in manifest['source_files'].items():
        data=git(repository,'show',REVISION+':'+path)
        blob=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
        if identity['git_blob']!=blob or identity['sha256']!=hashlib.sha256(data).hexdigest(): raise ValueError('UI identity mismatch '+path)
        sources[path]=data.decode().splitlines(keepends=True)
    if manifest['generator_sha256']!=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(): raise ValueError('UI generator mismatch')
    for path,identity in manifest['native_support'].items():
        if hashlib.sha256((Path(__file__).resolve().parent.parent/path).read_bytes()).hexdigest()!=identity['sha256']:
            raise ValueError('UI native support mismatch '+path)
    if len(manifest['semantic_origins'])!=len(ORIGINS): raise ValueError('UI semantic inventory mismatch')
    for origin,(path,label,first,last) in zip(manifest['semantic_origins'],ORIGINS):
        if (origin['path'],origin['label'],origin['first'],origin['last'])!=(path,label,first,last):
            raise ValueError('UI semantic inventory mismatch')
        if hashlib.sha256(''.join(sources[path][first-1:last]).encode()).hexdigest()!=origin['source_sha256']:
            raise ValueError('UI semantic identity mismatch '+label)
    globals_=manifest['context_globals']
    header=(Path(__file__).resolve().parent.parent/'src/player_ui_source.h').read_text()
    if any(not re.search(r'\b'+re.escape(name)+r'\b',header) for name in globals_): raise ValueError('UI context mismatch')
    if len(manifest['definitions'])!=len(DEFINITIONS): raise ValueError('UI complete body inventory mismatch')
    output=[PREAMBLE]; admitted=[]
    for unit,(file,symbol,first,last) in zip(manifest['definitions'],DEFINITIONS):
        path='src/app_level/'+file
        if (unit['path'],unit['symbol'],unit['first'],unit['last'])!=(path,symbol,first,last): raise ValueError('UI body inventory mismatch')
        selected=''.join(sources[path][first-1:last])
        if unit['source_sha256']!=hashlib.sha256(selected.encode()).hexdigest(): raise ValueError('UI body mismatch '+symbol)
        native,changes=transform(symbol,selected,globals_)
        output.extend(('#line %d "snap@%s/%s"\n'%(first,REVISION,path),native,'\n'))
        admitted.append(dict(unit,native_sha256=hashlib.sha256(native.encode()).hexdigest(),changes=changes))
    result=''.join(output)
    publish(output_path,result)
    publish(evidence_path,json.dumps(dict(manifest,manifest_sha256=hashlib.sha256(raw).hexdigest(),transformed_definitions=admitted,source_sha256=hashlib.sha256(result.encode()).hexdigest()),indent=2)+'\n')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('repository','manifest','output','evidence'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args(); prepare(args.repository,args.manifest,args.output,args.evidence)
if __name__=='__main__': main()
