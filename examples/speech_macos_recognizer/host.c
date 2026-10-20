/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned kind;
    speech_authorization authorization;
    cui_speech_error error;
    bool final, ends;
    size_t length;
    char text[SPEECH_RECOGNIZER_TEXT];
} message;
typedef struct pool pool;
struct speech_recognizer_endpoint {
    pool *pool;
    size_t references, first, count;
    bool open;
    cui_speech_error terminal;
    speech_recognizer_wake wake;
    void *wake_user;
    message events[SPEECH_RECOGNIZER_EVENTS];
};
struct pool {
    speech_mutex_ops mutex;
    void *lock;
    size_t references;
    speech_recognizer_endpoint endpoints[SPEECH_RECOGNIZER_ENDPOINTS];
};
struct speech_recognizer_host {
    pool *pool;
    speech_recognition_service service;
    speech_service_status status;
    speech_recognizer_wake wake;
    void *wake_user;
    speech_recognizer_endpoint *session, *permission;
    void *task, *permission_task;
    cui_speech_sink sink;
    bool finishing, ready, failed;
    cui_speech_error last_error;
    char utterance[SPEECH_RECOGNIZER_TEXT];
    size_t utterance_length;
    uint32_t committed_last;
};
static void lock(pool *p) {
    p->mutex.lock(p->lock);
}
static void unlock(pool *p) {
    p->mutex.unlock(p->lock);
}
static void pool_release(pool *p) {
    lock(p);
    bool last = --p->references == 0;
    unlock(p);
    if (last) {
        p->mutex.destroy(p->lock);
        free(p);
    }
}
static speech_recognizer_endpoint *reserve(speech_recognizer_host *h) {
    pool *p = h->pool;
    speech_recognizer_endpoint *result = NULL;
    lock(p);
    for (size_t i = 0; i < SPEECH_RECOGNIZER_ENDPOINTS; i++) {
        speech_recognizer_endpoint *e = &p->endpoints[i];
        if (!e->references) {
            *e = (speech_recognizer_endpoint){.pool = p,
                                              .references = 1,
                                              .open = true,
                                              .wake = h->wake,
                                              .wake_user = h->wake_user};
            p->references++;
            result = e;
            break;
        }
    }
    unlock(p);
    return result;
}
static bool space(pool *p) {
    bool available = false;
    lock(p);
    for (size_t i = 0; i < SPEECH_RECOGNIZER_ENDPOINTS; i++)
        available |= p->endpoints[i].references == 0;
    unlock(p);
    return available;
}
bool speech_recognizer_endpoint_retain(speech_recognizer_endpoint *e) {
    if (!e)
        return false;
    lock(e->pool);
    bool ok = e->references && e->references < SIZE_MAX;
    if (ok)
        e->references++;
    unlock(e->pool);
    return ok;
}
void speech_recognizer_endpoint_release(speech_recognizer_endpoint *e) {
    if (!e)
        return;
    pool *p = e->pool;
    lock(p);
    bool last = --e->references == 0;
    unlock(p);
    if (last)
        pool_release(p);
}
static void detach(speech_recognizer_endpoint *e) {
    lock(e->pool);
    e->open = false;
    e->count = 0;
    e->wake = NULL;
    e->wake_user = NULL;
    unlock(e->pool);
}
static uint32_t codepoint(const char *text, size_t length, size_t *position) {
    if (*position >= length)
        return UINT32_MAX;
    unsigned char c = (unsigned char)text[(*position)++];
    if (c < 128)
        return c ? c : UINT32_MAX;
    unsigned n = c >= 0xc2 && c <= 0xdf   ? 1
                 : c >= 0xe0 && c <= 0xef ? 2
                 : c >= 0xf0 && c <= 0xf4 ? 3
                                          : 9;
    if (n == 9 || n > length - *position)
        return UINT32_MAX;
    uint32_t value = c & ((1u << (6 - n)) - 1);
    for (unsigned i = 0; i < n; i++) {
        unsigned char tail = (unsigned char)text[(*position)++];
        if ((tail & 0xc0) != 0x80)
            return UINT32_MAX;
        value = (value << 6) | (tail & 63);
    }
    if ((n == 1 && value < 128) || (n == 2 && value < 2048) || (n == 3 && value < 65536) ||
        value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return UINT32_MAX;
    return value;
}
static bool valid(cui_str text) {
    if (text.length && !text.data)
        return false;
    size_t i = 0;
    while (i < text.length)
        if (codepoint(text.data, text.length, &i) == UINT32_MAX)
            return false;
    return true;
}
static bool post(speech_recognizer_endpoint *e, const message *m) {
    if (!e)
        return false;
    pool *p = e->pool;
    lock(p);
    if (!e->open) {
        unlock(p);
        return false;
    }
    if (!e->terminal) {
        if (m->length >= SPEECH_RECOGNIZER_TEXT || e->count == SPEECH_RECOGNIZER_EVENTS)
            e->terminal = CUI_SPEECH_CAPACITY;
        else {
            e->events[(e->first + e->count) % SPEECH_RECOGNIZER_EVENTS] = *m;
            e->count++;
        }
    }
    if (e->wake && !e->wake(e->wake_user) && !e->terminal)
        e->terminal = CUI_SPEECH_RECOGNIZER_ERROR;
    bool ok = !e->terminal;
    unlock(p);
    return ok;
}
bool speech_recognizer_endpoint_authorized(speech_recognizer_endpoint *e,
                                           speech_authorization auth) {
    return post(e, &(message){.kind = 1, .authorization = auth});
}
bool speech_recognizer_endpoint_result(speech_recognizer_endpoint *e, cui_str text, bool final,
                                       bool ends, cui_speech_error error) {
    if (error < CUI_SPEECH_OK || error > CUI_SPEECH_INVALID)
        error = CUI_SPEECH_RECOGNIZER_ERROR;
    message m = {.kind = 2, .length = text.length, .final = final, .ends = ends, .error = error};
    if (text.length >= SPEECH_RECOGNIZER_TEXT)
        m.error = CUI_SPEECH_CAPACITY;
    else if (!valid(text))
        m.error = CUI_SPEECH_RECOGNIZER_ERROR;
    if (m.error == CUI_SPEECH_CAPACITY || (m.error && !valid(text))) {
        m.kind = 3;
        m.length = 0;
    } else if (text.length)
        memcpy(m.text, text.data, text.length);
    return post(e, &m);
}
bool speech_recognizer_endpoint_error(speech_recognizer_endpoint *e, cui_speech_error error) {
    if (error <= CUI_SPEECH_OK || error > CUI_SPEECH_INVALID)
        error = CUI_SPEECH_RECOGNIZER_ERROR;
    return post(e, &(message){.kind = 3, .error = error});
}
static bool take(speech_recognizer_endpoint *e, message *m) {
    lock(e->pool);
    bool found = e->open && (e->terminal || e->count);
    if (found) {
        if (e->terminal) {
            *m = (message){.kind = 3, .error = e->terminal};
            e->terminal = CUI_SPEECH_OK;
            e->count = 0;
        } else {
            *m = e->events[e->first];
            e->first = (e->first + 1) % SPEECH_RECOGNIZER_EVENTS;
            e->count--;
        }
    }
    unlock(e->pool);
    return found;
}
cui_speech_error speech_recognizer_host_last_error(const speech_recognizer_host *h) {
    return h ? h->last_error : CUI_SPEECH_INVALID;
}
bool speech_recognizer_format_supported(cui_speech_format format) {
    return format.sample_rate == 16000 && format.channels == 1;
}
void speech_recognizer_host_refresh(speech_recognizer_host *h) {
    if (h)
        h->status = h->service.query(h->service.user);
}
speech_service_status speech_recognizer_host_status(const speech_recognizer_host *h) {
    return h ? h->status : (speech_service_status){0};
}
static cui_speech_error reason(speech_recognizer_host *h) {
    speech_service_status s = h->status;
    if (!s.supported || !s.usage_description || !s.locale_supported || !s.on_device ||
        !s.available || !speech_recognizer_format_supported(s.format))
        return CUI_SPEECH_UNSUPPORTED;
    if (h->permission || s.authorization != SPEECH_AUTH_GRANTED)
        return CUI_SPEECH_PERMISSION;
    return space(h->pool) ? CUI_SPEECH_OK : CUI_SPEECH_CAPACITY;
}
static bool available(void *user) {
    return reason(user) == CUI_SPEECH_OK;
}
static void close_operation(speech_recognizer_host *h, speech_recognizer_endpoint **endpoint,
                            void **token) {
    speech_recognizer_endpoint *e = *endpoint;
    if (!e)
        return;
    detach(e);
    void *service_token = *token;
    *endpoint = NULL;
    *token = NULL;
    if (service_token)
        h->service.close(h->service.user, service_token);
    speech_recognizer_endpoint_release(e);
}
static void close_session(void *user, void *token) {
    speech_recognizer_host *h = user;
    if (token != h || !h->session)
        return;
    h->sink = (cui_speech_sink){0};
    close_operation(h, &h->session, &h->task);
}
static cui_speech_error start(void *user, cui_speech_sink sink, void **token) {
    speech_recognizer_host *h = user;
    *token = NULL;
    if (h->session || !sink.state ||
        !speech_recognizer_format_supported(sink.state->recognizer.format))
        return CUI_SPEECH_INVALID;
    speech_recognizer_host_refresh(h);
    cui_speech_error error = reason(h);
    if (error)
        return error;
    h->session = reserve(h);
    if (!h->session)
        return CUI_SPEECH_CAPACITY;
    h->sink = sink;
    h->finishing = h->ready = h->failed = false;
    h->last_error = CUI_SPEECH_OK;
    h->utterance_length = 0;
    h->committed_last = 0;
    error = h->service.start(h->service.user, h->session, true, &h->task);
    if (error || !h->task) {
        close_session(h, h);
        return error ? error : CUI_SPEECH_RECOGNIZER_ERROR;
    }
    *token = h;
    return CUI_SPEECH_OK;
}
static void push(void *user, void *token, const int16_t *samples, size_t count) {
    speech_recognizer_host *h = user;
    if (token != h || !h->session || h->finishing || h->failed)
        return;
    while (count) {
        int16_t copy[256];
        size_t n = count < 256 ? count : 256;
        memcpy(copy, samples, n * sizeof *copy);
        cui_speech_error error = h->service.append(h->service.user, h->task, copy, n);
        if (error) {
            (void)speech_recognizer_endpoint_error(h->session, error);
            break;
        }
        samples += n;
        count -= n;
    }
}
static void finish(void *user, void *token) {
    speech_recognizer_host *h = user;
    if (token == h && h->session && !h->finishing) {
        h->finishing = true;
        h->service.finish(h->service.user, h->task);
    }
}
speech_recognizer_host *speech_recognizer_host_create(speech_recognition_service service,
                                                      speech_mutex_ops mutex,
                                                      speech_recognizer_wake wake,
                                                      void *wake_user) {
    if (!service.query || !service.authorize || !service.start || !service.append ||
        !service.finish || !service.close || !mutex.create || !mutex.destroy || !mutex.lock ||
        !mutex.unlock)
        return NULL;
    speech_recognizer_host *h = calloc(1, sizeof *h);
    pool *p = calloc(1, sizeof *p);
    if (!h || !p) {
        free(h);
        free(p);
        return NULL;
    }
    p->mutex = mutex;
    p->lock = mutex.create();
    if (!p->lock) {
        free(p);
        free(h);
        return NULL;
    }
    p->references = 1;
    h->pool = p;
    h->service = service;
    h->wake = wake;
    h->wake_user = wake_user;
    speech_recognizer_host_refresh(h);
    return h;
}
bool speech_recognizer_host_destroy(speech_recognizer_host *h) {
    if (!h)
        return true;
    if (h->session)
        return false;
    close_operation(h, &h->permission, &h->permission_task);
    pool_release(h->pool);
    free(h);
    return true;
}
cui_speech_error speech_recognizer_host_prepare(speech_recognizer_host *h) {
    if (!h || h->permission || h->session)
        return CUI_SPEECH_INVALID;
    speech_recognizer_host_refresh(h);
    speech_service_status s = h->status;
    if (!s.supported || !s.usage_description || !s.locale_supported)
        return CUI_SPEECH_UNSUPPORTED;
    if (s.authorization == SPEECH_AUTH_GRANTED)
        return CUI_SPEECH_OK;
    if (s.authorization != SPEECH_AUTH_UNKNOWN)
        return CUI_SPEECH_PERMISSION;
    h->permission = reserve(h);
    if (!h->permission)
        return CUI_SPEECH_CAPACITY;
    cui_speech_error error =
        h->service.authorize(h->service.user, h->permission, &h->permission_task);
    if (error || !h->permission_task) {
        close_operation(h, &h->permission, &h->permission_task);
        return error ? error : CUI_SPEECH_RECOGNIZER_ERROR;
    }
    return CUI_SPEECH_OK;
}
cui_speech_recognizer speech_recognizer_host_provider(speech_recognizer_host *h) {
    return h ? (cui_speech_recognizer){.user = h,
                                       .format = {16000, 1},
                                       .available = available,
                                       .start = start,
                                       .push_audio = push,
                                       .finish = finish,
                                       .close = close_session}
             : (cui_speech_recognizer){0};
}
static bool unspaced(uint32_t c) {
    return (c >= 0x3000 && c <= 0x30ff) || (c >= 0x3400 && c <= 0x9fff) ||
           (c >= 0xf900 && c <= 0xfaff) || (c >= 0xff00 && c <= 0xffef);
}
static bool whitespace(uint32_t c) {
    return c == 9 || c == 10 || c == 13 || c == 32 || c == 0xa0 || c == 0x3000 ||
           (c >= 0x2000 && c <= 0x200a);
}
static bool emit_text(speech_recognizer_host *h, const char *text, size_t length, bool commit) {
    if (!length)
        return commit ? cui_speech_report_phrase(h->sink, (cui_str){"", 0})
                      : cui_speech_report_hypothesis(h->sink, (cui_str){"", 0});
    size_t position = 0;
    uint32_t first = codepoint(text, length, &position), last = first;
    bool punctuation = first < 128 && strchr(",.?!;:)", (int)first);
    bool space_needed = h->committed_last && !whitespace(h->committed_last) && !whitespace(first) &&
                        !punctuation && !unspaced(h->committed_last) && !unspaced(first);
    char joined[SPEECH_RECOGNIZER_TEXT + 1];
    if (space_needed)
        joined[0] = ' ';
    memcpy(joined + space_needed, text, length);
    while (position < length)
        last = codepoint(text, length, &position);
    cui_str span = {joined, length + space_needed};
    bool ok = commit ? cui_speech_report_phrase(h->sink, span)
                     : cui_speech_report_hypothesis(h->sink, span);
    if (ok && commit)
        h->committed_last = last;
    return ok;
}
static bool starts_over(const char *old, size_t old_length, const char *text, size_t length) {
    size_t a = 0, b = 0, common = 0, total = 0;
    bool matching = true;
    while (a < old_length) {
        uint32_t before = codepoint(old, old_length, &a);
        uint32_t after = codepoint(text, length, &b);
        if (matching && before == after)
            common++;
        else
            matching = false;
        total++;
    }
    return common * 2 < total;
}
static bool result(speech_recognizer_host *h, const message *m) {
    if (m->error) {
        h->last_error = m->error;
        if (m->length && !emit_text(h, m->text, m->length, false))
            return false;
        h->failed = true;
        return cui_speech_report_error(h->sink, m->error);
    }
    if (h->utterance_length && starts_over(h->utterance, h->utterance_length, m->text, m->length))
        if (!emit_text(h, h->utterance, h->utterance_length, true))
            return false;
    h->utterance_length = 0;
    if (m->final) {
        h->failed = true; /* No subsequent callback can complete this session twice. */
        return emit_text(h, m->text, m->length, true) && cui_speech_report_finish(h->sink);
    }
    if (m->ends) {
        memcpy(h->utterance, m->text, m->length);
        h->utterance_length = m->length;
    }
    return emit_text(h, m->text, m->length, false);
}
bool speech_recognizer_host_poll(speech_recognizer_host *h, size_t budget) {
    if (!h)
        return false;
    bool changed = false;
    message m;
    if (h->permission && budget && take(h->permission, &m)) {
        h->status.authorization = m.kind == 1 ? m.authorization : SPEECH_AUTH_DENIED;
        h->last_error = m.kind != 1                              ? m.error
                        : m.authorization == SPEECH_AUTH_GRANTED ? CUI_SPEECH_OK
                                                                 : CUI_SPEECH_PERMISSION;
        close_operation(h, &h->permission, &h->permission_task);
        budget--;
        changed = true;
    }
    if (!h->session || h->failed || !budget)
        return changed;
    if (!h->ready) {
        h->ready = true;
        changed = true;
        if (!cui_speech_report_ready(h->sink)) {
            h->failed = true;
            return changed;
        }
    }
    if (!h->status.supported || !h->status.usage_description || !h->status.locale_supported ||
        !speech_recognizer_format_supported(h->status.format) || !h->status.on_device ||
        !h->status.available || h->status.authorization != SPEECH_AUTH_GRANTED) {
        h->failed = true;
        (void)cui_speech_report_error(h->sink, h->status.authorization != SPEECH_AUTH_GRANTED
                                                   ? CUI_SPEECH_PERMISSION
                                                   : CUI_SPEECH_RECOGNIZER_ERROR);
        return true;
    }
    while (budget-- && !h->failed && take(h->session, &m)) {
        changed = true;
        bool ok = m.kind == 2 ? result(h, &m)
                              : cui_speech_report_error(
                                    h->sink, m.kind == 3 ? m.error : CUI_SPEECH_RECOGNIZER_ERROR);
        if (m.kind == 3)
            h->last_error = m.error;
        if (!ok || m.kind != 2)
            h->failed = true;
    }
    return changed;
}
