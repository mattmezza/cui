/* SPDX-License-Identifier: MIT */
/* Optional private bridge role mapping. Compile with function/data sections and
 * --gc-sections; no Rust dependency is needed for this pure mapping probe. */
#include "../../backends/accesskit/cui_accessibility.c"
#include <assert.h>
int main(void) {
    cui_semantic s = {.role = CUI_ROLE_TEXTBOX};
    assert(role(s) == ACCESSKIT_ROLE_TEXT_INPUT);
    s.details.secret = true;
    assert(role(s) == ACCESSKIT_ROLE_TEXT_INPUT);
    s.details.password = true;
    assert(role(s) == ACCESSKIT_ROLE_PASSWORD_INPUT);
    s.details.readonly = true;
    assert(role(s) == ACCESSKIT_ROLE_PASSWORD_INPUT);
    s.details.secret = false;
    assert(role(s) == ACCESSKIT_ROLE_PASSWORD_INPUT);
    s.details.password = false;
    s.details.multiline = true;
    assert(role(s) == ACCESSKIT_ROLE_MULTILINE_TEXT_INPUT);
    return 0;
}
