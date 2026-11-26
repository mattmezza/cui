import * as c from './index';
const save=c.button('save').label('Save').primary().rounded_md().on_click(event => event.node.label('Saved'));
const calendar=c.calendar('calendar').date_value(1980,1,1).on_change(event=>console.log(event.text));
const parent=c.item('branch','Documents').step_status(0).checked(true);
const tree=c.tree('files').visual_size(240,180).children(parent,c.item('child','Notes').step_status(1));
const dialog=c.dialog('dialog','Edit').detail('Edit details').children(c.input('name','Alex'),save).on_close(event=>console.log(event.result));
const picker=c.date_picker('birthday').date_value(1980,1,1).open();
const app=new c.App({backend:'headless'}).mount(c.column().children(calendar,tree,dialog,picker,c.image('photo').image_rgba(new Uint8Array(16),2,2)));
app.scroll(5,5,0,-80).step();
const radio = c.radio('option', 'Option').animate_selection(app.supports_radio_motion()).checked();
if (app.supports_radio_tooltip()) radio.radio_tooltip('Save a copy', 350).checked();
radio.radio_tooltip(null);
c.radio_group('choices').animate_selection().tab_index(-2).children(c.item('option', 'Option'));
if (app.supports_radio_tab_index()) radio.tab_index(0).checked();
radio.tab_index(null);
// @ts-expect-error Tab order may be authored only on Radio and RadioGroup.
c.item('priority').tab_index(-2);
// @ts-expect-error Tab order may be authored only on Radio and RadioGroup.
c.checkbox('check').tab_index(-2);
// @ts-expect-error A tab index is numeric or null.
c.radio('option').tab_index('0');
c.radio_group('choices').children(
  c.item('priority', 'Priority').children(c.tag('recommended', 'Recommended'))
    .decorate_content(app.supports_radio_item_content()).radio_tooltip('Track shipment', 0),
);
// @ts-expect-error RadioGroup itself cannot have an option tooltip.
c.radio_group('choices').radio_tooltip('Group hint');
// @ts-expect-error The tooltip delay is numeric.
c.radio('option').radio_tooltip('Hint', '350');
// @ts-expect-error Only Items expose RadioGroup decorations.
c.button('action').decorate_content();
// @ts-expect-error Decoration opt-in takes a boolean.
c.item('priority').decorate_content('yes');
// @ts-expect-error Selection motion is specific to the Radio family.
c.checkbox('check').animate_selection();
// @ts-expect-error Tabs do not animate Radio marks.
c.tabs('tabs').animate_selection();
// @ts-expect-error The motion setting takes a boolean.
c.radio('option').animate_selection('yes');
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

import {stepper as visualStepper, item as visualStep} from './index';
visualStepper('visual-steps').stepper_visual(true, true, 'sm').design_size('lg')
  .children(visualStep('first', 'First').image_rgba(new Uint8Array([255,255,255,255]),1,1));
// @ts-expect-error unknown density
visualStepper().stepper_visual(true, false, 'huge');

const radioPaint: c.RadioPaint = {rootPadding: 0, markColor: 0, labelWeight: 2};
if (app.supports_radio_paint()) radio.radio_paint(c.RadioPaintLayer.BASE, radioPaint)
  .radio_paint(c.RadioPaintLayer.CHECKED, {markColor: 0xffffffff})
  .radio_paint(c.RadioPaintLayer.DISABLED, null);
// @ts-expect-error invalid layer
radio.radio_paint(6, {});
// @ts-expect-error invalid weight
radio.radio_paint(c.RadioPaintLayer.BASE, {labelWeight: 4});

visualStepper().stepper_fill().stepper_fill(true, 'xs', 300, 120).stepper_visual();
const stepperFillSupported: boolean = app.supports_stepper_fill();
// @ts-expect-error unsupported density
visualStepper().stepper_fill(false, 'huge');
// @ts-expect-error width must be numeric
visualStepper().stepper_fill(false, 'md', '300');

const fractionalSlide: c.Item = c.item("slide").carousel_basis(.5).carousel_basis(0);
const hasCarouselBasis: boolean = app.supports_carousel_basis();
void fractionalSlide; void hasCarouselBasis;

const verticalCarousel: c.Carousel = c.carousel("axis").carousel_axis("vertical").carousel_axis("horizontal");
const hasCarouselAxis: boolean = app.supports_carousel_axis();
const hasCarouselLooping: boolean = app.supports_carousel_looping();
const loopingCarousel: c.Carousel = c.carousel("loop").carousel_looping().carousel_looping(false);
// @ts-expect-error A Carousel loop option is boolean.
c.carousel("bad-loop").carousel_looping(1);
// @ts-expect-error Only a Carousel can loop.
c.item("bad-loop-item").carousel_looping();
// @ts-expect-error unsupported Carousel axis
c.carousel("bad-axis").carousel_axis("diagonal");
// @ts-expect-error Carousel axis is not an Item operation
c.item("bad-axis-item").carousel_axis("vertical");

const controlledCarousel: c.Carousel = c.carousel("controls").carousel_controls().carousel_controls({arrows: false}).carousel_controls({pagination: false}).carousel_controls({arrows: false, pagination: false});
const hasCarouselControls: boolean = app.supports_carousel_controls();
const carouselControlBounds: c.Bounds = controlledCarousel.numeric_part_bounds(2);
void hasCarouselControls; void carouselControlBounds;
// @ts-expect-error Carousel controls options are boolean.
c.carousel().carousel_controls({arrows: 1});
// @ts-expect-error Controls are exclusive to Carousel.
c.item().carousel_controls();

const hasCarouselControlSizes: boolean = app.supports_carousel_control_sizes();
const sizedCarousel: c.Carousel = c.carousel().carousel_control_sizes().carousel_control_sizes({arrows: "xs", pagination: "9xl"});
// @ts-expect-error control sizes use shared DesignSize names
c.carousel().carousel_control_sizes({arrows: "invalid"});
// @ts-expect-error only Carousel supports control sizes
c.item().carousel_control_sizes();

const fittedSlide: c.Item = c.item("fitted").carousel_fit().carousel_fit(false);
const hasCarouselFit: boolean = app.supports_carousel_fit();
// @ts-expect-error fit requires a boolean
fittedSlide.carousel_fit(1);

const navigationRail = c.carousel('navigation');
if (app.supports_carousel_navigation()) {
  const adjacentSlide: c.Item | undefined = navigationRail.carousel_adjacent();
  const previousSlide: c.Item | undefined = navigationRail.carousel_step(false);
  console.log(adjacentSlide, previousSlide);
}
// @ts-expect-error Carousel direction requires a boolean.
navigationRail.carousel_step(1);
// @ts-expect-error Navigation belongs to Carousel.
c.button('navigation-button').carousel_adjacent();

const hasTypedKbd: boolean = app.supportsKbdStroke();
const typedKbd = c.kbdStroke('save', 'K', {modifiers: 10, plain: true, outline: false, macSymbols: true, accessibleName: 'Save'});
c.column().children(typedKbd, c.kbd('legacy', 'Ctrl+K'));
// @ts-expect-error typed keyboard key must be a string
c.kbdStroke('bad', 4);
// @ts-expect-error presentation flag must be boolean
c.kbdStroke('bad', 'K', {plain: 1});
// @ts-expect-error option names use camelCase
c.kbdStroke('bad', 'K', {mac_symbols: true});
void hasTypedKbd;
