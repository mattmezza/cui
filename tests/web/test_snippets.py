#!/usr/bin/env python3
"""Check gallery usage coverage and compile representative displayed excerpts."""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'web/gallery.js'
EXTRACT = r'''
const fs=require('fs'),vm=require('vm');
const source=fs.readFileSync(process.argv[1],'utf8');
const pick=(name)=>{const start=source.indexOf('const '+name+' = {');
 if(start<0)throw Error(name+' missing');
 return source.slice(start,name==='stories'?source.indexOf('};',start)+2:source.indexOf('\n};',start)+3);};
const context={};vm.runInNewContext(pick('stories')+'\n'+pick('snippets')+'\nglobalThis.data={stories,snippets};',context);
process.stdout.write(JSON.stringify(context.data));
'''


def arguments(source, opening):
    """Count top-level call arguments, including nested C compound literals."""
    depth = 0
    count = 1
    for offset, char in enumerate(source[opening + 1:], opening + 1):
        if char in '([{':
            depth += 1
        elif char in ')]}':
            if char == ')' and depth == 0:
                return 0 if source[opening + 1:offset].strip() in ('', 'void') else count
            depth -= 1
        elif char == ',' and depth == 0:
            count += 1
    raise AssertionError('unclosed C call')


def main():
    data = json.loads(subprocess.check_output(['node', '-e', EXTRACT, str(SOURCE)], text=True))
    stories, snippets = data['stories'], data['snippets']
    missing = sorted(set(stories) - set(snippets))
    assert not missing, f'Usage excerpts missing for {missing}'
    assert len(stories) == 78, len(stories)
    headers = '\n'.join(path.read_text() for path in (ROOT / 'include/cui').glob('*.h'))
    declarations = re.sub(r'/\*.*?\*/|//[^\n]*', '', headers, flags=re.S)
    for story, snippet in snippets.items():
        assert snippet.strip() and 'No complete styled browser example' not in snippet, story
        calls = re.sub(r'/\*.*?\*/|//[^\n]*', '', snippet, flags=re.S)
        for match in re.finditer(r'\b(cui_[A-Za-z_0-9]+)\s*\(', calls):
            symbol = match.group(1)
            assert re.search(r'\b' + symbol + r'\s*\(', headers), (story, symbol)
            prototype = re.search(r'\b' + symbol + r'\s*\(', declarations)
            assert prototype, (story, symbol)
            expected = arguments(declarations, prototype.end() - 1)
            actual = arguments(calls, match.end() - 1)
            assert actual == expected, (story, symbol, actual, expected)
        for token in set(re.findall(r'\b(CUI_[A-Z_0-9]+)\b', snippet)):
            assert token in headers, (story, token)
    compiled = ('Button', 'Checkbox', 'Switch', 'Radio', 'Slider', 'Label', 'Badge',
                'Progress', 'Avatar', 'Kbd', 'Input', 'Textarea', 'NumberInput',
                'Image', 'Attachment', 'Icon',
                'Alert', 'Bubble', 'Empty', 'GroupBox', 'Skeleton', 'Spinner',
                'Tooltip', 'HoverCard', 'Focus Trap', 'Clipboard', 'Shimmer',
                'Message', 'MessageScroller')
    extra = {
        'Button': 'cui_str label = {0};',
        'Checkbox': 'cui_str label = {0}; cui_check_state state = CUI_CHECK_UNCHECKED;',
        'Switch': 'cui_str label = {0}; bool enabled = false; cui_switch_state spring = {0}; double now = 0;',
        'Radio': 'cui_str label = {0}; int selection = 0;',
        'Slider': 'float value = 0;',
        'Progress': 'float progress = 0;',
        'Input': 'cui_str label = {0}; cui_edit *edit = 0; cui_input_state state = {0};',
        'Textarea': 'cui_edit *edit = 0; cui_textarea_state state = {0};',
        'NumberInput': 'cui_number_state state = {0}; double value = 0; cui_str label = {0};',
        'Image': 'cui_image_state state = {0}; cui_image_source source = {0}; cui_image_loader_fn loader = 0; cui_image_options options = {0};',
        'Attachment': 'cui_attachment_state state = {0}; cui_attachment_options options = {0};',
        'Icon': 'cui_icon_state state = {0}; cui_icon_options options = {0};',
    }
    includes = '\n'.join(f'#include "cui/{name}.h"' for name in (
        'cui', 'cui_components', 'cui_content', 'cui_message', 'cui_platform', 'cui_status',
        'cui_textarea', 'cui_media', 'cui_icon'))
    setup = '''
static void draw_message_row(cui_context *ui, cui_id id, size_t index, void *user, float width) {
    (void)ui; (void)id; (void)index; (void)user; (void)width;
}
'''
    common = '''
    cui_context *ui = 0;
    cui_id id = 0, trigger_id = 0, scope_id = 0;
    cui_theme theme = {0};
    cui_hover_state hover = {0};
    cui_rect viewport = {0};
    cui_platform platform = {0};
    cui_shimmer_state shimmer = {0};
    cui_message_body bodies[1] = {0};
    size_t body_count = 1;
    cui_message_scroller_state *scroll_state = 0;
    double now_seconds = 0;
    bool reduced_motion = false;
    void *user = 0;
'''
    code = includes + '\n' + setup + '\n'.join(
        f'void check_{story.replace(" ", "_")}(void) {{\n{common}{extra.get(story, "")}\n{snippets[story]}\n}}'
        for story in compiled) + '\n'
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / 'snippets.c'
        source.write_text(code)
        subprocess.run(['cc', '-std=c11', '-fsyntax-only', '-Wall', '-Wextra',
                        '-Werror', '-Wno-unused-variable', '-Wno-unused-parameter',
                        '-I', str(ROOT / 'include'), str(source)], check=True)
    print(f'PASS: {len(stories)} usage excerpts covered; {len(compiled)} displayed excerpts compile')


if __name__ == '__main__':
    main()
