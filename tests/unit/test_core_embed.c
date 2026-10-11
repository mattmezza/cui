/* Simulate embedding implementation into an application translation unit
   that already has ordinary helper names and types. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  int value;
} box_node;
typedef struct {
  int value;
} focus_scope;
#define STACK_MAX 7
#define EVENT_MAX 11
static int fail(void) { return 1; }
static int select_context(void) { return 1; }
static int active(void) { return 1; }
static int measure(void) { return 1; }
static int error(void) { return 1; }
static int clay_error(void) { return 1; }
static int inside(void) { return 1; }
static int focusable(void) { return 1; }
static int scope_contains(void) { return 1; }
static int previous_allowed(void) { return 1; }
static int hit(void) { return 1; }
static int activate(void) { return 1; }
static int tab(void) { return 1; }
static int folded(void) { return 1; }
static int axis(void) { return 1; }
static int scalar(void) { return 1; }
static int copy_string(void) { return 1; }
static int hash(void) { return 1; }
static int color(void) { return 1; }
typedef struct {
  int value;
} snapshot;
typedef struct {
  int value;
} key_slot;
static int decode(void) { return 1; }
static int accepted(void) { return 1; }
static int boundary(void) { return 1; }
static int save_state(void) { return 1; }
static int record(void) { return 1; }
static int previous(void) { return 1; }
static int next(void) { return 1; }
static int replace(void) { return 1; }
static int erase(void) { return 1; }
static int restore(void) { return 1; }
static int mix(void) { return 1; }
static int lookup(void) { return 1; }
static int add_key(void) { return 1; }
static int extent(void) { return 1; }
static int clamp_scroll(void) { return 1; }
#include "../../src/base/cui_edit.c"
#include "../../src/base/cui_virtual.c"
#include "../../src/core/cui.c"
#ifdef CLAY_IMPLEMENTATION
#error Core leaked CLAY_IMPLEMENTATION
#endif
int main(void) {
  snapshot app_snapshot = {1};
  key_slot app_slot = {1};
  assert(app_snapshot.value + app_slot.value == 2);
  assert(decode() && accepted() && boundary() && save_state() && record() &&
         previous() && next() && replace() && erase() && restore() && mix() &&
         lookup() && add_key() && extent() && clamp_scroll());
  box_node b = {1};
  focus_scope s = {1};
  assert(b.value + s.value + STACK_MAX + EVENT_MAX == 20);
  assert(fail() + select_context() + active() + measure() + error() +
             clay_error() + inside() + focusable() + scope_contains() +
             previous_allowed() + hit() + activate() + tab() + folded() +
             axis() + scalar() + copy_string() + hash() + color() ==
         19);
  cui_context *ui = cui_create((cui_config){0});
  assert(ui);
  assert(cui_begin_frame(ui, 100, 100, 0));
  cui_begin_box(ui, 1, (cui_box){.width = 20, .height = 20});
  cui_text(ui, cui_string("embed"), 10, (cui_color){1, 2, 3, 255});
  cui_end_box(ui);
  cui_end_frame(ui);
  assert(cui_last_error(ui) == CUI_OK);
  cui_destroy(ui);
  char buffer[32] = "";
  cui_edit *editor = cui_edit_create(
      (cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
  assert(editor);
  assert(cui_edit_insert(editor, cui_string("safe")) == CUI_EDIT_OK);
  cui_edit_destroy(editor);
  cui_virtual_list *list = cui_virtual_create((cui_virtual_config){4, 10});
  assert(list);
  uint64_t keys[] = {1, 2};
  assert(cui_virtual_rebuild(list, keys, NULL, 2) == CUI_VIRTUAL_OK);
  assert(cui_virtual_total(list) == 20);
  cui_virtual_destroy(list);
  return 0;
}
