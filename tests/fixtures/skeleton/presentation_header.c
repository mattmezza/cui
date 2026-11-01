/* Compile C11/C++11 before and after existing content/palette declarations. */
#ifdef CUI_SKELETON_CONTENT_FIRST
#include "cui/cui_content.h"
#include "cui/cui_skeleton_palette.h"
#endif
#include "cui/cui_skeleton_presentation.h"
int main(void) {
    cui_skeleton_presentation p = cui_skeleton_presentation_default();
    return p.root.width == 1 && p.root.width_percent && p.root.height == 16 ? 0 : 1;
}
