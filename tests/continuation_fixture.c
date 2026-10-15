#include "continuation_fixture.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void enter(IsleCFixture* fixture) {
    ++fixture->entries;
    if (fixture->enter_hook != NULL) {
        fixture->enter_hook(fixture->hook_user);
    }
}

static void expect(IsleCFixture* fixture, int condition) {
    if (!condition) {
        ++fixture->failures;
    }
}

static void record(IsleCFixture* fixture, int value) {
    if (fixture->trace_count < 32) {
        fixture->trace[fixture->trace_count++] = value;
    } else {
        ++fixture->failures;
    }
}

static void nested_wait(IsleCFixture* fixture, int* outer) {
    int inner = 23;
    int* const address = &inner;
    record(fixture, 3);
    expect(fixture, isle_continuation_wait(fixture->domain, 3) == ISLE_CONT_OK);
    expect(fixture, inner == 23 && address == &inner && *outer == 11);
    inner += *outer;
    *outer = inner;
    record(fixture, 4);
    expect(fixture, isle_continuation_wait(fixture->domain, 1) == ISLE_CONT_OK);
    expect(fixture, inner == 34 && address == &inner && *outer == 34);
    record(fixture, 5);
}

void isle_c_fixture_nested(void* user) {
    IsleCFixture* const fixture = user;
    int outer = 11;
    int* const address = &outer;
    enter(fixture);
    record(fixture, 1);
    expect(fixture, isle_continuation_wait(fixture->domain, 0) == ISLE_CONT_OK);
    record(fixture, 2);
    nested_wait(fixture, &outer);
    expect(fixture, outer == 34 && address == &outer);
    fixture->resource = outer;
    record(fixture, 6);
}

static void finish_inner(IsleCFixture* fixture) {
    int local = 17;
    record(fixture, 10);
    expect(fixture, isle_continuation_wait(fixture->domain, 1) == ISLE_CONT_OK);
    expect(fixture, local == 17);
    (void)isle_continuation_finish(fixture->domain);
    ++fixture->after_retirement;
}

void isle_c_fixture_finish(void* user) {
    IsleCFixture* const fixture = user;
    enter(fixture);
    finish_inner(fixture);
    ++fixture->after_retirement;
}

void isle_c_fixture_immediate_finish(void* user) {
    IsleCFixture* const fixture = user;
    enter(fixture);
    (void)isle_continuation_finish(fixture->domain);
    ++fixture->after_retirement;
}

static void park_inner(IsleCFixture* fixture) {
    int local = fixture->resource;
    record(fixture, local);
    expect(fixture, isle_continuation_wait(fixture->domain, UINT64_MAX) == ISLE_CONT_OK);
    fixture->resource = -99;
    ++fixture->after_retirement;
}

void isle_c_fixture_park(void* user) {
    IsleCFixture* const fixture = user;
    enter(fixture);
    park_inner(fixture);
    ++fixture->after_retirement;
}

static void child_entry(void* user) {
    IsleCFixture* const fixture = user;
    int local = 53;
    enter(fixture);
    record(fixture, 40);
    expect(fixture, isle_continuation_wait(fixture->domain, 1) == ISLE_CONT_OK);
    expect(fixture, local == 53);
    fixture->resource = local;
    record(fixture, 41);
}

void isle_c_fixture_creator(void* user) {
    IsleCFixture* const fixture = user;
    IsleContinuationState state = ISLE_CONT_FAILED;
    IsleContinuationDomain* forbidden_domain = fixture->domain;
    IsleContinuation* unpublished = fixture->self;
    enter(fixture);
    record(fixture, 30);
    expect(fixture, isle_continuation_domain_check_access(fixture->domain) == ISLE_CONT_OK);
    expect(fixture, isle_continuation_state(fixture->self, &state) == ISLE_CONT_OK);
    expect(fixture, state == ISLE_CONT_RUNNING);
    expect(fixture, isle_continuation_create(fixture->domain, child_entry,
                                            fixture->child_data, &fixture->child) == ISLE_CONT_OK);
    expect(fixture, isle_continuation_state(fixture->child, &state) == ISLE_CONT_OK);
    expect(fixture, state == ISLE_CONT_READY && fixture->child_data->entries == 0);
    expect(fixture, isle_continuation_cancel(fixture->self) == ISLE_CONT_INVALID_STATE);
    expect(fixture, isle_continuation_destroy(fixture->self) == ISLE_CONT_INVALID_STATE);
    expect(fixture, isle_continuation_domain_destroy(fixture->domain) == ISLE_CONT_INVALID_STATE);
    expect(fixture, isle_continuation_domain_create(&forbidden_domain) == ISLE_CONT_INVALID_STATE);
    expect(fixture, forbidden_domain == fixture->domain);
    expect(fixture, isle_continuation_resume(fixture->child) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_create(fixture->domain, NULL, NULL,
                                            &unpublished) == ISLE_CONT_INVALID_ARGUMENT);
    expect(fixture, unpublished == fixture->self);
    if (fixture->other_domain != NULL) {
        expect(fixture, isle_continuation_domain_check_access(fixture->other_domain) == ISLE_CONT_WRONG_OWNER);
        expect(fixture, isle_continuation_create(fixture->other_domain, child_entry,
                                                fixture->child_data, &unpublished) == ISLE_CONT_WRONG_OWNER);
        expect(fixture, unpublished == fixture->self);
        expect(fixture, isle_continuation_wait(fixture->other_domain, 0) == ISLE_CONT_WRONG_OWNER);
    }
    expect(fixture, strlen(isle_continuation_error(fixture->domain)) != 0);
    record(fixture, 31);
    expect(fixture, isle_continuation_wait(fixture->domain, 2) == ISLE_CONT_OK);
    record(fixture, 32);
    (void)isle_continuation_finish(fixture->domain);
    ++fixture->after_retirement;
}

void isle_c_fixture_retire_other(void* user) {
    IsleCFixture* const fixture = user;
    IsleContinuationState state = ISLE_CONT_FAILED;
    enter(fixture);
    record(fixture, 20);
    expect(fixture, isle_continuation_state(fixture->victim, &state) == ISLE_CONT_OK);
    expect(fixture, state == ISLE_CONT_WAITING);
    if (fixture->operation == 0) {
        expect(fixture, isle_continuation_cancel(fixture->victim) == ISLE_CONT_OK);
        expect(fixture, isle_continuation_state(fixture->victim, &state) == ISLE_CONT_OK);
        expect(fixture, state == ISLE_CONT_CANCELLED);
        expect(fixture, fixture->victim_data->after_retirement == 0);
        fixture->victim_data->resource = 61;
    }
    expect(fixture, isle_continuation_destroy(fixture->victim) == ISLE_CONT_OK);
    fixture->victim = NULL;
    expect(fixture, fixture->victim_data->after_retirement == 0);
    fixture->victim_data->resource = 71;
    record(fixture, 21);
    expect(fixture, isle_continuation_wait(fixture->domain, 1) == ISLE_CONT_OK);
    expect(fixture, fixture->victim_data->resource == 71);
    record(fixture, 22);
}

static void fail_inner(IsleCFixture* fixture) {
    char message[] = "compiled C resource failure 77";
    record(fixture, 50);
    expect(fixture, isle_continuation_wait(fixture->domain, 1) == ISLE_CONT_OK);
    expect(fixture, isle_continuation_fail(fixture->domain, NULL) == ISLE_CONT_INVALID_ARGUMENT);
    (void)isle_continuation_fail(fixture->domain, message);
    ++fixture->after_retirement;
}

void isle_c_fixture_fail(void* user) {
    IsleCFixture* const fixture = user;
    enter(fixture);
    fail_inner(fixture);
    ++fixture->after_retirement;
}

void isle_c_fixture_wrong_owner(IsleCFixture* fixture) {
    IsleContinuationState state = ISLE_CONT_FAILED;
    IsleContinuation* unpublished = fixture->self;
    expect(fixture, isle_continuation_domain_check_access(fixture->domain) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_resume(fixture->self) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_cancel(fixture->self) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_destroy(fixture->self) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_state(fixture->self, &state) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, state == ISLE_CONT_FAILED);
    expect(fixture, isle_continuation_create(fixture->domain, child_entry,
                                            fixture, &unpublished) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, unpublished == fixture->self);
    expect(fixture, isle_continuation_wait(fixture->domain, 0) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_finish(fixture->domain) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_fail(fixture->domain, "foreign failure") == ISLE_CONT_WRONG_OWNER);
    expect(fixture, isle_continuation_domain_destroy(fixture->domain) == ISLE_CONT_WRONG_OWNER);
    expect(fixture, strlen(isle_continuation_error(fixture->domain)) != 0);
}
