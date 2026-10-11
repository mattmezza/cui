#include "cui/cui_virtual.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static void near(double a, double b) { assert(fabs(a - b) < 1e-7); }
static void test_boundaries(void) {
  cui_virtual_list *v = cui_virtual_create((cui_virtual_config){8, 10});
  assert(v);
  uint64_t keys[] = {0, 2, 3, 4};
  double heights[] = {10, 20, 30, 40};
  assert(cui_virtual_rebuild(v, keys, heights, 4) == CUI_VIRTUAL_OK);
  near(cui_virtual_total(v), 100);
  near(cui_virtual_offset(v, 2), 30);
  assert(cui_virtual_lower_bound(v, 0) == 0);
  assert(cui_virtual_lower_bound(v, 9) == 0);
  assert(cui_virtual_lower_bound(v, 10) == 1);
  assert(cui_virtual_lower_bound(v, 30) == 2);
  assert(cui_virtual_lower_bound(v, 100) == 4);
  cui_virtual_view r = cui_virtual_viewport(v, 10, 20, 0);
  assert(r.begin == 1 && r.end == 2);
  near(r.before, 10);
  near(r.after, 70);
  r = cui_virtual_viewport(v, 10, 21, 0);
  assert(r.begin == 1 && r.end == 3);
  r = cui_virtual_viewport(v, 10, 20, SIZE_MAX);
  assert(r.begin == 0 && r.end == 4);
  r = cui_virtual_viewport(v, 500, 20, 0);
  near(r.scroll, 80);
  assert(r.begin == 3 && r.end == 4);
  r = cui_virtual_viewport(v, 0, 200, 0);
  assert(r.begin == 0 && r.end == 4);
  near(r.scroll, 0);
  assert(cui_virtual_set_key_height(v, 2, 25) == CUI_VIRTUAL_OK);
  near(cui_virtual_total(v), 105);
  near(cui_virtual_offset(v, 2), 35);
  assert(cui_virtual_set_key_height(v, 999, 10) == CUI_VIRTUAL_NOT_FOUND);
  cui_virtual_anchor a = cui_virtual_capture_anchor(v, 40);
  assert(a.valid && a.key == 3);
  near(a.within, 5);
  uint64_t reorder[] = {99, 4, 3, 0, 2};
  assert(cui_virtual_rebuild(v, reorder, NULL, 5) == CUI_VIRTUAL_OK);
  near(cui_virtual_height(v, 0), 10);
  near(cui_virtual_height(v, 1), 40);
  near(cui_virtual_height(v, 4), 25);
  near(cui_virtual_restore_anchor(v, a, 20), 55);
  uint64_t removed[] = {99, 4, 0, 2};
  assert(cui_virtual_rebuild(v, removed, NULL, 4) == CUI_VIRTUAL_OK);
  near(cui_virtual_restore_anchor(v, a, 20), 55);
  uint64_t duplicate[] = {1, 1};
  assert(cui_virtual_rebuild(v, duplicate, NULL, 2) ==
         CUI_VIRTUAL_DUPLICATE_KEY);
  assert(cui_virtual_count(v) == 4);
  near(cui_virtual_height(v, 1), 40);
  assert(cui_virtual_set_height(v, 0, NAN) == CUI_VIRTUAL_INVALID);
  assert(cui_virtual_set_height(v, 0, 0) == CUI_VIRTUAL_INVALID);
  assert(cui_virtual_set_height(v, 0, -1) == CUI_VIRTUAL_INVALID);
  assert(cui_virtual_set_height(v, 0, DBL_MAX) == CUI_VIRTUAL_INVALID);
  double invalid[] = {10, 0};
  assert(cui_virtual_rebuild(v, reorder, invalid, 2) == CUI_VIRTUAL_INVALID);
  assert(cui_virtual_count(v) == 4);
  assert(cui_virtual_rebuild(v, NULL, NULL, 0) == CUI_VIRTUAL_OK);
  assert(!cui_virtual_capture_anchor(v, 0).valid);
  r = cui_virtual_viewport(v, 100, 20, 2);
  assert(r.begin == 0 && r.end == 0);
  cui_virtual_destroy(v);
  assert(!cui_virtual_create((cui_virtual_config){SIZE_MAX, 10}));
  assert(!cui_virtual_create((cui_virtual_config){4, NAN}));
  assert(!cui_virtual_create((cui_virtual_config){4, 0}));
}
static unsigned rng = 7;
static unsigned random_number(void) {
  rng = rng * 1664525u + 1013904223u;
  return rng;
}
static void test_random_reference(void) {
  enum { N = 257 };
  uint64_t keys[N];
  double heights[N];
  for (size_t i = 0; i < N; i++) {
    keys[i] = (uint64_t)i * 173;
    heights[i] = (double)(i % 47 + 1);
  }
  cui_virtual_list *v = cui_virtual_create((cui_virtual_config){N, 10});
  assert(v);
  assert(cui_virtual_rebuild(v, keys, heights, N) == CUI_VIRTUAL_OK);
  for (size_t step = 0; step < 20000; step++) {
    size_t change = random_number() % N;
    heights[change] = (double)(random_number() % 100 + 1);
    assert(cui_virtual_set_height(v, change, heights[change]) ==
           CUI_VIRTUAL_OK);
    size_t index = random_number() % (N + 1);
    double sum = 0;
    for (size_t i = 0; i < index; i++)
      sum += heights[i];
    near(cui_virtual_offset(v, index), sum);
    double total = 0;
    for (size_t i = 0; i < N; i++)
      total += heights[i];
    near(cui_virtual_total(v), total);
    double target = (double)(random_number() % ((unsigned)total + 100));
    size_t expected = 0;
    sum = 0;
    while (expected < N && sum + heights[expected] <= target) {
      sum += heights[expected];
      expected++;
    }
    assert(cui_virtual_lower_bound(v, target) == expected);
    cui_virtual_view r = cui_virtual_viewport(v, target, 123, 2);
    assert(r.begin <= r.end && r.end <= N);
    near(r.before, cui_virtual_offset(v, r.begin));
    near(r.after, total - cui_virtual_offset(v, r.end));
    assert(r.scroll >= 0 && r.scroll <= fmax(0, total - 123));
  }
  cui_virtual_destroy(v);
}
static void benchmark(void) {
  const size_t counts[] = {1000, 10000, 100000};
  volatile double checksum = 0;
  for (size_t k = 0; k < 3; k++) {
    size_t n = counts[k];
    uint64_t *keys = malloc(n * sizeof(*keys));
    double *heights = malloc(n * sizeof(*heights));
    assert(keys && heights);
    for (size_t i = 0; i < n; i++) {
      keys[i] = i;
      heights[i] = 20 + (double)(i % 31);
    }
    cui_virtual_list *v = cui_virtual_create((cui_virtual_config){n, 20});
    assert(v);
    clock_t start = clock();
    assert(cui_virtual_rebuild(v, keys, heights, n) == CUI_VIRTUAL_OK);
    double build_ms = (double)(clock() - start) * 1000 / CLOCKS_PER_SEC;
    start = clock();
    for (size_t i = 0; i < 100000; i++) {
      size_t index = (i * 7919) % n;
      checksum += cui_virtual_offset(v, index);
      checksum += (double)cui_virtual_lower_bound(
          v, (double)((i * 104729) % ((size_t)cui_virtual_total(v))));
      assert(cui_virtual_set_height(v, index, 20 + (double)(i % 53)) ==
             CUI_VIRTUAL_OK);
    }
    double operations_ms = (double)(clock() - start) * 1000 / CLOCKS_PER_SEC;
    printf("virtual %zu rows: rebuild %.3f ms; 100000 "
           "offset+lower-bound+update triples %.3f ms; %zu bytes\n",
           n, build_ms, operations_ms, cui_virtual_memory_bytes(v));
    cui_virtual_destroy(v);
    free(keys);
    free(heights);
  }
  assert(checksum > 0);
}
int main(int argc, char **argv) {
  (void)argv;
  test_boundaries();
  test_random_reference();
  if (argc > 1)
    benchmark();
  puts("virtual tests passed");
  return 0;
}
