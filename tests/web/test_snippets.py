#!/usr/bin/env python3
"""Check gallery usage coverage and compile every displayed excerpt."""
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
    compiled = tuple(stories)
    extra = {
        'Avatar': 'cui_bitmap photo = {0};',
        'Button': 'cui_str label = {0};',
        'Checkbox': 'cui_str label = {0}; cui_check_state state = CUI_CHECK_UNCHECKED;',
        'Switch': 'cui_str label = {0}; bool enabled = false; cui_switch_state spring = {0}; double now = 0;',
        'Radio': 'cui_str label = {0}; int selection = 0;',
        'Slider': 'float value = 0;',
        'Progress': 'float progress = 0; cui_progress_motion motion = {0};',
        'Input': 'cui_str label = {0}; cui_edit *edit = 0; cui_input_state state = {0};',
        'Textarea': 'cui_edit *edit = 0; cui_textarea_state state = {0};',
        'NumberInput': 'cui_number_state state = {0}; double value = 0; cui_str label = {0};',
        'Image': 'cui_image_state state = {0}; cui_image_source source = {0}; cui_image_loader_fn loader = 0; cui_image_options options = {0};',
        'Attachment': 'cui_attachment_state state = {0}; cui_attachment_options options = {0};',
        'Icon': 'cui_icon_state state = {0}; cui_icon_options options = {0};',
        'Toggle': 'cui_id group_id = 0; cui_toggle_item items[3] = {0}; bool selected[3] = {0}; cui_toggle_group_style_options options = {0};',
        'Tag': 'cui_tag_options options = {0};',
        'Notification': 'cui_notification_state state = {0}; cui_notification descriptor = {0}; double host_seconds = 0;',
        'Marker': 'cui_marker_state state = {0}; cui_marker_child children[1] = {0}; size_t count = 1; cui_marker_options options = {0};',
        'OtpInput': 'cui_otp_state state = {0}; cui_otp_options options = {0};',
        'Input Group': 'cui_id group_id = 0, input_id = 0; cui_edit *editor = 0; cui_input_group_state state = {0}; cui_input_group_addon addons[1] = {0}; size_t addon_count = 1; cui_input_group_options options = {0};',
        'Form': 'cui_form_field fields[1] = {0}; size_t count = 1; cui_form_content_fn draw_save_button = 0;',
        'Scrollable': 'cui_scroll_state state = {0}; cui_scroll_options options = {0};',
        'DataTable': 'cui_table_model *model = 0; cui_table_state state = {0}; cui_table_options options = {0}; cui_table_label_fn cell_label = 0; cui_table_cell_fn draw_cell = 0;',
        'Table': 'float width = 320;',
        'Dialog': 'cui_surface_state dialog = {0}; cui_str title = {0}, description = {0}; cui_surface_options options = {0};',
        'AlertDialog': 'cui_surface_state alert = {0}; cui_str title = {0}, description = {0}, confirm = {0}, cancel = {0}; cui_surface_options options = {0};',
        'Sheet': 'cui_surface_state sheet = {0}; cui_str title = {0}, description = {0}; cui_surface_options options = {0};',
        'Popover': 'cui_surface_state popover = {0}; cui_id trigger = 0; cui_str title = {0}, description = {0}; cui_surface_options options = {0};',
        'Tree': 'cui_tree_model *model = 0; cui_tree_state state = {0}; cui_id selection = 0; cui_tree_options options = {0};',
        'Editor': 'cui_id search_id = 0; cui_editor_state state = {0}; cui_editor_search_state search = {0};',
        'VirtualList': 'const cui_virtual_list *list = 0; cui_virtual_widget_state state = {0}; cui_virtual_render_item_fn paint_row = 0;',
        'Dock': 'cui_dock *dock = 0; cui_dock_view *view = 0; cui_dock_panel panels[2] = {0}; size_t count = 2;',
        'Plot': 'cui_bitmap bitmap = {0}; cui_plot_axis_options axis = {0};',
        'Calendar': 'cui_calendar_state state = {0}; cui_date_selection selection = {0}; cui_calendar_options options = {0};',
        'DatePicker': 'cui_str label = {0}; cui_date_picker_state state = {0}; cui_date_selection selection = {0}; cui_date_picker_options options = {0};',
        'ColorPicker': 'cui_color_state state = {0}; cui_color color = {0}; cui_color_picker_options options = {0};',
        'TimeField': 'cui_str label = {0}; cui_time_state state = {0}; cui_time time = {0}; cui_time_options options = {0};',
        'Combobox': 'cui_str label = {0}; cui_edit *query = 0; cui_choice_item items[4] = {0}, scratch[4] = {0}; cui_id selection = 0; cui_choice_popup popup = {0}; cui_choice_popup_options options = {0};',
        'Pagination': 'size_t page = 1, total_pages = 10;',
        'Rating': 'cui_str label = {0}; float rating = 0; cui_rating_options options = {0};',
        'Stepper': 'cui_step steps[3] = {0}; size_t current_step = 0;',
        'List': 'cui_choice_item items[3] = {0}; cui_id selection = 0; cui_list_state state = {0}; cui_list_options options = {0};',
        'Select': 'cui_str label = {0}, placeholder = {0}; cui_choice_item items[3] = {0}; cui_id selection = 0; cui_choice_popup popup = {0}; cui_choice_popup_options options = {0};',
        'Menu': 'cui_str label = {0}; cui_menu_entry entries[1] = {0}; size_t count = 1; cui_id action = 0; cui_menu_tree_state state = {0}; cui_menu_tree_options options = {0};',
        'DropdownButton': 'cui_id container = 0, action_id = 0, menu_id = 0, menu_action = 0; cui_str label = {0}, menu_name = {0}; cui_menu_entry entries[1] = {0}; size_t count = 1; cui_menu_tree_state menu_state = {0}; cui_dropdown_button_options options = {0};',
        'ContextMenu': 'cui_id target = 0, action = 0; cui_str label = {0}; cui_menu_entry entries[1] = {0}; size_t count = 1; cui_menu_tree_state state = {0}; cui_menu_tree_options options = {0};',
        'Accordion': 'cui_disclosure_item items[3] = {0}; bool expanded[3] = {0}; cui_accordion_options options = {0}; cui_disclosure_panel_fn draw_panel = 0;',
        'Collapsible': 'cui_str label = {0}; bool open = false; cui_disclosure_panel_fn draw_panel = 0;',
        'Tabs': 'cui_choice_item items[3] = {0}; cui_id selection = 0; cui_tabs_options options = {0}; cui_tab_panel_fn draw_panel = 0;',
        'DescriptionList': 'cui_description_item items[1] = {0}; size_t count = 1; cui_description_options options = {0};',
        'StatusBar': 'cui_chrome_region left = {0}, center = {0}, right = {0}; cui_status_bar_options options = {0};',
        'Resizable': 'cui_resizable_state state = {0}; cui_resizable_panel panels[1] = {0}; size_t count = 1; cui_resizable_options options = {0};',
        'Toolbar': 'cui_toolbar_item items[1] = {0}; size_t count = 1; cui_toolbar_options options = {0};',
        'TitleBar': 'cui_title_bar_state state = {0}; cui_chrome_item items[1] = {0}; size_t count = 1; cui_title_bar_options options = {0};',
        'Sidebar': 'cui_sidebar_state state = {0}; cui_sidebar_entry entries[1] = {0}; size_t count = 1; cui_sidebar_options options = {0};',
        'Settings': 'cui_settings_state state = {0}; cui_settings_page_state page_workspace[2] = {0}; cui_edit *search = 0; cui_setting_page pages[2] = {0};',
        'Carousel': 'cui_carousel_state state = {0}; cui_carousel_geometry geometry[4] = {0}; cui_carousel_item slides[4] = {0}; void (*draw_controls)(cui_context *, const cui_theme *, cui_carousel_state *, cui_id, void *) = 0; void (*draw_pages)(cui_context *, const cui_theme *, cui_carousel_state *, cui_id, void *) = 0;',
        'Command': 'cui_command_state state = {0}; cui_command_row rows[32] = {0}; cui_edit *query = 0; cui_command_entry entries[1] = {0}; size_t count = 1; cui_command_options options = {0};',
        'Questionnaire': 'cui_questionnaire_state state = {0}; cui_questionnaire_options options = {0}; cui_id root_id = 0, progress_id = 0, item_id = 0, heading_id = 0, group_id = 0, choice_id = 0; size_t item = 0, choice = 0; cui_questionnaire_choice_render_fn draw_choice_content = 0;',
        'Speech': 'cui_speech_state state = {0}; cui_speech_workspace workspace = {0}; cui_speech_recognizer recognizer = {0}; cui_speech_input input = {0}; cui_speech_event_fn on_event = 0; cui_speech_button_options options = {0}; cui_speech_waveform_options waveform = {0}; cui_id waveform_id = 0; double now = 0;',
        'Chart': 'cui_chart_options config = {0}; cui_chart_state state = {0};',
        'TextView': 'cui_text_view_state state = {0}; cui_text_view_workspace workspace = {0}; cui_str markdown = {0}; cui_text_view_options options = {0};',
        'Root View': 'cui_root root = {0}; cui_root_plugin_instance plugins[1] = {0}; cui_root_registry registry = {0}; void *window = 0; cui_root_surface dialogs[2] = {0}; cui_notification_state notifications = {0}; cui_root_content_fn content = 0; double now = 0;',
    }
    includes = '\n'.join(f'#include "cui/{path.name}"' for path in sorted((ROOT / 'include/cui').glob('*.h'))
                         if not path.stem.startswith(('cui_sdl', 'cui_sokol', 'cui_renderer', 'cui_software')))
    setup = '''
static void draw_message_row(cui_context *ui, cui_id id, size_t index, void *user, float width) {
    (void)ui; (void)id; (void)index; (void)user; (void)width;
}
static void draw_content(cui_context *ui) { (void)ui; }
static void draw_settings(cui_context *ui) { (void)ui; }
static void draw_profile(cui_context *ui) { (void)ui; }
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
        f'void check_{story.replace(" ", "_")}(void) {{\n'
        f'{common.replace("cui_theme theme = {0};", "" ) if story == "Theme" else common}'
        f'{extra.get(story, "")}\n{snippets[story]}\n}}'
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
