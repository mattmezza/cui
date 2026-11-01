'use strict';
// Each process uses an actual older bridge plus optionally one partial provider.
const assert=require('node:assert/strict');const c=require('..');const app=new c.App();
try{
 const complete=process.env.CUI_TIME_FIXTURE==='complete';assert.equal(app.supports_localized_time_field(),complete);
 assert.throws(()=>app.mount(c.column().children(c.localized_time_field('typed'))),complete?/invalid state/i:/unavailable/);
 assert.equal(app.nodes.size,0);assert.equal(app.root,null);
 app.mount(c.column().children(c.time_field('legacy').time_value(9,45,30)));app.step();
 assert.deepEqual(app.root.childrenValue[0].current_time(),[9,45,30]);
}finally{app.dispose();}
console.log('PASS optional localized TimeField fallback '+process.env.CUI_TIME_FIXTURE);
