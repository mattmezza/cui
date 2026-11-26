import {App, button, carousel, column, item} from '@cui/native';
const app = new App({backend:'headless',width:480,height:240});
const save = button('save').label('Save').on_click(event => event.node.label('Saved'));
const overview = item('overview', 'Overview').carousel_basis(0.5);
const activity = item('activity', 'Activity').carousel_basis(0.5);
const slides = carousel('slides').visual_size(240, 100).children(overview, activity);
app.mount(column().children(save, slides));
const delivered: number = app.step(1/60);
const width: number = save.bounds().width;
if (!app.supports_carousel_basis() || activity.bounds().x - overview.bounds().x !== 128) {
  throw new Error('installed Carousel did not lay out fractional slides');
}
if (!app.supports_carousel_navigation() || slides.carousel_adjacent() !== activity ||
    slides.carousel_step() !== activity || slides.selected() !== activity) {
  throw new Error('installed Carousel did not navigate to the next slide');
}
void delivered; void width;
app.dispose();
