'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const {App,IconSource,column,rating}=require('..');
const native=require(process.env.CUI_NODE_ADDON);
function withApp(fn){const app=new App();try{fn(app);}finally{app.dispose();}}
test('Rating validation and legacy capability fallback',()=>withApp(app=>{
  assert.equal(app.supports_rating_options(),process.env.CUI_RATING_UNSUPPORTED!=='1');
  for(const config of [{halfSteps:1},{width:NaN},{width:.5},{width:4097},{fillRgba:-1},{fillIcon:{}},{designSize:'huge'}])
    assert.throws(()=>rating().configure(config));
  if(!app.supports_rating_options()){
    assert.throws(()=>app.mount(column().children(rating().configure({halfSteps:true}))),/compatible bridge/);
    assert.equal(app.root,null);
    const node=rating('legacy').value(2);
    app.mount(column().children(node)).step();
    assert.equal(node.current_value(),2);
    assert.equal(node.get_config(),undefined);
    assert.throws(()=>node.configure({}),/compatible bridge/);
  }
}));
test('Rating exact fractional callback, preview, readonly and reset',()=>withApp(app=>{
  if(!app.supports_rating_options())return;
  const changes=[];
  const node=rating('rating').value(1.5).configure({halfSteps:true,hoverPreview:true,stepDownFilled:true,clearable:true,designSize:'lg',width:300,fillRgba:0}).on_change(e=>changes.push(e.value));
  const config=node.get_config();
  app.mount(column().children(node)).step();
  assert.deepEqual(node.get_config(),config);
  const b=node.numeric_part_bounds(13),x=b.x+b.width/4,y=b.y+b.height/2;
  app.pointer('move',x,y).step();
  assert.equal(node.current_value(),1.5);assert.deepEqual(changes,[]);
  app.pointer('down',x,y).pointer('up',x,y).step();
  assert.equal(node.current_value(),3.5);assert.deepEqual(changes,[3.5]);
  assert.throws(()=>node.configure({width:.5}));assert.deepEqual(node.get_config(),config);
  node.configure({...config,readonly:true});
  app.pointer('down',x,y).pointer('up',x,y).step();assert.deepEqual(changes,[3.5]);
  node.reset_config();assert.equal(node.get_config(),undefined);
}));
test('Rating mask app affinity and pin release preserve configuration',()=>withApp(app=>withApp(other=>{
  if(!app.supports_rating_options())return;
  const mask=app.icon_resource(IconSource.named('heart')),foreign=other.icon_resource(IconSource.named('heart'));
  const node=rating('masked').configure({outlineIcon:mask,fillIcon:mask});
  app.mount(column().children(node)).step();
  assert.equal(node.get_config().fillIcon,mask);
  assert.throws(()=>mask.close());
  assert.throws(()=>node.configure({fillIcon:foreign}));
  assert.equal(node.get_config().fillIcon,mask);
  node.reset_config();mask.close();foreign.close();
  assert.throws(()=>node.configure({fillIcon:mask}));
})));

test('direct Rating transaction rejects invalid flags and stale nodes',()=>withApp(app=>{
  if(!app.supports_rating_options())return;
  const root=native.makeNode(app.handle,null,2,'root');
  const node=native.makeNode(app.handle,root,43,'rating');
  native.ratingOptions(app.handle,1,node,1,0,300,0,0,0,null,null);
  const before=native.ratingOptions(app.handle,2,node);
  assert.throws(()=>native.ratingOptions(app.handle,1,node,256,0,300,0,0,0,null,null));
  assert.deepEqual(native.ratingOptions(app.handle,2,node),before);
  assert.throws(()=>native.ratingOptions(app.handle,1,root,0,0,0,0,0,0,null,null));
  native.removeNode(app.handle,node);
  assert.throws(()=>native.ratingOptions(app.handle,2,node));
}));
