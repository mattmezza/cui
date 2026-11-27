// SPDX-License-Identifier: MIT
'use strict';
const assert=require('node:assert/strict');
const c=require('../index.cjs');
const app=new c.App({backend:'headless',maxNodes:90});
try {
 const items=Array.from({length:12},(_,i)=>c.item('i'+i,'Message '+i));
 const view=c.virtual_list('inbox').visual_size(240,96).children(...items);
 const prepend=c.button('prepend').label('New message'),resize=c.button('resize').label('Expand message');
 app.mount(c.column('root').children(prepend,resize,view));
 if(process.env.CUI_EXPECT_VIRTUAL_LEGACY==='1') {
  assert.equal(app.supportsVirtualListRows(),false);app.step();
  assert.throws(()=>view.preserveVirtualListAnchor(),/both V1/);
  assert.throws(()=>view.setVirtualListRows([]),/both V1/);
 } else {
  assert.equal(app.supportsVirtualListRows(),true);
  const rows=items.map(item=>({item,extent:0}));
  let itemReads=0,extentReads=0;
  const snapshotRows=items.map(item=>({get item(){itemReads++;return item;},get extent(){extentReads++;return 0;}}));
  view.setVirtualListRows(snapshotRows);
  assert.equal(itemReads,items.length);assert.equal(extentReads,items.length);
  view.preserveVirtualListAnchor().setVirtualListRows(rows);app.step();view.scroll_to(65);app.step();
  const before=view.item_bounds(items[2]),stable=items[2];
  const added=c.item('new','New message');
  prepend.on_click(()=>{view.appendVirtualListItem(added);rows.unshift({item:added,extent:48});view.setVirtualListRows(rows);});
  resize.on_click(()=>{rows[0].extent=80;view.setVirtualListRows(rows);});
  const click=node=>{const {x,y,width,height}=node.bounds();app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();app.step();};
  click(prepend);assert.equal(items[2],stable);assert.equal(view.item_bounds(stable).y,before.y);
  click(resize);assert.equal(view.item_bounds(stable).y,before.y);assert.equal(view.scroll_offset(),145);
  for(const extent of [-1,.5,4097,NaN,Infinity,true])assert.throws(()=>view.setVirtualListRows([{item:added,extent},...rows.slice(1)]));
  assert.throws(()=>view.setVirtualListRows(rows.slice(1)));
  assert.throws(()=>view.setVirtualListRows(rows.map(()=>rows[0])));
  assert.throws(()=>view.appendVirtualListItem(items[0]));
  assert.throws(()=>view.appendVirtualListItem(c.item('new')));
  view.removeVirtualListItem(added);assert.throws(()=>view.setVirtualListRows(rows));
  assert.throws(()=>view.removeVirtualListItem(added));
  view.setVirtualListRows(rows.slice(1));app.step();view.preserveVirtualListAnchor(false);
  const foreign=new c.App({backend:'headless'});
  try {const other=c.item('other');foreign.mount(c.column().children(c.virtual_list().children(other)));assert.throws(()=>view.setVirtualListRows([{item:other,extent:0},...rows.slice(2)]));} finally {foreign.dispose();}
  for(const item of items)view.removeVirtualListItem(item);
  view.setVirtualListRows([]);app.step();
  const full=Array.from({length:64},(_,i)=>c.item('capacity-'+i));
  for(const child of full)view.appendVirtualListItem(child);
  view.setVirtualListRows(full.map((item,i)=>({item,extent:i%2?1:4096})));
  assert.throws(()=>view.appendVirtualListItem(c.item('overflow')));app.step();
 }
} finally {app.dispose();}
assert.throws(()=>app.supportsVirtualListRows(),/disposed/);
{
  const axisApp = new c.App({backend: 'headless'});
  try {
    const supported = axisApp.supportsVirtualListAxis();
    if (process.env.CUI_EXPECT_VIRTUAL_AXIS !== undefined)
      assert.equal(supported, process.env.CUI_EXPECT_VIRTUAL_AXIS === '1');
    const clips = Array.from({length: 6}, (_, i) => c.item(`clip-${i}`, `Clip ${i}`));
    const view = c.virtual_list('timeline').visual_size(96, 80).children(...clips)
      .setVirtualListAxis('horizontal');
    const root = c.column().children(view);
    if (!supported) {
      assert.throws(() => axisApp.mount(root), /VirtualList axis/);
      view.setVirtualListAxis('vertical');
      axisApp.mount(root);
      axisApp.step();
      assert.equal(view.item_bounds(clips[0]).height, 32);
    } else {
      axisApp.mount(root);
      let extent = 32;
      if (axisApp.supportsVirtualListRows()) {
        extent = 48;
        view.setVirtualListRows(clips.map(item => ({item, extent})));
      }
      view.scroll_to(20);
      axisApp.step();
      const first = view.item_bounds(clips[0]);
      const second = view.item_bounds(clips[1]);
      assert.equal(first.x - view.bounds().x, -20);
      assert.equal(first.width, extent);
      assert.equal(second.x - first.x, extent);
      assert.equal(first.y, second.y);
      assert.throws(() => view.setVirtualListAxis('diagonal'), /vertical or horizontal/);
      view.setVirtualListAxis('vertical');
      axisApp.step();
      assert.equal(view.scroll_offset(), 20);
      assert.equal(view.item_bounds(clips[0]).height, extent);
    }
  } finally {
    axisApp.dispose();
  }
}
const composedApp=new c.App({backend:'headless'});
try {
 if(composedApp.supportsVirtualListRows()) {
  const content=c.label('Composed content'),entry=c.item('entry').children(content);
  const list=c.virtual_list('composed').children(entry);
  composedApp.mount(c.column().children(list));
  list.setVirtualListRows([{item:entry,extent:0}]);list.removeVirtualListItem(entry);
  assert.equal(content.owner,null);list.setVirtualListRows([]);composedApp.step();
 }
} finally {composedApp.dispose();}
