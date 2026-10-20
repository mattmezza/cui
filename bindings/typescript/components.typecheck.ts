import * as c from './index';
const save=c.button('save').label('Save').primary().rounded_md().on_click(event => event.node.label('Saved'));
const calendar=c.calendar('calendar').date_value(1980,1,1).on_change(event=>console.log(event.text));
const parent=c.item('branch','Documents').step_status(0).checked(true);
const tree=c.tree('files').visual_size(240,180).children(parent,c.item('child','Notes').step_status(1));
const dialog=c.dialog('dialog','Edit').detail('Edit details').children(c.input('name','Alex'),save).on_close(event=>console.log(event.result));
const picker=c.date_picker('birthday').date_value(1980,1,1).open();
const app=new c.App({backend:'headless'}).mount(c.column().children(calendar,tree,dialog,picker,c.image('photo').image_rgba(new Uint8Array(16),2,2)));
app.scroll(5,5,0,-80).step();
// @ts-expect-error Static badges do not accept button-only styling.
c.badge('status','Ready').primary();
// @ts-expect-error AlertDialog uses its native actions rather than arbitrary children.
c.alert_dialog('confirm','Delete?').children(save);
// @ts-expect-error Only scrollable exposes scrolling state.
c.label('Text').scroll_extent(300,900);
// @ts-expect-error The picker has no Calendar-only day hit-test method.
picker.calendar_day_bounds(1980,1,1);
const panel=c.item('panel').panel_size(240).on_change(e=>console.log(e.value));
const split=c.resizable('split').visual_size(420,160).children(panel,c.item('other'));
split.resizable_handle_bounds(panel);
c.message_scroller('messages').children(c.item('message','Hello')).scroll_to(0);
c.carousel('slides').children(c.item('slide')).on_change(e=>console.log(e.selectedHandle));
c.text_view('markdown','**Hello**').text_format('markdown').select_text(0,5).selection_text();
c.editor('editor','first\nsecond').detail('c').on_change(e=>console.log(e.text));
// @ts-expect-error Editors do not expose TextView selection operations.
c.editor('editor').select_text(0,1);
// @ts-expect-error Resizable is not a selection group.
split.select(panel);
// @ts-expect-error Only native TextView parser formats are accepted.
c.text_view('view').text_format('rtf');
c.chart('chart').chart_data([['Jan',5]]).chart_kind('bar').current_datum(0);
c.table('table').children(c.item('header').children(c.item('name','Name')));
c.data_table('data').table_data(['Name'],[['Alex']]).on_change(e=>JSON.parse(e.text)).table_state();
c.plot('plot').plot_points([[20,30]]).current_point(0);
// @ts-expect-error Plots do not expose gestures or selection yet.
c.plot('plot').on_change(()=>{});
// @ts-expect-error Chart has fixed native chart kinds.
c.chart('chart').chart_kind('radar');
c.command('actions').children(c.item('save','Save')).on_change(e=>console.log(e.text)).selected();
c.virtual_list('rows').children(c.item('row')).scroll_to(500).item_bounds(panel);
// @ts-expect-error Command supports native selection readback, not programmatic selection.
c.command('actions').select(panel);
c.sidebar('navigation').children(c.item('settings','Settings')).open(false).selected();
c.data_table('data').selection_mode('cell');app.theme_preset(1).current_theme_preset();
// @ts-expect-error Selection mode is explicit.
c.data_table('data').selection_mode('range');
c.root_view().padding(24).children(c.column().children(save));
// @ts-expect-error Native root is a content container, not a label.
c.root_view().label('unused');
app.clipboard_set('Unicode ✓').clipboard_get();
c.speech('recognition').speech_report('hello',true).speech_transcript();
// @ts-expect-error Speech receives host transcripts, not arbitrary text styling.
c.speech('recognition').label('ignored');
c.questionnaire('survey').children(c.item('question','Choose').question(true).children(c.item('yes','Yes'))).questionnaire_state();
// @ts-expect-error Question flags are boolean.
c.item('question').question('required');
c.dock('workspace').children(panel,c.item('other')).dock_move_split(panel,c.item('reference'),'horizontal',.4).dock_serialize();
// @ts-expect-error Split axes are explicit.
c.dock('workspace').dock_move_split(panel,panel,'diagonal');

const settings = c.settings('preferences').children(c.item('general','General').children(
  c.item('account','Account').children(c.input('name','Alex').detail('Display name'))));
settings.settings_query('Account').settings_select(panel).settings_reset(panel).settings_page();
settings.settings_field_bounds(c.checkbox('updates','Updates'));
// @ts-expect-error Settings only supports field-level disabled state.
settings.disabled();
// @ts-expect-error Settings field bounds require a supported field control.
settings.settings_field_bounds(c.button('save'));

const styledAvatar: c.Avatar = c.avatar('person','Ada').design_size('md').identity_colors(true,4).anonymous_icon(true,'user');
const sharedSize: c.DesignSize = '9xl';
c.button('styled').design_size(sharedSize).size('base');
// @ts-expect-error Shared scale currently excludes Badge.
c.badge('bad').design_size('md');
// @ts-expect-error Identity colors are Avatar-only.
c.button('bad').identity_colors();
// @ts-expect-error Existing size() remains Button-only.
styledAvatar.size('md');
// @ts-expect-error Unsupported named design variant.
styledAvatar.design_size('huge');
// @ts-expect-error Foreign bindings do not accept borrowed icon-state objects.
styledAvatar.anonymous_icon(true, {});
