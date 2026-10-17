#ifndef ISLE_NATIVE_FACTORY_RESOURCES_FIXTURE_H
#define ISLE_NATIVE_FACTORY_RESOURCES_FIXTURE_H

#include "source_factory_resources.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFactoryFixture {
    IsleGame* game;
    int events[128];
    size_t count;
    unsigned failures, resumed;
    IsleSourceFactoryResources* nested_resources;
    IsleGameResult nested_result;
} NativeFactoryFixture;

typedef struct NativeFactoryTopology {
    IsleGameObject* objects[16];
    uint32_t ids[16], object_priorities[16];
    uint8_t links[16];
    IsleGameProcess* processes[16];
    IsleGameObject* process_objects[16];
    IsleGameSourceCallback functions[16];
    uint32_t process_priorities[16];
    uint8_t kinds[16];
    size_t object_count, process_count, public_identity_credits;
    unsigned overflow;
} NativeFactoryTopology;

void native_factory_bind(IsleGameObject*, NativeFactoryFixture*);
void native_factory_update(IsleGameObject*, void*);
void native_factory_animation(IsleGameObject*);
void native_factory_show(IsleGameObject*);
void native_factory_remove(IsleGameObject*);
void native_factory_nested_prepare(IsleGameObject*);
void native_factory_illegal_wait(IsleGameObject*);
void native_factory_topology(IsleGame*, NativeFactoryTopology*);
size_t native_factory_ready_workers(IsleSourceFactoryResources*);
IsleGameResult native_factory_invalid_kind(IsleGame*, IsleSourceFactoryResources*);

#ifdef __cplusplus
}
#endif
#endif
