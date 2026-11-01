'use strict';
// Synthetic provider overrides get_config only: no UI/painter evidence claimed.
const assert=require('node:assert/strict');const c=require('..');const native=require(process.env.CUI_NODE_ADDON);const app=new c.App();
try {
 assert.equal(app.supports_slider_model(),true);
 const node=native.makeNode(app.handle,null,2,'config-fixture');
 assert.throws(()=>native.sliderModel(app.handle,4,node,null,null),/invalid Slider config layout/);
 assert.throws(()=>native.sliderModel(app.handle,3,node,null,null),/expected Slider scalar array/);
 console.log('PASS native malformed inactive parent length and malformed argument rejection');
}finally{app.dispose();}
