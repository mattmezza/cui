'use strict';
const test = require('node:test'), assert = require('node:assert/strict');
const c = require('..'), native = require(process.env.CUI_NODE_ADDON);
test('Carousel slot role, parent and duplicate validation', () => {
  for (const role of ['', 'bogus', 1]) assert.throws(() => c.carousel_slot('x', role));
  for (const key of ['', 'a\0b', 'x'.repeat(4097)]) assert.throws(() => c.carousel_slot(key, 'previous'));
  assert.throws(() => c.column().children(c.carousel_slot('x', 'previous')), /direct owner/);
  const rail = c.carousel('rail'), first = c.carousel_slot('p','previous');
  assert.throws(() => rail.children(first,c.carousel_slot('p2','previous')), /duplicate/);
  assert.equal(first.parent,null); assert.equal(rail.childrenValue.length,0);
});
test('missing optional addon entry preflights atomically', () => {
  const app = new c.App(), original = native.carouselSlots;
  try {
    native.carouselSlots = undefined;
    assert.equal(app.supports_carousel_slots(),false);
    const slot=c.carousel_slot('p','previous');
    assert.throws(() => app.mount(c.column().children(c.carousel('rail').children(slot))), /unavailable/);
    assert.equal(slot.owner,null); assert.equal(app.nodes.size,0);
  } finally { native.carouselSlots=original; app.dispose(); }
});
test('Carousel slots retain passive content and reject nested controls', () => {
  const app = new c.App();
  try {
    if (process.env.CUI_TEST_LEGACY_CAROUSEL_SLOTS === '1') {
      assert.equal(app.supports_carousel_slots(), false);
      const slot = c.carousel_slot('p', 'previous');
      assert.throws(() => app.mount(c.column().children(c.carousel('rail').children(slot))), /unavailable/);
      assert.equal(app.nodes.size, 0);
      assert.equal(app.supports_carousel_controls(), true);
      const fallback = c.carousel('fallback').carousel_controls().children(c.item('first'), c.item('second'));
      app.mount(c.column().children(fallback)).step();
      assert.ok(fallback.numeric_part_bounds(2).width > 0);
      return;
    }
    assert.equal(app.supports_carousel_slots(),true);
    const bad=c.carousel_slot('bad','previous').children(c.column().children(c.button('button')));
    assert.throws(() => app.mount(c.column().children(c.carousel('badrail').children(bad))), /passive/);
    assert.equal(app.nodes.size,0);
    const previous=c.carousel_slot('p','previous','Go back').children(c.label('Back'));
    const marker=c.carousel_slot('m','marker').children(c.label('One'));
    const first=c.item('first').children(marker), second=c.item('second'), events=[];
    const rail=c.carousel('rail').carousel_controls().on_change(e=>events.push(e)).children(previous,
      first,second,c.carousel_slot('n','next').children(c.label('Next')));
    app.mount(c.column().children(rail)).step();
    assert.equal(rail.selected(),first);
    const b=rail.numeric_part_bounds(2);
    app.pointer('down',b.x+b.width/2,b.y+b.height/2);
    app.pointer('up',b.x+b.width/2,b.y+b.height/2);
    app.step();
    assert.equal(rail.selected(),second); assert.equal(events.length,1);
    const {slot, generation} = events[0].selectedHandle;
    assert.ok(app.nodes.get(`${slot}:${generation}`) === second, 'event identifies the second slide');
    assert.equal(previous.owner,app); assert.equal(marker.owner,app);
  } finally { app.dispose(); }
});
