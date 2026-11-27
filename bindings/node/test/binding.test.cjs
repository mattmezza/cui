// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, row, column, label, button, avatar, stepper, item} = require('..');

async function main() {
  const stepApp=new App({width:800,height:800});
  const iconBytes=new Uint8Array([255,255,255,255]);
  const one=item('one','First').image_rgba(iconBytes,1,1), two=item('two','Second');
  const steps=stepper('steps').stepper_visual().design_size('lg').children(one,two);
  const changes=[];steps.on_change(e=>changes.push(e));
  stepApp.mount(column().children(steps));stepApp.step();iconBytes[0]=0;
  let part=steps.numeric_part_bounds(2);
  for(let i=0;i<2;i++){stepApp.pointer('down',part.x+part.width/2,part.y+part.height/2).pointer('up',part.x+part.width/2,part.y+part.height/2);stepApp.step();}
  assert.equal(steps.selected(),two);assert.equal(changes.length,1);
  for(const density of ['md','xs','sm','lg']){steps.stepper_visual(true,true,density);stepApp.step();assert(steps.numeric_part_bounds(2).y>steps.numeric_part_bounds(1).y);}
  one.image_rgba(new Uint8Array([255,255,255,255]),1,1);
  steps.stepper_visual(false);stepApp.step();
  assert.throws(()=>steps.stepper_visual(true,false,'bad'));
  assert.throws(()=>one.image_rgba(new Uint8Array(4),1,1,0xffffffff));
  stepApp.dispose();assert.throws(()=>steps.stepper_visual());
  const fillApp=new App({backend:'headless',width:800,height:800});
  assert.equal(fillApp.supports_stepper_fill(),true);
  const fa=item('fa','First'),fb=item('fb','Second');
  const fill=stepper('fill').stepper_visual().stepper_fill(false,'md',360,64).children(fa,fb);
  const fillEvents=[];fill.on_change(e=>fillEvents.push(e));
  fillApp.mount(column().children(fill));fillApp.step();
  assert(Math.abs(fill.bounds().width-360)<1);
  part=fill.numeric_part_bounds(2);
  for(let i=0;i<2;i++){fillApp.pointer('down',part.x+part.width/2,part.y+part.height/2).pointer('up',part.x+part.width/2,part.y+part.height/2);fillApp.step();}
  assert.equal(fill.selected(),fb);assert.equal(fillEvents.length,1);
  for(const density of ['md','xs','sm','lg']){fill.stepper_fill(true,density,240,240);fillApp.step();assert(fill.numeric_part_bounds(2).y>fill.numeric_part_bounds(1).y);}
  fill.stepper_visual();fillApp.step();assert.equal(fill.stepperFillValue,null);
  const supportsFill=fillApp.supports_stepper_fill;
  fillApp.supports_stepper_fill=()=>false;
  assert.throws(()=>fill.stepper_fill(),/cui_bridge_set_stepper_fill/);
  assert.equal(fill.stepperFillValue,null);
  fillApp.supports_stepper_fill=supportsFill;fillApp.dispose();
  for(const value of [-1,4097,NaN,Infinity,true,'1'])assert.throws(()=>stepper().stepper_fill(false,'md',value));
  const missingApp=new App({backend:'headless'});missingApp.supports_stepper_fill=()=>false;
  const missing=stepper().stepper_fill();
  assert.throws(()=>missingApp.mount(column().children(missing)),/cui_bridge_set_stepper_fill/);
  assert(!missing.owner);
  missingApp.mount(column().children(stepper().stepper_fill().stepper_visual()));missingApp.dispose();
  {
  const fillApp=new App({backend:'headless',width:800,height:800});
  const expectedWrap=process.env.CUI_EXPECT_STEPPER_WRAP!=='0';
  assert.equal(fillApp.supports_stepper_wrapped_fill(),expectedWrap);
  assert.equal(fillApp.supports_stepper_fill(),true);
  if(!expectedWrap){
    const unavailable=stepper().stepper_wrapped_fill();
    assert.throws(()=>fillApp.mount(column().children(unavailable)),/cui_bridge_set_stepper_wrapped_fill_v1/);
    assert(!unavailable.owner);
    fillApp.mount(column().children(stepper().stepper_fill()));fillApp.step();fillApp.dispose();
  } else {
  const fa=item('fa','First step title with several words').detail('Description with enough text to wrap'),fb=item('fb','Second step title').detail('Second description');
  const fill=stepper('fill').stepper_visual().stepper_wrapped_fill(false,'md',360,64).children(fa,fb);
  fill.select(fa);
  const fillEvents=[];fill.on_change(e=>fillEvents.push(e));
  fillApp.mount(column().children(fill));fillApp.step();
  assert.equal(fill.selected(),fa);
  assert(Math.abs(fill.bounds().width-360)<1);
  part=fill.numeric_part_bounds(2);
  for(let i=0;i<2;i++){fillApp.pointer('down',part.x+part.width/2,part.y+part.height/2).pointer('up',part.x+part.width/2,part.y+part.height/2);fillApp.step();}
  assert.equal(fill.selected(),fb);assert.equal(fillEvents.length,1);
  for(const density of ['md','xs','sm','lg']){fill.stepper_wrapped_fill(true,density,240,240);fillApp.step();assert(fill.numeric_part_bounds(2).y>fill.numeric_part_bounds(1).y);}
  fill.stepper_fill();fillApp.step();assert.equal(fill.stepperWrappedFillValue,null);assert.equal(fill.selected(),fb);
  fill.stepper_wrapped_fill();fillApp.step();
  fill.stepper_visual();fillApp.step();assert.equal(fill.stepperWrappedFillValue,null);
  const supportsFill=fillApp.supports_stepper_wrapped_fill;
  fillApp.supports_stepper_wrapped_fill=()=>false;
  assert.throws(()=>fill.stepper_wrapped_fill(),/cui_bridge_set_stepper_wrapped_fill_v1/);
  assert.equal(fill.stepperWrappedFillValue,null);
  fillApp.supports_stepper_wrapped_fill=supportsFill;fillApp.dispose();
  for(const value of [-1,4097,NaN,Infinity,true,'1'])assert.throws(()=>stepper().stepper_wrapped_fill(false,'md',value));
  const missingApp=new App({backend:'headless'});missingApp.supports_stepper_wrapped_fill=()=>false;
  const missing=stepper().stepper_wrapped_fill();
  assert.throws(()=>missingApp.mount(column().children(missing)),/cui_bridge_set_stepper_wrapped_fill_v1/);
  assert(!missing.owner);
  missingApp.mount(column().children(stepper().stepper_wrapped_fill().stepper_visual()));missingApp.dispose();
  }
  }
  const avatarApp = new App({width:600,height:400});
  const person = avatar('person','Ada Lovelace').design_size('md').identity_colors(true,4);
  const anonymous = avatar('anonymous').anonymous_icon(true,'user');
  const legacy = avatar('legacy','L').visual_size(60,60);
  avatarApp.mount(column().children(person,anonymous,legacy)); avatarApp.step();
  assert.equal(person.bounds().width,48); assert.equal(anonymous.bounds().width,40); assert.equal(legacy.bounds().width,60);
  person.design_size('lg').identity_colors(false).anonymous_icon(false); avatarApp.step();
  assert.equal(person.bounds().width,80);
  person.design_size('inherit'); avatarApp.step(); assert.equal(person.bounds().width,40);
  for(const bad of [12,-1,1.5,true])assert.throws(()=>person.identity_colors(true,bad));
  for(const bad of ['\0bad','x'.repeat(4097),'\uD800'])assert.throws(()=>anonymous.anonymous_icon(true,bad));
  assert.throws(()=>label().design_size('md')); assert.throws(()=>button().identity_colors());
  assert.throws(()=>avatar().size('md'));
  assert.equal(button().design_size('lg').size('sm').sizeValue,2);
  avatarApp.dispose(); assert.throws(()=>person.design_size('md'));
  for (const dispose of [false, true]) {
    const closingApp = new App();
    const closer = button('close').label('Close').on_click(() =>
      dispose ? closingApp.dispose() : closingApp.close());
    closingApp.mount(row().children(closer)); closingApp.step();
    const bounds = closer.bounds();
    closingApp.pointer('down', bounds.x + 2, bounds.y + 2)
      .pointer('up', bounds.x + 2, bounds.y + 2);
    assert.equal(closingApp.step(), 1, 'closing within a callback stops event delivery safely');
    closingApp.dispose();
  }
  let clicks = 0;
  const status = label('Ready');
  const save = button('save').primary().rounded_md().medium().hover_color('#4466cc')
    .label('Save changes').on_click(() => { clicks++; status.label(`Saved ${clicks}`); });
  const defaultSize = button('default-size').label('Equal');
  const explicitBase = button('explicit-base').base().label('Equal');
  const sizeApp = new App({width: 480, height: 240});
  sizeApp.mount(row().children(defaultSize, explicitBase));
  sizeApp.step();
  const defaultBounds = defaultSize.bounds();
  const baseBounds = explicitBase.bounds();
  assert.equal(defaultBounds.width, baseBounds.width, 'default width equals explicit BASE');
  assert.equal(defaultBounds.height, baseBounds.height, 'default height equals explicit BASE');
  sizeApp.dispose();
  const app = new App({width: 480, height: 240});
  app.mount(column().gap_units(2).children(row().gap_px(8).children(status, save)));
  app.step();
  const b = save.bounds();
  assert.ok(b.width > 50 && b.height > 20 && b.x > 0 && b.y > 0, 'CUI computed button geometry');
  app.pointer('move', b.x + 3, b.y + 3).pointer('down', b.x + 3, b.y + 3).pointer('up', b.x + 3, b.y + 3);
  assert.equal(app.step(), 1);
  assert.equal(clicks, 1);
  assert.ok(status.bounds().width > 0, 'callback mutation is accepted by native scene');
  app.step();
  app.key('down', 'tab').key('up', 'tab').key('down', 'enter').key('up', 'enter');
  assert.equal(app.step(), 1, 'keyboard activation reaches native button');
  assert.equal(clicks, 2);
  save.disabled();
  app.pointer('move', b.x + 3, b.y + 3).pointer('down', b.x + 3, b.y + 3).pointer('up', b.x + 3, b.y + 3);
  assert.equal(app.step(), 0, 'disabled button does not click');
  assert.equal(clicks, 2);
  assert.throws(() => app.mount(row()), /already has a root/);
  assert.throws(() => save.children(label('x')), /does not accept children/);
  assert.throws(() => save.hover_color('bad'), /color/);
  const other = new App();
  assert.throws(() => other.mount(row().children(save)), /unattached/);
  other.dispose();
  app.dispose();
  assert.throws(() => app.step(), /disposed/);
  assert.throws(() => save.bounds(), /disposed/);
  assert.throws(() => save.label('late'), /disposed/);

  const staleApp = new App();
  const staleButton = button('old').label('Old');
  staleApp.mount(row().children(staleButton)); staleApp.step();
  staleButton.remove();
  assert.throws(() => staleButton.bounds(), /not mounted/);
  staleApp.dispose();

  const timerApp = new App(); timerApp.mount(row().children(button('timer').label('Timer')));
  let timerRan = false;
  timerApp.start(120);
  await new Promise(resolve => setTimeout(() => { timerRan = true; resolve(); }, 40));
  assert.equal(timerRan, true, 'native frame scheduling leaves Node event loop responsive');
  timerApp.stop().dispose();
  if (global.gc) { global.gc(); await new Promise(resolve => setImmediate(resolve)); }
  console.log('Node binding tests passed');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
