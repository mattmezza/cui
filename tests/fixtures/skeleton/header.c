/* Compile as strict C11 and C++11, standalone and after content header. */
#ifdef CUI_SKELETON_CONTENT_FIRST
#include "cui/cui_content.h"
#endif
#include "cui/cui_skeleton_palette.h"
#include "cui/cui_content.h"
int main(void) {
    cui_skeleton_palette palette = {{21, 34, 55, 0}};
    cui_skeleton_palette copy = palette;
    return copy.background.r == 21 && copy.background.a == 0 ? 0 : 1;
}
