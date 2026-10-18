#!/usr/bin/env python3
"""Import own GAME photo roll bodies into private build storage."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_engine import git, publish

REVISION = 'df8815e75f3d79618466e01e0827c6165b93d11c'
SOURCE = 'src/app_render/47380.c'
SOURCE_SHA256 = 'b416fa9d9c6126c2beb936ba373b9dbf8479e1600292b510c2d432b89cd21936'
DEFINITIONS = [('getPokemonName',293,327),('func_8009BB4C',329,364),
 ('func_8009BBF4',366,368),('func_8009BC00',370,372),
 ('func_8009BC0C',374,383),('func_8009BC68',385,387),
 ('func_8009BC74',389,391),('func_8009BC80',393,398),
 ('func_8009BCC4',400,428),('makePhoto',738,835),
 ('func_8009CDC0',838,844),('func_8009CDE4',846,851),('func_8009CE00',853,868)]

def materialize(text):
    import re
    lines=text.splitlines(keepends=True)
    output=[]; records=[]
    for name,start,end in DEFINITIONS:
        original=''.join(lines[start-1:end]); body=original
        counts={}
        def change(old,new,n=None):
            nonlocal body
            count=body.count(old)
            if n is not None and count!=n:
                raise ValueError((name,old,count,n))
            if not count: raise ValueError((name,old,'absent'))
            body=body.replace(old,new); counts[old]=count
        # All record copies operate on semantic native values, not guest layouts.
        if name=='makePhoto':
            change('    UNUSED s32 pad[3];\n','',1)
            change('    if (gPhotoCount < 0 || gPhotoCount >= 60) {',
                '    source->status = ISLE_GAME_OK;\n    if (gPhotoCount < 0 || gPhotoCount >= 60) {',1)
            change('    if (pokemonObj != NULL) {',
                '    if (isle_player_photo_require_taken_context(source) != ISLE_GAME_OK) return -1;\n\n    if (pokemonObj != NULL) {',1)
            change('    memcpy(temp_s4, sp38, 0x20);',
                '    temp_s4->levelID = sp38->main.levelID;\n'
                '    temp_s4->unk_00_24 = sp38->main.unk_00_24;\n'
                '    temp_s4->unk_00_16 = sp38->main.unk_00_16;\n'
                '    temp_s4->unk_04 = sp38->main.unk_04;\n'
                '    temp_s4->unk_08 = sp38->main.unk_08;\n'
                '    temp_s4->unk_14 = sp38->main.unk_14;',1)
            for dst,src,size,n in [
                ('temp_s4->items[i]','sp38->main.items[i]','temp_s4->items[0]',1),
                ('temp_s4->effects[i]','sp38->main.effects[i]','temp_s4->effects[0]',1),
                ('temp_s4->pokemons[i]','sp38->main.pokemons[i]','temp_s4->pokemons[0]',3),
                ('temp_s4->pokemons[0]','sp38->main.pokemons[i]','temp_s4->pokemons[0]',1),
                ('temp_s4->pokemons[idx]','sp38->main.pokemons[i]','temp_s4->pokemons[1]',1)]:
                change(f'memcpy(&{dst}, &{src}, sizeof({size}));',f'{dst} = {src};',n)
        if name=='func_8009BB4C':
            change('    s32 temp = D_800AE4E4[150];',
                '    s32 temp;\n    if (!D_800AE4E4) { source->status = ISLE_GAME_INVALID_STATE; return -1; }\n    temp = D_800AE4E4[150];',1)
        if name=='getPokemonName':
            change('char* getPokemonName','const char* getPokemonName',1)
            change('    if (pkmnID > 0 && pkmnID <= POKEDEX_MAX) {',
                '    source->status = ISLE_GAME_OK;\n    if (!D_800AE284) { source->status = ISLE_GAME_INVALID_STATE; return NULL; }\n    if (pkmnID > 0 && pkmnID <= POKEDEX_MAX) {',1)
            change('        if (checkPlayerFlag(PFID_HAS_DASH_ENGINE) == 0) {',
                '        int32_t has_dash = isle_player_photo_checkPlayerFlag(source, 5);\n        if (source->status != ISLE_GAME_OK) return NULL;\n        if (has_dash == 0) {',1)
            change('        if (!func_800BF3D4_5C274(pkmnID)) {',
                '        int32_t registered = isle_player_photo_func_800BF3D4_5C274(source, pkmnID);\n        if (source->status != ISLE_GAME_OK) return NULL;\n        if (!registered) {',1)
            # Source's actual byte encoding, not UTF-8 text in Msg buffers.
            body=re.sub(r'"([^"\\]*)"',lambda m:'"'+''.join('\\x%02x'%byte for byte in m[1].encode('euc_jp'))+'"',body)
        body=re.sub(r'\b'+name+r'\(([^)]*)\)',
            lambda m:'isle_player_photo_'+name+'(IslePlayerPhotoSource* source'+
            (', '+m[1] if m[1] not in ('','void') else '')+')',body,count=1)
        for old,new in [('UnkThing','IslePlayerPhotoContext'),('PhotoData','IslePlayerPhotoData'),
                        ('GObj','IsleGameObject'),('s32','int32_t'),('u8','uint8_t')]:
            body=re.sub(r'\b'+old+r'\b',new,body)
        for var in ['gPhotoCount','D_800AE27C','D_800AE280','D_800BDF20','D_800B0598',
                    'PokemonDetector_Photo','PokemonDetector_TakenPhotoIndex','gPokemonIdInFocus',
                    'D_800AE284','D_800AE4E4']:
            body=re.sub(r'\b'+var+r'\b','source->'+var,body)
        for original_id,value in {'1004':1004,'1010':1010,'1018':1018,'1022':1022,'1035':1035,
            'KOFFING_SMOKE':1028,'603':603,'SHELLDER':90,'500':500,'MOLTRES_EGG':600,'601':601,'ZAPDOS_EGG':602}.items():
            body=re.sub(r'\bPokemonID_'+original_id+r'\b',str(value),body)
        body=body.replace('POKEDEX_MAX','151')
        records.append({'name':name,'first':start,'last':end,
          'sha256':hashlib.sha256(original.encode()).hexdigest(),'rewrites':counts,
          'native_sha256':hashlib.sha256(body.encode()).hexdigest()})
        output.append(body)
    return '\n'.join(output),records

def effect_materialize(text, projection, misc):
    """Complete original effect selection/projection/sort, semantic particles."""
    records=[]
    def extract(source,name):
        match=re.search(r'^[^\n]*\b'+name+r'\([^\n]*\) \{',source,re.M)
        if not match: raise ValueError(('missing definition',name))
        start=match.start(); end=source.index('{',match.start()); depth=1; end+=1
        while depth:
            if source[end]=='{': depth+=1
            elif source[end]=='}': depth-=1
            end+=1
        body=source[start:end]+'\n'
        records.append({'name':name,'sha256':hashlib.sha256(body.encode()).hexdigest()})
        return body
    output=[]
    def replace(body,old,new,count):
        if body.count(old)!=count: raise ValueError((old,body.count(old),count))
        records[-1].setdefault('rewrites',{})[old]=count
        return body.replace(old,new)
    for name in ['swap','qsortInternal','qsort2']:
        body=extract(misc,name)
        declaration=body.split('\n',1)[0]
        body=replace(body,declaration,
            declaration.replace(name+'(',name+'(IslePlayerPhotoSource* source, ',1),1)
        for function in ['swap','qsortInternal']:
            # Replace calls, not the now-prefixed definition.
            body=re.sub(r'\b'+function+r'\((?!IslePlayerPhotoSource)',function+'(source, ',body)
        body=re.sub(r'\bqsortItemSize\b','source->qsortItemSize',body)
        body=re.sub(r'\bqsortCompareFunc\b','source->qsortCompareFunc',body)
        if name=='qsortInternal':
            body=replace(body,'qsortInternal(source, first, s1 - source->qsortItemSize);',
                'if (s1 > first) qsortInternal(source, first, s1 - source->qsortItemSize);',1)
            body=replace(body,'last = s1 - source->qsortItemSize;',
                'if (s1 == first) break;\n            last = s1 - source->qsortItemSize;',1)
        output.append(body)
    output.insert(0,'static void qsortInternal(IslePlayerPhotoSource*, uint8_t*, uint8_t*);\n')
    for name in ['func_8009C584','func_8009C5C4']:
        output.append(extract(text,name))
    body=extract(projection,'func_80364494_5048A4')
    body=replace(body,'func_80364494_5048A4(', 'func_80364494_5048A4(IslePlayerPhotoSource* source, ',1)
    body=body.replace('D_803B1518_551928','source->D_803B1518_551928')
    output.append(body)
    body=extract(text,'func_8009C604')
    body=replace(body,'void func_8009C604(UnkThing* arg0)',
        'void isle_player_photo_capture_effects(IslePlayerPhotoSource* source, UnkThing* arg0, Particle* particles)',1)
    body=replace(body,'    s32 count;','    s32 count;\n    Particle* D_800BE1A8[1] = {particles};',1)
    body=replace(body,'    particle = sp60[count - i - 1].particle; // TODO fake match\n','',1)
    body=replace(body,'                if (0) {\n                } // TODO fake match\n','',1)
    body=replace(body,'func_80364494_5048A4(particle,','func_80364494_5048A4(source, particle,',1)
    body=replace(body,'qsort2(sp60,','qsort2(source, sp60,',2)
    body=replace(body,'(s16) (particle->size * 128.0f)','(s16) (s32) (particle->size * 128.0f)',1)
    for field in ['posX','posY','posZ','size']:
        body=re.sub(r'(arg0->main.effects\[i\]\.'+field+r' = )([^;]+);',r'\1(s32) (\2);',body)
    body=replace(body,'        arg0->main.effects[i].primColor = particle->primColor;',
        '\n'.join('        arg0->main.effects[i].primColor['+str(i)+'] = particle->primColor.'+c+';' for i,c in enumerate('rgba')),1)
    output.append(body)
    body=extract(text,'func_8009BF48')
    body=replace(body,'s32 func_8009BF48(PokemonPhotoData* arg0, GObj* arg1)',
        'int32_t isle_player_photo_func_8009BF48(IslePlayerPhotoSource* source, IslePlayerPokemonPhoto* arg0, const IslePlayerPhotoPokemonState* state)',1)
    body=replace(body,'    Pokemon* pokemon = GET_POKEMON(arg1);','    const IslePlayerPhotoPokemonState* pokemon = state;\n    (void) source;',1)
    body=replace(body,'    DObj* model = arg1->data.dobj;\n','',1)
    body=replace(body,'arg1->flags & GOBJ_FLAG_HIDDEN','state->hidden',1)
    body=replace(body,'func_80364718_504B28(arg1)','state->culled',1)
    body=replace(body,'func_8009BD4C(pokemonID, pokemon->modelAnims, D_800AD4B8, ARRAY_COUNT(D_800AD4B8))',
        '(state->animation_index & 63)',1)
    body=replace(body,'func_8009BD4C(pokemonID, pokemon->modelAnims, D_800AD9A4, ARRAY_COUNT(D_800AD9A4))',
        '(state->animation_index & 63)',1)
    body=replace(body,'pokemon->specialPoseID;','pokemon->specialPoseID & 31;',1)
    body=replace(body,'func_8009BDDC(arg0->pokemonID, arg0->unk_00_13)','state->default_pose',1)
    body=replace(body,'getNumberOfPoses()','164',1)
    native_transform=''.join(text.splitlines(keepends=True)[526:554])
    body=replace(body,native_transform,
        '    arg0->animationTime = state->animationTime;\n    arg0->position = state->position;\n    arg0->yaw = state->yaw;\n',1)
    names={'MOLTRES_EGG':600,'601':601,'ZAPDOS_EGG':602,'603':603,
        'GATE':1001,'EVOLUTION_CONTROLLER':1002,'1003':1003,'1006':1006,'1008':1008,
        '1009':1009,'1010':1010,'1012':1012,'1013':1013,'KOFFING_SMOKE':1028,
        'SMOKE_PUFF':1029,'LAVA_SPLASH':1030,'VOLCANO_SMOKE':1031,'1033':1033,
        '1037':1037,'1038':1038,'1000':1000}
    for name,value in names.items(): body=re.sub(r'\bPokemonID_'+name+r'\b',str(value),body)
    body=body.replace('POKEDEX_MAX','151')
    output.append(body)
    body=extract(text,'func_8009C304')
    body=replace(body,'s32 func_8009C304(ItemPhotoData* arg0, GObj* obj)',
        'int32_t isle_player_photo_func_8009C304(IslePlayerPhotoSource* source, IslePlayerItemPhoto* arg0, const IslePlayerPhotoItemState* state)',1)
    body=replace(body,'    DObj* dobj;\n    u8 temp_t3;\n    UNK_PTR* payload;',
        '    (void) source;',1)
    body=replace(body,'    dobj = obj->data.dobj;\n','',1)
    body=replace(body,'obj->flags & GOBJ_FLAG_HIDDEN','state->hidden',1)
    for component in 'xyz':
        body=replace(body,'dobj->position.v.'+component,'state->position.'+component,1)
    body=replace(body,'    payload = dobj->firstChild->payload.any;\n','',1)
    for identity,family in [('D_800EB460',2),('D_800EDAE0',3),('D_800E9168',0),('D_800EAF00',1)]:
        body=replace(body,'payload == '+identity,'state->family == '+str(family),1)
    body=replace(body,'obj->animationTime','state->model_time',2)
    body=replace(body,'dobj->firstChild->mobjList->timePassed','state->material_time',2)
    body=replace(body,'dobj->scale.v.x','state->scale',1)
    output.append(body)
    body=extract(text,'func_8009C8E4')
    body=replace(body,'void func_8009C8E4(OMCamera* arg0, MovementState* arg1, UnkThing* arg2)',
        'IsleGameResult isle_player_photo_capture_context(IslePlayerPhotoSource* source, UnkThing* arg2)',1)
    body=replace(body,'    func_803643E0_5047F0(arg0);\n    func_8009C4F4(arg2, arg1, arg0);',
        '    if ((source->status = isle_player_photo_capture_camera(source, arg2)) != ISLE_GAME_OK) return source->status;',1)
    for old,new in [('func_8009C25C(arg2, 3)','isle_player_photo_capture_pokemon(source, arg2)'),
                    ('func_8009C450(arg2, 4)','isle_player_photo_capture_items(source, arg2)'),
                    ('func_8009C604(arg2)','isle_player_photo_capture_fx(source, arg2)')]:
        body=replace(body,'    '+old+';','    if ((source->status = '+new+') != ISLE_GAME_OK) return source->status;',1)
    body=body.rsplit('}',1)[0]+'    return ISLE_GAME_OK;\n}\n'
    output.append(body)
    result='\n'.join(output)
    for old,new in [('UnkThing','IslePlayerPhotoContext'),('Particle','IslePlayerPhotoParticle'),
                    ('s32','int32_t'),('s16','int16_t'),('u32','uint32_t'),('u8','uint8_t'),('f32','float')]:
        result=re.sub(r'\b'+old+r'\b',new,result)
    for name in ['swap','qsortInternal','qsort2','func_8009C584','func_8009C5C4','func_80364494_5048A4']:
        result=re.sub(r'\b'+name+r'\b','isle_player_photo_'+name,result)
    # Each private function remains TU-local, avoiding unrelated symbol owners.
    result=re.sub(r'(?m)^(void|int32_t) (isle_player_photo_(?:swap|qsortInternal|qsort2|func_8009C584|func_8009C5C4|func_80364494_5048A4))',r'static \1 \2',result)
    result='typedef struct { IslePlayerPhotoParticle* particle; float unk_04, unk_08; } Unk8009C604;\n'+result
    return result,records

def registered_materialize(text):
    original=''.join(text.splitlines(keepends=True)[127:138])
    body=original
    changes=[
        ('bool func_800BF3D4_5C274(s32 pkmnID)',
         'int32_t isle_player_photo_func_800BF3D4_5C274(IslePlayerPhotoSource* source, int32_t pkmnID)'),
        ('    s32 photoIdx;','    int32_t photoIdx;'),
        ('    PhotoData* photoData;','    int32_t photoData;'),
        ('    photoIdx = func_8009BB4C(pkmnID);',
         '    photoIdx = isle_player_photo_func_8009BB4C(source, pkmnID);\n    if (source->status != ISLE_GAME_OK) return -1;'),
        ('ARRAY_COUNT(D_800C21B0_5F050->data.unk_180)','69'),
        ('    photoData = (D_800C21B0_5F050->data.unk_180[photoIdx].unk_04.s32 == -1) ? NULL : &D_800C21B0_5F050->data.unk_180[photoIdx];',
         '    photoData = isle_player_photo_report_photo_present(source, photoIdx);\n    if (source->status != ISLE_GAME_OK) return -1;'),
        ('    return photoData != NULL;','    return photoData != 0;')]
    for old,new in changes:
        if body.count(old)!=1: raise ValueError(('registered query rewrite',old))
        body=body.replace(old,new)
    return body,{'name':'func_800BF3D4_5C274','source':'src/more_funcs/5BF20.c',
        'first':128,'last':138,'sha256':hashlib.sha256(original.encode()).hexdigest(),
        'rewrites':{old:1 for old,new in changes},'native_sha256':hashlib.sha256(body.encode()).hexdigest()}

def detector_controls_materialize(text):
    output=[]; records=[]
    lines=text.splitlines(keepends=True)
    for name,first,last,enabled in [
            ('PokemonDetector_Enable',185,187,'true'),
            ('PokemonDetector_Disable',189,191,'false')]:
        original=''.join(lines[first-1:last])
        changes=[
            ('void '+name+'(void)', 'IsleGameResult isle_player_photo_'+name+'(IslePlayerPhotoSource* source)'),
            ('    PokemonDetector_IsEnabled = '+enabled+';',
             '    if (source == NULL || source->owner == NULL) return ISLE_GAME_INVALID_ARGUMENT;\n'
             '    if (source->status != ISLE_GAME_OK) return source->status;\n'
             '    source->status = isle_player_photo_native_detector_enabled(source, '+('1' if enabled=='true' else '0')+');\n'
             '    return source->status;')]
        body=original
        for old,new in changes:
            if body.count(old)!=1: raise ValueError(('Detector control rewrite',name,old))
            body=body.replace(old,new)
        output.append(body)
        records.append({'name':name,'source':'src/app_level/pokemon_detect.c',
            'first':first,'last':last,'sha256':hashlib.sha256(original.encode()).hexdigest(),
            'changes':[{'find':old,'replace':new,'occurrences':1} for old,new in changes],
            'native_sha256':hashlib.sha256(body.encode()).hexdigest()})
    return '\n'.join(output),records

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-repo',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    text=git(args.source_repo,'show',REVISION+':'+SOURCE).decode()
    if hashlib.sha256(text.encode()).hexdigest()!=SOURCE_SHA256:
        raise ValueError('photo source differs from admitted own GAME pin')
    body,records=materialize(text)
    projection=git(args.source_repo,'show',REVISION+':src/app_level/5047F0.c').decode()
    misc=git(args.source_repo,'show',REVISION+':src/sys/misc.c').decode()
    if hashlib.sha256(projection.encode()).hexdigest()!='20749e3eaa523fd5786616e74c18b33d158eabfb8d26b89bc587fc7b4c1ead03':
        raise ValueError('own projection pin changed')
    if hashlib.sha256(misc.encode()).hexdigest()!='f6607a602e8479b0647a6e87a8e290af614e3b1b8d5e36295a2ac9c7c45e7f32':
        raise ValueError('own sort pin changed')
    effects,effect_records=effect_materialize(text,projection,misc)
    body+='\n'+effects
    records+=effect_records
    saved=git(args.source_repo,'show',REVISION+':src/more_funcs/5BF20.c').decode()
    if hashlib.sha256(saved.encode()).hexdigest()!='ce66ec4b0fbab3ee4161b6f2713f57dce59853c652592c2b9d715baa1211e333':
        raise ValueError('own registered-species pin changed')
    registered,record=registered_materialize(saved)
    body+='\n'+registered
    records.append(record)
    detector=git(args.source_repo,'show',REVISION+':src/app_level/pokemon_detect.c').decode()
    if hashlib.sha256(detector.encode()).hexdigest()!='a8f0d6937417d5a63b70a7e6674f64877a8b62e25f8c8865b96420a6f2cdba5b':
        raise ValueError('own Detector controls pin changed')
    controls,control_records=detector_controls_materialize(detector)
    body+='\n'+controls
    records+=control_records
    args.output.mkdir(parents=True,exist_ok=True)
    publish(args.output/'player_photo_original.inc',body)
    publish(args.output/'player-photo-authority.json',(json.dumps({
        'revision':REVISION,'source':SOURCE,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),
        'definitions':records},indent=2)+'\n'))
if __name__=='__main__': main()
