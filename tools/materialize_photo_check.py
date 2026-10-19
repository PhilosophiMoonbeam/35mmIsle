#!/usr/bin/env python3
"""Materialize the admitted original PhotoCheck15 C closure into private build storage.

Public files contain selectors/semantic bridge only; no SDK bodies or GAME payloads.
The three intentional assessment/handle corrections are explicit counted rewrites,
not claims that the original ROM supplied an initialized return value.
"""
import argparse
import ast
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
PRIMARY_SOURCES = {'src/photo_check/98C330.c','src/photo_check/993C50.c',
                   'src/photo_check/993F80.c'}
SUPPORT_DEFINITIONS = {'src/window/8486E0.c':{'func_80374F30_8486E0'}}
BRIDGE = {
    'qsort2':'pc_qsort',
    'FocusMark_Create':'pc_focus_create','FocusMark_MoveFront':'pc_focus_front',
    'FocusMark_SetPos':'pc_focus_position','FocusMark_SetSize':'pc_focus_size',
    'FocusMark_SetTargetPos':'pc_focus_target','FocusMark_Show':'pc_focus_show',
    'UIButtonImage_DeleteAll':'pc_buttons_clear','UIElement_Create':'pc_element_create',
    'UIElement_Delete':'pc_element_delete','UIElement_Draw':'pc_element_draw',
    'UIElement_DrawBackground':'pc_element_background','UIElement_FillRect':'pc_element_fill',
    'UIElement_PrintAsciiString':'pc_print_ascii','UIElement_PrintText':'pc_print_source',
    'UIElement_SetColor':'pc_element_color','UIElement_SetFont':'pc_element_font',
    'UIElement_SetState':'pc_element_state','UIElement_SetTextPos':'pc_element_position',
    'UIElement_VertScroll':'pc_element_scroll','UIFrame_Create':'pc_frame_create',
    'UIFrame_GetBackgroundColor':'pc_frame_color','UIFrame_SetOpacity':'pc_frame_opacity',
    'UIFrame_SetPos':'pc_frame_position','UIFrame_SetSize':'pc_frame_size',
    'UIFrame_SetStyle':'pc_frame_style','UIFrame_Show':'pc_frame_show',
    'UIFrame_ShowBackground':'pc_frame_background','UILayout_GetButtons':'pc_selector',
    'UILayout_HideButtons':'pc_layout_hide','UILayout_HideHeaderElement':'pc_layout_hide_header',
    'UILayout_Init':'pc_layout_initialize','UILayout_SetHeaderFlags':'pc_layout_header',
    'UILayout_ShowPanel':'pc_layout_panel','UILayout_WaitPanelTransitionComplete':'pc_layout_wait',
    'UIText_GetStringWidth':'pc_width','UIText_Initialize':'pc_text_initialize',
    'UIText_SetPrintDelay':'pc_text_delay','UIText_SetShadowOffset':'pc_text_shadow',
    'UIText_SetSpacing':'pc_text_spacing','auPlaySong':'pc_song','auPlaySound':'pc_sound',
    'auPlaySoundWithParams':'pc_sound_params','auSetBGMVolume':'pc_volume',
    'auSetBGMVolumeSmooth':'pc_volume_smooth','checkPlayerFlag':'pc_flag',
    'func_800A7F68':'pc_aux_camera','func_800A85E8':'pc_process','func_800AA38C':'pc_input',
    'func_800AA85C':'pc_repeat','func_800AA870':'pc_input_mask','func_800AADF0':'pc_scene_result',
    'func_800AAE28':'pc_input_initialize','func_800AAED0':'pc_notices_or',
    'func_800AAEE8':'pc_notices_test','func_800BF3D4_5C274':'pc_report_present',
    'func_800BF574_5C414':'pc_temporary','func_800BF5D8_5C478':'pc_set_temporary',
    'func_800BF690_5C530':'pc_set_report','func_800BF710_5C5B0':'pc_report',
    'func_800BF7D4_5C674':'pc_set_report_score','func_800BF818_5C6B8':'pc_report_score',
    'func_800BF954_5C7F4':'pc_set_album','func_800BFB50_5C9F0':'pc_selected',
    'func_800BFB84_5CA24':'pc_selected_count','func_800BFC5C_5CAFC':'pc_selected_course',
    'func_800C0290_5D130':'pc_unlock_progress','func_800C02A0_5D140':'pc_unlock_set',
    'func_800C0314_5D1B4':'pc_pending_item','func_8036EB80_842330':'pc_text_skip',
    'func_8036EB8C_84233C':'pc_character_register',
    'func_80371C68_845418':'pc_sprite_synchronous',
    'func_80371D14_8454C4':'pc_sprite_stackful',
    'func_80371DC0_845570':'pc_sibling','func_80371E68_845618':'pc_child',
    'func_8037452C_847CDC':'pc_score','func_80374714_847EC4':'pc_preview',
    'func_80374D20_8484D0':'pc_replay_cleanup','func_80374D40_8484F0':'pc_replay_initialize',
    'func_8037501C_8487CC':'pc_number','func_8037519C_84894C':'pc_formatted',
    'getAlbumCapacity':'pc_album_capacity','getAlbumPhoto':'pc_album',
    'getLevelId':'pc_level_id','getLevelName':'pc_level_name',
    'getPlayerName':'pc_player_name','getPokemonName':'pc_pokemon_name',
    'ohWait':'pc_wait','omDeleteGObj':'pc_delete',
}
CALLBACKS = {'func_801DCA14_98C484', 'func_801DDCF8_98D768',
             'func_801DE02C_98DA9C', 'func_801E39DC_99344C'}
TYPES = {'s32':'int32_t','u32':'uint32_t','s16':'int16_t','u16':'uint16_t',
         's8':'int8_t','u8':'uint8_t','f32':'float'}
READ_ONLY = {'SpecialPokemonNames':'pc_special_names', 'PhotoComments':'pc_photo_comments',
             'D_801F3E3C_9A38AC':'pc_pitch', 'D_801F4150_9A3BC0':'pc_button_table'}
GLOBALS = ['D_801F3E28_9A3898', 'D_801F3E2C_9A389C', 'D_801F3E30_9A38A0', 'D_801F3E34_9A38A4', 'D_801F3E38_9A38A8', 'D_801F3E60_9A38D0', 'D_801F3E64_9A38D4', 'D_801F4144_9A3BB4', 'D_801F4148_9A3BB8', 'D_801F4180_9A3BF0', 'D_801F4194_9A3C04', 'D_801F4198_9A3C08', 'D_802290A0_9D8B10', 'D_802290D8_9D8B48', 'D_802290DC_9D8B4C', 'D_802290E0_9D8B50', 'D_802290E4_9D8B54', 'D_802290E8_9D8B58', 'D_80229168_9D8BD8', 'D_80229170_9D8BE0', 'D_80229180_9D8BF0', 'D_80229184_9D8BF4', 'D_80229188_9D8BF8', 'D_8022918C_9D8BFC', 'D_80229190_9D8C00', 'D_80229194_9D8C04', 'D_802291A0_9D8C10', 'D_80229838_9D92A8', 'D_80229840', 'D_80369F80']
CONSTANTS = {
    'SCREEN_WIDTH':'320','SCREEN_HEIGHT':'240','POKEDEX_MAX':'151',
    'A_BUTTON':'0x8000','B_BUTTON':'0x4000','START_BUTTON':'0x1000',
    'STICK_SLOW_RIGHT':'0x40000','STICK_SLOW_LEFT':'0x80000',
    'LINK_6':'6','DL_LINK_0':'0','SCENE_OAKS_LAB_2':'13',
    'PFID_HAS_APPLE':'0','PFID_HAS_PESTER_BALL':'1','PFID_HAS_FLUTE':'2',
    'PFID_HAS_DASH_ENGINE':'5','PFID_14':'14',
    'SP_HIDDEN':'ISLE_SP_HIDDEN','SP_TRANSPARENT':'ISLE_SP_TRANSPARENT',
    'PokemonID_MEW':'151','PokemonID_KOFFING_SMOKE':'1028',
    'PokemonID_1004':'1004','PokemonID_1010':'1010','PokemonID_1018':'1018',
    'PokemonID_1022':'1022','PokemonID_1035':'1035',
    'SOUND_ID_69':'69','SOUND_ID_85':'85','SOUND_ID_94':'94',
}
ASSETS = {'D_801E5DB0_995820':'0x801e5db0','D_801E5EE0_995950':'0x801e5ee0',
          'D_801F2378_9A1DE8':'0x801f2378','D_801F26B0_9A2120':'0x801f26b0',
          'D_801F2910_9A2380':'0x801f2910','D_801F29F0_9A2460':'0x801f29f0',
          'D_801F2AD0_9A2540':'0x801f2ad0','D_801F3B10_9A3580':'0x801f3b10'}
TOKENS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/',re.S)

def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()

def code_only(text, transform):
    output=[]; position=0
    for match in TOKENS.finditer(text):
        output.append(transform(text[position:match.start()]))
        output.append(match[0]); position=match.end()
    output.append(transform(text[position:]))
    return ''.join(output)

def encode_literals(text):
    def literal(match):
        token=match[0]
        if token.startswith('"'):
            value=ast.literal_eval(token).encode('euc_jp')
            return '"'+''.join('\\x%02x'%byte for byte in value)+'"'
        if token.startswith("'"):
            value=ast.literal_eval(token).encode('euc_jp')
            if len(value)>1:
                return str(int.from_bytes(value,'big'))
        return token
    return TOKENS.sub(literal,text)

def replace_exact(body, old, new, count, records, label):
    actual=body.count(old)
    if actual!=count: raise ValueError((label,old,actual,count))
    records.append({'kind':'exact','label':label,'find':old,'replace':new,'count':count})
    return body.replace(old,new)

def rewrite_definition(original, name, names):
    records=[]
    semantic_counts={}
    def sub(pattern, replacement, code, label):
        result,count=re.subn(pattern,replacement,code)
        if count: semantic_counts[label]=semantic_counts.get(label,0)+count
        return result
    def change(old,new,count=1,label=None):
        nonlocal original
        original=replace_exact(original,old,new,count,records,label or name)
    if name=='func_801DCC74_98C6E4':
        # Entry has different native signature: typed independent continuation
        # backing, actual Source result, no context access after scene retirement.
        return None,None,[{'kind':'native-entry','implementation':'isle_photo_check_func_801DCC74_98C6E4','source':'src/photo_check_source.c','operations':['viApplyScreenSettings(801e5410)','arena_extent=80369f80-80229840','gtlDisableNearClipping(1)','omSetupScene(801e542c)','return actual D800AF3C0 after retirement']}]
    if name=='func_801DCA48_98C4B8':
        start=original.index('{')
        records.append({'kind':'native-camera','span':'98C330.c355-377','contract':'isle_photo_check_native_camera; C registers exact idle kind0/priority0 after return'})
        original=original[:start]+'{\n    pc_camera_create(source);\n}\n'
    if name=='func_801DCBF4_98C664':
        change('UIMem_CreateHeap(D_801F70A0_9A6B10, sizeof(D_801F70A0_9A6B10));',
               'pc_check(source, isle_photo_check_native_ui_heap(source, 0x32000));')
    if name=='func_801DDCF8_98D768':
        change('    s32 sp2C;\n','')
        change('    sp2C = (s32) arg0->userData;\n','',
               label='remove-dead-unused-pointer-to-int-read')
    if name=='func_801DD720_98D190':
        change('    Bitmap* sp24;\n    u8* sp20;\n    u8* sp1C;\n','')
        first=original.index('    sp24 = UIMem_Allocate(0x30);')
        last=original.index('\n    return gobj;',first)
        change(original[first:last], '    pc_thumbnail(source, idx, sobj);\n')
    if name=='func_801DE204_98DC74':
        change('    sp210 += sp1FA;', '    if (!isPokemonSign) sp210 += sp1FA;', label='intentional-dead-sign-arithmetic-correction')
        # The guarded original new/poor branch retains its dialogue and clearing.
        # Both poor completion paths must write zero, including unskipped dialogue.
        change('            sp1EC = 0;\n            UIElement_Draw(D_802290E4_9D8B54);',
               '            UIElement_Draw(D_802290E4_9D8B54);',label='move-poor-return-outside-skip-guard')
        change('        if (sp204 == 0) {\n            if (!isPokemonSign)',
               '        if (sp204 != 0) sp1EC = 0; /* INTENTIONAL original missing-assignment correction. */\n        if (sp204 == 0) {\n            if (!isPokemonSign)',label='intentional-new-poor-return-zero-on-both-paths')
        change('if (sp3C != NULL)', 'if (sp3C != 0)')
    if name=='func_801E2454_991EC4':
        change('            if (sp54 != 0) {\n                func_801E1FA8_991A18(photo, 0, sp54);\n            } else {\n                func_801E242C_991E9C(photo->unk_0, 0, 0);\n            }',
               '            func_801E1FA8_991A18(photo, 0, sp54); /* sp54=-26; PANIC branch unreachable. */',label='proven-entry-PANIC-exclusion')
        change('    char* sp48;', '    const char* sp48;')
    if name=='func_801E2ED4_992944':
        # Observe only actual calls at the two pinned summary boundaries.
        # Typed C wrappers preserve the original helper's button return/waits.
        change('    func_80374F30_8486E0(D_802290DC_9D8B4C, true);\n    auPlaySound(0x4E);',
               '    pc_ack_at(source, D_802290DC_9D8B4C, true, 2197);\n    auPlaySound(0x4E);',
               label='native-save-ack-callsite-observation')
        change('    func_80374F30_8486E0(D_802290DC_9D8B4C, true);\n}',
               '    pc_ack_at(source, D_802290DC_9D8B4C, true, 2302);\n}',
               label='native-final-ack-callsite-observation')
    if name=='func_801E39DC_99344C':
        change('        sp48 = func_801E4428_993E98();',
               '        sp48 = func_801E4428_993E98();\n        sp3C = D_802290E0_9D8B50; /* Summary replaced the evaluation element. */',label='intentional-current-evaluation-handle-correction')
        change('    char* levelName;', '    const char* levelName;')
    if name=='func_801E460C_99407C':
        change('char* arg0, char* arg1','const char* arg0, const char* arg1')
    if name=='func_801E41E0_993C50':
        change('void* func_801E41E0_993C50', 'const void* func_801E41E0_993C50')
    if name in {'func_801E4E04_994874','func_801E5030_994AA0'}:
        change('    Foo2* sp24;', '    const IsleSelectedPhoto* sp24;')
        for old,new in [('sp24->var_0','sp24->photo'),('sp24->unk4_31','sp24->for_oak'),
                        ('sp24->unk4_30','sp24->source_second_flag'),('sp24->unk4_29','sp24->for_album')]:
            count=original.count(old)
            if count: change(old,new,count)
    # Use exact source byte counts and only rewrite code, never text/comment content.
    original=code_only(original,lambda s:sub(r'\bUNUSED\s+[^;]+;','',s,'remove-matching-only-padding'))
    original=code_only(original,lambda s:sub(r'\b(\w+)->userData\s*=\s*([^;]+);',r'pc_userdata_set(source, \1, \2);',s,'actual-userdata-set'))
    # The preview API takes the actual thumbnail child, not an address into a view.
    original=code_only(original,lambda s:sub(r'&(D_802290A0_9D8B10\[[^]\n]+\]\.sobj)->sprite',r'\1',s,'thumbnail-child-not-sprite-pointer'))
    expression=r'[A-Za-z_]\w*(?:(?:\[[^]\n]+\])|(?:\.\w+))*'
    original=code_only(original,lambda s:sub(r'('+expression+r')->data\.sobj->sprite',r'(*pc_sprite(source, pc_root(source, \1)))',s,'actual-root-sprite-view'))
    original=code_only(original,lambda s:sub(r'('+expression+r')->data\.sobj',r'pc_root(source, \1)',s,'actual-root-handle'))
    original=code_only(original,lambda s:sub(r'('+expression+r')->sprite',r'(*pc_sprite(source, \1))',s,'actual-sprite-view'))
    for asset,address in ASSETS.items():
        original=code_only(original,lambda s,a=asset,v=address:sub(r'&'+a+r'\b',v,s,'prototype:'+a))
    original=code_only(original,lambda s:sub('&D_801F4150_9A3BC0','D_801F4150_9A3BC0',s,'typed-button-table'))
    head,inner=original.split('{',1)
    ret,args=head.split(name+'(',1)
    args=args.rsplit(')',1)[0].strip()
    public=name in CALLBACKS
    signature=ret.strip()+' isle_photo_check_'+name+'('
    signature+=args if public else 'Context* source'+(', '+args if args not in ('','void') else '')
    signature+=')'
    if not public: signature='static '+signature
    if public:
        inner='\n    Context* source = (Context*)isle_post_course_source_require(arg0, 15);'+inner
    def tokens(code):
        for original_name in names:
            code=sub(r'\b'+original_name+r'\b','isle_photo_check_'+original_name,code,'function:'+original_name)
        for original_name,native in BRIDGE.items():
            code=sub(r'\b'+original_name+r'\b',native,code,'bridge:'+original_name)
        for variable in GLOBALS:
            code=sub(r'\b'+variable+r'\b','source->'+variable,code,'context-field:'+variable)
        for original_name,native in {**TYPES,**CONSTANTS,**READ_ONLY}.items():
            code=sub(r'\b'+original_name+r'\b',native,code,'semantic-token:'+original_name)
        # Explicit context on direct original helper/bridge calls, never OM callbacks.
        calls=[BRIDGE[n] for n in BRIDGE]+['isle_photo_check_'+n for n in names if n not in CALLBACKS]
        for function in calls:
            code=sub(r'\b'+function+r'\(\s*(?!Context\b)',function+'(source, ',code,'context-call:'+function)
            code=sub(re.escape(function+'(source, )'),function+'(source)',code,'empty-call:'+function)
        for function in ['source->D_801F4198_9A3C08']:
            code=sub(re.escape(function+'('),function+'(source, ',code,'context-comparator:'+function)
        return code
    signature=code_only(signature,tokens)
    # The signature has already acquired context; do not insert it twice.
    signature=signature.replace('(source, Context* source','(Context* source')
    inner=code_only(inner,tokens)
    result=encode_literals(signature+' {'+inner)
    declaration=encode_literals(signature)+';\n'
    records.append({'kind':'semantic-native','counts':semantic_counts,'operations':['typed context and globals','native signed-bit working records','actual Window handles/sprite views','canonical Photos getters/mutations','typed returning owner hooks','original callback identities','source-byte strings'],'native_sha256':digest(result)})
    return result,declaration,records

def initializer(text, name):
    match=re.search(r'\b'+name+r'\[\]\s*=\s*\{(.*?)\};',text,re.S)
    if not match: raise ValueError(('missing initialized data',name))
    return match[1]

def data_materialize(texts):
    main=texts['src/photo_check/98C330.c']
    authority=texts['src/photo_check/98D0F0.c']
    token=re.compile(r'"(?:\\.|[^"\\])*"|NULL|0x00000000|(?<![A-Za-z_])0(?![A-Za-z_0-9])')
    special=token.findall(initializer(main,'SpecialPokemonNames'))
    comments=token.findall(initializer(main,'PhotoComments'))
    authoritative=token.findall(initializer(authority,'D_801F3E68_9A38D8'))
    # The combined authority begins two padding words, then the NULL special name.
    normalized=lambda values:[None if v in ('NULL','0','0x00000000') else ast.literal_eval(v) for v in values]
    if len(special)!=13 or len(comments)!=168: raise ValueError('photo string table extent')
    if normalized(authoritative[2:15])!=normalized(special): raise ValueError('special-name authority mismatch')
    if normalized(authoritative[15:])!=normalized(comments): raise ValueError('comment authority mismatch')
    pitch=initializer(main,'D_801F3E3C_9A38AC')
    if len(pitch.split(','))-1!=9: raise ValueError('pitch extent')
    buttons=[int(v,16) for v in re.findall(r'0x[0-9A-Fa-f]+',initializer(authority,'D_801F4150_9A3BC0'))]
    if buttons!=[34,0,17,0,35,0,34,0,5,0,35,0]: raise ValueError('button-table words')
    output='static const char* const pc_special_names[13] = {'+','.join('NULL' if v is None else json.dumps(v,ensure_ascii=False) for v in normalized(special))+'};\n'
    output+='static const char* const pc_photo_comments[168] = {'+','.join('NULL' if v is None else json.dumps(v,ensure_ascii=False) for v in normalized(comments))+'};\n'
    output+='static const float pc_pitch[9] = {'+pitch+'};\n'
    output+='static const IslePostCourseButton pc_button_table[6] = {'+','.join('{%d,{NULL,0}}'%buttons[i] for i in range(0,12,2))+'};\n'
    return encode_literals(output)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-repo',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--manifest',type=Path,default=Path(__file__).resolve().parents[1]/'cmake/photo-check-inputs.json')
    parser.add_argument('--manifest-sha256',required=True)
    args=parser.parse_args()
    manifest_raw=args.manifest.read_bytes()
    if not re.fullmatch(r'[0-9a-f]{64}',args.manifest_sha256) or hashlib.sha256(manifest_raw).hexdigest()!=args.manifest_sha256:
        raise ValueError('PhotoCheck externally admitted recipe fingerprint')
    manifest=json.loads(manifest_raw)
    root=Path(__file__).resolve().parents[1]
    if not manifest['native_support']: raise ValueError('missing PhotoCheck native support guards')
    for path,identity in manifest['native_support'].items():
        if hashlib.sha256((root/path).read_bytes()).hexdigest()!=identity['sha256']:
            raise ValueError(('PhotoCheck native support digest',path))
    if manifest['revision']!=REVISION: raise ValueError('unadmitted GAME revision')
    texts={}
    for path,identity in manifest['source_files'].items():
        raw=git(args.source_repo,'show',REVISION+':'+path)
        if hashlib.sha256(raw).hexdigest()!=identity['sha256']: raise ValueError(('source digest',path))
        if hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()!=identity['blob']: raise ValueError(('source blob',path))
        texts[path]=raw.decode()
    names=sorted(record['name'] for record in manifest['definitions'])
    definitions=[]; declarations=[]; records=[]
    for selected in manifest['definitions']:
        if selected['source'] not in PRIMARY_SOURCES:
            if selected['name'] not in SUPPORT_DEFINITIONS.get(selected['source'],set()):
                raise ValueError(('unadmitted support definition',selected['source'],selected['name']))
        original=''.join(texts[selected['source']].splitlines(keepends=True)[selected['first']-1:selected['last']])
        if digest(original)!=selected['sha256']: raise ValueError(('selected body',selected['name']))
        native,declaration,rewrites=rewrite_definition(original,selected['name'],names)
        if native is not None: definitions.append(native); declarations.append(declaration)
        records.append({**selected,'rewrites':rewrites,'native_sha256':digest(native) if native else None})
    body='\n'.join(definitions)
    decl=''.join(declarations)
    data=data_materialize(texts)
    # UI numeric constants are pinned separately in the manifest, not SDK bodies.
    decl=manifest['window_enum_declarations']+'\n'+decl
    bridge=(root/'src/photo_check_source.c').read_text()
    game=bridge
    for include,content in [('photo_check_original.inc',body),('photo_check_declarations.inc',decl),('photo_check_data.inc',data)]:
        marker='#include "'+include+'"'
        if game.count(marker)!=1: raise ValueError(('PhotoCheck bridge include closure',include))
        game=game.replace(marker,content)
    outputs={'body_sha256':digest(body),'declarations_sha256':digest(decl),'data_sha256':digest(data),'game_sha256':digest(game)}
    if records!=manifest['native_definitions']:
        raise ValueError('PhotoCheck native definition/rewrite fingerprint')
    if outputs!=manifest['native_outputs']:
        raise ValueError('PhotoCheck whole-output fingerprint')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    publish(args.output,body)
    publish(args.output.with_name('photo_check_declarations.inc'),decl)
    publish(args.output.with_name('photo_check_data.inc'),data)
    publish(args.output.with_name('game.c'),game)
    publish(Path(str(args.output)+'.manifest.json'),json.dumps({
        'provider':manifest['provider'],'revision':REVISION,'definitions':records,
        'recipe_sha256':hashlib.sha256(manifest_raw).hexdigest(),
        'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'native_support':manifest['native_support'],
        'data_origins':manifest['data_origins'],'source_files':manifest['source_files'],
        'support_allowlist':manifest['support_allowlist'],
        'native_contract':manifest['native_contract'],
        'native_contract_origins':manifest['native_contract_origins'],
        'native_pointer_corrections':manifest['native_pointer_corrections'],
        'intentional_source_defect_corrections':manifest['intentional_source_defect_corrections'],
        'outputs':outputs},indent=2)+'\n')

if __name__=='__main__':
    main()
