'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const {App,column,button,popover}=require('..');
test('Popover authoring validation and unsupported preflight',()=>{
  for(const c of [{anchor:8},{anchor:true},{width:NaN},{height:4097},{offset:-4097},{reference_rem:257},{arrow:1},{unknown:2}])assert.throws(()=>popover().popover_options(c));
  const app=new App();try{
    const trigger=button('trigger'),p=popover('p').anchor(trigger).popover_options({width:180});
    const root=column().children(trigger,p);
    if(!app.supports_popover_options()){
      assert.throws(()=>app.mount(root),/compatible bridge/);assert.equal(app.root,null);
      p.popover_options(null);app.mount(root).step();p.open();app.step();assert.equal(p.is_open(),true);p.open(false);
    }
  }finally{app.dispose();}
});
test('Popover placement, Button callback opening, content focus and Escape',()=>{
  const app=new App({width:640,height:480});try{
    if(process.env.CUI_POPOVER_UNSUPPORTED === "1"){assert.equal(app.supports_popover_options(),false);return;}
    assert.equal(app.supports_popover_options(),true);
    let activated=0,closed=0;
    const trigger=button('Account'),action=button('Profile').on_click(()=>activated++);
    const p=popover('menu').anchor(trigger).popover_options({anchor:0,width:180,height:120,hide_close:true,focus_content:true}).children(action).on_close(()=>closed++);
    trigger.on_click(()=>p.open());app.mount(column().children(trigger,p)).step();
    const b=trigger.bounds(),x=b.x+b.width/2,y=b.y+b.height/2;
    app.pointer('down',x,y).pointer('up',x,y).step();app.step();
    assert.equal(p.is_open(),true);assert.ok(Math.abs(p.bounds().width-180)<1);assert.ok(p.bounds().y>=b.y+b.height);
    app.key('down','enter').key('up','enter').step();assert.equal(activated,1);
    app.key('down','escape').key('up','escape').step();assert.equal(p.is_open(),false);assert.equal(closed,1);
    p.popover_options(null);
  }finally{app.dispose();}
});
