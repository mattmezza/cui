'use strict';
const test = require('node:test'), assert = require('node:assert/strict');
const c = require('..'), native = require(process.env.CUI_NODE_ADDON);
test('slot authoring validates role, parent, width and duplicate roles', () => {
  for (const role of ['', 'bogus', 1]) assert.throws(() => c.attachment_slot('x', role));
  for (const width of [0, -1, 4097, NaN, Infinity, Number.MIN_VALUE]) assert.throws(() => c.attachment_slot('x', 'actions', width));
  for (const role of ['media','overlay','metadata']) assert.throws(() => c.attachment_slot('x', role, 1));
  for (const key of ['', 'a\0b', 'x'.repeat(4097)]) assert.throws(() => c.attachment_slot(key, 'media'));
  assert.throws(() => c.column().children(c.attachment_slot('x', 'media')), /direct Attachment/);
  assert.throws(() => c.attachment().children(c.button('x')), /attachment_slot/);
  const card = c.attachment();
  const first = c.attachment_slot('a', 'media');
  const duplicate = c.attachment_slot('b', 'media');
  assert.throws(() => card.children(first, duplicate), /duplicate/);
  assert.equal(first.parent, null);
  assert.equal(card.childrenValue.length, 0);
});
test('missing addon entry preflights without mutating the tree and ordinary cards work', () => {
 const app = new c.App(), original = native.attachmentSlots;
 try {
  native.attachmentSlots = undefined;
  assert.equal(app.supports_attachment_slots(),false);
  const card = c.attachment('card','File').children(c.attachment_slot('slot','metadata').children(c.label('Custom')));
  assert.throws(() => app.mount(c.column().children(card)),/unavailable/);
  assert.equal(app.root,null); assert.equal(card.owner,null); assert.equal(app.nodes.size,0);
  app.mount(c.column().children(c.attachment('ordinary','Ordinary'))).step_only();
 } finally { native.attachmentSlots=original; app.dispose(); }
 assert.throws(() => app.supports_attachment_slots(),/disposed/);
});
test('factory slots retain normal control events, gap, disabled and recursive removal', {skip:process.env.CUI_TEST_LEGACY_ATTACHMENT_SLOTS==='1'}, () => {
 const app = new c.App();
 try {
  assert.equal(app.supports_attachment_slots(),true);
  let clicks=0;
  const child=c.button('Save').on_click(()=>clicks++);
  const slots=['media','overlay','metadata'].map(role=>c.attachment_slot(role,role).gap(2).children(c.label(role)));
  const actions=c.attachment_slot('actions','actions',120).gap(3).disabled().children(child);
  const card=c.attachment('card','File').visual_size(400,0).children(...slots,actions);
  app.mount(c.column().children(card)).step_only().step_only();
  assert.ok(actions.bounds().width>0);assert.ok(child.bounds().width>0);
  const click=()=>{const b=child.bounds();app.pointer('move',b.x+b.width/2,b.y+b.height/2).pointer('down',b.x+b.width/2,b.y+b.height/2).step_only();app.pointer('up',b.x+b.width/2,b.y+b.height/2).step();};
  click();assert.equal(clicks,0);actions.disabled(false);app.step_only();click();assert.equal(clicks,1);
  const saved=child.bounds();assert.ok(saved.width>0);
  card.remove();assert.throws(()=>child.bounds());
 } finally {app.dispose();}
});

test('old provider rejects slots before mount but retains ordinary children', {skip:process.env.CUI_TEST_LEGACY_ATTACHMENT_SLOTS!=='1'}, () => {
 const app=new c.App();
 try {
  assert.equal(app.supports_attachment_slots(),false);
  const card=c.attachment('legacy','File').children(c.attachment_slot('slot','metadata'));
  assert.throws(()=>app.mount(c.column().children(card)),/unavailable/);
  assert.equal(app.root,null);assert.equal(app.nodes.size,0);assert.equal(card.owner,null);
  app.mount(c.column().children(c.button('Ordinary'),c.attachment('plain','File'))).step_only();
 } finally {app.dispose();}
});

test('notifications keep their app-wide stack', {skip: process.env.CUI_TEST_LEGACY_ATTACHMENT_SLOTS === '1'}, () => {
  const app = new c.App();
  try {
    const slot = c.attachment_slot('metadata', 'metadata').children(
      c.column().children(c.notification('notice', 'Global notice')));
    const card = c.attachment('file', 'Report.pdf').children(slot);
    assert.throws(() => app.mount(c.column().children(card)), /app-wide stack/);
    assert.equal(app.root, null);
    assert.equal(card.owner, null);
  } finally {
    app.dispose();
  }
});
