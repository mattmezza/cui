/* SPDX-License-Identifier: MIT */
const $ = id => document.getElementById(id);
const canvas = $('preview'), ctx = canvas.getContext('2d');
const decoder = new TextDecoder(), encoder = new TextEncoder();
const bitmapCache=new Map();let bitmapCacheBytes=0;
function cacheBitmap(key,bitmap,bytes){
 if(bytes>16*1024*1024)return;
 while(bitmapCache.size>=64||bitmapCacheBytes+bytes>16*1024*1024){
  const oldest=bitmapCache.keys().next().value,entry=bitmapCache.get(oldest);
  bitmapCacheBytes-=entry.bytes;bitmapCache.delete(oldest);
 }
 bitmapCache.set(key,{bitmap,bytes});bitmapCacheBytes+=bytes;
}
function paintBitmap(i,x,y,w,h,radii){
 const field=n=>wasm.gallery_bitmap(i,n),pointer=wasm.gallery_bitmap_pixels(i);
 const bw=field(0),bh=field(1),stride=field(2),sx=field(3),sy=field(4),sw=field(5),sh=field(6);
 if(!pointer||!bw||!bh||stride<bw*4||!sw||!sh)return;
 const gray=field(8),tinted=field(9),tint=tinted?[field(10),field(11),field(12),field(13)]:[255,255,255,255];
 const key=[pointer,bw,bh,stride,wasm.gallery_bitmap_revision(i,1),wasm.gallery_bitmap_revision(i,0),gray,...tint].join(':');
 let bitmap=bitmapCache.get(key)?.bitmap;
 if(!bitmap){
  const source=new Uint8Array(wasm.memory.buffer,pointer,stride*bh),pixels=new Uint8ClampedArray(bw*bh*4);
  for(let row=0;row<bh;row++)for(let col=0;col<bw;col++){
   const at=row*stride+col*4,to=(row*bw+col)*4;
   const shade=gray?Math.round(.2126*source[at]+.7152*source[at+1]+.0722*source[at+2]):0;
   for(let channel=0;channel<3;channel++)pixels[to+channel]=Math.floor((gray?shade:source[at+channel])*tint[channel]/255);
   pixels[to+3]=Math.floor(source[at+3]*tint[3]/255);
  }
  bitmap=document.createElement('canvas');bitmap.width=bw;bitmap.height=bh;
  bitmap.getContext('2d').putImageData(new ImageData(pixels,bw,bh),0,0);
  cacheBitmap(key,bitmap,pixels.length);
 }
 ctx.save();ctx.beginPath();ctx.roundRect(x,y,w,h,radii);ctx.clip();ctx.globalAlpha=field(7);
 ctx.imageSmoothingEnabled=false;ctx.drawImage(bitmap,sx,sy,sw,sh,x,y,w,h);ctx.restore();
}
const stories = {Button:0,Checkbox:1,Switch:2,Toggle:3,Radio:4,Slider:5,Input:6,Label:7,Badge:8,Progress:9,Theme:11,Alert:12,Avatar:13,Bubble:14,Empty:15,GroupBox:16,Kbd:17,Skeleton:18,Spinner:19,Accordion:20,Collapsible:21,Tabs:22,Tooltip:23,HoverCard:24,Editor:25,VirtualList:26,Dock:27,Plot:28,Calendar:29,List:30,Select:31,Menu:32,NumberInput:33,Pagination:34,Rating:35,Stepper:36,Combobox:37,'Focus Trap':38,Clipboard:39,DatePicker:40,TimeField:41,ColorPicker:42,Tree:43,Scrollable:44,DataTable:45,Table:46,Dialog:48,AlertDialog:49,Sheet:50,Popover:51,DropdownButton:52,Form:53,'Input Group':54,OtpInput:55,Tag:56,Marker:57,Notification:58,Shimmer:59,Message:60,MessageScroller:61,Image:62,Attachment:63};
const modelStories = new Set(['Editor','VirtualList','Dock','Plot','Focus Trap']);
const snippets = {
 Button:'cui_button_ex(ui, id, &theme, label,\n    (cui_button_options){\n        .variant = CUI_BUTTON_OUTLINE,\n        .size = CUI_CONTROL_SMALL\n    });',
 Checkbox:'cui_checkbox(ui, id, &theme,\n    cui_string("Receive updates"),\n    &checked, false);',
 Switch:'cui_switch(ui, id, &theme,\n    cui_string("Notifications"),\n    &enabled, false);',
 Toggle:'cui_toggle(ui, id, &theme,\n    cui_string("Pin this item"),\n    &pinned, false);',
 Radio:'cui_radio(ui, id, &theme,\n    cui_string("Priority"),\n    &selection, 1, false);',
 Slider:'cui_slider(ui, id, &theme,\n    cui_string("Volume"),\n    &value, 0, 100, 1, false);',
 Input:'cui_input_ex(ui, id, &theme, label, edit,\n    &state, (cui_input_options){.width = 280});',
 Tag:'cui_tag(ui, id, &theme, cui_string("Workspace"), options);\n/* Rich children use cui_begin_tag / cui_end_tag. */',
 Notification:'/* Queue pushes after the frame; descriptors stay application-owned. */\ncui_notification_push(&state, descriptor, host_seconds);\ncui_notification_advance result = cui_notifications(\n    ui, id, &theme, &state, viewport, host_seconds);',
 Marker:'cui_marker(ui, id, &theme, &state, children, count, options);\n/* Schedule the deadline from cui_base_marker. */',
 OtpInput:'cui_otp_result result = cui_otp(ui, id, &theme, &state, options);\n/* Schedule a host frame at result.next_wake_seconds. */',
 'Input Group':'cui_input_group(ui, group_id, input_id, &theme, editor, &state,\n    addons, addon_count, options);',
 Form:'/* Application owns editors, validation and callbacks. */\ncui_form(ui, id, &theme, fields, count,\n    (cui_form_options){.columns = 2,\n        .name = cui_string("Profile"),\n        .render_footer = draw_save_button});',
 Scrollable:'if (cui_scrollable_begin(ui, id, &theme,\n    &state, options)) {\n    draw_content(ui);\n    cui_scrollable_end(ui, id, &theme, &state, options);\n}',
 DataTable:'cui_data_table(ui, id, &theme, model,\n    &state, options, cell_label, draw_cell, user);',
 Table:'cui_table_begin(ui, id, &theme, width);\n/* Compose rows and cells; end each begin. */\ncui_table_end(ui);',
 Dialog:'if (cui_surface_begin(ui, &theme, &dialog, title,\n    description, options)) {\n    /* Compose arbitrary children. */\n    cui_surface_end(ui);\n}',
 AlertDialog:'cui_alert_dialog(ui, &theme, &alert, title,\n    description, confirm, cancel, options);',
 Sheet:'options.edge = CUI_SHEET_RIGHT;\nif (cui_surface_begin(ui, &theme, &sheet, title,\n    description, options)) {\n    draw_settings(ui);\n    cui_surface_end(ui);\n}',
 Popover:'options.anchor = cui_bounds(ui, trigger);\nif (cui_surface_begin(ui, &theme, &popover, title,\n    description, options)) {\n    draw_profile(ui);\n    cui_surface_end(ui);\n}',
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
 DropdownButton:'cui_dropdown_button_result result = cui_dropdown_button(\n    ui, container, action_id, menu_id, &theme,\n    label, menu_name, entries, count, &menu_action,\n    &menu_state, options);',
 ContextMenu:'/* Call after painting the receiving area. */\ncui_context_menu(ui, target, &theme, label,\n    entries, count, &action, &state, options);',
 Accordion:'cui_accordion(ui, id, &theme, items, 3,\n    expanded, options, draw_panel, user);',
 Collapsible:'cui_collapsible(ui, id, &theme, label,\n    &open, false, draw_panel, user);',
 Tabs:'cui_tabs(ui, id, &theme, items, 3,\n    &selection, options, draw_panel, user);',
 Avatar:'cui_avatar(ui, id, &theme,\n    cui_string("Alex Morgan"), 56);',
 Kbd:'cui_kbd(ui, id, &theme,\n    cui_string("Ctrl + K"));',
 Theme:'cui_theme theme = cui_theme_dark();\ncui_button(ui, id, &theme,\n    cui_string("Primary action"), false);',
 Image:'cui_image(ui, id, &state, source, loader, user, options);\n/* The host loader supplies borrowed RGBA8 frames. */',
 Attachment:'cui_attachment(ui, id, &theme, &state, options);\n/* Compose scrolling cards with cui_attachment_group. */'
};
let wasm, catalog, selected, framePending=false, wakeTimer, lastTime=performance.now();
const reducedMotion=matchMedia('(prefers-reduced-motion: reduce)');
const systemDark=matchMedia('(prefers-color-scheme: dark)');
function effectiveTheme(){return $('theme').value==='3'?(systemDark.matches?1:0):Number($('theme').value);}
function syncTheme(){const mode=$('theme').value;document.body.classList.toggle('system',mode==='3');document.body.classList.toggle('dark',mode==='1');document.body.classList.toggle('contrast',mode==='2');requestFrame();}
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
 clearTimeout(wakeTimer);
 if(!selected || !(selected.name in stories))return;
 const width=canvas.clientWidth,height=selected.name==='Message'?500:selected.name==='MessageScroller'?550:selected.name==='Image'?320:selected.name==='Attachment'?420:selected.name==='Theme'?360:selected.name==='Accordion'?650:selected.name==='Collapsible'?380:selected.name==='Notification'?720:selected.name==='Form'?360:selected.name==='Calendar'?420:selected.name==='DatePicker'?380:selected.name==='ColorPicker'?620:selected.name==='DataTable'?340:['Menu','ContextMenu'].includes(selected.name)?420:260,dpr=Math.min(devicePixelRatio||1,3);
 canvas.style.height=`${height}px`;
 canvas.width=Math.round(width*dpr);canvas.height=height*dpr;ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
 wasm.gallery_table_pin(Number($('table-pin').value));
 wasm.gallery_table_mode(Number($('table-mode').value));
 wasm.gallery_table_selection_mode(Number($('table-selection').value));
 wasm.gallery_surface_variant(Number($('surface-variant').value));
 wasm.gallery_input_mode(Number($('input-mode').value));
 wasm.gallery_disclosure_options(Number($('disclosure-mode').value),Number($('disclosure-multiple').checked));
 wasm.gallery_image_options(Number($('image-state').value),Number($('image-fit').value),Number($('image-grayscale').checked),Number($('image-opacity').checked));
 wasm.gallery_attachment_options(Number($('attachment-status').value),Number($('attachment-vertical').checked));
 wasm.gallery_message_options(Number($('message-alignment').value),Number($('message-variant').value),Number($('message-inset').value),Number($('message-avatar').checked),Number($('message-scrollbar').checked));
 wasm.gallery_dropdown_options(...['variant','size','halves','action','anchor'].map(k=>Number($('dropdown-'+k).value)),Number($('dropdown-outline').checked),Number($('dropdown-selected').checked));
 wasm.gallery_tabs_options(Number($('tabs-mode').value),Number($('tabs-size').value),Number($('tabs-variant').value),Number($('tabs-manual').checked));
 wasm.gallery_shimmer_mode(Number($('shimmer-mode').value));
 wasm.gallery_notification_options(Number($('notification-anchor').value),Number($('notification-kind').value),Number($('notification-autohide').checked));
 wasm.gallery_tag_options(Number($('tag-variant').value),Number($('tag-color').value),Number($('tag-outline').checked),Number($('tag-pill').checked));
 wasm.gallery_marker_options(Number($('marker-variant').value),Number($('marker-loading').value),Number($('marker-alignment').value),Number($('marker-icon').checked));
 wasm.gallery_otp_options(Number($('otp-groups').value),Number($('otp-mask').checked));
 wasm.gallery_group_mode(Number($('group-mode').value));
 wasm.gallery_form_mode(Number($('form-mode').value));
 wasm.gallery_density(Number($('density').value));
 wasm.gallery_selection_mode(Number($('selection-mode').value));
 const now=performance.now();wasm.gallery_time(now/1000,Math.min((now-lastTime)/1000,.1),Number(reducedMotion.matches));lastTime=now;
 if(!wasm.gallery_frame(selected.name==='Menu'&&$('menu-kind').value==='1'?47:stories[selected.name],effectiveTheme(),Number($('disabled').checked),width,height))throw Error(`CUI layout failed (error ${wasm.gallery_error()})`);
 let clips=0;
 for(let i=0;i<wasm.gallery_count();i++){
  const f=n=>wasm.gallery_command(i,n);const type=f(0),x=f(1),y=f(2),w=f(3),h=f(4);
  if(type===3){ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();clips++;continue;}
  if(type===4){if(clips){ctx.restore();clips--;}continue;}
  ctx.fillStyle=ctx.strokeStyle=`rgba(${f(5)},${f(6)},${f(7)},${f(8)/255})`;
  if(type===2){ctx.font=`${f(11)}px system-ui`;ctx.textBaseline='top';ctx.fillText(stringAt(wasm.gallery_command_text(i),wasm.gallery_command_length(i)),x,y);}
  else if(w>0&&h>0){
   const radii=(f(12)?[f(13),f(14),f(16),f(15)]:[f(9),f(9),f(9),f(9)]).map(r=>Math.min(Math.max(0,r),Math.min(w,h)/2));
   if(type===5){paintBitmap(i,x,y,w,h,radii);continue;}
   ctx.beginPath();ctx.roundRect(x,y,w,h,radii);
   if(type===1){
    const [left,right,top,bottom]=(f(17)?[f(18),f(19),f(20),f(21)]:[f(10),f(10),f(10),f(10)]).map(e=>Math.max(0,e));
    const innerWidth=w-left-right,innerHeight=h-top-bottom;
    if(innerWidth>0&&innerHeight>0){
     const inner=[{x:radii[0]-left,y:radii[0]-top},{x:radii[1]-right,y:radii[1]-top},{x:radii[2]-right,y:radii[2]-bottom},{x:radii[3]-left,y:radii[3]-bottom}].map(r=>({x:Math.max(0,r.x),y:Math.max(0,r.y)}));
     ctx.roundRect(x+left,y+top,innerWidth,innerHeight,inner);
    }
    ctx.fill('evenodd');
   }else ctx.fill();
  }
 }
 while(clips-- >0)ctx.restore();
 syncSemantics();
 if(selected.name==='Checkbox')$('checkbox-state').value=String(wasm.gallery_checkbox_state(-1));
 if(selected.name==='Attachment')$('attachment-status').value=String(wasm.gallery_attachment_stats(9));
 $('table-selection').value=String(wasm.gallery_table_selection_mode(-1));
 window.cuiGallery.renderedComponent=selected.name;window.cuiGallery.frame=(window.cuiGallery.frame||0)+1;
 const clipboard=wasm.gallery_clipboard_request();if(clipboard)void clipboardAction(clipboard);
 if(wasm.gallery_redraw())requestFrame();
 const wake=wasm.gallery_next_wake();if(wake>0)wakeTimer=setTimeout(requestFrame,Math.max(1,wake*1000-performance.now()));
 $('preview-note').textContent=`${modelStories.has(selected.name)?'Partial base model · ':''}${wasm.gallery_count()} draw commands · ${wasm.gallery_semantic_count()} semantic nodes · Mouse, Tab, Enter / Space and arrow keys`;
}
async function clipboardAction(action){
 let okay=1,text;
 try {if(!navigator.clipboard)throw Error('unavailable');if(action===1){await navigator.clipboard.writeText('Hello from CUI');text='Copied: Hello from CUI';}else text=`Read: ${await navigator.clipboard.readText()}`;}
 catch {okay=0;text='';}
 const bytes=encoder.encode(text),pointer=wasm.malloc(bytes.length+1);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_clipboard_result(okay,pointer,bytes.length);wasm.free(pointer);requestFrame();
}
const semanticNodes = new Map();
const roles=['','button','checkbox','switch','radio','slider','textbox','text','tab','group','tablist','tabpanel','list','listbox','option','menu','menuitem','dialog','alert','status','img','progressbar','tree','treeitem','menuitemcheckbox','menuitemradio','table','row','cell','columnheader','gridcell','grid','scrollbar','rowheader','rowgroup','alertdialog','link','listitem','log'];
const interactiveRoles=[1,2,3,4,5,6,8,14,16,23,24,25,29,30,32,33,36];
function syncSemantics(){
 const root=$('accessible'),seen=new Set(),entries=[];
 for(let i=0;i<wasm.gallery_semantic_count();i++){
  const f=n=>wasm.gallery_semantic(i,n),role=roles[f(0)];if(!role)continue;
  const id=`cui-semantic-${wasm.gallery_semantic_id(i)}`;seen.add(id);
  let el=semanticNodes.get(id);
  if(!el){el=document.createElement(interactiveRoles.includes(f(0))&&!['treeitem','columnheader'].includes(role)?'button':'div');el.id=id;if(el.tagName==='BUTTON'){el.type='button';el.tabIndex=-1;}semanticNodes.set(id,el);}
  if(role==='text')el.removeAttribute('role');else el.setAttribute('role',role);
  const name=stringAt(wasm.gallery_semantic_name(i),wasm.gallery_semantic_length(i));
  if(role==='text')el.textContent=name;
  el.setAttribute('aria-label',name);
  if(f(33)&&el.tagName!=='BUTTON'){el.tabIndex=-1;el.setAttribute('aria-disabled',String(Boolean(f(5))));el.onfocus=()=>{if(!f(5)){wasm.gallery_focus(i);requestFrame();}};el.onkeydown=e=>{const key=keys[e.key]||0;if(key&&!f(5)){e.preventDefault();wasm.gallery_event(3,0,0,key,mods(e),Number(e.repeat));requestFrame();}};}else if(el.tagName!=='BUTTON'){el.removeAttribute('tabindex');el.onfocus=null;el.onkeydown=null;}
  const description=stringAt(wasm.gallery_semantic_description(i),wasm.gallery_semantic_description_length(i));
  if(description)el.setAttribute('aria-description',description);else el.removeAttribute('aria-description');
  if(role==='columnheader'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};}
  if(el.tagName==='BUTTON'){if(el.textContent!==name)el.textContent=name;el.disabled=Boolean(f(5));el.onclick=e=>{e.stopPropagation();wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};}
  else el.setAttribute('aria-label',name);
  if(['checkbox','switch','radio','menuitemcheckbox','menuitemradio'].includes(role))el.setAttribute('aria-checked',role==='checkbox'&&f(8)===2?'mixed':String(Boolean(f(6))));
  el.setAttribute('aria-busy',String(Boolean(f(14))));
  if(['listbox','tree','grid'].includes(role))el.setAttribute('aria-multiselectable',String(Boolean(f(15))));
  if(role==='treeitem'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0);requestFrame();};el.setAttribute('aria-level',String(f(30)||f(8)+1));if(f(31))el.setAttribute('aria-posinset',String(f(31)));if(f(32))el.setAttribute('aria-setsize',String(f(32)));el.setAttribute('aria-disabled',String(Boolean(f(5))));}
  if(role==='tab'||role==='option'||role==='treeitem')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(f(16))el.setAttribute('aria-orientation',f(16)===2?'vertical':'horizontal');else el.removeAttribute('aria-orientation');
  for(const [field,attribute] of [[22,'aria-rowindex'],[23,'aria-colindex'],[24,'aria-rowcount'],[25,'aria-colcount']]){if(f(field)||(field>=24&&['grid','table'].includes(role)))el.setAttribute(attribute,String(f(field)));else el.removeAttribute(attribute);}
  if(role==='gridcell'||role==='columnheader'||role==='row'||role==='rowheader')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(role==='columnheader')el.setAttribute('aria-sort',f(8)===1?'ascending':f(8)===2?'descending':'none');
  if(role==='dialog'||role==='alertdialog')el.setAttribute('aria-modal',String(selected.name!=='Popover'));else el.removeAttribute('aria-modal');
  if(role==='textbox'){el.setAttribute('aria-readonly',String(Boolean(f(26))));el.setAttribute('aria-required',String(Boolean(f(27))));el.setAttribute('aria-invalid',String(Boolean(f(28))));el.setAttribute('aria-multiline',String(Boolean(f(29))));}
  if(role==='slider'||role==='scrollbar'){el.setAttribute('aria-valuenow',String(f(8)));el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));}
  if(selected.name==='Accordion'||selected.name==='Collapsible'||((selected.name==='Select'||selected.name==='Menu'||selected.name==='DatePicker')&&f(0)===1))el.setAttribute('aria-expanded',String(Boolean(f(10))));else el.removeAttribute('aria-expanded');
  entries.push({el,parent:`cui-semantic-${wasm.gallery_semantic_parent(i)}`,controls:`cui-semantic-${wasm.gallery_semantic_controls(i)}`,labelledBy:`cui-semantic-${wasm.gallery_semantic_labelled_by(i)}`});
 }
 for(const [id,el] of semanticNodes)if(!seen.has(id)){el.remove();semanticNodes.delete(id);}
 const positions=new Map();
 for(const {el,parent,controls,labelledBy} of entries){
  const container=seen.has(parent)?semanticNodes.get(parent):root;
  const position=positions.get(container)||0,reference=container.children[position];
  if(reference!==el)container.insertBefore(el,reference||null);positions.set(container,position+1);
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
 $('table-pin').hidden=selected.name!=='DataTable';$('table-pin').value='0';
 $('table-mode').hidden=selected.name!=='DataTable';$('table-mode').value='0';
 $('table-selection').hidden=selected.name!=='DataTable';$('table-selection').value='2';
 $('surface-variant').hidden=selected.name!=='Sheet';$('surface-variant').value='0';
 $('disclosure-mode').hidden=!['Accordion','Collapsible'].includes(selected.name);$('disclosure-mode').value='0';
 $('disclosure-multiple-control').hidden=selected.name!=='Accordion';$('disclosure-multiple').checked=false;
 for(const id of ['message-alignment','message-variant','message-inset','message-avatar-control'])$(id).hidden=selected.name!=='Message';
 $('checkbox-state').hidden=selected.name!=='Checkbox';
 for(const id of ['image-state','image-fit','image-grayscale-control','image-opacity-control'])$(id).hidden=selected.name!=='Image';
 for(const id of ['attachment-status','attachment-vertical-control'])$(id).hidden=selected.name!=='Attachment';
 $('message-scrollbar-control').hidden=selected.name!=='MessageScroller';
 $('message-alignment').value='0';$('message-variant').value='1';$('message-inset').value='0';$('message-avatar').checked=true;$('message-scrollbar').checked=true;
 for(const id of ['dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline-control','dropdown-selected-control'])$(id).hidden=selected.name!=='DropdownButton';
 for(const k of ['variant','size','halves','action','anchor'])$('dropdown-'+k).value='0';$('dropdown-outline').checked=false;$('dropdown-selected').checked=false;
 for(const id of ['tabs-mode','tabs-size','tabs-variant','tabs-manual-control'])$(id).hidden=selected.name!=='Tabs';
 $('tabs-mode').value='0';$('tabs-size').value='0';$('tabs-variant').value='1';$('tabs-manual').checked=false;
 $('shimmer-mode').hidden=selected.name!=='Shimmer';$('shimmer-mode').value='0';
 for(const id of ['notification-anchor','notification-kind','notification-autohide-control'])$(id).hidden=selected.name!=='Notification';
 $('notification-anchor').value='2';$('notification-kind').value='1';$('notification-autohide').checked=true;
 for(const id of ['tag-variant','tag-color','tag-outline-control','tag-pill-control'])$(id).hidden=selected.name!=='Tag';
 $('tag-variant').value='0';$('tag-color').value='14';$('tag-outline').checked=false;$('tag-pill').checked=false;
 for(const id of ['marker-variant','marker-loading','marker-alignment','marker-icon-control'])$(id).hidden=selected.name!=='Marker';
 $('marker-variant').value='0';$('marker-loading').value='0';$('marker-alignment').value='0';$('marker-icon').checked=false;
 $('otp-groups').hidden=selected.name!=='OtpInput';$('otp-groups').value='2';
 $('otp-mask-control').hidden=selected.name!=='OtpInput';$('otp-mask').checked=false;
 $('group-mode').hidden=selected.name!=='Input Group';$('group-mode').value='0';
 $('form-mode').hidden=selected.name!=='Form';$('form-mode').value='0';
 $('input-mode').hidden=selected.name!=='Input';$('input-mode').value='0';
 $('menu-kind').hidden=selected.name!=='Menu';$('menu-kind').value='0';
 $('selection-mode').hidden=!['List','Select','Combobox','DataTable'].includes(selected.name);$('selection-mode').value='0';
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
canvas.addEventListener('blur',()=>{wasm?.gallery_event(7,0,0,0,0,0);wasm?.gallery_event(0,-100,-100,0,0,0);wasm?.gallery_focus(0xffffffff);requestFrame();});
for(const [event,type] of [['keydown',3],['keyup',4]])canvas.addEventListener(event,e=>{
 if(!wasm)return;const key=keys[e.key]||0;
 if(e.key==='Tab'){
  const enabled=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).filter(i=>!wasm.gallery_semantic(i,5)&&wasm.gallery_semantic(i,9)===0&&(interactiveRoles.includes(wasm.gallery_semantic(i,0))||wasm.gallery_semantic(i,33)));
  const focused=enabled.findIndex(i=>wasm.gallery_semantic(i,7));
  if(!wasm.gallery_modal()&&(!enabled.length||(e.shiftKey&&focused===0)||(!e.shiftKey&&focused===enabled.length-1&&focused>=0)))return;
 }
 if(key){e.preventDefault();wasm.gallery_event(type,0,0,key,mods(e),Number(e.repeat));}
 if(type===3 && e.key.length===1 && !e.ctrlKey&&!e.metaKey&&!e.altKey && (selected.name==='Input'||selected.name==='Form'||selected.name==='Input Group'||selected.name==='OtpInput'||selected.name==='Editor'||selected.name==='NumberInput'||selected.name==='Combobox'||selected.name==='TimeField'||['Tree','List','Select','Menu','ContextMenu','DropdownButton'].includes(selected.name))){
  e.preventDefault();const bytes=encoder.encode(e.key),pointer=wasm.malloc(bytes.length);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);
 }requestFrame();
});
reducedMotion.addEventListener('change',requestFrame);
systemDark.addEventListener('change',()=>{if($('theme').value==='3')requestFrame();});
$('search').addEventListener('input',navigation);window.addEventListener('hashchange',()=>{if(catalog)selectComponent();});
$('theme').addEventListener('change',syncTheme);
for(const id of ['image-state','image-fit','image-grayscale','image-opacity','attachment-status','attachment-vertical'])$(id).addEventListener('change',requestFrame);
$('checkbox-state').addEventListener('change',()=>{wasm.gallery_checkbox_state(Number($('checkbox-state').value));requestFrame();});
$('selection-mode').addEventListener('change',requestFrame);
$('menu-kind').addEventListener('change',requestFrame);
$('input-mode').addEventListener('change',requestFrame);
for(const id of ['message-alignment','message-variant','message-inset','message-avatar','message-scrollbar','dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline','dropdown-selected','tabs-mode','tabs-size','tabs-variant','tabs-manual','shimmer-mode','disclosure-mode','disclosure-multiple','notification-anchor','notification-kind','notification-autohide','tag-variant','tag-color','tag-outline','tag-pill','marker-variant','marker-loading','marker-alignment','marker-icon'])$(id).addEventListener('change',requestFrame);
$('otp-groups').addEventListener('change',requestFrame);
$('otp-mask').addEventListener('change',requestFrame);
$('group-mode').addEventListener('change',requestFrame);
$('form-mode').addEventListener('change',requestFrame);
$('table-pin').addEventListener('change',requestFrame);
 $('table-mode').addEventListener('change',requestFrame);
 $('table-selection').addEventListener('change',requestFrame);
$('surface-variant').addEventListener('change',requestFrame);
$('density').addEventListener('change',requestFrame);
$('disabled').addEventListener('change',requestFrame);$('menu').onclick=()=>document.body.classList.toggle('menu-open');new ResizeObserver(requestFrame).observe(canvas);
try{
 const [response,data]=await Promise.all([fetch('./gallery.wasm?v=__CUI_WASM_VERSION__'),fetch('./catalog.json',{cache:'no-cache'}).then(r=>r.json())]);if(!response.ok)throw Error('Cannot fetch WebAssembly module');catalog=data;
 const module=await WebAssembly.compile(await response.arrayBuffer());
 const imports={wasi_snapshot_preview1:wasiImports(),host:{measure:(ptr,len,size)=>{ctx.font=`${size}px system-ui`;return ctx.measureText(stringAt(ptr,len)).width;}}};
 const instance=await WebAssembly.instantiate(module,imports);wasm=instance.exports;wasm._initialize();if(!wasm.gallery_init())throw Error('CUI initialization failed');
 window.cuiGallery={exports:wasm,stories,catalog,render};selectComponent();
}catch(error){fatal(error);}
