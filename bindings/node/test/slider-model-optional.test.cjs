'use strict';
// Distinct process uses only the addon linked against the supplied old bridge.
const assert=require('node:assert/strict');const c=require('..');const app=new c.App();
try {
 const expected=process.env.CUI_SLIDER_FIXTURE==='complete';
 assert.equal(app.supports_slider_model(),expected);
 const node=c.slider_model('typed',{},c.slider_single(40));
 assert.throws(()=>app.mount(c.column().children(node)),expected?/invalid state|Invalid state/i:/unavailable/);
 assert.equal(app.nodes.size,0);assert.equal(app.root,null);
 // Failed typed preflight leaves old trees usable; complete fixture enable intentionally fails.
 app.mount(c.column().children(c.slider('legacy').range(0,100,1).value(40)));app.step();
 console.log('PASS optional group '+process.env.CUI_SLIDER_FIXTURE);
}finally{app.dispose();}
