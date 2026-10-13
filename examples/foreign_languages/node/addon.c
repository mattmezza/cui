/* SPDX-License-Identifier: MIT */
#include <node_api.h>
#include "../host.h"
#include <stdlib.h>
typedef struct { napi_env env; napi_value callback; int failed; } callback_context;
static void changed(void *user, int count) {
    callback_context *ctx = user;
    napi_value arg, receiver, result;
    if (ctx->failed) return;
    if (napi_create_int32(ctx->env, count, &arg) != napi_ok ||
        napi_get_undefined(ctx->env, &receiver) != napi_ok ||
        napi_call_function(ctx->env, receiver, ctx->callback, 1, &arg, &result) != napi_ok)
        ctx->failed = 1;
}
static napi_value run(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3], result;
    napi_valuetype type;
    if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok || argc < 1 ||
        napi_typeof(env, args[0], &type) != napi_ok || type != napi_function) {
        napi_throw_type_error(env, NULL, "run requires a callback"); return NULL;
    }
    char *font = NULL;
    unsigned frames = 0;
    if (argc > 1) {
        size_t length;
        if (napi_get_value_string_utf8(env, args[1], NULL, 0, &length) != napi_ok) {
            napi_throw_type_error(env, NULL, "font must be a string"); return NULL;
        }
        font = malloc(length+1);
        if (!font) { napi_throw_error(env, NULL, "allocation failed"); return NULL; }
        if (napi_get_value_string_utf8(env, args[1], font, length+1, &length) != napi_ok) {
            free(font); return NULL;
        }
    }
    if (argc > 2 && napi_get_value_uint32(env, args[2], &frames) != napi_ok) {
        free(font); napi_throw_type_error(env, NULL, "frames must be a number"); return NULL;
    }
    callback_context ctx = {env, args[0], 0};
    int status = cui_example_run(font, frames, changed, &ctx);
    free(font);
    if (ctx.failed) return NULL; /* Preserve callback's pending exception. */
    if (napi_create_int32(env, status, &result) != napi_ok) return NULL;
    return result;
}
static napi_value init(napi_env env, napi_value exports) {
    napi_value fn;
    if (napi_create_function(env, "run", NAPI_AUTO_LENGTH, run, NULL, &fn) != napi_ok ||
        napi_set_named_property(env, exports, "run", fn) != napi_ok) return NULL;
    return exports;
}
NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
