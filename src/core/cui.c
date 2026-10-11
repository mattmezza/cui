/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#define CLAY_IMPLEMENTATION
#include "clay.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#define STACK_MAX 256
#define EVENT_MAX 256
typedef struct {
  uint32_t clay_id;
  size_t parent;
  bool clip;
} box_node;
typedef struct {
  cui_id id;
  size_t parent;
  bool modal;
} focus_scope;
struct cui_context {
  focus_scope focus_scopes[STACK_MAX], previous_focus_scopes[STACK_MAX];
  size_t focus_stack[STACK_MAX], focus_depth, focus_scope_count,
      previous_focus_scope_count, top_modal, previous_top_modal;
  size_t *semantic_scopes, *previous_semantic_scopes;
  Clay_Context *clay;
  void *arena;
  cui_config config;
  cui_draw_command *commands;
  size_t count;
  box_node *nodes;
  size_t *semantic_nodes;
  size_t node_count, node_stack[STACK_MAX];
  cui_rect *previous_hits;
  cui_semantic *semantics, *previous;
  size_t semantic_count, previous_count;
  cui_id *ids, *activations;
  size_t id_count, activation_count;
  cui_event queued[EVENT_MAX], events[EVENT_MAX];
  size_t queued_count, event_count;
  cui_id event_targets[EVENT_MAX];
  uint64_t scopes[STACK_MAX];
  size_t scope_count;
  bool opened[STACK_MAX];
  size_t depth, overflow_depth, element_count;
  char *strings;
  size_t string_used;
  cui_id focus, capture, space_capture;
  float pointer_x, pointer_y, dt;
  cui_error error;
  bool in_frame, focus_visible, dirty;
};
static void fail(cui_context *ui, cui_error e) {
  if (ui && ui->error == CUI_OK)
    ui->error = e;
}
static void select_context(cui_context *ui) {
  Clay_SetCurrentContext(ui->clay);
}
static bool active(cui_context *ui) {
  if (!ui)
    return false;
  if (!ui->in_frame) {
    fail(ui, CUI_ERROR_LIFECYCLE);
    return false;
  }
  select_context(ui);
  return true;
}
static Clay_Dimensions measure(Clay_StringSlice s, Clay_TextElementConfig *c,
                               void *u) {
  cui_context *ui = u;
  if (ui->config.measure) {
    cui_size z =
        ui->config.measure(ui->config.measure_user,
                           (cui_str){s.chars, (size_t)s.length}, c->fontSize);
    return (Clay_Dimensions){z.width, z.height};
  }
  return (Clay_Dimensions){s.length * c->fontSize * 0.6f, c->fontSize * 1.3f};
}
static void clay_error(Clay_ErrorData e) {
  cui_error kind = CUI_ERROR_LAYOUT;
  if (e.errorType == CLAY_ERROR_TYPE_DUPLICATE_ID)
    kind = CUI_ERROR_DUPLICATE_ID;
  if (e.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED ||
      e.errorType == CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED ||
      e.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED ||
      e.errorType == CLAY_ERROR_TYPE_HASH_MAP_CAPACITY_EXCEEDED)
    kind = CUI_ERROR_CAPACITY;
  fail(e.userData, kind);
}
cui_str cui_string(const char *s) { return (cui_str){s, s ? strlen(s) : 0}; }
cui_context *cui_create(cui_config config) {
  cui_context *ui = calloc(1, sizeof(*ui));
  if (!ui)
    return NULL;
  if (!config.max_elements)
    config.max_elements = 4096;
  if (!config.max_commands)
    config.max_commands = 8192;
  if (!config.frame_bytes)
    config.frame_bytes = 262144;
  /* Clay reports arena sizes through uint32_t, so retain a conservative
     supported element ceiling before its sizing or any allocations. */
  if (config.frame_bytes > (size_t)PTRDIFF_MAX ||
      config.max_elements > 1000000 ||
      config.max_commands > (size_t)PTRDIFF_MAX / sizeof(cui_draw_command) ||
      config.max_elements > SIZE_MAX / sizeof(cui_semantic) ||
      config.max_elements > SIZE_MAX / sizeof(cui_id) ||
      config.max_elements > SIZE_MAX / sizeof(box_node) ||
      config.max_elements > SIZE_MAX / sizeof(size_t) ||
      config.max_elements > SIZE_MAX / sizeof(cui_rect)) {
    free(ui);
    return NULL;
  }
  ui->config = config;
  ui->dirty = true;
  ui->top_modal = ui->previous_top_modal = SIZE_MAX;
  ui->commands = calloc(config.max_commands, sizeof(*ui->commands));
  ui->semantics = calloc(config.max_elements, sizeof(*ui->semantics));
  ui->previous = calloc(config.max_elements, sizeof(*ui->previous));
  ui->ids = calloc(config.max_elements, sizeof(*ui->ids));
  ui->activations = calloc(EVENT_MAX, sizeof(*ui->activations));
  ui->semantic_scopes =
      calloc(config.max_elements, sizeof(*ui->semantic_scopes));
  ui->previous_semantic_scopes =
      calloc(config.max_elements, sizeof(*ui->previous_semantic_scopes));
  ui->strings = malloc(config.frame_bytes);
  ui->nodes = calloc(config.max_elements, sizeof(*ui->nodes));
  ui->semantic_nodes = calloc(config.max_elements, sizeof(*ui->semantic_nodes));
  ui->previous_hits = calloc(config.max_elements, sizeof(*ui->previous_hits));
  Clay_Context *saved = Clay_GetCurrentContext();
  Clay_SetCurrentContext(NULL);
  Clay_SetMaxElementCount((int32_t)(config.max_elements + 32));
  size_t n = Clay_MinMemorySize();
  ui->arena = malloc(n);
  if (!ui->commands || !ui->semantics || !ui->previous || !ui->ids ||
      !ui->activations || !ui->semantic_scopes ||
      !ui->previous_semantic_scopes || !ui->strings || !ui->nodes ||
      !ui->semantic_nodes || !ui->previous_hits || !ui->arena) {
    Clay_SetCurrentContext(saved);
    cui_destroy(ui);
    return NULL;
  }
  ui->clay =
      Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(n, ui->arena),
                      (Clay_Dimensions){config.width, config.height},
                      (Clay_ErrorHandler){clay_error, ui});
  if (ui->clay)
    Clay_SetMeasureTextFunction(measure, ui);
  Clay_SetCurrentContext(saved);
  if (!ui->clay || ui->error) {
    cui_destroy(ui);
    return NULL;
  }
  return ui;
}
void cui_destroy(cui_context *ui) {
  if (!ui)
    return;
  if (Clay_GetCurrentContext() == ui->clay)
    Clay_SetCurrentContext(NULL);
  free(ui->semantic_scopes);
  free(ui->previous_semantic_scopes);
  free(ui->nodes);
  free(ui->semantic_nodes);
  free(ui->previous_hits);
  free(ui->commands);
  free(ui->semantics);
  free(ui->previous);
  free(ui->ids);
  free(ui->activations);
  free(ui->strings);
  free(ui->arena);
  free(ui);
}
static bool inside(cui_rect r, float x, float y) {
  return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y && x < r.x + r.w &&
         y < r.y + r.h;
}
static bool focusable(const cui_semantic *s) {
  return !s->disabled && s->role != CUI_ROLE_NONE && s->role != CUI_ROLE_LABEL;
}
static bool scope_contains(const focus_scope *scopes, size_t child,
                           size_t ancestor) {
  if (ancestor == SIZE_MAX)
    return true;
  while (child != SIZE_MAX) {
    if (child == ancestor)
      return true;
    child = scopes[child].parent;
  }
  return false;
}
static bool previous_allowed(cui_context *ui, size_t i) {
  return scope_contains(ui->previous_focus_scopes,
                        ui->previous_semantic_scopes[i],
                        ui->previous_top_modal);
}
static cui_id hit(cui_context *ui, float x, float y) {
  for (size_t i = ui->previous_count; i > 0; i--)
    if (focusable(&ui->previous[i - 1]) && previous_allowed(ui, i - 1) &&
        inside(ui->previous_hits[i - 1], x, y))
      return ui->previous[i - 1].id;
  return 0;
}
static void activate(cui_context *ui, cui_id id) {
  if (id && ui->activation_count < EVENT_MAX)
    ui->activations[ui->activation_count++] = id;
}
static void tab(cui_context *ui, bool reverse) {
  size_t found = ui->previous_count;
  for (size_t i = 0; i < ui->previous_count; i++)
    if (ui->previous[i].id == ui->focus)
      found = i;
  for (size_t step = 1; step <= ui->previous_count; step++) {
    size_t i =
        reverse
            ? (found == ui->previous_count
                   ? ui->previous_count - step
                   : (found + ui->previous_count - step) % ui->previous_count)
            : (found == ui->previous_count
                   ? step - 1
                   : (found + step) % ui->previous_count);
    if (focusable(&ui->previous[i]) && previous_allowed(ui, i)) {
      ui->focus = ui->previous[i].id;
      break;
    }
  }
  ui->focus_visible = true;
}
bool cui_queue_event(cui_context *ui, cui_event e) {
  if (!ui)
    return false;
  if (ui->in_frame) {
    fail(ui, CUI_ERROR_LIFECYCLE);
    return false;
  }
  if (ui->queued_count == EVENT_MAX) {
    fail(ui, CUI_ERROR_CAPACITY);
    return false;
  }
  ui->queued[ui->queued_count++] = e;
  ui->dirty = true;
  return true;
}
bool cui_begin_frame(cui_context *ui, float w, float h, float dt) {
  if (!ui)
    return false;
  if (ui->in_frame) {
    fail(ui, CUI_ERROR_LIFECYCLE);
    return false;
  }
  if (!isfinite(w) || !isfinite(h) || w < 0 || h < 0 || !isfinite(dt) ||
      dt < 0) {
    fail(ui, CUI_ERROR_LAYOUT);
    return false;
  }
  if (ui->previous_top_modal != SIZE_MAX) {
    bool allowed = false;
    for (size_t i = 0; i < ui->previous_count; i++)
      if (ui->previous[i].id == ui->focus && focusable(&ui->previous[i]) &&
          previous_allowed(ui, i))
        allowed = true;
    if (!allowed) {
      ui->focus = 0;
      for (size_t i = 0; i < ui->previous_count; i++)
        if (focusable(&ui->previous[i]) && previous_allowed(ui, i)) {
          ui->focus = ui->previous[i].id;
          break;
        }
    }
  }
  ui->event_count = ui->queued_count;
  memcpy(ui->events, ui->queued, ui->queued_count * sizeof(*ui->events));
  ui->queued_count = 0;
  ui->activation_count = 0;
  for (size_t i = 0; i < ui->event_count; i++) {
    cui_event e = ui->events[i];
    ui->event_targets[i] = ui->focus;
    if (e.type == CUI_EVENT_POINTER_MOVE || e.type == CUI_EVENT_POINTER_UP)
      ui->event_targets[i] = ui->capture;
    if (e.type == CUI_EVENT_POINTER_DOWN) {
      ui->event_targets[i] = hit(ui, e.x, e.y);
      if (!ui->event_targets[i] && ui->previous_top_modal != SIZE_MAX)
        ui->event_targets[i] =
            ui->previous_focus_scopes[ui->previous_top_modal].id;
    }
    if (e.type == CUI_EVENT_KEY_DOWN && e.key == CUI_KEY_ESCAPE &&
        ui->previous_focus_scope_count) {
      size_t scope = ui->previous_focus_scope_count - 1;
      for (size_t j = 0; j < ui->previous_count; j++)
        if (ui->previous[j].id == ui->focus &&
            ui->previous_semantic_scopes[j] != SIZE_MAX) {
          scope = ui->previous_semantic_scopes[j];
          break;
        }
      ui->event_targets[i] = ui->previous_focus_scopes[scope].id;
    }
    if (e.type == CUI_EVENT_POINTER_MOVE || e.type == CUI_EVENT_POINTER_DOWN ||
        e.type == CUI_EVENT_POINTER_UP) {
      ui->pointer_x = e.x;
      ui->pointer_y = e.y;
    }
    if (e.type == CUI_EVENT_POINTER_DOWN) {
      ui->capture = hit(ui, e.x, e.y);
      if (ui->capture || ui->previous_top_modal == SIZE_MAX)
        ui->focus = ui->capture;
      ui->focus_visible = false;
    } else if (e.type == CUI_EVENT_POINTER_UP) {
      if (ui->capture == hit(ui, e.x, e.y))
        activate(ui, ui->capture);
      ui->capture = 0;
    } else if (e.type == CUI_EVENT_CANCEL) {
      ui->capture = 0;
      ui->space_capture = 0;
    } else if (e.type == CUI_EVENT_KEY_DOWN && !e.repeat) {
      if (e.key == CUI_KEY_TAB)
        tab(ui, (e.modifiers & CUI_MOD_SHIFT) != 0);
      else if (e.key == CUI_KEY_ENTER)
        activate(ui, ui->focus);
      else if (e.key == CUI_KEY_SPACE)
        ui->space_capture = ui->focus;
      else if (e.key == CUI_KEY_ESCAPE) {
        ui->capture = 0;
        ui->space_capture = 0;
      }
    } else if (e.type == CUI_EVENT_KEY_UP && e.key == CUI_KEY_SPACE) {
      if (ui->space_capture == ui->focus)
        activate(ui, ui->focus);
      ui->space_capture = 0;
    }
  }
  ui->node_count = 0;
  ui->overflow_depth = 0;
  ui->focus_depth = ui->focus_scope_count = 0;
  ui->top_modal = SIZE_MAX;
  ui->semantic_count = ui->id_count = ui->element_count = ui->scope_count =
      ui->depth = ui->string_used = 0;
  ui->count = 0;
  select_context(ui);
  Clay_SetLayoutDimensions((Clay_Dimensions){w, h});
  Clay_BeginLayout();
  ui->config.width = w;
  ui->config.height = h;
  ui->dt = dt;
  ui->in_frame = true;
  ui->dirty = false;
  return true;
}
static uint32_t folded(cui_id id) {
  /* Avalanche in a CUI domain: Clay auto IDs combine parent ID + child
     ordinal, so consecutive raw public IDs can alias auto text IDs. */
  uint64_t x = id ^ UINT64_C(0x4355492d454c454d);
  x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
  x ^= x >> 31;
  uint32_t out = (uint32_t)x ^ (uint32_t)(x >> 32);
  return out ? out : 1;
}
static Clay_SizingAxis axis(float n) {
  if (n > 0)
    return (Clay_SizingAxis){.size = {.minMax = {n, n}},
                             .type = CLAY__SIZING_TYPE_FIXED};
  return (Clay_SizingAxis){.size = {.minMax = {0, 0}},
                           .type = n < 0 ? CLAY__SIZING_TYPE_GROW
                                         : CLAY__SIZING_TYPE_FIT};
}
static uint16_t scalar(float x) {
  return !isfinite(x) || x < 0 ? 0 : x > 65535 ? 65535 : (uint16_t)x;
}
void cui_begin_box(cui_context *ui, cui_id id, cui_box b) {
  if (!active(ui))
    return;
  if (ui->depth == STACK_MAX || ui->overflow_depth) {
    ui->overflow_depth++;
    fail(ui, CUI_ERROR_CAPACITY);
    return;
  }
  bool ok = ui->element_count < ui->config.max_elements &&
            (ui->depth == 0 || ui->opened[ui->depth - 1]);
  if (id)
    for (size_t i = 0; i < ui->id_count; i++)
      if (ui->ids[i] == id || folded(ui->ids[i]) == folded(id)) {
        fail(ui, CUI_ERROR_DUPLICATE_ID);
        ok = false;
        break;
      }
  ui->opened[ui->depth++] = ok;
  if (!ok) {
    if (ui->element_count >= ui->config.max_elements)
      fail(ui, CUI_ERROR_CAPACITY);
    return;
  }
  ui->element_count++;
  if (id)
    ui->ids[ui->id_count++] = id;
  uint32_t internal_id =
      folded(id ? id : (cui_id)(ui->element_count + 0x80000000u));
  size_t parent = ui->depth > 1 ? ui->node_stack[ui->depth - 2] : SIZE_MAX;
  ui->nodes[ui->node_count] = (box_node){internal_id, parent, b.clip};
  ui->node_stack[ui->depth - 1] = ui->node_count++;
  Clay__OpenElementWithId((Clay_ElementId){.id = internal_id});
  uint16_t bw = scalar(b.border_width);
  Clay__ConfigureOpenElement((Clay_ElementDeclaration){
      .layout = {.sizing = {axis(b.width), axis(b.height)},
                 .padding = CLAY_PADDING_ALL(scalar(b.padding)),
                 .childGap = scalar(b.gap),
                 .layoutDirection =
                     b.row ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM},
      .backgroundColor = {b.background.r, b.background.g, b.background.b,
                          b.background.a},
      .cornerRadius = CLAY_CORNER_RADIUS(b.radius),
      .clip = {.horizontal = b.clip, .vertical = b.clip},
      .border = {.color = {b.border.r, b.border.g, b.border.b, b.border.a},
                 .width = {bw, bw, bw, bw, 0}}});
}
void cui_end_box(cui_context *ui) {
  if (!active(ui))
    return;
  if (ui->overflow_depth) {
    ui->overflow_depth--;
    return;
  }
  if (!ui->depth) {
    fail(ui, CUI_ERROR_LIFECYCLE);
    return;
  }
  if (ui->opened[--ui->depth])
    Clay__CloseElement();
}
static cui_str copy_string(cui_context *ui, cui_str s) {
  if (!s.length)
    return (cui_str){"", 0};
  if (!s.data || s.length > (size_t)INT32_MAX ||
      s.length >= ui->config.frame_bytes - ui->string_used) {
    fail(ui, CUI_ERROR_CAPACITY);
    return (cui_str){"", 0};
  }
  char *p = ui->strings + ui->string_used;
  memcpy(p, s.data, s.length);
  p[s.length] = 0;
  ui->string_used += s.length + 1;
  return (cui_str){p, s.length};
}
void cui_text(cui_context *ui, cui_str t, float size, cui_color c) {
  if (!active(ui))
    return;
  if (ui->overflow_depth || (ui->depth && !ui->opened[ui->depth - 1]))
    return;
  if (ui->element_count == ui->config.max_elements) {
    fail(ui, CUI_ERROR_CAPACITY);
    return;
  }
  ui->element_count++;
  t = copy_string(ui, t);
  Clay__OpenTextElement(
      (Clay_String){false, (int32_t)t.length, t.data},
      (Clay_TextElementConfig){.fontSize = scalar(size),
                               .textColor = {c.r, c.g, c.b, c.a}});
}
static uint64_t hash(uint64_t h, const char *s) {
  if (s)
    while (*s) {
      h ^= (unsigned char)*s++;
      h *= 1099511628211ULL;
    }
  return h ? h : 1;
}
cui_id cui_id_from(cui_context *ui, const char *s) {
  return hash(ui && ui->scope_count ? ui->scopes[ui->scope_count - 1]
                                    : 14695981039346656037ULL,
              s);
}
bool cui_push_id(cui_context *ui, const char *key) {
  if (!active(ui))
    return false;
  if (ui->scope_count == STACK_MAX) {
    fail(ui, CUI_ERROR_CAPACITY);
    return false;
  }
  uint64_t h = cui_id_from(ui, key);
  h ^= 255;
  h *= 1099511628211ULL;
  ui->scopes[ui->scope_count++] = h;
  return true;
}
void cui_pop_id(cui_context *ui) {
  if (!active(ui))
    return;
  if (!ui->scope_count)
    fail(ui, CUI_ERROR_LIFECYCLE);
  else
    ui->scope_count--;
}
cui_rect cui_bounds(cui_context *ui, cui_id id) {
  if (!ui || !id)
    return (cui_rect){0};
  for (size_t i = 0; i < ui->previous_count; i++)
    if (ui->previous[i].id == id)
      return ui->previous[i].bounds;
  select_context(ui);
  Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = folded(id)});
  return (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width,
                    d.boundingBox.height};
}
cui_interaction cui_interact(cui_context *ui, cui_id id, cui_role role,
                             cui_str name, bool disabled, bool checked,
                             float value) {
  cui_interaction out = {0};
  if (!active(ui) || !id)
    return out;
  if (ui->overflow_depth || (ui->depth && !ui->opened[ui->depth - 1]))
    return out;
  for (size_t i = 0; i < ui->semantic_count; i++)
    if (ui->semantics[i].id == id) {
      fail(ui, CUI_ERROR_DUPLICATE_ID);
      return out;
    }
  if (ui->semantic_count == ui->config.max_elements) {
    fail(ui, CUI_ERROR_CAPACITY);
    return out;
  }
  if (disabled) {
    if (ui->focus == id)
      ui->focus = 0;
    if (ui->capture == id)
      ui->capture = 0;
    if (ui->space_capture == id)
      ui->space_capture = 0;
  }
  cui_rect bounds = cui_bounds(ui, id);
  if (!disabled) {
    out.hovered = inside(bounds, ui->pointer_x, ui->pointer_y);
    out.pressed = ui->capture == id || ui->space_capture == id;
    out.focused = ui->focus == id;
    out.focus_visible = out.focused && ui->focus_visible;
  }
  for (size_t i = 0; i < ui->activation_count; i++)
    if (ui->activations[i] == id) {
      out.activated = !disabled;
      ui->activations[i] = 0;
    }
  ui->semantic_scopes[ui->semantic_count] =
      ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX;
  ui->semantic_nodes[ui->semantic_count] =
      ui->depth && ui->opened[ui->depth - 1] ? ui->node_stack[ui->depth - 1]
                                             : SIZE_MAX;
  ui->semantics[ui->semantic_count++] =
      (cui_semantic){id,       role,    bounds,      copy_string(ui, name),
                     disabled, checked, out.focused, value};
  return out;
}
static cui_color color(Clay_Color c) {
  return (cui_color){(uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a};
}
cui_draw_list cui_end_frame(cui_context *ui) {
  if (!active(ui))
    return (cui_draw_list){0};
  if (ui->depth || ui->overflow_depth || ui->scope_count || ui->focus_depth)
    fail(ui, CUI_ERROR_LIFECYCLE);
  ui->overflow_depth = 0;
  while (ui->depth)
    cui_end_box(ui);
  Clay_RenderCommandArray a = Clay_EndLayout(ui->dt);
  ui->count = 0;
  for (int32_t i = 0; i < a.length; i++) {
    Clay_RenderCommand *c = &a.internalArray[i];
    cui_draw_command d = {.bounds = {c->boundingBox.x, c->boundingBox.y,
                                     c->boundingBox.width,
                                     c->boundingBox.height}};
    switch (c->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      d.type = CUI_DRAW_RECT;
      d.color = color(c->renderData.rectangle.backgroundColor);
      d.radius = c->renderData.rectangle.cornerRadius.topLeft;
      break;
    case CLAY_RENDER_COMMAND_TYPE_BORDER:
      d.type = CUI_DRAW_BORDER;
      d.color = color(c->renderData.border.color);
      d.radius = c->renderData.border.cornerRadius.topLeft;
      d.stroke = c->renderData.border.width.left;
      break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT:
      d.type = CUI_DRAW_TEXT;
      d.text = (cui_str){c->renderData.text.stringContents.chars,
                         (size_t)c->renderData.text.stringContents.length};
      d.font_size = c->renderData.text.fontSize;
      d.color = color(c->renderData.text.textColor);
      break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
      d.type = CUI_DRAW_CLIP_PUSH;
      break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
      d.type = CUI_DRAW_CLIP_POP;
      break;
    default:
      continue;
    }
    if (ui->count == ui->config.max_commands) {
      fail(ui, CUI_ERROR_CAPACITY);
      ui->count = 0;
      break;
    }
    ui->commands[ui->count++] = d;
  }
  bool retained = false, captured = false, space = false;
  for (size_t i = 0; i < ui->semantic_count; i++) {
    cui_semantic *s = &ui->semantics[i];
    Clay_ElementData d =
        Clay_GetElementData((Clay_ElementId){.id = folded(s->id)});
    s->bounds = (cui_rect){d.boundingBox.x, d.boundingBox.y,
                           d.boundingBox.width, d.boundingBox.height};
    cui_rect clipped = s->bounds;
    for (size_t n = ui->semantic_nodes[i]; n != SIZE_MAX;
         n = ui->nodes[n].parent) {
      if (ui->nodes[n].clip) {
        Clay_ElementData cd =
            Clay_GetElementData((Clay_ElementId){.id = ui->nodes[n].clay_id});
        float left = fmaxf(clipped.x, cd.boundingBox.x),
              top = fmaxf(clipped.y, cd.boundingBox.y);
        float right = fminf(clipped.x + clipped.w,
                            cd.boundingBox.x + cd.boundingBox.width),
              bottom = fminf(clipped.y + clipped.h,
                             cd.boundingBox.y + cd.boundingBox.height);
        clipped = (cui_rect){left, top, fmaxf(0, right - left),
                             fmaxf(0, bottom - top)};
      }
    }
    ui->previous_hits[i] = clipped;
    if (focusable(s)) {
      retained |= s->id == ui->focus;
      if (scope_contains(ui->focus_scopes, ui->semantic_scopes[i],
                         ui->top_modal)) {
        captured |= s->id == ui->capture;
        space |= s->id == ui->space_capture;
      }
    }
    s->focused = s->id == ui->focus && !s->disabled;
  }
  if (!retained)
    ui->focus = 0;
  if (ui->top_modal != SIZE_MAX) {
    bool allowed = false;
    for (size_t i = 0; i < ui->semantic_count; i++)
      if (ui->semantics[i].id == ui->focus && focusable(&ui->semantics[i]) &&
          scope_contains(ui->focus_scopes, ui->semantic_scopes[i],
                         ui->top_modal))
        allowed = true;
    if (!allowed) {
      ui->focus = 0;
      for (size_t i = 0; i < ui->semantic_count; i++)
        if (focusable(&ui->semantics[i]) &&
            scope_contains(ui->focus_scopes, ui->semantic_scopes[i],
                           ui->top_modal)) {
          ui->focus = ui->semantics[i].id;
          ui->focus_visible = true;
          ui->dirty = true;
          break;
        }
    }
  }
  for (size_t i = 0; i < ui->semantic_count; i++)
    ui->semantics[i].focused =
        ui->semantics[i].id == ui->focus && !ui->semantics[i].disabled;
  if (!captured)
    ui->capture = 0;
  if (!space)
    ui->space_capture = 0;
  memcpy(ui->previous_focus_scopes, ui->focus_scopes,
         ui->focus_scope_count * sizeof(*ui->focus_scopes));
  ui->previous_focus_scope_count = ui->focus_scope_count;
  ui->previous_top_modal = ui->top_modal;
  memcpy(ui->previous_semantic_scopes, ui->semantic_scopes,
         ui->semantic_count * sizeof(*ui->semantic_scopes));
  memcpy(ui->previous, ui->semantics,
         ui->semantic_count * sizeof(*ui->previous));
  ui->previous_count = ui->semantic_count;
  ui->in_frame = false;
  return (cui_draw_list){ui->commands, ui->count, ui->config.width,
                         ui->config.height};
}
const cui_semantic *cui_semantics(cui_context *ui, size_t *count) {
  if (count)
    *count = ui ? ui->semantic_count : 0;
  return ui ? ui->semantics : NULL;
}
cui_id cui_focused(cui_context *ui) { return ui ? ui->focus : 0; }
void cui_focus(cui_context *ui, cui_id id) {
  if (ui) {
    ui->focus = id;
    ui->focus_visible = true;
    ui->dirty = true;
  }
}
const cui_event *cui_frame_events(cui_context *ui, size_t *count) {
  if (count)
    *count = ui ? ui->event_count : 0;
  return ui ? ui->events : NULL;
}
cui_error cui_last_error(cui_context *ui) {
  return ui ? ui->error : CUI_ERROR_LIFECYCLE;
}
void cui_clear_error(cui_context *ui) {
  if (ui)
    ui->error = CUI_OK;
}
bool cui_needs_redraw(cui_context *ui) { return ui && ui->dirty; }
void cui_invalidate(cui_context *ui) {
  if (ui)
    ui->dirty = true;
}

void cui_update_semantic(cui_context *ui, cui_id id, bool checked,
                         float value) {
  if (!active(ui))
    return;
  for (size_t i = 0; i < ui->semantic_count; i++)
    if (ui->semantics[i].id == id) {
      ui->semantics[i].checked = checked;
      ui->semantics[i].value = value;
      return;
    }
}
cui_id cui_event_target(cui_context *ui, size_t index) {
  return ui && index < ui->event_count ? ui->event_targets[index] : 0;
}

bool cui_begin_focus_scope(cui_context *ui, cui_id id, bool modal) {
  if (!active(ui))
    return false;
  if (!id) {
    fail(ui, CUI_ERROR_LIFECYCLE);
    return false;
  }
  if (ui->focus_depth == STACK_MAX || ui->focus_scope_count == STACK_MAX) {
    fail(ui, CUI_ERROR_CAPACITY);
    return false;
  }
  for (size_t i = 0; i < ui->focus_scope_count; i++)
    if (ui->focus_scopes[i].id == id) {
      fail(ui, CUI_ERROR_DUPLICATE_ID);
      return false;
    }
  size_t n = ui->focus_scope_count++;
  ui->focus_scopes[n] = (focus_scope){
      id, ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX,
      modal};
  ui->focus_stack[ui->focus_depth++] = n;
  if (modal)
    ui->top_modal = n;
  return true;
}
void cui_end_focus_scope(cui_context *ui) {
  if (!active(ui))
    return;
  if (!ui->focus_depth)
    fail(ui, CUI_ERROR_LIFECYCLE);
  else
    ui->focus_depth--;
}
bool cui_focus_restore(cui_context *ui, cui_id trigger) {
  if (!ui)
    return false;
  for (size_t i = 0; i < ui->previous_count; i++)
    if (ui->previous[i].id == trigger && focusable(&ui->previous[i])) {
      ui->focus = trigger;
      ui->focus_visible = true;
      ui->dirty = true;
      return true;
    }
  ui->focus = 0;
  return false;
}
