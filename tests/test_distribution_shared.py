#!/usr/bin/env python3
"""Offline structural guard for private umbrella test object consolidation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONSUMERS = ('dist_video_controls', 'dist_dropdown_button_styles_all',
             'dist_dropdown_button_metadata_all', 'dist_questionnaire_geometry_all',
             'dist_questionnaire_presentation_all', 'dist_questionnaire_progress',
             'dist_questionnaire_observer', 'dist_date_time_all',
             'dist_stepper_visual_all', 'dist_popover_surface')


def validate(cmake, provider, independent):
    errors = []
    if provider != '/* SPDX-License-Identifier: MIT */\n#define CUI_IMPLEMENTATION_ALL\n#include "cui_all.h"\n':
        errors.append('provider must compile shipped complete implementation only')
    if '#define CUI_IMPLEMENTATION_ALL\n#include "cui_all.h"' not in independent or 'int main(void)' not in independent:
        errors.append('independent same-TU smoke required')
    block = cmake.split('# Private test-only umbrella implementation.', 1)[-1]
    if 'add_library(dist_all_implementation OBJECT tests/distribution/all_implementation.c)' not in block:
        errors.append('private object provider required')
    if 'cui_warnings(dist_all_implementation)' not in block:
        errors.append('provider must retain warning/assertion/sanitizer policy')
    if 'target_compile_definitions(${target} PRIVATE CUI_DIST_ALL_SHARED=1)' not in block or 'target_link_libraries(${target} PRIVATE dist_all_implementation)' not in block:
        errors.append('consumers require declaration mode and shared implementation')
    listed = block.split('foreach(target', 1)[-1].split(')', 1)[0].split()
    if listed != list(CONSUMERS):
        errors.append('only approved feature consumers may share implementation')
    if any(x in block for x in ('-O0', '-O1', '/Od', 'install(', 'PRIVATE cui)')):
        errors.append('no lowered optimization, export or source-library fallback')
    return errors


class DistributionSharedTests(unittest.TestCase):
    def test_actual_contract(self):
        self.assertEqual(validate((ROOT/'CMakeLists.txt').read_text(),
                                 (ROOT/'tests/distribution/all_implementation.c').read_text(),
                                 (ROOT/'tests/distribution/all.c').read_text()), [])

    def test_feature_sources_keep_direct_implementation_default(self):
        fixtures = ('video_controls.c', 'dropdown_button_styles_all.c',
                    'dropdown_button_metadata_all.c', 'questionnaire_geometry_all.c',
                    'questionnaire_presentation_all.c', 'questionnaire_progress.c',
                    'questionnaire_observer.c', 'date_time_picker.c', 'stepper_visual.c')
        for name in fixtures:
            with self.subTest(name=name):
                text = (ROOT/'tests/distribution'/name).read_text()
                self.assertIn('#ifndef CUI_DIST_ALL_SHARED\n#define CUI_IMPLEMENTATION_ALL\n#endif\n#include "cui_all.h"', text)
                self.assertNotIn('cui_internal', text)
                self.assertNotIn('../src/', text)
        cmake = (ROOT/'CMakeLists.txt').read_text()
        self.assertIn('#ifndef CUI_DIST_ALL_SHARED\\n#define CUI_IMPLEMENTATION_ALL\\n#endif\\n', cmake)

    def test_rejects_coverage_reduction(self):
        cmake=(ROOT/'CMakeLists.txt').read_text()
        provider=(ROOT/'tests/distribution/all_implementation.c').read_text()
        independent=(ROOT/'tests/distribution/all.c').read_text()
        for bad in (cmake.replace('OBJECT tests/distribution/all_implementation.c', 'STATIC tests/distribution/all_implementation.c'),
                    cmake.replace('cui_warnings(dist_all_implementation)', ''),
                    cmake.replace(' dist_video_controls\n', ' dist_all\n'),
                    cmake+'\n# -O0\n'):
            self.assertTrue(validate(bad,provider,independent))
        self.assertTrue(validate(cmake, provider.replace('CUI_IMPLEMENTATION_ALL','CUI_IMPLEMENTATION'), independent))
        self.assertTrue(validate(cmake,provider,independent.replace('#define CUI_IMPLEMENTATION_ALL','')))


if __name__ == '__main__':
    unittest.main()
