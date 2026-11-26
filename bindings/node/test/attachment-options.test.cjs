'use strict';
const test=require('node:test'), assert=require('node:assert/strict');
const {App,attachment,column,label}=require('..');
const native=require(process.env.CUI_NODE_ADDON);
function withApp(fn){const app=new App();try{fn(app);}finally{app.dispose();}}
test('Attachment options validate before mutation and own pending bytes',()=>{
  for(const options of [{vertical:1},{mediaAspect:NaN},{mediaAspect:.01},{titleStatus:5},{titleStatus:0},{descriptionStatus:"done"},{title:{weight:4}},{title:{colorRgba:-1}},{title:{italic:1}},{title:{fontSize:Infinity}}])assert.throws(()=>attachment().attachment_options(options));
  const node=attachment('copy','Title'), data=new Uint8Array([255,0,0,255]);
  node.attachment_preview_rgba(data,1,1);data.fill(0);
  for(const args of [[new Uint8Array(3),1,1],[new Uint8Array(4),1.5,1],[new Uint8Array(4),1,1,3],[new Uint16Array(2),1,1],[{},1,1]])assert.throws(()=>node.attachment_preview_rgba(...args));
  assert.throws(()=>label().attachment_options(null));
  assert.throws(()=>attachment('bad','two\nlines').attachment_options({}));
  const styled=attachment('styled','Title').attachment_options({});
  assert.throws(()=>styled.label('two\nlines'));assert.throws(()=>styled.detail('two\rlines'));
  styled.attachment_options(null).label('two\nlines');
  withApp(app=>{
    const original=native.attachmentOptions;
    let copied;
    native.attachmentOptions=function(...args){if(args[1]===4)copied=Buffer.from(args[3]);return original(...args);};
    try{app.mount(column().children(node)).step();}finally{native.attachmentOptions=original;}
    assert.deepEqual(copied,Buffer.from([255,0,0,255]));
    const pending=Object.getOwnPropertySymbols(node).find(s=>s.description==='Attachment preview');
    assert.equal(node[pending],null);
  });
});
test('Attachment retained geometry, replacement and independent reset',()=>withApp(app=>{
  assert.equal(app.supports_attachment_options(),true);assert.equal(app.supports_attachment_preview(),true);
  const node=attachment('card','Document title').detail('Copied media').visual_size(240,0).design_size('lg')
    .attachment_options({vertical:true,media:true,mediaAspect:2,titleStatus:'complete',title:{fontSize:20,colorRgba:0}}).attachment_preview_rgba(new Uint8Array([255,0,0,255]),1,1);
  app.mount(column().children(node)).step();const vertical=node.bounds();assert.equal(vertical.width,240);
  node.attachment_options({media:true}).attachment_preview_rgba(new Uint8Array([0,255,0,255]),1,1);app.step();assert.ok(node.bounds().height<vertical.height);
  node.clear_attachment_preview().attachment_options(null);app.step();assert.equal(node.bounds().width,240);
  assert.throws(()=>node.attachment_options({title:{letterSpacing:1}}));
  node.attachment_options({vertical:true});app.step();
}));
test('Attachment addon validates binary config and preview buffers',()=>withApp(app=>{
  const root=native.makeNode(app.handle,null,2,'root'), node=native.makeNode(app.handle,root,52,'card');
  assert.throws(()=>native.attachmentOptions(app.handle,2,node,{}));
  assert.throws(()=>native.attachmentOptions(app.handle,2,node,Buffer.alloc(79)));
  assert.throws(()=>native.attachmentOptions(app.handle,4,node,new Uint16Array(2),1,1,4));
  assert.throws(()=>native.attachmentOptions(app.handle,4,node,new Uint8Array(3),1,1,4));
  assert.throws(()=>native.attachmentOptions(app.handle,5,root));
  native.removeNode(app.handle,node);assert.throws(()=>native.attachmentOptions(app.handle,5,node));
}));
test('Attachment ordinary legacy tree remains usable without optional addon capability',()=>withApp(app=>{
  const original=native.attachmentOptions;native.attachmentOptions=undefined;
  try{
    assert.equal(app.supports_attachment_options(),false);assert.equal(app.supports_attachment_preview(),false);
    assert.throws(()=>app.mount(column().children(attachment().attachment_options({}))),/optional/);assert.equal(app.root,null);
    const node=attachment('ordinary','Legacy');app.mount(column().children(node)).step();
    assert.throws(()=>node.clear_attachment_preview(),/optional/);
  }finally{native.attachmentOptions=original;}
}));

test('Attachment optional config and preview are independent and failures preserve geometry',()=>withApp(app=>{
  const original=native.attachmentOptions;
  native.attachmentOptions=function(...args){if(args[1]===0)return false;return original(...args);};
  let node;
  try {
    assert.equal(app.supports_attachment_options(),false);assert.equal(app.supports_attachment_preview(),true);
    node=attachment('independent','Title').attachment_preview_rgba(new Uint8Array([255,0,0,255]),1,1).visual_size(240,0);
    app.mount(column().children(node)).step();
    assert.throws(()=>node.attachment_options({vertical:true}),/optional/);
  } finally {native.attachmentOptions=original;}
  node.attachment_options({vertical:true,media:true});app.step();const before=node.bounds();
  native.attachmentOptions=function(...args){if(args[1]===2||args[1]===4)throw new Error('injected setter failure');return original(...args);};
  try {
    assert.throws(()=>node.attachment_options({}),/injected/);
    assert.throws(()=>node.attachment_preview_rgba(new Uint8Array(4),1,1),/injected/);
  } finally {native.attachmentOptions=original;}
  app.step();assert.deepEqual(node.bounds(),before);
}));

test('Attachment mounted previews borrow synchronously and rollback keeps pending bytes',()=>{
  const pendingKey=node=>Object.getOwnPropertySymbols(node).find(s=>s.description==='Attachment preview');
  withApp(app=>{
    const first=attachment('first','Title').attachment_preview_rgba(new Uint8Array([255,0,0,255]),1,1);
    const later=attachment('later','Title').attachment_options({title:{letterSpacing:1}});
    assert.throws(()=>app.mount(column().children(first,later)));
    assert.deepEqual(first[pendingKey(first)][0],Buffer.from([255,0,0,255]));
  });
  withApp(app=>{
    const card=attachment('card','Title');
    app.mount(column().children(card));
    const original=native.attachmentOptions;
    const pixels = new Uint8Array([0, 255, 0, 255]);
    const shared = new Uint8Array(new SharedArrayBuffer(4));
    shared.set([255, 0, 0, 255]);
    const submitted = [];
    native.attachmentOptions = function(...args) {
      if (args[1] === 4) submitted.push(args[3]);
      return original(...args);
    };
    try {
      card.attachment_preview_rgba(pixels, 1, 1);
      card.attachment_preview_rgba(shared, 1, 1);
    } finally {
      native.attachmentOptions = original;
    }
    assert.equal(submitted[0], pixels);
    assert.notEqual(submitted[1].buffer, shared.buffer);
    assert.deepEqual([...submitted[1]], [255, 0, 0, 255]);
    assert.equal(card[pendingKey(card)],null);
    pixels.fill(0);
    app.step();
  });
});
