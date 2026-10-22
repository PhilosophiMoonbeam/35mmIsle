#!/usr/bin/env python3
"""Strict pinned Original Album provider. Only private build outputs contain GAME C."""
import argparse,ast,hashlib,json,re,subprocess,tempfile,os
from pathlib import Path
REVISION = "df8815e75f3d79618466e01e0827c6165b93d11c"
RECIPE_SHA256 = "ea0991df819086bd6d9df45574b8fe34dce610d58dc83a60caf687e1f1ce1c98"
PRIMARY_SOURCES = ('src/pokemon_album/9A6B10.c','src/pokemon_album/9A6F70.c','src/pokemon_album/9ABB50.c','src/pokemon_album/9ADAD0.c')
SOURCE_FILES = {
    'src/pokemon_album/9A6B10.c': {'blob': '001d38eaa9957fbb147277302be90ab5b8f27e59', 'sha256': '46f319b13b49fe7c1ad78202ed679002bd086021ee97fc47980e4a5b073f64f8', 'bytes': 3609},
    'src/pokemon_album/9A6F70.c': {'blob': 'fe2b0e71cb5bab99b393ce1bee05747647ee1255', 'sha256': '841bc70a06db446e29823e31f56047952ed4c9735d051da4226a164325d78609', 'bytes': 49495},
    'src/pokemon_album/9ABB50.c': {'blob': 'e2f76f70561cbd8e88d2e2d0f2c2868ce660ebcc', 'sha256': 'fd31c23a302dc80b08f358dfba73bbca4937a3da9ec53db0e90cab8106100220', 'bytes': 26687},
    'src/pokemon_album/9ADAD0.c': {'blob': '0891bf965bcea0059b702a322f6f0590d69736ff', 'sha256': '7c09f315410a70b265bf815901cd8e0ae1a0b94bfefa5894695cf187dc318bbc', 'bytes': 6269},
    'src/pokemon_album/pokemon_album.h': {'blob': '1d242aa1d85f66acad12abd42f5d3bee90de7c01', 'sha256': '1576b9e234dab811abb34372e92efc79529a22f14c1dedec43399924e8e7b6e4', 'bytes': 2061},
    'include/common_structs.h': {'blob': '2f1b52e408dd916f2e002a5efc252684a8bcf6fe', 'sha256': 'bb84c8391dfd7a7382de6ca8119d05b139e7475dc7f2270c82cc88e1d9c25509', 'bytes': 9431},
    'include/enums.h': {'blob': '18d9da14c9daf13f5bb6a4ec35d25eeefbbdca07', 'sha256': '02b0bfbbc6d010f41039073612a7a031ee2112bdf07fd78cd4b18a17b302e096', 'bytes': 9833},
    'src/window/window.h': {'blob': 'c7490b352a5c4b99e570f26ba9a556cb20afb305', 'sha256': 'b0140107b3fe440dd3ae5ddde311ebe89fa38b8e51ec2d95a1f9aff10764cdba', 'bytes': 6585},
    'src/window/8486E0.c': {'blob': 'a822b0a8a2148ccda8b4557707f81d3a71dc19d8', 'sha256': '67bc8e26889891abec021e0303b58d8a9b791321db2ca8ec1658b81d8d7437ed', 'bytes': 4374},
    'src/window/text.c': {'blob': '3189fdc9bed7cbaf6b841c3899051b52b4b0097b', 'sha256': '1e2785fd42cda4941a9a28f301dc24f7fbfe47bde045619d6593d19ae8742151', 'bytes': 23076},
}
CALLBACKS = ['func_801DCA14_9A6C64', 'album_BlinkCursor', 'album_BlinkLetter', 'album_CreateAlbumUI']
BRIDGE = {'FocusMark_Create': 'ab_focus_create', 'FocusMark_MoveFront': 'ab_focus_front', 'FocusMark_SetPos': 'ab_focus_position', 'FocusMark_SetSize': 'ab_focus_size', 'FocusMark_SetTargetPos': 'ab_focus_target', 'FocusMark_SetTargetSize': 'ab_focus_target_size', 'FocusMark_Show': 'ab_focus_show', 'FocusMark_StopAnimation': 'ab_focus_stop', 'UIElement_Create': 'ab_element_create', 'UIElement_Delete': 'ab_element_delete', 'UIElement_Draw': 'ab_element_draw', 'UIElement_DrawBackground': 'ab_element_background', 'UIElement_PrintText': 'ab_print_source', 'UIElement_SetColor': 'ab_element_color', 'UIElement_SetFont': 'ab_element_font', 'UIElement_SetPos': 'ab_element_absolute', 'UIElement_SetSpriteColor': 'ab_element_tint', 'UIElement_SetState': 'ab_element_state', 'UIElement_SetTextPos': 'ab_element_position', 'UIFrame_Create': 'ab_frame_create', 'UIFrame_FadeIn': 'ab_frame_fade_in', 'UIFrame_FadeOut': 'ab_frame_fade_out', 'UIFrame_SetPos': 'ab_frame_position', 'UIFrame_Show': 'ab_frame_show', 'UILayout_CreateButtons': 'ab_layout_create', 'UILayout_GetButtons': 'ab_selector', 'UILayout_HideButtons': 'ab_layout_hide', 'UILayout_HideHeaderElement': 'ab_layout_hide_header', 'UILayout_Init': 'ab_layout_initialize', 'UILayout_SetHeaderFlags': 'ab_layout_header', 'UILayout_ShowHeaderElement': 'ab_layout_show_header', 'UILayout_ShowPanel': 'ab_layout_panel', 'UILayout_WaitPanelTransitionComplete': 'ab_layout_wait', 'UIText_GetStringWidth': 'ab_width', 'UIText_Initialize': 'ab_text_initialize', 'UIText_SetFont': 'ab_text_font', 'UIText_SetPrintDelay': 'ab_text_delay', 'UIText_SetShadowOffset': 'ab_text_shadow', 'UIText_SetSpacing': 'ab_text_spacing', 'auPlaySong': 'ab_song', 'auPlaySound': 'ab_sound', 'auSetBGMVolume': 'ab_volume', 'auSetBGMVolumeSmooth': 'ab_volume_smooth', 'func_800A7F68': 'ab_aux_camera', 'func_800A85E8': 'ab_process', 'func_800AA38C': 'ab_input', 'func_800AA85C': 'ab_repeat', 'func_800AA870': 'ab_input_mask', 'func_800AADF0': 'ab_scene_result', 'func_800AAE28': 'ab_input_initialize', 'func_8036D4B4_840C64': 'ab_font_option', 'func_80371C68_845418': 'ab_sprite_synchronous', 'func_80371D14_8454C4': 'ab_sprite_stackful', 'func_80371DC0_845570': 'ab_sibling', 'func_80371E68_845618': 'ab_child', 'func_8037452C_847CDC': 'ab_score', 'func_80374714_847EC4': 'ab_preview', 'func_80374D20_8484D0': 'ab_replay_cleanup', 'func_80374D40_8484F0': 'ab_replay_initialize', 'func_8037501C_8487CC': 'ab_number', 'func_8037519C_84894C': 'ab_formatted', 'getAlbumCapacity': 'ab_album_capacity', 'getAlbumPhoto': 'ab_album', 'getAlbumComment': 'ab_get_comment', 'setAlbumComment': 'ab_set_comment', 'removeAlbumPhoto': 'ab_remove_album', 'func_800BF954_5C7F4': 'ab_set_album', 'getLevelName': 'ab_level_name', 'getPlayerName': 'ab_player_name', 'getPokemonName': 'ab_pokemon_name', 'ohWait': 'ab_wait', 'omDeleteGObj': 'ab_delete'}
GLOBALS = ['D_80208B50_9D2DA0', 'D_80208B54_9D2DA4', 'album_IsEditingComment', 'album_CursorAlpha', 'album_CursorAlphaRate', 'album_LetterAlpha', 'album_LetterAlphaRate', 'D_80208B88_9D2DD8', 'album_D_8024EFE0_A19230', 'album_D_8024FFE8_A1A238', 'album_D_8024FFEC_A1A23C', 'album_UiCommentActiveLetter', 'album_UiAlbumStats', 'album_UiAlbumHeader', 'album_UiPhotoComment', 'album_D_80250000_A1A250', 'album_UiRows', 'album_UiPageNumber', 'album_D_8025005C_A1A2AC', 'album_BackgroundSObj', 'album_BackgroundGObj', 'album_CoverTitleSObj', 'album_CoverTitleGObj', 'album_D_8025007C_A1A2CC', 'album_PhotoCorners', 'album_D_8025009C_A1A2EC', 'album_CommentBackground', 'album_Cursor', 'album_PhotoDescBackground', 'album_DraggedPhotoGObj', 'album_DraggedPhotoSObj', 'album_AlbumPage', 'album_D_802500B8_A1A308', 'album_CharGridId', 'album_CommentCursorPos', 'album_Photos', 'album_D_80250108_A1A358', 'album_D_8025010C_A1A35C', 'album_D_80250110_A1A360', 'D_80208B90_9D2DE0', 'album_SelectedPhoto', 'D_80208C18_9D2E68', 'album_CurrentColumn', 'album_CurrentRow', 'album_DraggedPhotoColumn', 'album_DraggedPhotoRow', 'D_80208C2C_9D2E7C', 'D_80208C30_9D2E80', 'album_CoverButtons', 'album_PanelButtons', 'album_D_80208C40_9D2E90', 'album_EmptyComment', 'album_D_80250170_A1A3C0', 'album_D_80250550_A1A7A0', 'album_D_80250554_A1A7A4', 'album_D_80250558_A1A7A8', 'album_D_80250938_A1AB88', 'album_PhotoCount', 'album_LastPhotoIndex', 'album_FirstPhotoIndex']
CONSTANTS = {'SCREEN_WIDTH': '320', 'SCREEN_HEIGHT': '240', 'POKEDEX_MAX': '151', 'A_BUTTON': '0x8000', 'B_BUTTON': '0x4000', 'START_BUTTON': '0x1000', 'STICK_SLOW_UP': '0x10000', 'STICK_SLOW_DOWN': '0x20000', 'STICK_SLOW_RIGHT': '0x40000', 'STICK_SLOW_LEFT': '0x80000', 'LINK_6': '6', 'DL_LINK_0': '0', 'SCENE_OAKS_LAB_2': '13', 'SP_HIDDEN': 'ISLE_SP_HIDDEN', 'SP_TRANSPARENT': 'ISLE_SP_TRANSPARENT', 'UI_FLAG_32BIT': '0x400', 'BGM_PLAYER_MAIN': '0', 'SONG_ID_10': '10', 'FRAME_STYLE_0': '0', 'SOUND_ID_65': '65', 'SOUND_ID_66': '66', 'SOUND_ID_67': '67', 'SOUND_ID_69': '69', 'SOUND_ID_71': '71', 'SOUND_ID_74': '74', 'SOUND_ID_75': '75', 'SOUND_ID_85': '85', 'SOUND_ID_94': '94', 'BUTTON_RETURN': '5', 'BUTTON_GO_TO_LAB': '17', 'BUTTON_DELETE': '19', 'BUTTON_ENLARGE': '20', 'BUTTON_ARRANGE': '26', 'BUTTON_COMMENT': '27', 'BUTTON_CHECK': '28', 'BUTTON_OPEN': '30', 'BUTTON_RETURN_TO_COVER': '31', 'BUTTON_SPLITTER': '34', 'BUTTON_NONE': '35'}
TYPES = {'s32': 'int32_t', 'u32': 'uint32_t', 's16': 'int16_t', 'u16': 'uint16_t', 's8': 'int8_t', 'u8': 'uint8_t', 'f32': 'float', 'bool': 'int'}
ASSETS = {'D_801E4518_9AE768': '0x801e4518', 'D_801ED550_9B77A0': '0x801ed550', 'D_801F2D28_9BCF78': '0x801f2d28', 'D_801F2F88_9BD1D8': '0x801f2f88', 'D_801F3108_9BD358': '0x801f3108', 'D_801F3288_9BD4D8': '0x801f3288', 'D_801F3408_9BD658': '0x801f3408', 'D_801F3588_9BD7D8': '0x801f3588', 'D_801F6B50_9C0DA0': '0x801f6b50', 'D_801FB1D0_9C5420': '0x801fb1d0', 'D_801FFA10_9C9C60': '0x801ffa10', 'D_801FFC80_9C9ED0': '0x801ffc80', 'D_80200058_9CA2A8': '0x80200058', 'D_80200400_9CA650': '0x80200400', 'D_802005B8_9CA808': '0x802005b8', 'D_802006E8_9CA938': '0x802006e8', 'D_80201728_9CB978': '0x80201728', 'D_80208B10_9D2D60': '0x80208b10'}
NATIVE = ['album_InitCamera', 'album_Start', 'func_801DC8C0_9A6B10', 'func_801DC904_9A6B54', 'func_801DCC74_9A6EC4']
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
    records.append({'kind':'exact','label':label,'find_sha256':digest(old),'replace_sha256':digest(new),'find_bytes':len(old.encode()),'replace_bytes':len(new.encode()),'count':count})
    return body.replace(old,new)

def definitions(text):
    found=[]
    masked=TOKENS.sub(lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),text)
    for match in re.finditer(r'(?m)^([A-Za-z_][A-Za-z_0-9 \t*]*?)\s+([A-Za-z_]\w*)\(([^;{}]*)\)\s*\{',masked):
        depth=1
        end=match.end()
        while end<len(masked) and depth:
            depth+=(masked[end]=='{')-(masked[end]=='}')
            end+=1
        if depth:raise ValueError(('unterminated definition',match[2]))
        if text[end:end+1]=='\n':end+=1
        raw=text[match.start():end]
        found.append({'name':match[2],'first':text[:match.start()].count('\n')+1,'last':text[:end].count('\n'),'sha256':digest(raw),'raw':raw})
    return found

def rewrite_definition(original,name,names):
    records=[]
    def change(old,new,count=1):
        nonlocal original
        original=replace_exact(original,old,new,count,records,name)
    if name in NATIVE:
        return None,None,[{'kind':'typed-native-lowering','implementation':{'func_801DC8C0_9A6B10':'isle_album_source_display_list','func_801DC904_9A6B54':'isle_album_source_render_state','album_InitCamera':'ab_camera_create','func_801DCC74_9A6EC4':'isle_album_func_801DCC74_9A6EC4','album_Start':'isle_album_source_entry'}[name]}]
    if name=='album_Init':
        change('UIMem_CreateHeap(album_Heap, sizeof(album_Heap));','ab_check(source, source->native->ui_heap(source, 0x46000));')
    if name=='func_801DDC70_9A7EC0':
        change('album_Photos[i].isShown != NULL','album_Photos[i].isShown != 0')
    if name=='album_GetCommentTextWidth':
        brace=original.index('{')
        change(original[brace:],'{\n    return ab_comment_width(source, wideStr, dst);\n}\n')
    if name in ('album_CreatePhoto','func_801DDE64_9A80B4'):
        change('    Bitmap* '+('bitmap' if name=='album_CreatePhoto' else 'bitmap')+';\n    u8* dest;\n    u8* src;\n','')
        start=original.index('    bitmap = ')
        stop=original.index('\n    return gobj;',start)
        change(original[start:stop],'    ab_thumbnail(source, sobj, 0x80201728);\n')
    if name=='func_801DE0C8_9A8318':
        change('    album_D_8025010C_A1A35C = sobj;','    album_D_8025010C_A1A35C = sobj;\n    ab_thumbnail(source, sobj, 0x80208b10);')
    if name=='func_801DE080_9A82D0':
        change('func_80374714_847EC4(func_801E4068_9AE2B8(), &album_DraggedPhotoSObj->sprite);','ab_preview_working(source, album_D_80250170_A1A3C0.photoData, album_DraggedPhotoSObj);')
    if name=='func_801DE03C_9A828C':
        change('    SObj* sp4;','    SObj* sp4 = ab_unwritten_sp4(source);')
    if name=='func_801DF120_9A9370':
        change('ARRAY_COUNT(album_CoverTitleSObj) + 1','5')
    if name=='func_801DF2E0_9A9530':
        change('    sobj->sprite = D_801F2D28_9BCF78;','    ab_rebind(source, sobj, 0x801f2d28);')
        change('    sobj->sprite = D_801ED550_9B77A0;','    ab_rebind(source, sobj, 0x801ed550);')
    if name=='album_SetBackgroundTexture':
        change('        sobj->sprite = D_801F2F88_9BD1D8;','        ab_rebind(source, sobj, 0x801f2f88);')
        change('        sobj->sprite = D_801E4518_9AE768;','        ab_rebind(source, sobj, 0x801e4518);')
    if name in ('func_801DF2E0_9A9530','album_SetBackgroundTexture'):
        change('    sobj->unk_58 = NULL;\n    sobj->unk_5C = NULL;','    ab_check(source, source->native->sprite_clear_links(source, sobj));')
    if name=='album_DeletePhoto':
        change('    sobj->sprite = album_D_8025010C_A1A35C->sprite;','    ab_clone_sprite(source, sobj, album_D_8025010C_A1A35C);')
    if name=='func_801DEA4C_9A8C9C':
        change('    sp64[31] = 0;','    ab_comment_store(source, D_80208B54_9D2DA4, 31, 0);')
        change('168 - UIText_GetStringWidth(sp60)', '168 - ab_letter_width(source, sp60, album_D_8024FFE8_A1A238)')
        change('(sp60[0] << 8) | sp60[1]', '((uint32_t)(uint8_t)sp60[0] << 8) | (uint8_t)sp60[1]')
    if name=='album_DeleteCharInComment':
        change('        comment[--album_CommentCursorPos] = 0;','        ab_comment_store(source, D_80208B54_9D2DA4, --album_CommentCursorPos, 0);')
    if name=='album_PressedCharacterInGrid':
        change('    comment[31] = 0;','    ab_comment_store(source, D_80208B54_9D2DA4, 31, 0);')
        change("                    comment[album_CommentCursorPos] = '　';","                    ab_comment_store(source, D_80208B54_9D2DA4, album_CommentCursorPos, '　');")
        change('comment[album_CommentCursorPos] = 0;','ab_comment_store(source, D_80208B54_9D2DA4, album_CommentCursorPos, 0);',2)
        change('comment[--album_CommentCursorPos] = 0;','ab_comment_store(source, D_80208B54_9D2DA4, --album_CommentCursorPos, 0);',2)
        change('        comment[album_CommentCursorPos] = (album_CharGrids[album_CharGridId][row * 10 + column * 2] << 8) |\n                                          album_CharGrids[album_CharGridId][row * 10 + column * 2 + 1];','        ab_comment_store(source, D_80208B54_9D2DA4, album_CommentCursorPos, (ab_grid(source, album_CharGridId)[row * 10 + column * 2] << 8) | ab_grid(source, album_CharGridId)[row * 10 + column * 2 + 1]);')
    if name=='album_GetPhotoComment':
        change('return getAlbumComment(idx)->text;','return ab_comment(source, idx);')
    if name=='func_801E4084_9AE2D4':
        # This value is consumed ONLY when the Original scan never assigns last.
        change('    album_LastPhotoIndex = lastPhotoIndex;','    album_LastPhotoIndex = photoCount ? lastPhotoIndex : ab_empty_last(source);')
    for rec in ('album_D_80250170_A1A3C0','album_D_80250558_A1A7A8'):
        for slot in ('index','sp1C'):
            old=f'{rec}.photoData = *getAlbumPhoto({slot});'
            if old in original: change(old,f'ab_copy_record(source, {rec}.photoData, {slot});',2 if name=='func_801E3B34_9ADD84' else 1)
        old=f'{rec}.photoData.unk_04.f32 = -1.0f;'
        if old in original: change(old,f'ab_check(source, isle_photo_working_clear({rec}.photoData));')
    old='album_D_80250558_A1A7A8.photoData = album_D_80250170_A1A3C0.photoData;'
    if old in original:change(old,'ab_check(source, isle_photo_working_copy(album_D_80250558_A1A7A8.photoData, album_D_80250170_A1A3C0.photoData));')
    original=code_only(original,lambda s:re.sub(r'\balbum_D_80250170_A1A3C0\.photoData\.unk_04\.f32',r'ab_working_present(source, album_D_80250170_A1A3C0.photoData)',s))
    original=code_only(original,lambda s:re.sub(r'&(album_D_802(?:50170_A1A3C0|50558_A1A7A8)\.photoData)',r'\1',s))
    if name=='func_801E4068_9AE2B8':
        change('return album_D_80250170_A1A3C0.photoData;','return ab_working_photo(source, album_D_80250170_A1A3C0.photoData);')
    counts={}
    def sub(pattern,replacement,code,label):
        result,count=re.subn(pattern,replacement,code)
        if count:counts[label]=counts.get(label,0)+count
        return result
    original=code_only(original,lambda s:sub(r'\bUNUSED\s+[^;]+;','',s,'padding'))
    original=code_only(original,lambda s:sub(r'album_CoverTitleSObj\[([^]\n]+)\]\s*=\s*(?!=)([^;\n]+);',r'ab_cover_set(source, \1, \2);',s,'bounded-cover-store'))
    original=code_only(original,lambda s:sub(r'album_CoverTitleSObj\[([^]\n]+)\]',r'ab_cover_get(source, \1)',s,'bounded-cover-read'))
    original=code_only(original,lambda s:sub(r'album_CharGrids\[([^]\n]+)\]',r'ab_grid(source, \1)',s,'finite-character-grid'))
    original=code_only(original,lambda s:sub(r'album_PanelButtons\[([^]\n]+)\]\.id',r'ab_slot_id(source, \1)',s,'live-layout-slot'))
    original=code_only(original,lambda s:sub(r'&(album_(?:Photos\[[^]\n]+\]\.sObj|DraggedPhotoSObj|D_8025010C_A1A35C))->sprite',r'\1',s,'actual-thumbnail-handle'))
    exp=r'[A-Za-z_]\w*(?:(?:\[[^]\n]+\])|(?:\.\w+))*'
    original=code_only(original,lambda s:sub(r'('+exp+r')->data\.sobj',r'ab_root(source, \1)',s,'root-handle'))
    # Parenthesized cover/root expressions are rewritten before simple identifiers.
    original=code_only(original,lambda s:sub(r'(ab_cover_get\(source, [^\n]+?\)|ab_root\(source, [^\n]+?\)|'+exp+r')->sprite\b',r'(*ab_sprite(source, \1))',s,'mutable-sprite-view'))
    for asset,address in ASSETS.items():original=code_only(original,lambda s,a=asset,v=address:sub(r'&'+a+r'\b',v,s,'asset:'+a))
    original=code_only(original,lambda s:sub(r'\bPhotoData\s*\*',r'const PhotoData*',s,'readonly-photo'))
    original=code_only(original,lambda s:sub(r'\bs16\s*\*',r'const uint16_t*',s,'readonly-comment'))
    if name!='album_GetCommentTextWidth':
        original=code_only(original,lambda s:sub(r'\bchar\s*\*',r'const char*',s,'readonly-text'))
    original=code_only(original,lambda s:sub(r'\bUIButton\s*\*',r'const UIButton*',s,'readonly-buttons'))
    head,inner=original.split('{',1)
    ret,args=head.split(name+'(',1)
    args=args.rsplit(')',1)[0].strip()
    public=name in CALLBACKS
    signature=('' if public else 'static ')+ret.strip()+' isle_album_'+name+'('
    signature+=args if public else 'Context* source'+(', '+args if args not in ('','void') else '')
    signature+=')'
    if public:
        argument=re.search(r'([A-Za-z_]\w*)\s*$',args)[1]
        inner='\n    Context* source = (Context*)isle_post_course_source_require('+argument+', 12);'+inner
    def tokens(code):
        for old,native in {**BRIDGE,**{n:'isle_album_'+n for n in names if n not in NATIVE},'album_InitCamera':'ab_camera_create'}.items():
            code=sub(r'\b'+old+r'\b',native,code,'function:'+old)
        for variable in GLOBALS:code=sub(r'\b'+variable+r'\b','source->'+variable,code,'field:'+variable)
        for old,new in {**TYPES,**CONSTANTS}.items():code=sub(r'\b'+old+r'\b',new,code,'token:'+old)
        # Existing explicit helpers already carry context. Only transformed calls need it.
        for function in list(BRIDGE.values())+['isle_album_'+n for n in names if n not in CALLBACKS and n not in NATIVE]+['ab_camera_create']:
            code=sub(r'\b'+function+r'\(\s*(?!Context\b)',function+'(source, ',code,'context:'+function)
            code=sub(re.escape(function+'(source, )'),function+'(source)',code,'empty:'+function)
        return code
    signature=code_only(signature,tokens).replace('(source, Context* source','(Context* source')
    result=encode_literals(signature+' {'+code_only(inner,tokens))
    records.append({'kind':'semantic-native','counts':counts})
    return result,encode_literals(signature)+';\n',records

def initialized_data(texts):
    ui=texts['src/pokemon_album/9A6F70.c']
    control=texts['src/pokemon_album/9ABB50.c']
    output=[]
    match=re.search(r'char\* album_CharGrids\[\]\s*=\s*\{(.*?)\};',ui,re.S)
    if not match:raise ValueError('missing character initializer')
    literals=re.findall(r'"(?:\\.|[^"\\])*"',match[1])
    page=''.join(ast.literal_eval(s) for s in literals).encode('euc_jp')
    if len(page)!=190:raise ValueError('character grid exact extent')
    output.append('static const uint8_t ab_character_grid_0[190]={'+','.join(str(b) for b in page)+'};')
    match=re.search(r's32 D_80208B58_9D2DA8\[6\]\s*=\s*\{(.*?)\};',ui,re.S)
    if not match:raise ValueError('missing sign table')
    signs=match[1]
    for key,value in {'PokemonID_1004':'1004','PokemonID_1010':'1010','PokemonID_1018':'1018','PokemonID_1022':'1022','PokemonID_KOFFING_SMOKE':'1028','PokemonID_1035':'1035'}.items():
        signs=re.sub(r'\b'+key+r'\b',value,signs)
    if [int(v.strip()) for v in signs.split(',') if v.strip()]!=[1004,1010,1018,1022,1028,1035]:raise ValueError('sign table values')
    output.append('static const int32_t D_80208B58_9D2DA8[6]={'+signs+'};')
    for name,count in [('album_CoverButtonsDef',4),('album_AlbumButtonsDef',6),('album_PhotoButtonsDef',6)]:
        match=re.search(r'UIButton '+name+r'\[\]\s*=\s*\{(.*?)\};',control,re.S)
        if not match:raise ValueError(('missing buttons',name))
        entries=re.findall(r'\{\s*(BUTTON_\w+)\s*,\s*(NULL|"(?:\\.|[^"\\])*")\s*\}',match[1])
        if len(entries)!=count:raise ValueError(('button extent',name))
        values=[]
        for button,literal in entries:
            size=0 if literal=='NULL' else len(ast.literal_eval(literal).encode('euc_jp'))
            values.append('{'+CONSTANTS[button]+',{'+literal+','+str(size)+'}}')
        output.append('static const UIButton '+name+'['+str(count)+']={'+','.join(values)+'};')
    return encode_literals('\n'.join(output)+'\n')

def declarations(texts):
    header=texts['src/pokemon_album/pokemon_album.h']
    matches=re.findall(r'enum AlbumPages\s*\{.*?\};',header,re.S)
    if len(matches)!=1:raise ValueError('album page enum closure')
    return 'enum { FONT_8=8,FONT_12=12,UI_BACKGROUND=0,UI_FOREGROUND=1,UI_NORMAL=0,UI_HIDDEN=1,HEADER_PREV=1,HEADER_NEXT=2,HEADER_TITLE=4 };\n'+matches[0]+'\n'

def compose(texts,selected,bridge):
    names=[item['name'] for item in selected]
    bodies=[];decl=[declarations(texts)];records=[]
    for item in selected:
        matches=[d for d in definitions(texts[item['source']]) if d['name']==item['name']]
        if len(matches)!=1:raise ValueError(('nonunique selector',item['name']))
        original=matches[0]
        if any(original[key]!=item[key] for key in ('first','last','sha256')):raise ValueError(('selector digest/span',item['name']))
        native,prototype,rewrites=rewrite_definition(original['raw'],item['name'],names)
        if native is None:
            implementation=rewrites[0]['implementation']
            selected_native=[d for d in definitions(bridge) if d['name']==implementation]
            if len(selected_native)!=1:raise ValueError(('native lowering selector',implementation))
            native_digest=selected_native[0]['sha256']
        else:native_digest=digest(native)
        record={key:value for key,value in item.items() if key not in ('native_sha256','rewrites')}
        record.update(native_sha256=native_digest,rewrites=rewrites)
        records.append(record)
        if native is not None:bodies.append(native);decl.append(prototype)
    data=initialized_data(texts)
    body='\n'.join(bodies)
    declaration=''.join(decl)
    output=bridge
    for include,content in [('album_declarations.inc',declaration),('album_data.inc',data),('album_original.inc',body)]:
        marker='#include "'+include+'"'
        if output.count(marker)!=1:raise ValueError(('native include extent',include))
        output=output.replace(marker,content)
    return output,records,{'declarations_sha256':digest(declaration),'data_sha256':digest(data),'body_sha256':digest(body),'game_sha256':digest(output)}

def canonical(value):
    return json.dumps(value,sort_keys=True,separators=(',',':')).encode()

def git(repo,*args):
    return subprocess.run(['git','-C',str(repo),*args],check=True,stdout=subprocess.PIPE).stdout

def publish(path,raw):
    path.parent.mkdir(parents=True,exist_ok=True)
    descriptor,name=tempfile.mkstemp(prefix=path.name+'.',dir=path.parent)
    try:
        with os.fdopen(descriptor,'wb') as stream:stream.write(raw)
        os.replace(name,path)
    finally:
        if os.path.exists(name):os.unlink(name)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-repo',required=True,type=Path)
    parser.add_argument('--support-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--build-root',required=True,type=Path)
    parser.add_argument('--manifest',type=Path,default=Path(__file__).resolve().parents[1]/'cmake/album-inputs.json')
    args=parser.parse_args()
    recipe=json.loads(args.manifest.read_text())
    content={k:v for k,v in recipe.items() if k not in ('generator_sha256','recipe_sha256')}
    if digest(canonical(content).decode())!=RECIPE_SHA256 or recipe['recipe_sha256']!=RECIPE_SHA256:raise ValueError('recipe integrity')
    if hashlib.sha256(Path(__file__).read_bytes()).hexdigest()!=recipe['generator_sha256']:raise ValueError('generator integrity')
    if recipe['revision']!=REVISION or recipe['source_files']!=SOURCE_FILES:raise ValueError('source pin/file closure')
    if recipe['provider']!='isle-album-private' or recipe['scene_id']!=12 or recipe['resource_kind']!=2:raise ValueError('provider contract')
    if recipe['expected_definition_count']!=100:raise ValueError('definition count contract')
    for path,expected in recipe['native_support'].items():
        if hashlib.sha256((args.support_root/path).read_bytes()).hexdigest()!=expected['sha256']:raise ValueError(('native support',path))
    texts={}
    for path,identity in SOURCE_FILES.items():
        raw=git(args.source_repo,'show',REVISION+':'+path)
        if hashlib.sha256(raw).hexdigest()!=identity['sha256']:raise ValueError(('file sha256',path))
        if hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()!=identity['blob']:raise ValueError(('file blob',path))
        if len(raw)!=identity['bytes']:raise ValueError(('file byte count',path))
        texts[path]=raw.decode()
    actual=[]
    for path in PRIMARY_SOURCES:
        actual.extend((path,d['name']) for d in definitions(texts[path]))
    expected=[(d['source'],d['name']) for d in recipe['definitions']]
    if len(actual)!=100 or len(set(actual))!=100 or len(expected)!=100 or sorted(actual)!=sorted(expected):raise ValueError('unique complete Original definition closure')
    if recipe['callback_roots']!=CALLBACKS:raise ValueError('registered callback closure')
    bridge=(args.support_root/'src/album_source.c').read_text()
    game,records,outputs=compose(texts,recipe['definitions'],bridge)
    if records!=recipe['definitions']:raise ValueError('native per-unit/rewrite integrity')
    if outputs!=recipe['outputs']:raise ValueError('whole output integrity')
    target=args.build_root/'_deps/isle-album-private'
    # Check every selector/rewrite/support/output before publishing any byte.
    evidence={'provider':recipe['provider'],'revision':REVISION,'source_files':SOURCE_FILES,
              'definitions':records,'outputs':outputs,'recipe_sha256':RECIPE_SHA256,
              'generator_sha256':recipe['generator_sha256'],'native_support':recipe['native_support'],
              'native_contract':recipe['native_contract'],'data_origins':recipe['data_origins']}
    publish(target/'game.c',game.encode())
    publish(target/'evidence.json',(json.dumps(evidence,indent=2)+'\n').encode())

if __name__=='__main__':main()
