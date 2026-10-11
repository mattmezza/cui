/* SPDX-License-Identifier: MIT */
#ifndef CUI_EDIT_H
#define CUI_EDIT_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cui_edit cui_edit;
typedef enum {
  CUI_EDIT_OK,
  CUI_EDIT_INVALID_UTF8,
  CUI_EDIT_CAPACITY,
  CUI_EDIT_BOUNDARY,
  CUI_EDIT_NO_HISTORY,
  CUI_EDIT_INVALID_ARGUMENT,
  CUI_EDIT_LINE_BREAK
} cui_edit_result;
typedef struct {
  char *buffer;
  size_t capacity;
  bool multiline;
  size_t history_bytes, max_history_steps;
} cui_edit_config;
typedef struct {
  size_t length, cursor, anchor;
  bool can_undo, can_redo;
  uint64_t revision;
  bool multiline;
} cui_edit_state;
typedef enum {
  CUI_EDIT_PREVIOUS,
  CUI_EDIT_NEXT,
  CUI_EDIT_LINE_START,
  CUI_EDIT_LINE_END,
  CUI_EDIT_START,
  CUI_EDIT_END
} cui_edit_motion;
/* The application owns the NUL-terminated buffer. Only this model may mutate
   it while attached. Capacity includes NUL; offsets are UTF-8 byte offsets. */
cui_edit *cui_edit_create(cui_edit_config config);
void cui_edit_destroy(cui_edit *edit);
cui_edit_state cui_edit_get_state(const cui_edit *edit);
cui_str cui_edit_get_text(const cui_edit *edit);
bool cui_edit_valid_utf8(cui_str text);
cui_edit_result cui_edit_select(cui_edit *edit, size_t anchor, size_t cursor);
cui_edit_result cui_edit_move(cui_edit *edit, cui_edit_motion motion,
                              bool extend);
cui_edit_result cui_edit_insert(cui_edit *edit, cui_str text);
cui_edit_result cui_edit_backspace(cui_edit *edit);
cui_edit_result cui_edit_delete(cui_edit *edit);
cui_edit_result cui_edit_undo(cui_edit *edit);
cui_edit_result cui_edit_redo(cui_edit *edit);
/* Applies events already routed to id; returns the first rejected edit while
   continuing independent later events. Disabled input is ignored. */
cui_edit_result cui_edit_apply_events(cui_edit *edit, cui_context *ui,
                                      cui_id id, bool disabled);
#ifdef __cplusplus
}
#endif
#endif
