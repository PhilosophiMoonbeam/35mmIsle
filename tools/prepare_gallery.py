#!/usr/bin/env python3
"""Strict Gallery Original-C private provider; requires immutable Git blobs."""
import argparse, ast, hashlib, json, re
from pathlib import Path
from prepare_engine import git, publish
REVISION="df8815e75f3d79618466e01e0827c6165b93d11c"
TOKENS=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/',re.S)
def digest(x): return hashlib.sha256(x.encode()).hexdigest()
def code_only(text,fn):
    out=[]; pos=0
    for m in TOKENS.finditer(text): out.extend((fn(text[pos:m.start()]),m[0])); pos=m.end()
    out.append(fn(text[pos:])); return ''.join(out)
def encode_literals(text):
    def literal(m):
        t=m[0]
        if t.startswith('"'):
            b=ast.literal_eval(t).encode('euc_jp')
            return '"'+''.join('\\x%02x'%c for c in b)+'"'
        return t
    return TOKENS.sub(literal,text)
BRIDGE={'FocusMark_Create': 'gc_focus_create', 'FocusMark_MoveFront': 'gc_focus_front', 'FocusMark_SetPos': 'gc_focus_position', 'FocusMark_SetTargetPos': 'gc_focus_target', 'FocusMark_SetTargetSize': 'gc_focus_target_size', 'FocusMark_Show': 'gc_focus_show', 'FocusMark_StopAnimation': 'gc_focus_stop', 'UIElement_Create': 'gc_element_create', 'UIElement_Delete': 'gc_element_delete', 'UIElement_Draw': 'gc_element_draw', 'UIElement_DrawBackground': 'gc_element_background', 'UIElement_PrintText': 'gc_print_source', 'UIElement_SetColor': 'gc_element_color', 'UIElement_SetFont': 'gc_element_font', 'UIElement_SetState': 'gc_element_state', 'UIElement_SetTextPos': 'gc_element_position', 'UIFrame_Create': 'gc_frame_create', 'UIFrame_FadeIn': 'gc_frame_fade_in', 'UIFrame_FadeOut': 'gc_frame_fade_out', 'UIFrame_GetBackgroundColor': 'gc_frame_color', 'UIFrame_SetPos': 'gc_frame_position', 'UIFrame_SetStyle': 'gc_frame_style', 'UIFrame_Show': 'gc_frame_show', 'UIFrame_ShowBackground': 'gc_frame_background', 'UILayout_CreateButtons': 'gc_layout_create', 'UILayout_DisableButton': 'gc_layout_disable', 'UILayout_GetButtons': 'gc_selector', 'UILayout_HideButtons': 'gc_layout_hide', 'UILayout_HideHeaderElement': 'gc_layout_hide_header', 'UILayout_Init': 'gc_layout_initialize', 'UILayout_SetHeaderFlags': 'gc_layout_header', 'UILayout_ShowHeaderElement': 'gc_layout_show_header', 'UILayout_ShowPanel': 'gc_layout_panel', 'UILayout_WaitPanelTransitionComplete': 'gc_layout_wait', 'UIText_GetStringWidth': 'gc_width', 'UIText_Initialize': 'gc_text_initialize', 'UIText_SetFont': 'gc_text_font', 'UIText_SetPrintDelay': 'gc_text_delay', 'UIText_SetShadowOffset': 'gc_text_shadow', 'UIText_SetSpacing': 'gc_text_spacing', 'auPlaySong': 'gc_song', 'auPlaySound': 'gc_sound', 'auPlaySoundWithParams': 'gc_sound_params', 'auSetBGMVolume': 'gc_volume', 'auSetBGMVolumeSmooth': 'gc_volume_smooth', 'contIsPrinterAvailable': 'gc_printer_available', 'contPrinterGetStatus': 'gc_printer_status', 'contPrinterSendCommand': 'gc_printer_command', 'func_800A7460': 'gc_fade_poll', 'func_800A7860': 'gc_fade', 'func_800A7F68': 'gc_aux_camera', 'func_800A85E8': 'gc_process', 'func_800AA38C': 'gc_input', 'func_800AA85C': 'gc_repeat', 'func_800AA870': 'gc_input_mask', 'func_800AADF0': 'gc_scene_result', 'func_800AAE28': 'gc_input_initialize', 'func_800BF244_5C0E4': 'gc_save', 'func_800BF3D4_5C274': 'gc_report_present', 'func_800BF574_5C414': 'gc_temporary', 'func_800BF710_5C5B0': 'gc_report', 'func_800BF818_5C6B8': 'gc_report_score', 'func_800BF864_5C704': 'gc_report_count', 'func_80371C68_845418': 'gc_sprite_synchronous', 'func_80371D14_8454C4': 'gc_sprite_stackful', 'func_80371DC0_845570': 'gc_sibling', 'func_80371E68_845618': 'gc_child', 'func_80374714_847EC4': 'gc_preview', 'func_80374D20_8484D0': 'gc_replay_cleanup', 'func_80374D40_8484F0': 'gc_replay_initialize', 'func_8037501C_8487CC': 'gc_number', 'func_8037519C_84894C': 'gc_formatted', 'func_8036EB80_842330': 'gc_text_skip', 'getAlbumPhoto': 'gc_album', 'getPokemonName': 'gc_pokemon_name', 'gtlWaitAllGfxTasksDone': 'gc_graphics_join', 'initObjectsOnPhoto': 'gc_present', 'ohPauseObjectProcesses': 'gc_pause', 'ohWait': 'gc_wait', 'UIMem_Deallocate': 'gc_deallocate'}
CONSTANTS={'SCREEN_WIDTH': '320', 'SCREEN_HEIGHT': '240', 'A_BUTTON': '0x8000', 'B_BUTTON': '0x4000', 'START_BUTTON': '0x1000', 'Z_TRIG': '0x2000', 'POKEDEX_MAX': '151', 'PokemonID_BULBASAUR': '1', 'LINK_6': '6', 'LINK_17': '17', 'LINK_18': '18', 'DL_LINK_0': '0', 'SCENE_24': '24', 'SCENE_MAIN_MENU': '8', 'SCENE_GALLERY': '22', 'SP_HIDDEN': 'ISLE_SP_HIDDEN', 'PokemonID_1004': '1004', 'PokemonID_1010': '1010', 'PokemonID_1022': '1022', 'PokemonID_1018': '1018', 'PokemonID_1035': '1035', 'PokemonID_KOFFING_SMOKE': '1028', 'SOUND_ID_75': '75', 'SOUND_ID_74': '74', 'SOUND_ID_67': '67', 'SOUND_ID_66': '66', 'SOUND_ID_65': '65', 'SOUND_ID_69': '69', 'SOUND_ID_85': '85', 'SOUND_ID_94': '94', 'STICK_SLOW_UP': '0x10000', 'STICK_SLOW_DOWN': '0x20000', 'STICK_SLOW_RIGHT': '0x40000', 'STICK_SLOW_LEFT': '0x80000'}
ASSETS={'D_801E80E8_A05DA8': '0x801e80e8', 'D_801E82A0_A05F60': '0x801e82a0', 'D_801E83D0_A06090': '0x801e83d0', 'D_801E8890_A06550': '0x801e8890', 'D_801E8D50_A06A10': '0x801e8d50', 'D_801E8FB0_A06C70': '0x801e8fb0', 'D_801EA1B0_A07E70': '0x801ea1b0'}
GLOBALS=['D_801E16BC_9FF37C', 'D_801EA1F4_A07EB4', 'D_801EA1F8_A07EB8', 'D_801EA1FC_A07EBC', 'D_801EA200_A07EC0', 'D_801EA204_A07EC4', 'D_801EA288_A07F48', 'D_801EA28C_A07F4C', 'D_801EA290_A07F50', 'D_801EA294_A07F54', 'D_801EA298_A07F58', 'D_801EA29C_A07F5C', 'D_801EA2A0_A07F60', 'D_801EA2A4_A07F64', 'D_801EA2A8_A07F68', 'D_801EA2AC_A07F6C', 'D_801EA2B0_A07F70', 'D_801EA7F0_A084B0', 'D_801EA7F4_A084B4', 'D_801EA7F8_A084B8', 'D_801EA7FC_A084BC', 'D_801EA800_A084C0', 'D_80230810_A4E4D0', 'D_80230818_A4E4D8', 'D_80230848_A4E508', 'D_80230868_A4E528', 'D_8023086C_A4E52C', 'D_80230870_A4E530', 'D_80230874_A4E534', 'D_80230878_A4E538', 'D_8023087C_A4E53C', 'D_80230880_A4E540', 'D_80230884_A4E544', 'D_80230888_A4E548', 'D_8023088C_A4E54C', 'D_80230890_A4E550', 'D_802308A0_A4E560', 'D_802308A4_A4E564', 'D_802308A8_A4E568', 'D_802308AC_A4E56C', 'D_802308B0_A4E570', 'D_802308B4_A4E574', 'D_802308B8_A4E578', 'D_802308C0_A4E580', 'D_802309B0_A4E670', 'D_802309C0_A4E680', 'D_80230AD4_A4E794', 'D_80230AD8_A4E798', 'D_80230ADC_A4E79C', 'D_80230AE0_A4E7A0']
CALLBACKS={'func_801DCF50_9FAC10', 'func_801DF078_9FCD38', 'func_801DCA58_9FA718'}
def rewrite(original,name,names):
    changes=[]
    def change(old,new,count=1):
        nonlocal original
        if original.count(old)!=count: raise ValueError((name,old,original.count(old),count))
        original=original.replace(old,new); changes.append({'old':old,'new':new,'count':count})
    if name=='func_801DCEBC_9FAB7C':
        return None,None,[{'native-entry':'isle_gallery_source_entry','original':'argument==22 route; setup; return actual D800AF3C0 after retirement'}]
    if name in {'func_801DC8C0_9FA580','func_801DC948_9FA608','func_801DCA98_9FA758','func_801DCC60_9FA920'}:
        replacements={
          'func_801DC8C0_9FA580':('void isle_gallery_func_801DC8C0_9FA580(Context* source,void** dlist)','gc_check(source,source->native->display_list(source,dlist));'),
          'func_801DC948_9FA608':('void isle_gallery_func_801DC948_9FA608(Context* source,int32_t arg0,int32_t arg1)','gc_check(source,source->native->render_state(source,arg0,arg1));'),
          'func_801DCA98_9FA758':('void isle_gallery_func_801DCA98_9FA758(Context* source)','gc_camera(source,0);'),
          'func_801DCC60_9FA920':('void isle_gallery_func_801DCC60_9FA920(Context* source)','gc_camera(source,1);')}
        sig,body=replacements[name]
        return sig+' { '+body+' }\n',sig+';',[{'mechanical-sdk-body':name,'native':body}]
    if name=='func_801DCDDC_9FAA9C':
        change('D_801E1690_9FF350.zBuffer = gtlMalloc(0x25800, 0x40);\n    viApplyScreenSettings(&D_801E1690_9FF350);','gc_check(source,source->native->screen(source,0x25800,0x40));')
        change('UIMem_CreateHeap(&D_801EA808_A084C8, 0x46000);','gc_check(source,source->native->ui_heap(source,0x46000));')
    if name in {'func_801DD28C_9FAF4C','func_801DD4C4_9FB184','func_801DE0B8_9FBD78'}:
        for decl in ['    Bitmap* bitmap;\n','    u8* dest;\n','    u8* src;\n']: change(decl,'')
        start=original.index('    bitmap = (Bitmap*) UIMem_Allocate(0x10);')
        end=original.index('\n    return gobj;',start)
        change(original[start:end],'    gc_thumbnail(source,sobj);\n')
        if name=='func_801DE0B8_9FBD78': change('    s32 i;\n','')
    if name=='func_801DE318_9FBFD8':
        for n in ['D_801E8FB0_A06C70','D_801E8890_A06550','D_801E8D50_A06A10']:
            change('D_8023086C_A4E52C->sprite = '+n+';','gc_rebind(source,D_8023086C_A4E52C,'+ASSETS[n]+');')
        change('D_80230868_A4E528->data.sobj->unk_58 = NULL;\n    D_80230868_A4E528->data.sobj->unk_5C = NULL;','gc_clear_matrices(source,gc_root(source,D_80230868_A4E528));')
    if name=='func_801DE04C_9FBD0C':
        change('SET_SPRITE_POS(sobj->sprite, arg0, arg1);','gc_sprite(source,sobj)->x=arg0; gc_sprite(source,sobj)->y=arg1;')
    if name=='func_801DE87C_9FC53C': change('cam = sp4C->data.cam;','cam = sp4C;')
    if name=='func_801E1384_9FF044': change('(PhotoData*) func_800BF574_5C414(i)','func_800BF574_5C414(i)')
    if name=='func_801E1504_9FF1C4':
        change('    PhotoData sp18[ARRAY_COUNT(D_802309B0_A4E670)];\n','')
        change('sp18[spE9C] = *D_802309B0_A4E670[spE9C];','gc_snapshot(source,spE9C,D_802309B0_A4E670[spE9C]);')
        change('func_800BF5D8_5C478(spE9C, &sp18[spE9C]);','gc_write_snapshot(source,spE9C);')
    # Actual callback ABIs retain object-only signatures. Direct C functions get
    # explicit stable typed source, including identity-compared functions.
    def transform(code):
        # Original previews take Sprite*, but native transport uses exact SObj.
        code=re.sub(r'&((?:D_\w+\[[^\]]+\]\.sobj|D_\w+))->sprite',r'\1',code)
        code=re.sub(r'((?:D_\w+\[[^\]]+\]\.gobj|D_\w+|gobj|sp28|obj))->data\.sobj->sprite',r'(*gc_sprite(source,gc_root(source,\1)))',code)
        code=re.sub(r'((?:D_\w+\[[^\]]+\]\.sobj|D_\w+|sobj))->sprite',r'(*gc_sprite(source,\1))',code)
        code=re.sub(r'((?:D_\w+|gobj|sp28|obj))->data\.sobj',r'gc_root(source,\1)',code)
        code=re.sub(r'([A-Za-z_]\w*)->flags (\|=|&=) (~?)GOBJ_FLAG_HIDDEN;',lambda m:'gc_hidden(source,'+m[1]+','+('1' if m[2]=='|=' else '0')+');',code)
        code=re.sub(r'omGObjListHead\[([^\]]+)\]',r'gc_first(source,\1)',code)
        code=re.sub(r'\b(sp1C|sp28)->next',r'gc_next(source,\1)',code)
        code=code.replace('sp28->id','gc_id(source,sp28)')
        code=re.sub(r'D_802308A0_A4E560\[([^\]]+)\]\.id',r'gc_slot_id(source,\1)',code)
        for old,new in sorted(ASSETS.items(),key=lambda kv:-len(kv[0])):
            code=code.replace('&'+old,new)
        for old,new in BRIDGE.items():
            code=re.sub(r'\b'+old+r'\s*\(\s*',new+'(source, ',code)
            code=code.replace(new+'(source, )',new+'(source)')
        for old in sorted(names,key=len,reverse=True):
            pref='isle_gallery_'+old
            if old in CALLBACKS: code=re.sub(r'\b'+old+r'\b',pref,code)
            else:
                code=re.sub(r'\b'+old+r'\s*\(\s*',pref+'(source, ',code)
                code=code.replace(pref+'(source, )',pref+'(source)')
                code=re.sub(r'\b'+old+r'\b',pref,code)
        for old in GLOBALS: code=re.sub(r'\b'+old+r'\b','source->'+old,code)
        for old in ['D_801EA210_A07ED0','D_801EA248_A07F08']: code=re.sub(r'\b'+old+r'\b','source->initial->'+old,code)
        for old,new in CONSTANTS.items(): code=re.sub(r'\b'+old+r'\b',new,code)
        for old,new in {'s32':'int32_t','u32':'uint32_t','s16':'int16_t','u16':'uint16_t','s8':'int8_t','u8':'uint8_t','f32':'float'}.items(): code=re.sub(r'\b'+old+r'\b',new,code)
        return code
    # Isolate header so implicit source argument insertion cannot affect it.
    head,body=original.split('{',1)
    if name in CALLBACKS:
        signature=code_only(head,lambda c: c.replace(name,'isle_gallery_'+name)).strip()
        body='\n    Context* source=isle_gallery_source_require(arg0,isle_gallery_'+name+');'+code_only(body,transform)
    else:
        head=head.replace(name+'(', 'isle_gallery_'+name+'(Context* source, ')
        head=head.replace('Context* source, void)', 'Context* source)')
        signature=code_only(head,lambda c: re.sub(r'\b(s32|u32)\b',lambda m: {'s32':'int32_t','u32':'uint32_t'}[m[0]],c)).strip()
        body=code_only(body,transform)
    result=encode_literals(signature+' {'+body)
    return result,signature+';',changes

SELECTOR_SHA256='c0b584853c3e4ba320b55ac04a14fcd9cfd48112c9e5373a74b7516b12a4d761'
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-repo',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--manifest',type=Path,default=Path(__file__).resolve().parents[1]/'cmake/gallery-inputs.json')
    parser.add_argument('--manifest-sha256',required=True)
    parser.add_argument('--project-root',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args()
    if args.output.name!='game.c' or args.output.parent.name!='isle-gallery-private': raise ValueError('require <build>/_deps/isle-gallery-private/game.c')
    if args.output.parent.parent.name!='_deps': raise ValueError('require private _deps output')
    manifest_bytes=args.manifest.read_bytes(); manifest=json.loads(manifest_bytes)
    if not re.fullmatch(r'[0-9a-f]{64}',args.manifest_sha256) or hashlib.sha256(manifest_bytes).hexdigest()!=args.manifest_sha256:
        raise ValueError('Gallery externally admitted recipe fingerprint')
    if manifest['revision']!=REVISION or manifest['provider']!='gallery-22-23-original-c': raise ValueError('unadmitted Gallery provider')
    selection=manifest['definitions']
    if len(selection)!=61 or digest(json.dumps(selection,sort_keys=True,separators=(',',':')))!=SELECTOR_SHA256: raise ValueError('unadmitted selector closure')
    if len(set(r['name'] for r in selection))!=len(selection): raise ValueError('duplicate Source selectors')
    required_support={
        'include/35mmisle/continuation.h',
        'include/35mmisle/engine.h',
        'include/35mmisle/process.hpp',
        'src/application_photos.cpp',
        'src/application_photos.hpp',
        'src/application_photos_source.h',
        'src/application_runtime.cpp',
        'src/application_runtime.hpp',
        'src/continuation.cpp',
        'src/engine.c',
        'src/engine_internal.h',
        'src/gallery_scene_native.cpp',
        'src/gallery_scene_native.hpp',
        'src/gallery_source.c',
        'src/gallery_source.h',
        'src/game_state.hpp',
        'src/photo_presentation.cpp',
        'src/photo_presentation.h',
        'src/photo_presentation.hpp',
        'src/photo_presentation_core.inc',
        'src/photo_replay.cpp',
        'src/photo_replay.hpp',
        'src/post_course_native_helpers.hpp',
        'src/post_course_replay.cpp',
        'src/post_course_replay.hpp',
        'src/post_course_replay_source.c',
        'src/post_course_replay_source.h',
        'src/post_course_resources.cpp',
        'src/post_course_resources.hpp',
        'src/post_course_scene.cpp',
        'src/post_course_scene.hpp',
        'src/post_course_scene_hooks.cpp',
        'src/post_course_scene_internal.hpp',
        'src/post_course_scene_render.cpp',
        'src/post_course_scene_source.c',
        'src/post_course_scene_source.h',
        'src/post_course_window.cpp',
        'src/post_course_window.hpp',
        'src/post_course_window_source.c',
        'src/post_course_window_source.h',
        'src/process.cpp',
        'src/scene_graphics.cpp',
        'src/scene_graphics.hpp',
        'src/scene_services.cpp',
        'src/scene_services.hpp',
        'src/screen_transition_resources.cpp',
        'src/screen_transition_resources.hpp',
        'src/window_memory.cpp',
        'src/window_memory.hpp',
        'tools/prepare_gallery.py',
    }
    if set(manifest['native_support'])!=required_support: raise ValueError('native support closure')
    for path,identity in manifest['native_support'].items():
        raw=(args.project_root/path).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=identity['sha256']: raise ValueError(('native support digest',path))
    texts={}
    for path,identity in manifest['source_files'].items():
        raw=git(args.source_repo,'show',REVISION+':'+path)
        if hashlib.sha256(raw).hexdigest()!=identity['sha256']: raise ValueError(('source digest',path))
        if hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()!=identity['blob']: raise ValueError(('source blob',path))
        texts[path]=raw.decode()
    # Source files are selected by immutable blobs, never checkout bytes/HEAD.
    names={r['name'] for r in selection}; definitions=[]; declarations=[]; records=[]; by_unit={}
    for record in selection:
        text=texts[record['source']]
        original=''.join(text.splitlines(keepends=True)[record['first']-1:record['last']])
        if digest(original)!=record['sha256']: raise ValueError(('selected Source body',record['name']))
        matches=list(re.finditer(r'(?m)^'+re.escape(record['signature'])+r'[ \t]*\{',text))
        if len(matches)!=1 or text.count('\n',0,matches[0].start())+1!=record['first']:
            raise ValueError(('nonunique Source definition/signature/span',record['name']))
        native,declaration,rewrites=rewrite(original,record['name'],names)
        if native is not None:
            definitions.append(native); declarations.append(declaration)
            by_unit.setdefault(record['source'],[]).append(native)
        records.append({**record,'native_sha256':digest(native) if native is not None else None,'rewrites':rewrites})
    body='\n'.join(definitions); decl='\n'.join(declarations)
    bridge=(args.project_root/'src/gallery_source.c').read_text()
    if bridge.count('#include "gallery_original.inc"')!=1 or bridge.count('#include "gallery_declarations.inc"')!=1: raise ValueError('bridge include closure')
    game=bridge.replace('#include "gallery_original.inc"',body).replace('#include "gallery_declarations.inc"',decl)
    units={path:{'source_sha256':manifest['source_files'][path]['sha256'],'native_sha256':digest('\n'.join(parts))} for path,parts in by_unit.items()}
    outputs={'game.c':digest(game),'gallery_original.inc':digest(body),'gallery_declarations.inc':digest(decl)}
    if records!=manifest['native_definitions'] or units!=manifest['native_units']:
        raise ValueError('Gallery native definition/unit/rewrite fingerprint')
    if outputs!=manifest['native_outputs']:
        raise ValueError('Gallery whole-output fingerprint')
    # Every per-function/per-unit/output hash is bound to immutable Source,
    # selector digest, recipe bytes and the checked generator/native support.
    evidence={'schema':1,'provider':manifest['provider'],'revision':REVISION,
       'source_files':manifest['source_files'],'native_support':manifest['native_support'],
       'selector_sha256':SELECTOR_SHA256,'recipe_sha256':hashlib.sha256(manifest_bytes).hexdigest(),
       'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
       'definitions':records,'candidate_count':60,'support_definition_count':1,
       'units':units,
       'outputs':outputs,
       'data_origins':manifest['data_origins'],'native_contract':manifest['native_contract'],
       'portability_origins':manifest['portability_origins']}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    publish(args.output,game)
    publish(args.output.with_name('gallery_original.inc'),body)
    publish(args.output.with_name('gallery_declarations.inc'),decl)
    publish(args.output.with_name('evidence.json'),json.dumps(evidence,indent=2)+'\n')
if __name__=='__main__': main()
