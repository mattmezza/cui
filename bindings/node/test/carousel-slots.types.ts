import {App, carousel, carousel_slot, item, label, CarouselSlot} from '..';
const slot: CarouselSlot = carousel_slot('p', 'previous', 'Go back').children(label('Back'));
carousel('rail').children(slot, item('first').children(carousel_slot('m', 'marker')));
const available: boolean = new App().supports_carousel_slots();
// @ts-expect-error unsupported role
carousel_slot('x', 'actions');
// @ts-expect-error labels are strings
carousel_slot('x', 'next', 2);
