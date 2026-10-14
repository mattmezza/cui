/* SPDX-License-Identifier: MIT */
#include "cui/cui_regex.h"
#include <assert.h>
#include <string.h>
int main(void) {
    for (unsigned cycle = 0; cycle < 1000; cycle++) {
        cui_regex regex = {0};
        cui_regex_session s = {0};
        assert(cui_regex_compile(cui_string("\\p{Greek}+"), (cui_regex_options){0}, &regex) ==
               CUI_REGEX_OK);
        assert(cui_regex_session_create(&regex, 1048576, &s) == CUI_REGEX_OK);
        cui_text_match m = cui_regex_matcher(&s);
        void *lease = NULL;
        assert(m.ops.begin(m.object, m.generation, &lease) == CUI_TEXT_MATCH_OK && lease);
        void *unchanged = (void *)(uintptr_t)123;
        assert(m.ops.begin(m.object, m.generation, &unchanged) == CUI_TEXT_MATCH_BUSY &&
               unchanged == (void *)(uintptr_t)123);
        assert(cui_regex_session_destroy(&s) == CUI_REGEX_BUSY);
        assert(cui_regex_session_rebind(&s, &regex, 0) == CUI_REGEX_BUSY);
        bool found = false;
        assert(m.ops.match(lease, cui_string("ΔΣ"), (cui_text_match_buffer){0}, &found) ==
                   CUI_TEXT_MATCH_OK &&
               found);
        found = true;
        assert(m.ops.match(lease, (cui_str){"\xff", 1}, (cui_text_match_buffer){0}, &found) ==
                   CUI_TEXT_MATCH_INVALID &&
               found);
        assert(m.ops.match(lease, cui_string("123"), (cui_text_match_buffer){0}, &found) ==
                   CUI_TEXT_MATCH_OK &&
               !found);
        assert(m.ops.end(m.object, lease) == CUI_TEXT_MATCH_OK);
        assert(m.ops.end(m.object, lease) == CUI_TEXT_MATCH_STALE);
        uint64_t generation = s.generation;
        void *state = s.private_state;
        assert(cui_regex_session_rebind(&s, &regex, 0) == CUI_REGEX_CAPACITY &&
               s.generation == generation && s.private_state == state);
        assert(cui_regex_session_rebind(&s, &regex, 1048576) == CUI_REGEX_OK &&
               s.generation != generation);
        lease = NULL;
        assert(m.ops.begin(m.object, m.generation, &lease) == CUI_TEXT_MATCH_STALE && !lease);
        assert(cui_regex_destroy(&regex) == CUI_REGEX_OK);
        m = cui_regex_matcher(&s);
        assert(m.ops.begin(m.object, m.generation, &lease) == CUI_TEXT_MATCH_OK);
        assert(m.ops.match(lease, cui_string("Σ"), (cui_text_match_buffer){0}, &found) ==
                   CUI_TEXT_MATCH_OK &&
               found);
        assert(m.ops.end(m.object, lease) == CUI_TEXT_MATCH_OK);
        assert(cui_regex_session_destroy(&s) == CUI_REGEX_OK &&
               cui_regex_session_destroy(&s) == CUI_REGEX_OK);
    }
    cui_regex r = {0};
    assert(cui_regex_compile(cui_string("["), (cui_regex_options){0}, &r) == CUI_REGEX_PATTERN &&
           !r.private_state && !r.generation);
    assert(cui_regex_compile(cui_string("a"), (cui_regex_options){.flags = 1u << 31}, &r) ==
               CUI_REGEX_INVALID &&
           !r.private_state);
    assert(cui_regex_compile((cui_str){"\xff", 1}, (cui_regex_options){0}, &r) ==
               CUI_REGEX_INVALID &&
           !r.private_state);
    assert(cui_regex_compile(cui_string("a"),
                             (cui_regex_options){.max_pattern_bytes = 1, .max_text_bytes = 1},
                             &r) == CUI_REGEX_OK);
    cui_regex_session s = {0};
    assert(cui_regex_session_create(&r, 1048576, &s) == CUI_REGEX_OK);
    cui_text_match m = cui_regex_matcher(&s);
    void *lease = NULL;
    assert(m.ops.begin(m.object, m.generation, &lease) == CUI_TEXT_MATCH_OK);
    bool found = true;
    assert(m.ops.match(lease, cui_string("aa"), (cui_text_match_buffer){0}, &found) ==
               CUI_TEXT_MATCH_CAPACITY &&
           found);
    assert(m.ops.match(lease, (cui_str){(char *)&found, 1}, (cui_text_match_buffer){0}, &found) ==
               CUI_TEXT_MATCH_INVALID &&
           found);
    assert(m.ops.end(m.object, lease) == CUI_TEXT_MATCH_OK);
    assert(cui_regex_session_destroy(&s) == CUI_REGEX_OK);
    assert(cui_regex_destroy(&r) == CUI_REGEX_OK);
    cui_regex empty = {0};
    assert(cui_regex_compile((cui_str){NULL, 1}, (cui_regex_options){0}, &empty) ==
           CUI_REGEX_INVALID);
    assert(cui_regex_compile((cui_str){(const char *)(uintptr_t)(UINTPTR_MAX - 1), 8},
                             (cui_regex_options){0}, &empty) == CUI_REGEX_INVALID);
    assert(!empty.private_state && !empty.generation);
    assert(cui_regex_compile(cui_string("a"), (cui_regex_options){0}, &empty) == CUI_REGEX_OK);
    void *saved_state = empty.private_state;
    uint64_t saved_generation = empty.generation;
    assert(cui_regex_session_create(&empty, 1048576, (cui_regex_session *)(void *)&empty) ==
           CUI_REGEX_INVALID);
    assert(empty.private_state == saved_state && empty.generation == saved_generation);
    assert(cui_regex_destroy(&empty) == CUI_REGEX_OK);
    return 0;
}
