/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "fetch.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    pthread_t ui;
    unsigned entered, exited, wakes, observed_cancel, fail_first_wakes;
    bool release[16], cooperate, fail_wake;
    af_status status[16];
    char urls[16][AF_URL_MAX+1];
} fixture;
static struct timespec deadline(long milliseconds) {
    struct timespec t; assert(!clock_gettime(CLOCK_REALTIME,&t));
    t.tv_sec+=milliseconds/1000; t.tv_nsec+=(milliseconds%1000)*1000000;
    if (t.tv_nsec>=1000000000) { ++t.tv_sec; t.tv_nsec-=1000000000; } return t;
}
static void init(fixture *f) {
    memset(f,0,sizeof *f); f->ui=pthread_self();
    assert(!pthread_mutex_init(&f->mutex,NULL)); assert(!pthread_cond_init(&f->changed,NULL));
}
static void finish(fixture *f) { assert(!pthread_cond_destroy(&f->changed)); assert(!pthread_mutex_destroy(&f->mutex)); }
static void transport(void *user, const char *url, af_cancel_fn cancel, void *token, af_result *result) {
    fixture *f=user; assert(!pthread_equal(f->ui,pthread_self()));
    pthread_mutex_lock(&f->mutex); unsigned i=f->entered++; assert(i<16);
    memcpy(f->urls[i],url,strlen(url)+1); pthread_cond_broadcast(&f->changed);
    struct timespec limit=deadline(5000);
    while (!f->release[i] && !(f->cooperate && cancel(token))) {
        struct timespec until=deadline(5);
        int rc=pthread_cond_timedwait(&f->changed,&f->mutex,&until);
        assert(rc==0 || rc==ETIMEDOUT);
        struct timespec now; assert(!clock_gettime(CLOCK_REALTIME,&now));
        assert(now.tv_sec<limit.tv_sec || (now.tv_sec==limit.tv_sec && now.tv_nsec<limit.tv_nsec));
    }
    bool cancelled=f->cooperate && cancel(token);
    f->observed_cancel+=cancelled;
    result->status=cancelled ? AF_CANCELLED : f->status[i];
    result->size=result->status==AF_TOO_LARGE ? AF_BODY_MAX+1 : 4;
    memcpy(result->body,"DATA",4); snprintf(result->error,sizeof result->error,"injected-%u",i);
    ++f->exited; pthread_cond_broadcast(&f->changed); pthread_mutex_unlock(&f->mutex);
}
static bool wake(void *user) {
    fixture *f=user; assert(!pthread_equal(f->ui,pthread_self()));
    pthread_mutex_lock(&f->mutex); ++f->wakes; bool success=!f->fail_wake && f->wakes>f->fail_first_wakes;
    pthread_cond_broadcast(&f->changed); pthread_mutex_unlock(&f->mutex); return success;
}
static void wait_count(fixture *f, unsigned *counter, unsigned desired) {
    struct timespec until=deadline(5000); pthread_mutex_lock(&f->mutex);
    while (*counter<desired) assert(!pthread_cond_timedwait(&f->changed,&f->mutex,&until));
    pthread_mutex_unlock(&f->mutex);
}
static void release(fixture *f, unsigned i) {
    pthread_mutex_lock(&f->mutex); f->release[i]=true; pthread_cond_broadcast(&f->changed); pthread_mutex_unlock(&f->mutex);
}
static void basic(void) {
    fixture f; init(&f); f.status[0]=AF_FAILURE;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    af_model m; af_model_init(&m); char url[]="https://example.invalid/copied";
    struct timespec before,after; assert(!clock_gettime(CLOCK_MONOTONIC,&before));
    assert(af_model_start(&m,w,url)==AF_STARTED);
    assert(!clock_gettime(CLOCK_MONOTONIC,&after));
    assert((after.tv_sec-before.tv_sec)+(after.tv_nsec-before.tv_nsec)/1e9 < 1.0);
    memset(url,'X',sizeof url-1); wait_count(&f,&f.entered,1);
    assert(!strcmp(f.urls[0],"https://example.invalid/copied"));
    assert(m.loading && f.exited==0 && m.applied==0);
    assert(af_model_start(&m,w,"https://example.invalid/duplicate")==AF_DUPLICATE);
    assert(m.generation==1); release(&f,0); wait_count(&f,&f.wakes,1);
    assert(m.loading && m.applied==0); /* Worker never applies UI state. */
    assert(af_model_drain(&m,w)==1 && !m.loading && m.result.status==AF_FAILURE);
    assert(m.result.generation==1 && !memcmp(m.result.body,"DATA",4));
    assert(af_model_start(&m,w,"https://example.invalid/retry")==AF_STARTED);
    wait_count(&f,&f.entered,2); release(&f,1); wait_count(&f,&f.wakes,2);
    assert(af_model_drain(&m,w)==1 && m.result.status==AF_SUCCESS && m.applied==2);
    assert(m.generation==2 && af_model_drain(&m,w)==0);
    assert(af_model_start(&m,w,"http://not-https.invalid")==AF_INVALID);
    char huge[AF_URL_MAX+2]; memset(huge,'a',sizeof huge); huge[sizeof huge-1]=0;
    assert(af_model_start(&m,w,huge)==AF_INVALID);
    af_worker_destroy(w); finish(&f);
}
typedef struct { af_model *model; bool applied; af_start_result start; } wrong_thread;
static void *foreign_apply(void *user) {
    wrong_thread *t=user; af_result result={.generation=t->model->generation};
    t->applied=af_model_apply(t->model,&result);
    t->start=af_model_start(t->model,NULL,"https://example.invalid/"); return NULL;
}
static void stale(void) {
    fixture f; init(&f); af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    af_model m; af_model_init(&m);
    assert(af_model_start(&m,w,"https://example.invalid/old")==AF_STARTED);
    wait_count(&f,&f.entered,1);
    wrong_thread other={.model=&m}; pthread_t thread;
    assert(!pthread_create(&thread,NULL,foreign_apply,&other)); assert(!pthread_join(thread,NULL));
    assert(!other.applied && other.start==AF_WRONG_THREAD && m.loading && !m.applied);
    assert(af_model_cancel(&m,w) && !m.loading && m.generation==2);
    assert(af_model_start(&m,w,"https://example.invalid/new")==AF_STARTED);
    /* Old transport deliberately ignores cancellation: logical cancellation
     * cannot claim physical interruption. Pending storage still copies new URL. */
    release(&f,0); wait_count(&f,&f.entered,2); release(&f,1); wait_count(&f,&f.wakes,2);
    af_result old,newer; assert(af_worker_take(w,&old)); assert(af_worker_take(w,&newer));
    assert(old.generation==1 && newer.generation==3);
    /* Deliberately deliver in reverse order to test defensive generation checks.
     * Actual transport execution is serialized, never concurrent. */
    assert(af_model_apply(&m,&newer)); assert(!af_model_apply(&m,&old));
    assert(!af_model_apply(&m,&newer)); assert(m.applied==1 && m.stale==2 && m.result.generation==3);
    m.generation=UINT64_MAX-1;
    assert(af_model_start(&m,w,"https://example.invalid/exhausted")==AF_INVALID);
    af_worker_destroy(w); finish(&f);
}
static void cancel_and_shutdown(void) {
    fixture f; init(&f); f.cooperate=true;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    af_model m; af_model_init(&m);
    assert(af_model_start(&m,w,"https://example.invalid/cancel")==AF_STARTED);
    wait_count(&f,&f.entered,1); assert(af_model_cancel(&m,w)); wait_count(&f,&f.wakes,1);
    assert(af_model_drain(&m,w)==1 && m.stale==1 && m.result.status==AF_CANCELLED);
    assert(af_model_start(&m,w,"https://example.invalid/pending-shutdown")==AF_STARTED);
    wait_count(&f,&f.entered,2);
    af_worker_destroy(w); /* Must join a blocked cooperative transport. */
    assert(f.exited==2 && f.observed_cancel==2);
    finish(&f); /* No callback can run after fixture destruction. */
}
static void queued_shutdown(void) {
    fixture f; init(&f); f.cooperate=true;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    assert(af_worker_submit(w,"https://example.invalid/active",1));
    wait_count(&f,&f.entered,1);
    assert(af_worker_submit(w,"https://example.invalid/queued",2));
    /* A newer job may cancel the active transport before stop acquires the
     * lock. Both schedules must join safely; queued work must never outlive it. */
    af_worker_destroy(w);
    assert(f.entered>=1 && f.entered<=2 && f.exited==f.entered);
    assert(f.observed_cancel==f.entered); finish(&f);
}
static void bounded_mailbox(void) {
    fixture f; init(&f); af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    for (unsigned i=0;i<3;++i) {
        assert(af_worker_submit(w,"https://example.invalid/bounded",i+1));
        wait_count(&f,&f.entered,i+1); release(&f,i); wait_count(&f,&f.exited,i+1);
        if (i<2) wait_count(&f,&f.wakes,i+1);
    }
    /* Third publisher is blocked on two occupied completion slots. Stop must
     * wake it, discard unpublished output, and join without UI draining. */
    af_worker_destroy(w); assert(f.wakes==2); finish(&f);
}
static void transient_wake_failure(void) {
    fixture f; init(&f); f.fail_first_wakes=1;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    af_model m; af_model_init(&m);
    assert(af_model_start(&m,w,"https://example.invalid/retry-wake")==AF_STARTED);
    wait_count(&f,&f.entered,1); release(&f,0); wait_count(&f,&f.wakes,2);
    assert(af_worker_wake_failures(w)==1);
    assert(af_model_drain(&m,w)==1 && m.applied==1 && !m.loading);
    af_worker_destroy(w); finish(&f);
}
static void shutdown_during_wake_retry(void) {
    fixture f; init(&f); f.fail_wake=true;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    assert(af_worker_submit(w,"https://example.invalid/stop-retry",1));
    wait_count(&f,&f.entered,1); release(&f,0); wait_count(&f,&f.wakes,1);
    af_worker_destroy(w); /* Stop also wakes any notification-delay wait. */
    assert(f.wakes>=1 && f.wakes<=3 && f.exited==1); finish(&f);
}
static void pending_and_wake_failure(void) {
    fixture f; init(&f); f.fail_wake=true; f.status[0]=AF_TOO_LARGE;
    af_worker *w=af_worker_create(transport,&f,wake,&f); assert(w);
    assert(af_worker_submit(w,"https://example.invalid/active",1)); wait_count(&f,&f.entered,1);
    char queued[]="https://example.invalid/pending";
    assert(af_worker_submit(w,queued,2)); memset(queued,'X',sizeof queued-1);
    assert(!af_worker_submit(w,"https://example.invalid/full",3));
    release(&f,0); wait_count(&f,&f.entered,2);
    assert(!strcmp(f.urls[1],"https://example.invalid/pending"));
    release(&f,1); wait_count(&f,&f.wakes,6);
    af_result r; assert(af_worker_take(w,&r) && r.status==AF_TOO_LARGE && r.size==0);
    assert(af_worker_take(w,&r) && r.generation==2);
    /* Wake failure does not discard mailbox output. Take retries are bounded. */
    assert(af_worker_wake_failures(w)>=1);
    af_worker_destroy(w); finish(&f);
}
int main(void) {
    basic(); stale(); cancel_and_shutdown(); queued_shutdown(); bounded_mailbox(); transient_wake_failure(); shutdown_during_wake_retry(); pending_and_wake_failure();
    puts("PASS async worker: owned bounded jobs/results, cancellation, stale delivery, join and UI thread"); return 0;
}
