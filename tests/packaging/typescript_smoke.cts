import {App, IconSource, button, carousel, column, item, kbdStroke, rotating_spinner,
  type KbdStrokeOptions, type SpinnerPresentation} from '@cui/native';
const app = new App({backend:'headless',width:480,height:240});
if (!app.supports_stepper_wrapped_fill() || !app.supportsVirtualListAxis() ||
    !app.supportsKbdStyle() || !app.supports_responsive_notifications()) {
  throw new Error('installed provider lacks an alpha.15 layout capability');
}
if (!app.supports_notification_action() ||
    !app.supports_notification_auto_hide() ||
    !app.supports_stepper_centered_fill() ||
    !app.supports_dialog_composition() ||
    !app.supports_popover_secondary_trigger()) {
  throw new Error('installed provider lacks an alpha.16 overlay or Stepper capability');
}
const save = button('save').label('Save').on_click(event => event.node.label('Saved'));
const overview = item('overview', 'Overview').carousel_basis(0.5);
const activity = item('activity', 'Activity').carousel_basis(0.5);
const settings = item('settings', 'Settings').carousel_basis(0.5);
const slides = carousel('slides').visual_size(240, 100).children(overview, activity, settings);
const shortcutStyle: KbdStrokeOptions = {modifiers:2, outline:true, accessibleName:'Save shortcut'};
const hint = kbdStroke('shortcut', 'ß', shortcutStyle);
const reference = kbdStroke('reference', 'sS', shortcutStyle);
const loading = rotating_spinner('loading', {easing:'linear'});
app.mount(column().children(save, slides, hint, reference, loading));
if (!app.supports_spinner_icon_resource()) throw new Error('installed Spinner artwork unavailable');
const artwork = app.icon_resource(IconSource.svg(
  '<svg viewBox="0 0 24 24"><path d="M12 2 L22 12 H12 Z"/></svg>'));
loading.set_icon_resource(artwork);
const delivered: number = app.step(1/60);
const sample: SpinnerPresentation | undefined = loading.presentation();
if (!sample || sample.nextWakeSeconds <= sample.coreTime || artwork.bitmap().width <= 0) {
  throw new Error('installed Spinner failed to render custom artwork');
}
loading.set_icon_resource(null);
artwork.close();
const width: number = save.bounds().width;
if (!app.supports_carousel_basis() || activity.bounds().x - overview.bounds().x !== 128) {
  throw new Error('installed Carousel did not lay out fractional slides');
}
if (!app.supports_carousel_navigation() || slides.carousel_adjacent() !== activity ||
    slides.carousel_step() !== activity || slides.selected() !== activity) {
  throw new Error('installed Carousel did not navigate to the next slide');
}
const hintBounds = hint.bounds();
const referenceBounds = reference.bounds();
if (!app.supportsKbdStroke() || hintBounds.width <= 0 || hintBounds.height <= 0 ||
    hintBounds.width !== referenceBounds.width || hintBounds.height !== referenceBounds.height) {
  throw new Error('installed typed Kbd did not lay out Ctrl+SS');
}
void delivered; void width;
app.dispose();
