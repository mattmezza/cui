/* SPDX-License-Identifier: MIT */
const $ = id => document.getElementById(id);
const canvas = $('preview'), ctx = canvas.getContext('2d');
const decoder = new TextDecoder(), encoder = new TextEncoder();
const stories = {Button:0,Checkbox:1,Switch:2,Toggle:3,Radio:4,Slider:5,Input:6,Label:7,Badge:8,Progress:9,Theme:11,Alert:12,Avatar:13,Bubble:14,Empty:15,GroupBox:16,Kbd:17,Skeleton:18,Spinner:19,Accordion:20,Collapsible:21,Tabs:22,Tooltip:23,HoverCard:24,Editor:25,VirtualList:26,Dock:27,Plot:28,Calendar:29,List:30,Select:31,Menu:32,NumberInput:33,Pagination:34,Rating:35,Stepper:36,Combobox:37,'Focus Trap':38,Clipboard:39,DatePicker:40,TimeField:41,ColorPicker:42,Tree:43,Scrollable:44,DataTable:45,Table:46};
const modelStories = new Set(['Editor','VirtualList','Dock','Plot','Focus Trap']);
const snippets = {
 Button:'cui_button_ex(ui, id, &theme, label,\n    (cui_button_options){\n        .variant = CUI_BUTTON_OUTLINE,\n        .size = CUI_CONTROL_SMALL\n    });',
 Checkbox:'cui_checkbox(ui, id, &theme,\n    cui_string("Receive updates"),\n    &checked, false);',
 Switch:'cui_switch(ui, id, &theme,\n    cui_string("Notifications"),\n    &enabled, false);',
 Toggle:'cui_toggle(ui, id, &theme,\n    cui_string("Pin this item"),\n    &pinned, false);',
 Radio:'cui_radio(ui, id, &theme,\n    cui_string("Priority"),\n    &selection, 1, false);',
 Slider:'cui_slider(ui, id, &theme,\n    cui_string("Volume"),\n    &value, 0, 100, 1, false);',
 Input:'cui_input_ex(ui, id, &theme, label, edit,\n    &state, (cui_input_options){.width = 280});',
 Scrollable:'if (cui_scrollable_begin(ui, id, &theme,\n    &state, options)) {\n    draw_content(ui);\n    cui_scrollable_end(ui, id, &theme, &state, options);\n}',
 DataTable:'cui_data_table(ui, id, &theme, model,\n    &state, options, cell_label, draw_cell, user);',
 Table:'cui_table_begin(ui, id, &theme, width);\n/* Compose rows and cells; end each begin. */\ncui_table_end(ui);',
 Tree:'cui_tree(ui, id, &theme, model, &state,\n    &selection, options);',
 Label:'cui_label(ui, &theme,\n    cui_string("A useful label"));',
 Badge:'cui_badge(ui, id, &theme,\n    cui_string("In review"));',
 Progress:'cui_progress(ui, id, &theme,\n    progress / 100.0f, 300);',

 Editor:'cui_edit_undo(edit);\ncui_edit_redo(edit);',
 VirtualList:'cui_virtual_view view = cui_virtual_viewport(\n    list, scroll, viewport_height, 1);',
 Dock:'cui_dock_split(dock, tab,\n    CUI_DOCK_SPLIT_HORIZONTAL, 0.5,\n    new_panel_key, false, &new_tab);',
 Plot:'cui_plot_zoom(&scale, 1.5, anchor_pixel);\ncui_plot_ticks(&scale, 5, ticks, 32, &count);',
 Calendar:'cui_calendar(ui, id, &theme, &state,\n    &selection, options);',
 DatePicker:'cui_date_picker(ui, id, &theme, label,\n    &state, &selection, options);',
 ColorPicker:'cui_color_picker(ui, id, &theme,\n    &state, &color, options);',
 TimeField:'cui_time_field(ui, id, &theme, label,\n    &state, &time, options);',
 Combobox:'cui_combobox(ui, id, &theme, label, query,\n    items, 4, scratch, 4, &selection,\n    &popup, options);',
 NumberInput:'cui_number_input(ui, id, &theme, label,\n    &state, &value, options);',
 Pagination:'cui_pagination(ui, id, &theme,\n    &page, total_pages, true, false);',
 Rating:'cui_rating(ui, id, &theme, label,\n    &rating, options);',
 Stepper:'cui_stepper(ui, id, &theme, steps, 3,\n    &current_step, true, false);',
 List:'cui_list(ui, id, &theme, items, 3,\n    &selection, &state, options);',
 Select:'cui_select(ui, id, &theme, label, placeholder,\n    items, 3, &selection, &popup, options);',
 Menu:'cui_menu_tree(ui, id, &theme, label, entries,\n    count, &action, &state, options);',
 ContextMenu:'/* Call after painting the receiving area. */\ncui_context_menu(ui, target, &theme, label,\n    entries, count, &action, &state, options);',
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
 const width=canvas.clientWidth,height=selected.name==='Calendar'?420:selected.name==='DatePicker'?380:selected.name==='ColorPicker'?620:selected.name==='DataTable'?340:['Menu','ContextMenu'].includes(selected.name)?420:260,dpr=Math.min(devicePixelRatio||1,3);
 canvas.style.height=`${height}px`;
 canvas.width=Math.round(width*dpr);canvas.height=height*dpr;ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
 wasm.gallery_input_mode(Number($('input-mode').value));
 wasm.gallery_density(Number($('density').value));
 wasm.gallery_selection_mode(Number($('selection-mode').value));
 const now=performance.now();wasm.gallery_time(now/1000,Math.min((now-lastTime)/1000,.1),Number(reducedMotion.matches));lastTime=now;
 if(!wasm.gallery_frame(selected.name==='Menu'&&$('menu-kind').value==='1'?47:stories[selected.name],Number($('theme').value),Number($('disabled').checked),width,height))throw Error('CUI layout failed');
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
 window.cuiGallery.renderedComponent=selected.name;window.cuiGallery.frame=(window.cuiGallery.frame||0)+1;
 const clipboard=wasm.gallery_clipboard_request();if(clipboard)void clipboardAction(clipboard);
 if(wasm.gallery_redraw())requestFrame();
 $('preview-note').textContent=`${modelStories.has(selected.name)?'Partial base model · ':''}${wasm.gallery_count()} draw commands · ${wasm.gallery_semantic_count()} semantic nodes · Mouse, Tab, Enter / Space and arrow keys`;
}
async function clipboardAction(action){
 let okay=1,text;
 try {if(!navigator.clipboard)throw Error('unavailable');if(action===1){await navigator.clipboard.writeText('Hello from CUI');text='Copied: Hello from CUI';}else text=`Read: ${await navigator.clipboard.readText()}`;}
 catch {okay=0;text='';}
 const bytes=encoder.encode(text),pointer=wasm.malloc(bytes.length+1);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_clipboard_result(okay,pointer,bytes.length);wasm.free(pointer);requestFrame();
}
const semanticNodes = new Map();
const roles=['','button','checkbox','switch','radio','slider','textbox','text','tab','group','tablist','tabpanel','list','listbox','option','menu','menuitem','dialog','alert','status','img','progressbar','tree','treeitem','menuitemcheckbox','menuitemradio','table','row','cell','columnheader','gridcell','grid','scrollbar'];
const interactiveRoles=[1,2,3,4,5,6,8,14,16,23,24,25,29,30,32];
function syncSemantics(){
 const root=$('accessible'),seen=new Set(),entries=[];
 for(let i=0;i<wasm.gallery_semantic_count();i++){
  const f=n=>wasm.gallery_semantic(i,n),role=roles[f(0)];if(!role||role==='text')continue;
  const id=`cui-semantic-${wasm.gallery_semantic_id(i)}`;seen.add(id);
  let el=semanticNodes.get(id);
  if(!el){el=document.createElement(interactiveRoles.includes(f(0))&&!['treeitem','columnheader'].includes(role)?'button':'div');el.id=id;if(el.tagName==='BUTTON'){el.type='button';el.tabIndex=-1;}semanticNodes.set(id,el);}
  el.setAttribute('role',role);
  const name=stringAt(wasm.gallery_semantic_name(i),wasm.gallery_semantic_length(i));
  el.setAttribute('aria-label',name);
  if(role==='columnheader'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};}
  if(el.tagName==='BUTTON'){if(el.textContent!==name)el.textContent=name;el.disabled=Boolean(f(5));el.onclick=e=>{e.stopPropagation();wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};}
  else el.setAttribute('aria-label',name);
  if(['checkbox','switch','radio','menuitemcheckbox','menuitemradio'].includes(role))el.setAttribute('aria-checked',String(Boolean(f(6))));
  el.setAttribute('aria-busy',String(Boolean(f(14))));
  if(['listbox','tree'].includes(role))el.setAttribute('aria-multiselectable',String(Boolean(f(15))));
  if(role==='treeitem'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};el.setAttribute('aria-level',String(f(30)||f(8)+1));if(f(31))el.setAttribute('aria-posinset',String(f(31)));if(f(32))el.setAttribute('aria-setsize',String(f(32)));el.setAttribute('aria-disabled',String(Boolean(f(5))));}
  if(role==='tab'||role==='option'||role==='treeitem')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(f(16))el.setAttribute('aria-orientation',f(16)===2?'vertical':'horizontal');else el.removeAttribute('aria-orientation');
  for(const [field,attribute] of [[22,'aria-rowindex'],[23,'aria-colindex'],[24,'aria-rowcount'],[25,'aria-colcount']]){if(f(field))el.setAttribute(attribute,String(f(field)));else el.removeAttribute(attribute);}
  if(role==='gridcell'||role==='columnheader'||role==='row')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(role==='columnheader')el.setAttribute('aria-sort',f(8)===1?'ascending':f(8)===2?'descending':'none');
  if(role==='textbox'){el.setAttribute('aria-readonly',String(Boolean(f(26))));el.setAttribute('aria-required',String(Boolean(f(27))));el.setAttribute('aria-invalid',String(Boolean(f(28))));el.setAttribute('aria-multiline',String(Boolean(f(29))));}
  if(role==='slider'||role==='scrollbar'){el.setAttribute('aria-valuenow',String(f(8)));el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));}
  if(selected.name==='Accordion'||selected.name==='Collapsible'||((selected.name==='Select'||selected.name==='Menu'||selected.name==='DatePicker')&&f(0)===1))el.setAttribute('aria-expanded',String(Boolean(f(10))));else el.removeAttribute('aria-expanded');
  entries.push({el,parent:`cui-semantic-${wasm.gallery_semantic_parent(i)}`,controls:`cui-semantic-${wasm.gallery_semantic_controls(i)}`,labelledBy:`cui-semantic-${wasm.gallery_semantic_labelled_by(i)}`});
 }
 for(const [id,el] of semanticNodes)if(!seen.has(id)){el.remove();semanticNodes.delete(id);}
 for(const {el,parent,controls,labelledBy} of entries){
  const container=seen.has(parent)?semanticNodes.get(parent):root;
  if(el.parentElement!==container)container.append(el);
  if(seen.has(controls))el.setAttribute('aria-controls',controls);else el.removeAttribute('aria-controls');
  if(seen.has(labelledBy))el.setAttribute('aria-labelledby',labelledBy);else el.removeAttribute('aria-labelledby');
 }
 for(const {el} of entries)if(el.getAttribute('role')==='treeitem'){const toggle=Array.from(el.children).find(child=>child.getAttribute('role')==='button'&&['Expand','Collapse'].includes(child.textContent));if(toggle)el.setAttribute('aria-expanded',String(toggle.textContent==='Collapse'));else el.removeAttribute('aria-expanded');}
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
 let name='Button';try{name=decodeURIComponent(location.hash.slice(1))||'Button';}catch{/* Malformed percent escapes select the default component. */}selected=catalog.components.find(c=>c.name===name)||catalog.components.find(c=>c.name==='Button');
 document.title=`${selected.name} · CUI`; $('name').textContent=selected.name; $('status').textContent=selected.status.replaceAll('_',' ');
 const live=selected.name in stories;
 window.cuiGallery.renderedComponent=live?'':selected.name;
 if(modelStories.has(selected.name))$('status').textContent+=' / base model'; $('preview-card').hidden=!live; $('pending').hidden=live;
 $('description').textContent=live?`Explore ${selected.name.toLowerCase()} through the same CUI core, base behavior and theme used by native applications.`:`${selected.name} is part of the complete GPUI Kit 0.7.1 comparison catalog.`;
 if(modelStories.has(selected.name))$('description').textContent='Interactive demonstration of the implemented base model. The complete styled component remains pending.';
 $('pending-message').textContent=selected.status==='not_started'?'Implementation and browser story are pending. This entry stays visible to preserve the complete component scope.':'Repository implementation exists for part of this entry. A browser story is pending; see implementation scope and source for details.';
 $('snippet').textContent=snippets[selected.name]||(live?`/* See the public ${selected.public_header || 'component'} API. */`:'/* No complete styled browser example yet. */');
 $('source').href=`https://github.com/mattmezza/cui/${selected.public_header?'blob/main/'+selected.public_header:'blob/main/docs/gpui-parity.json'}`;
 $('reference').href=selected.reference_url;
 $('features').replaceChildren();for(const feature of selected.features){const row=document.createElement('tr'),name=document.createElement('td'),status=document.createElement('td');name.textContent=feature.name;status.textContent=feature.status.replaceAll('_',' ');status.className=feature.status;row.append(name,status);$('features').append(row);}
 $('limitations').replaceChildren();for(const message of [...selected.limitations,...(live?['Canvas text uses the browser system font; native FreeType / HarfBuzz rendering is separate.','Browser semantics are experimental. IME composition and a complete platform accessibility bridge remain pending.']:[])]){const li=document.createElement('li');li.textContent=message;$('limitations').append(li);}
 $('input-mode').hidden=selected.name!=='Input';$('input-mode').value='0';
 $('menu-kind').hidden=selected.name!=='Menu';$('menu-kind').value='0';
 $('selection-mode').hidden=!['List','Select','Combobox'].includes(selected.name);$('selection-mode').value='0';
 $('disabled').checked=false;document.body.classList.remove('menu-open');navigation();requestFrame();
}
const keys={Tab:1,Enter:2,' ':3,Escape:4,ArrowLeft:5,ArrowRight:6,ArrowUp:7,ArrowDown:8,Home:9,End:10,Backspace:11,Delete:12,a:13,A:13,z:14,Z:14,y:15,Y:15,c:16,C:16,x:17,X:17,v:18,V:18,PageUp:19,PageDown:20};
function mods(e){return(e.shiftKey?1:0)|(e.ctrlKey?2:0)|(e.altKey?4:0)|(e.metaKey?8:0);}
for(const [event,type] of [['pointermove',0],['pointerdown',1],['pointerup',2]])canvas.addEventListener(event,e=>{
 if(!wasm)return;const rect=canvas.getBoundingClientRect();if(type===1){canvas.focus();canvas.setPointerCapture(e.pointerId);}wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,e.button===2?1:e.button===1?2:0);requestFrame();
});
canvas.addEventListener('wheel',e=>{
 if(!wasm)return;const rect=canvas.getBoundingClientRect(),x=e.clientX-rect.left,y=e.clientY-rect.top;
 const scale=e.deltaMode===1?32:e.deltaMode===2?canvas.clientHeight:1,dx=e.deltaX*scale,dy=e.deltaY*scale;
 const consumes=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).some(i=>{const f=n=>wasm.gallery_semantic(i,n);if(!f(17)||f(5)||x<f(1)||y<f(2)||x>=f(1)+f(3)||y>=f(2)+f(4))return false;return(dx>0&&f(18)<f(20))||(dx<0&&f(18)>0)||(dy>0&&f(19)<f(21))||(dy<0&&f(19)>0);});
 if(consumes){e.preventDefault();wasm.gallery_scroll(x,y,-dx/32,-dy/32);requestFrame();}
},{passive:false});
canvas.addEventListener('pointerleave',()=>{wasm?.gallery_event(0,-100,-100,0,0,0);requestFrame();});
canvas.addEventListener('contextmenu',e=>e.preventDefault());
canvas.addEventListener('pointercancel',()=>{wasm?.gallery_event(7,0,0,0,0,0);requestFrame();});
canvas.addEventListener('blur',()=>{wasm?.gallery_event(7,0,0,0,0,0);requestFrame();});
for(const [event,type] of [['keydown',3],['keyup',4]])canvas.addEventListener(event,e=>{
 if(!wasm)return;const key=keys[e.key]||0;
 if(e.key==='Tab'){
  const enabled=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).filter(i=>!wasm.gallery_semantic(i,5)&&wasm.gallery_semantic(i,9)===0&&interactiveRoles.includes(wasm.gallery_semantic(i,0)));
  const focused=enabled.findIndex(i=>wasm.gallery_semantic(i,7));
  if(!wasm.gallery_modal()&&(!enabled.length||(e.shiftKey&&focused===0)||(!e.shiftKey&&focused===enabled.length-1&&focused>=0)))return;
 }
 if(key){e.preventDefault();wasm.gallery_event(type,0,0,key,mods(e),Number(e.repeat));}
 if(type===3 && e.key.length===1 && !e.ctrlKey&&!e.metaKey&&!e.altKey && (selected.name==='Input'||selected.name==='Editor'||selected.name==='NumberInput'||selected.name==='Combobox'||selected.name==='TimeField'||['Tree','List','Select','Menu','ContextMenu'].includes(selected.name))){
  e.preventDefault();const bytes=encoder.encode(e.key),pointer=wasm.malloc(bytes.length);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);
 }requestFrame();
});
reducedMotion.addEventListener('change',requestFrame);
$('search').addEventListener('input',navigation);window.addEventListener('hashchange',()=>{if(catalog)selectComponent();});
$('theme').addEventListener('change',()=>{document.body.classList.toggle('dark',$('theme').value==='1');document.body.classList.toggle('contrast',$('theme').value==='2');requestFrame();});
$('selection-mode').addEventListener('change',requestFrame);
$('menu-kind').addEventListener('change',requestFrame);
$('input-mode').addEventListener('change',requestFrame);
$('density').addEventListener('change',requestFrame);
$('disabled').addEventListener('change',requestFrame);$('menu').onclick=()=>document.body.classList.toggle('menu-open');new ResizeObserver(requestFrame).observe(canvas);
try{
 const [response,data]=await Promise.all([fetch('./gallery.wasm?v=__CUI_WASM_VERSION__'),fetch('./catalog.json',{cache:'no-cache'}).then(r=>r.json())]);if(!response.ok)throw Error('Cannot fetch WebAssembly module');catalog=data;
 const module=await WebAssembly.compile(await response.arrayBuffer());
 const imports={wasi_snapshot_preview1:wasiImports(),host:{measure:(ptr,len,size)=>{ctx.font=`${size}px system-ui`;return ctx.measureText(stringAt(ptr,len)).width;}}};
 const instance=await WebAssembly.instantiate(module,imports);wasm=instance.exports;wasm._initialize();if(!wasm.gallery_init())throw Error('CUI initialization failed');
 window.cuiGallery={exports:wasm,stories,catalog,render};selectComponent();
}catch(error){fatal(error);}
