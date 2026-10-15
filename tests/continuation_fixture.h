#ifndef ISLE_CONTINUATION_FIXTURE_H
#define ISLE_CONTINUATION_FIXTURE_H

#include "35mmisle/continuation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IsleCFixture {
    IsleContinuationDomain* domain;
    IsleContinuationDomain* other_domain;
    IsleContinuation* self;
    IsleContinuation* child;
    IsleContinuation* victim;
    struct IsleCFixture* child_data;
    struct IsleCFixture* victim_data;
    void (*enter_hook)(void* user);
    void* hook_user;
    int trace[32];
    int trace_count;
    int failures;
    int entries;
    int after_retirement;
    int resource;
    int operation;
} IsleCFixture;

void isle_c_fixture_nested(void* user);
void isle_c_fixture_finish(void* user);
void isle_c_fixture_immediate_finish(void* user);
void isle_c_fixture_park(void* user);
void isle_c_fixture_creator(void* user);
void isle_c_fixture_retire_other(void* user);
void isle_c_fixture_fail(void* user);
void isle_c_fixture_wrong_owner(IsleCFixture* fixture);

#ifdef __cplusplus
}
#endif

#endif
