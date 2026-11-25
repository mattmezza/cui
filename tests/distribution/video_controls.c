/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIST_ALL_SHARED
#define CUI_IMPLEMENTATION_ALL
#endif
#include "cui_all.h"
#include <assert.h>
int main(void) {
    cui_video_controls_options options = cui_video_controls_defaults();
    assert(options.visible & CUI_VIDEO_CONTROL_PLAY);
    assert(options.play_icon.length && options.pause_icon.length);
    assert(cui_video_part_id(42, CUI_VIDEO_PART_PLAY));
    assert(cui_video_part_id(42, CUI_VIDEO_PART_PLAY) !=
           cui_video_part_id(42, CUI_VIDEO_PART_TIMELINE));
    assert(!cui_video_controls(NULL, 42, NULL, NULL, (cui_video_view){0}, options));
    assert(cui_video_part_id(42, CUI_VIDEO_PART_MUTE) !=
           cui_video_part_id(42, CUI_VIDEO_PART_VOLUME));
    assert(!cui_video_controls_with_audio(NULL, 42, NULL, NULL, (cui_video_view){0}, options,
                                          &(cui_video_audio_view){0}, NULL));
    return 0;
}
