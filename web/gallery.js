/* SPDX-License-Identifier: MIT */
const $ = id => document.getElementById(id);
const canvas = $('preview'), ctx = canvas.getContext('2d');
const decoder = new TextDecoder(), encoder = new TextEncoder();
const stories = {Button:0,Checkbox:1,Switch:2,Toggle:3,Radio:4,Slider:5,Input:6,Label:7,Badge:8,Progress:9,Theme:11,Alert:12,Avatar:13,Bubble:14,Empty:15,GroupBox:16,Kbd:17,Skeleton:18,Spinner:19,Accordion:20,Collapsible:21,Tabs:22,Tooltip:23,HoverCard:24,Editor:25,VirtualList:26,Dock:27,Plot:28,Calendar:29,List:30,Select:31,Menu:32};
const modelStories = new Set(['Editor','VirtualList','Dock','Plot','Calendar']);
const snippets = {
 Button:'if (cui_button(ui, id, &theme,\n    cui_string("Run action"), false)) {\n    run_action();\n}',
 Checkbox:'cui_checkbox(ui, id, &theme,\n    cui_string("Receive updates"),\n    &checked, false);',
 Switch:'cui_switch(ui, id, &theme,\n    cui_string("Notifications"),\n    &enabled, false);',
 Toggle:'cui_toggle(ui, id, &theme,\n    cui_string("Pin this item"),\n    &pinned, false);',
 Radio:'cui_radio(ui, id, &theme,\n    cui_string("Priority"),\n    &selection, 1, false);',
 Slider:'cui_slider(ui, id, &theme,\n    cui_string("Volume"),\n    &value, 0, 100, 1, false);',
 Input:'cui_input(ui, id, &theme,\n    cui_string("Display name"),\n    edit, false);',
 Label:'cui_label(ui, &theme,\n    cui_string("A useful label"));',
 Badge:'cui_badge(ui, id, &theme,\n    cui_string("In review"));',
 Progress:'cui_progress(ui, id, &theme,\n    progress / 100.0f, 300);',

 Editor:'cui_edit_undo(edit);\ncui_edit_redo(edit);',
 VirtualList:'cui_virtual_view view = cui_virtual_viewport(\n    list, scroll, viewport_height, 1);',
 Dock:'cui_dock_split(dock, tab,\n    CUI_DOCK_SPLIT_HORIZONTAL, 0.5,\n    new_panel_key, false, &new_tab);',
 Plot:'cui_plot_zoom(&scale, 1.5, anchor_pixel);\ncui_plot_ticks(&scale, 5, ticks, 32, &count);',
 Calendar:'date = cui_date_add_months(date, 1);\nint days = cui_date_days_in_month(\n    date.year, date.month);',
 List:'cui_list(ui, id, &theme, items, 3,\n    &selection, &state, options);',
 Select:'cui_select(ui, id, &theme, label, placeholder,\n    items, 3, &selection, &popup, options);',
 Menu:'if (cui_menu(ui, id, &theme, label,\n    items, 3, &action, &popup, options)) {\n    dispatch(action);\n}',
 Accordion:'cui_accordion(ui, id, &theme, items, 3,\n    expanded, options, draw_panel, user);',
 Collapsible:'cui_collapsible(ui, id, &theme, label,\n    &open, false, draw_panel, user);',
 Tabs:'cui_tabs(ui, id, &theme, items, 3,\n    &selection, options, draw_panel, user);',
 Avatar:'cui_avatar(ui, id, &theme,\n    cui_string("Alex Morgan"), 56);',
 Kbd:'cui_kbd(ui, id, &theme,\n    cui_string("Ctrl + K"));',
 Theme:'cui_theme theme = cui_theme_dark();\ncui_button(ui, id, &theme,\n    cui_string("Primary action"), false);'
};
let wasm, catalog, selected, framePending=false, lastTime=performance.now();
const reducedMotion=matchMedia('(prefers-reduced-motion: reduce)');
function stringAt(pointer,length){return decoder.decode(new Uint8Array(wasm.memory.buffer,pointer,length));}
function fatal(error){$('preview-note').textContent=`Browser preview unavailable: ${error.message}`; console.error(error);}
function wasiImports(){
 return {
  proc_exit:code=>{throw new Error(`CUI exited ${code}`)},
  fd_close:()=>8,fd_seek:()=>8,fd_fdstat_get:()=>8,
  fd_write:(fd,iovs,count,written)=>{const dv=new DataView(wasm.memory.buffer);let size=0;for(let i=0;i<count;i++){size+=dv.getUint32(iovs+i*8+4,true);}dv.setUint32(written,size,true);return 0;},
  environ_sizes_get:(count,bytes)=>{const dv=new DataView(wasm.memory.buffer);dv.setUint32(count,0,true);dv.setUint32(bytes,0,true);return 0;},
  environ_get:()=>0,
  clock_time_get:(clock,precision,ptr)=>{new DataView(wasm.memory.buffer).setBigUint64(ptr,BigInt(Math.round(performance.now()*1e6)),true);return 0;},
  random_get:(ptr,length)=>{crypto.getRandomValues(new Uint8Array(wasm.memory.buffer,ptr,length));return 0;}
 };
}
function requestFrame(){if(framePending||!wasm)return;framePending=true;requestAnimationFrame(()=>{framePending=false;try{render();}catch(e){fatal(e);}});}
function render(){
 if(!selected || !(selected.name in stories))return;
 const width=canvas.clientWidth,height=260,dpr=Math.min(devicePixelRatio||1,3);
 canvas.width=Math.round(width*dpr);canvas.height=height*dpr;ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
 const now=performance.now();wasm.gallery_time(now/1000,Math.min((now-lastTime)/1000,.1),Number(reducedMotion.matches));lastTime=now;
 if(!wasm.gallery_frame(stories[selected.name],Number($('theme').value),Number($('disabled').checked),width,height))throw Error('CUI layout failed');
 let clips=0;
 for(let i=0;i<wasm.gallery_count();i++){
  const f=n=>wasm.gallery_command(i,n);const type=f(0),x=f(1),y=f(2),w=f(3),h=f(4);
  if(type===3){ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();clips++;continue;}
  if(type===4){if(clips){ctx.restore();clips--;}continue;}
  ctx.fillStyle=ctx.strokeStyle=`rgba(${f(5)},${f(6)},${f(7)},${f(8)/255})`;
  if(type===2){ctx.font=`${f(11)}px system-ui`;ctx.textBaseline='top';ctx.fillText(stringAt(wasm.gallery_command_text(i),wasm.gallery_command_length(i)),x,y);}
  else {ctx.beginPath();const stroke=type===1?f(10):0, inset=stroke/2;
   ctx.roundRect(x+inset,y+inset,Math.max(0,w-stroke),Math.max(0,h-stroke),Math.max(0,f(9)-inset));
   if(type===1){ctx.lineWidth=stroke;ctx.stroke();}else ctx.fill();}
 }
 while(clips-- >0)ctx.restore();
 syncSemantics();
 if(wasm.gallery_redraw())requestFrame();
 $('preview-note').textContent=`${modelStories.has(selected.name)?'Partial base model · ':''}${wasm.gallery_count()} draw commands · ${wasm.gallery_semantic_count()} semantic nodes · Mouse, Tab, Enter / Space and arrow keys`;
}
function syncSemantics(){
 const root=$('accessible');root.replaceChildren();
 for(let i=0;i<wasm.gallery_semantic_count();i++){
  const f=n=>wasm.gallery_semantic(i,n),role=['','button','checkbox','switch','radio','slider','textbox','text','tab','group','tablist','tabpanel','list','listbox','option','menu','menuitem','dialog','alert','status','img','progressbar','tree','treeitem'][f(0)];
  if(!role || ![1,2,3,4,5,6,8,14,16,23].includes(f(0)))continue;
  const el=document.createElement('button');el.type='button';el.tabIndex=-1;el.textContent=stringAt(wasm.gallery_semantic_name(i),wasm.gallery_semantic_length(i));el.setAttribute('role',role);el.disabled=Boolean(f(5));
  if(['checkbox','switch','radio'].includes(role))el.setAttribute('aria-checked',String(Boolean(f(6))));
  if(role==='slider'){el.setAttribute('aria-valuenow',String(f(8)));el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));}
  if(role==='tab'||role==='option')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(selected.name==='Accordion'||selected.name==='Collapsible'||((selected.name==='Select'||selected.name==='Menu')&&f(0)===1))el.setAttribute('aria-expanded',String(Boolean(f(10))));
  el.onclick=()=>{wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};
  root.append(el);
 }
}
function navigation(){
 const query=$('search').value.toLowerCase(), nav=$('catalog');nav.replaceChildren();let category='';
 for(const item of catalog.components.filter(item=>item.name.toLowerCase().includes(query))){
  if(item.category!==category){category=item.category;const h=document.createElement('h3');h.textContent=category.replaceAll('_',' ');nav.append(h);}
  const a=document.createElement('a');a.href=`#${encodeURIComponent(item.name)}`;a.classList.toggle('active',selected?.name===item.name);if(selected?.name===item.name)a.setAttribute('aria-current','page');
  const text=document.createElement('span');text.textContent=item.name;const dot=document.createElement('span');dot.className=`dot ${item.name in stories?'live':''}`;dot.title=item.name in stories?'Live browser story':'Browser story pending';a.append(text,dot);nav.append(a);
 }
}
function selectComponent(){
 const name=decodeURIComponent(location.hash.slice(1))||'Button';selected=catalog.components.find(c=>c.name===name)||catalog.components.find(c=>c.name==='Button');
 document.title=`${selected.name} · CUI`; $('name').textContent=selected.name; $('status').textContent=selected.status.replaceAll('_',' ');
 const live=selected.name in stories;
 if(modelStories.has(selected.name))$('status').textContent+=' / base model'; $('preview-card').hidden=!live; $('pending').hidden=live;
 $('description').textContent=live?`Explore ${selected.name.toLowerCase()} through the same CUI core, base behavior and theme used by native applications.`:`${selected.name} is part of the complete GPUI Kit 0.7.1 comparison catalog.`;
 if(modelStories.has(selected.name))$('description').textContent='Interactive demonstration of the implemented base model. The complete styled component remains pending.';
 $('pending-message').textContent=selected.status==='not_started'?'Implementation and browser story are pending. This entry stays visible to preserve the complete component scope.':'Repository implementation exists for part of this entry. A browser story is pending; see implementation scope and source for details.';
 $('snippet').textContent=snippets[selected.name]||(live?`/* See the public ${selected.public_header || 'component'} API. */`:'/* No complete styled browser example yet. */');
 $('source').href=`https://github.com/mattmezza/cui/${selected.public_header?'blob/main/'+selected.public_header:'blob/main/docs/gpui-parity.json'}`;
 $('limitations').replaceChildren();for(const message of [...selected.limitations,...(live?['Canvas text uses the browser system font; native FreeType / HarfBuzz rendering is separate.','Browser semantics are experimental. IME composition and a complete platform accessibility bridge remain pending.']:[])]){const li=document.createElement('li');li.textContent=message;$('limitations').append(li);}
 $('disabled').checked=false;document.body.classList.remove('menu-open');navigation();requestFrame();
}
const keys={Tab:1,Enter:2,' ':3,Escape:4,ArrowLeft:5,ArrowRight:6,ArrowUp:7,ArrowDown:8,Home:9,End:10,Backspace:11,Delete:12,a:13,A:13};
function mods(e){return(e.shiftKey?1:0)|(e.ctrlKey?2:0)|(e.altKey?4:0)|(e.metaKey?8:0);}
for(const [event,type] of [['pointermove',0],['pointerdown',1],['pointerup',2]])canvas.addEventListener(event,e=>{
 if(!wasm)return;const rect=canvas.getBoundingClientRect();if(type===1){canvas.focus();canvas.setPointerCapture(e.pointerId);}wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0);requestFrame();
});
canvas.addEventListener('pointerleave',()=>{wasm?.gallery_event(0,-100,-100,0,0,0);requestFrame();});
canvas.addEventListener('pointercancel',()=>{wasm?.gallery_event(7,0,0,0,0,0);requestFrame();});
canvas.addEventListener('blur',()=>{wasm?.gallery_event(7,0,0,0,0,0);requestFrame();});
for(const [event,type] of [['keydown',3],['keyup',4]])canvas.addEventListener(event,e=>{
 if(!wasm)return;const key=keys[e.key]||0;
 if(e.key==='Tab'){
  const enabled=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).filter(i=>!wasm.gallery_semantic(i,5)&&wasm.gallery_semantic(i,9)===0&&[1,2,3,4,5,6,8,14,16,23].includes(wasm.gallery_semantic(i,0)));
  const focused=enabled.findIndex(i=>wasm.gallery_semantic(i,7));
  if(!wasm.gallery_modal()&&(!enabled.length||(e.shiftKey&&focused===0)||(!e.shiftKey&&focused===enabled.length-1&&focused>=0)))return;
 }
 if(key){e.preventDefault();wasm.gallery_event(type,0,0,key,mods(e),Number(e.repeat));}
 if(type===3 && e.key.length===1 && !e.ctrlKey&&!e.metaKey&&!e.altKey && (selected.name==='Input'||selected.name==='Editor')){
  e.preventDefault();const bytes=encoder.encode(e.key),pointer=wasm.malloc(bytes.length);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);
 }requestFrame();
});
$('search').addEventListener('input',navigation);window.addEventListener('hashchange',()=>{if(catalog)selectComponent();});
$('theme').addEventListener('change',()=>{document.body.classList.toggle('dark',$('theme').value==='1');document.body.classList.toggle('contrast',$('theme').value==='2');requestFrame();});
$('disabled').addEventListener('change',requestFrame);$('menu').onclick=()=>document.body.classList.toggle('menu-open');new ResizeObserver(requestFrame).observe(canvas);
try{
 const [response,data]=await Promise.all([fetch('./gallery.wasm'),fetch('./catalog.json').then(r=>r.json())]);if(!response.ok)throw Error('Cannot fetch WebAssembly module');catalog=data;
 const module=await WebAssembly.compile(await response.arrayBuffer());
 const imports={wasi_snapshot_preview1:wasiImports(),host:{measure:(ptr,len,size)=>{ctx.font=`${size}px system-ui`;return ctx.measureText(stringAt(ptr,len)).width;}}};
 const instance=await WebAssembly.instantiate(module,imports);wasm=instance.exports;wasm._initialize();if(!wasm.gallery_init())throw Error('CUI initialization failed');
 window.cuiGallery={exports:wasm,stories,catalog,render};selectComponent();
}catch(error){fatal(error);}
