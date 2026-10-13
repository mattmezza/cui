/* SPDX-License-Identifier: MIT */
const $ = id => document.getElementById(id);
// Freeze the asset root before shareable paths change the current document URL.
const siteAssetRoot=new URL('.',document.baseURI);
const siteBaseElement=document.querySelector('base')||document.head.appendChild(document.createElement('base'));
siteBaseElement.href=siteAssetRoot.href;
function componentSlug(name){return name.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');}
function shareRoute(path,raw,title,description){
 document.body.dataset.route=raw;
 const address=new URL(path,siteAssetRoot);address.hash=raw;
 if(location.href!==address.href)history.replaceState(null,'',address.href);
 const canonical=new URL(path,document.body.dataset.siteUrl||'https://matteo.merola.co/cui/').href;
 document.querySelector('link[rel=canonical]')?.setAttribute('href',canonical);
 for(const [selector,value] of [['meta[name=description]',description],['meta[property="og:title"]',title],['meta[property="og:description"]',description],['meta[property="og:url"]',canonical],['meta[name="twitter:title"]',title],['meta[name="twitter:description"]',description]])document.querySelector(selector)?.setAttribute('content',value);
}
const canvas = $('preview'), ctx = canvas.getContext('2d');
const decoder = new TextDecoder(), encoder = new TextEncoder();
const textBaselineOffsets=new Map();
function canvasFont(size,weight=0,italic=false,monospace=false,familyId=0){
 const families={1:"Arial, Helvetica, sans-serif",2:"Georgia, Times New Roman, serif",3:"ui-monospace, SFMono-Regular, Menlo, Consolas, monospace"};
 const family=families[familyId]||(monospace?'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace':'system-ui');
 return `${italic?'italic ':''}${[400,500,600,700][weight]||400} ${size}px ${family}`;
}
function canvasFontMetrics(size,weight=0,italic=false,monospace=false,familyId=0){
 if(!Number.isFinite(size)||size<=0)return null;
 ctx.font=canvasFont(size,weight,italic,monospace,familyId);
 const metrics=ctx.measureText('');
 const ascent=metrics.fontBoundingBoxAscent,descent=metrics.fontBoundingBoxDescent,lineHeight=size*1.25;
 if(!Number.isFinite(ascent)||!Number.isFinite(descent)||!Number.isFinite(lineHeight)||ascent<0||descent<0||ascent+descent<=0||lineHeight<=0)return null;
 return [ascent,descent,lineHeight];
}
function textBaselineOffset(size,weight=0,italic=false,monospace=false,familyId=0){
 const key=`${size}:${weight}:${Number(italic)}:${Number(monospace)}:${familyId}`;
 if(textBaselineOffsets.has(key))return textBaselineOffsets.get(key);
 const metrics=canvasFontMetrics(size,weight,italic,monospace,familyId);
 const offset=metrics?(metrics[2]-metrics[0]-metrics[1])/2+metrics[0]:null;
 textBaselineOffsets.set(key,offset);return offset;
}
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
 const bw=field(0),bh=field(1),stride=field(2);let sx=field(3),sy=field(4),sw=field(5),sh=field(6);
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
 // Isolate a crop before scaling so filtering cannot bleed excluded texels.
 if(sx!==0||sy!==0||sw!==bw||sh!==bh){
  const cropKey=`crop:${key}:${sx}:${sy}:${sw}:${sh}`,cw=Math.max(1,Math.ceil(sw)),ch=Math.max(1,Math.ceil(sh));
  let cropped=bitmapCache.get(cropKey)?.bitmap;
  if(!cropped){cropped=document.createElement('canvas');cropped.width=cw;cropped.height=ch;const cropCtx=cropped.getContext('2d');cropCtx.imageSmoothingEnabled=false;cropCtx.drawImage(bitmap,sx,sy,sw,sh,0,0,cw,ch);cacheBitmap(cropKey,cropped,cw*ch*4);}
  bitmap=cropped;sx=0;sy=0;sw=cw;sh=ch;
 }
 ctx.save();ctx.beginPath();ctx.roundRect(x,y,w,h,radii);ctx.clip();ctx.globalAlpha=field(7);
 ctx.imageSmoothingEnabled=true;ctx.imageSmoothingQuality='high';ctx.drawImage(bitmap,sx,sy,sw,sh,x,y,w,h);ctx.restore();
}
const stories = {Button:0,Checkbox:1,Switch:2,Toggle:3,Radio:4,Slider:5,Input:6,Label:7,Badge:8,Progress:9,Theme:11,Alert:12,Avatar:13,Bubble:14,Empty:15,GroupBox:16,Kbd:17,Skeleton:18,Spinner:19,Accordion:20,Collapsible:21,Tabs:22,Tooltip:23,HoverCard:24,Editor:25,VirtualList:26,Dock:27,Plot:28,Calendar:29,List:30,Select:31,Menu:32,NumberInput:33,Pagination:34,Rating:35,Stepper:36,Combobox:37,'Focus Trap':38,Clipboard:39,DatePicker:40,TimeField:41,ColorPicker:42,Tree:43,Scrollable:44,DataTable:45,Table:46,Dialog:48,AlertDialog:49,Sheet:50,Popover:51,DropdownButton:52,Form:53,'Input Group':54,OtpInput:55,Tag:56,Marker:57,Notification:58,Shimmer:59,Message:60,MessageScroller:61,Image:62,Attachment:63,DescriptionList:64,StatusBar:65,Resizable:66,Icon:67,Toolbar:68,TitleBar:69,Sidebar:70,Textarea:71,Settings:72,Carousel:73,Command:74,'Root View':75,TextView:76,Chart:77,Speech:78,Questionnaire:79};
const modelStories = new Set(['Focus Trap']);
const snippets = {
 Button:'cui_button_ex(ui, id, &theme, label,\n    (cui_button_options){\n        .variant = CUI_BUTTON_OUTLINE,\n        .size = CUI_CONTROL_SMALL\n    });',
 Checkbox:'cui_checkbox_ex(ui, id, &theme, label, &state,\n    (cui_checkbox_options){.size = CUI_CONTROL_SMALL,\n                           .required = true});',
 Switch:'cui_switch_ex(ui, id, &theme, label, &enabled,\n    &spring, (cui_switch_options){.now_seconds = now});',
 Toggle:'cui_toggle_group_ex(ui, group_id, &theme,\n    items, selected, 3, options);',
 Radio:'cui_radio_ex(ui, id, &theme, label, &selection, 1,\n    (cui_radio_options){.size = CUI_CONTROL_LARGE});',
 Slider:'cui_slider(ui, id, &theme,\n    cui_string("Volume"),\n    &value, 0, 100, 1, false);',
 Input:'cui_input_ex(ui, id, &theme, label, edit,\n    &state, (cui_input_options){.size = CUI_INPUT_SMALL,\n                                .width = 280});',
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
 Badge:'// Define draw_inbox to compose your button, avatar or other content.\nvoid draw_inbox(cui_context *, void *);\ncui_badge_options badge = cui_badge_defaults();\nbadge.count = 125; // Displays 99+; zero hides the marker.\ncui_badge_ex(ui, id, &theme, badge, draw_inbox, user);',
 Progress:'cui_progress_options options = cui_progress_defaults();\noptions.motion = &motion; // retain zero-initialized state\noptions.now_seconds = now_seconds;\ncui_progress_ex(ui, id, &theme, progress / 100.0f, 300, options);',
 Alert:'cui_alert(ui, id, &theme, (cui_alert_options){\n    .kind = CUI_ALERT_INFO,\n    .title = cui_string("Heads up"),\n    .message = cui_string("Your changes were saved"),\n    .width = 360});',
 Bubble:'cui_begin_bubble(ui, id, &theme, CUI_BUBBLE_FILLED, 240);\ncui_label(ui, &theme, cui_string("Message"));\ncui_end_bubble(ui);',
 Empty:'cui_empty(ui, id, &theme, cui_string("No results"),\n    cui_string("Try a different search"), 320);',
 GroupBox:'cui_begin_group_box(ui, id, &theme, cui_string("Preferences"), 320);\ncui_label(ui, &theme, cui_string("Your settings"));\ncui_end_group_box(ui);',
 Skeleton:'cui_skeleton(ui, id, &theme, 240, 20, 6,\n    cui_loading_phase(now_seconds, 1.2f), false, reduced_motion);',
 Spinner:'cui_spinner(ui, id, &theme, 24,\n    cui_loading_phase(now_seconds, .8f), reduced_motion);',
 Tooltip:'/* Paint trigger before calling the tooltip. */\ncui_tooltip(ui, id, &theme, &hover, cui_string("Helpful detail"),\n    (cui_hover_options){.trigger = trigger_id, .viewport = viewport});',
 HoverCard:'/* Paint trigger before opening the card. */\nif (cui_begin_hover_card(ui, id, &theme, &hover, cui_string("Profile"),\n    (cui_hover_options){.trigger = trigger_id, .viewport = viewport})) {\n    cui_label(ui, &theme, cui_string("Alex Morgan"));\n    cui_end_hover_card(ui);\n}',
 'Focus Trap':'if (cui_begin_focus_scope(ui, scope_id, true)) {\n    /* Paint focusable children here. */\n    cui_end_focus_scope(ui);\n}',
 Clipboard:'/* Install clipboard_set/get on cui_platform first. */\nif (cui_button(ui, id, &theme, cui_string("Copy"), false))\n    platform.clipboard_set(platform.user, cui_string("Copied text"));',
 Shimmer:'cui_shimmer_text(ui, id, &theme, &shimmer,\n    cui_string("Loading content"), cui_shimmer_options_default());',
 Message:'cui_message(ui, id, &theme, (cui_message_options){\n    .width = 360, .body = bodies, .body_count = body_count});',
 MessageScroller:'cui_message_scroller(ui, id, &theme, scroll_state,\n    draw_message_row, user, cui_message_scroller_options_default());',

 Editor:'/* Retain edit, state, query and workspace; call find bar every frame. */\ncui_editor_find_bar(ui, search_id, id, &theme, &state, &search, 640, false);\ncui_editor(ui, id, &theme, cui_string("C code"), &state, cui_editor_default());',
 VirtualList:'/* Retain list and state; paint_row composes each visible item. */\ncui_virtual_list_draw(ui, id, &theme, list, &state,\n    (cui_virtual_widget_options){.scroll = {\n        .width = 640, .height = 320}, .overscan = 1},\n    paint_row, user);',
 Dock:'/* Retain dock and view; panels hold your content callbacks. */\ncui_dock_view_render(ui, id, &theme, dock, view, panels, count,\n    (cui_dock_view_options){.width = 640, .height = 400,\n        .tab_height = 32, .allow_reorder = true});',
 Plot:'/* Build pixels with cui_plot_raster; retain pixels and axis labels. */\ncui_plot_view(ui, id, &theme,\n    &(cui_plot_view_options){.bitmap = bitmap, .axis = &axis,\n        .name = cui_string("Trend")});',
 Calendar:'cui_calendar(ui, id, &theme, &state,\n    &selection, options);',
 DatePicker:'cui_date_picker(ui, id, &theme, label,\n    &state, &selection, options);',
 ColorPicker:'cui_color_picker(ui, id, &theme,\n    &state, &color, options);',
 TimeField:'cui_time_field(ui, id, &theme, label,\n    &state, &time, options);',
 Combobox:'cui_combobox(ui, id, &theme, label, query,\n    items, 4, scratch, 4, &selection,\n    &popup, options);',
 NumberInput:'cui_number_input(ui, id, &theme, label,\n    &state, &value, (cui_number_options){\n        .size = CUI_NUMBER_SMALL, .step = 1});',
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
 Attachment:'cui_attachment(ui, id, &theme, &state, options);\n/* Compose scrolling cards with cui_attachment_group. */',
 DescriptionList:'cui_description_list(ui, id, &theme, items, count, options);',
 StatusBar:'cui_status_bar(ui, id, &theme, left, center, right, options);',
 Resizable:'cui_resizable(ui, id, &theme, &state, panels, count, options);',
 Icon:'cui_icon_set_name(&state, cui_string("bell"), NULL, NULL);\ncui_icon(ui, id, &theme, &state, options);',
 Toolbar:'cui_toolbar(ui, id, &theme, items, count, options);\n/* Items render commands, groups or an editor. */',
 TitleBar:'cui_title_bar(ui, id, &theme, &state, items, count, options);\n/* A portable host decides which window actions exist. */',
 Sidebar:'cui_sidebar(ui, id, &theme, &state, entries, count, options);\n/* Compose groups, menu items, header and footer in entry callbacks. */',
 Textarea:'cui_textarea(ui, id, &theme, cui_string("Notes"), edit,\n    &state, (cui_textarea_options){.width = 320, .rows = 4});',
 Settings:'cui_settings_state_init(&state, page_workspace, 2, search);\ncui_settings(ui, id, &theme, pages, 2, &state,\n    (cui_settings_options){.width = 800, .height = 500});',
 Carousel:'cui_carousel_state_init(&state, geometry, 4, 4);\ncui_carousel(ui, id, &theme, &state, slides, 4,\n    (cui_carousel_options){.width = 480, .height = 200,\n        .controls = draw_controls, .pagination = draw_pages});',
 Command:'cui_command_init(&state, rows, 32, query);\ncui_command(ui, id, &theme, &state, entries, count, options);',
 Questionnaire:`/* Application owns state, choices and render callbacks.
 * draw_choice_content has cui_questionnaire_choice_render_fn signature. */
options.render_content = draw_choice_content;
options.render_user = &theme;
if (cui_questionnaire_begin(ui, root_id, &theme, &state, options)) {
    cui_questionnaire_progress(ui, progress_id, &theme, &state, options);
    if (cui_questionnaire_item_begin(ui, item_id, &state, item, options)) {
        cui_questionnaire_heading(ui, heading_id, &theme, &state, item, options);
        if (cui_questionnaire_choices_begin(ui, group_id, &state, item, options)) {
            cui_questionnaire_choice_control(ui, choice_id, &theme, &state, item, choice, options);
            cui_questionnaire_part_end(ui);
        }
        cui_questionnaire_part_end(ui);
    }
    cui_questionnaire_end(ui);
}`,

 Speech:'cui_speech_init(&state, workspace, recognizer, input, on_event, user);\ncui_speech_button(ui, id, &theme, &state, options, now);\ncui_speech_waveform(ui, waveform_id, &theme, &state, waveform, now);',
 Chart:'cui_chart_options options = cui_chart_theme(config, &theme);\ncui_chart_render_animated(ui, &state, &options);\ncui_chart(ui, id, &state, &options, cui_string("Activity"));',
 TextView:'cui_text_view_init(&state, workspace, CUI_TEXT_VIEW_MARKDOWN);\ncui_text_view_set_text(&state, markdown);\ncui_text_view(ui, id, &theme, &state, options);',
 'Root View':'cui_root_init(&root, plugins, 1, &registry, window);\ncui_root_bind_facilities(&root, dialogs, 2, &notifications);\ncui_root_render(ui, id, &root, &theme, viewport, NULL, content, user, now);'
};
let wasm, catalog, documents, themes, selected, framePending=false, wakeTimer, lastTime=performance.now();
const reducedMotion=matchMedia('(prefers-reduced-motion: reduce)');
const systemDark=matchMedia('(prefers-color-scheme: dark)');
let themeChoice='system';
const lightDefault='default-light',darkDefault='default-dark';
function selectedPreset(){
 if(!themes)return null;
 const id=themeChoice==='system'?(systemDark.matches?darkDefault:lightDefault):themeChoice;
 return themes.findIndex(theme=>theme.id===id);
}
function effectiveTheme(){return themeChoice==='high-contrast'?2:themes?.[selectedPreset()]?.mode==='dark'?1:0;}
function paletteOption(id){return $('palette-menu').querySelector(`[data-palette="${id}"]`);}
function updatePaletteSelection(){
 const active=themeChoice==='system'?'system':themeChoice;
 for(const option of $('palette-menu').querySelectorAll('[role="option"]')){
  const chosen=option.dataset.palette===active;
  option.setAttribute('aria-selected',String(chosen));option.tabIndex=chosen?0:-1;
  if(chosen)option.querySelector('.palette-check').removeAttribute('hidden');
  else option.querySelector('.palette-check').setAttribute('hidden','');
 }
 $('palette-label').textContent=active==='system'?'Follow System':active==='high-contrast'?'High contrast':themes?.find(t=>t.id===active)?.name||'Follow System';
}
function syncTheme(){
 const preset=themes?.[selectedPreset()];const colors=preset?.colors||{};
 const tokens={'--bg':'background','--surface':'popover.background','--text':'foreground','--muted':'muted.foreground','--line':'border','--accent':'primary.background','--accent-foreground':'primary.foreground','--soft':'muted.background','--code':'muted.background'};
 for(const [css,key] of Object.entries(tokens)){
  if(colors[key])document.body.style.setProperty(css,colors[key]);
  else document.body.style.removeProperty(css);
 }
 const dark=themeChoice==='high-contrast'||preset?.mode==='dark';
 document.body.classList.toggle('system',themeChoice==='system');document.body.classList.toggle('dark',dark);document.body.classList.toggle('contrast',themeChoice==='high-contrast');
 document.body.style.colorScheme=dark?'dark':'light';
 $('theme').value=themeChoice==='system'?'3':themeChoice==='high-contrast'?'2':dark?'1':'0';
 updatePaletteSelection();requestFrame();
}
function chooseTheme(id){
 if(id!=='system'&&id!=='high-contrast'&&!themes.some(theme=>theme.id===id))return;
 themeChoice=id;try{localStorage.setItem('cui.theme',id);}catch{}syncTheme();closePalette(true);
}
function closePalette(restoreFocus=false){
 $('palette-menu').hidden=true;$('palette-trigger').setAttribute('aria-expanded','false');
 if(restoreFocus)$('palette-trigger').focus();
}
function openPalette(){
 $('palette-menu').hidden=false;$('palette-trigger').setAttribute('aria-expanded','true');
 (paletteOption(themeChoice)||paletteOption('system'))?.focus();
}
function buildPaletteMenu(){
 const menu=$('palette-menu');menu.replaceChildren();
 for(const [label,items] of [['System',[{id:'system',name:'Follow System'}]],['Light',themes.filter(t=>t.mode==='light')],['Dark',themes.filter(t=>t.mode==='dark')],['Accessibility',[{id:'high-contrast',name:'High contrast'}]]]){
  const group=document.createElement('div');group.setAttribute('role','group');group.setAttribute('aria-label',label);
  const heading=document.createElement('div');heading.className='palette-group-heading';heading.textContent=label;group.append(heading);
  for(const item of items){
   const option=document.createElement('button');option.type='button';option.setAttribute('role','option');option.dataset.palette=item.id;option.textContent=item.name;
   const check=document.createElementNS('http://www.w3.org/2000/svg','svg');check.setAttribute('viewBox','0 0 24 24');check.setAttribute('aria-hidden','true');check.classList.add('palette-check');
   const path=document.createElementNS('http://www.w3.org/2000/svg','path');path.setAttribute('d','m5 12 4 4L19 6');check.append(path);option.append(check);
   option.addEventListener('click',()=>chooseTheme(item.id));group.append(option);
  }menu.append(group);
 }
 updatePaletteSelection();
}
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
 if(!selected || (!selected.example && !(selected.name in stories)))return;
 const designScale=($('design-size').hidden?0:Number($('design-size').value)),designHeights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264];
 const gaps=[0,4,4,6,6,8,8,12,12,16,20,24,28,32,40];
 canvas.style.minWidth=selected.name==='OtpInput'&&designScale>0?`${6*designHeights[designScale]+7*gaps[designScale]+48}px`:'0';
 const width=canvas.clientWidth;let height=selected.example?840:selected.name==='Button'&&designScale>0?Math.max(260,(canvas.clientWidth<600?7:4)*designHeights[designScale]+124):selected.name==='Alert'?540:selected.name==='Toolbar'?460:selected.name==='Badge'?400:selected.name==='Editor'?560:selected.name==='Plot'?440:selected.name==='Dock'?520:selected.name==='VirtualList'?360:selected.name==='Message'?500:selected.name==='MessageScroller'?550:selected.name==='Sidebar'?500:selected.name==='Textarea'?360:selected.name==='Carousel'?420:selected.name==='Command'?480:selected.name==='Root View'?520:selected.name==='TextView'?640:selected.name==='Chart'?440:selected.name==='Speech'?400:selected.name==='Questionnaire'?560:selected.name==='Settings'?640:selected.name==='Image'?320:selected.name==='Attachment'?420:selected.name==='DescriptionList'?360:selected.name==='Resizable'?360:selected.name==='Theme'?1150:selected.name==='Accordion'?650:selected.name==='Collapsible'?380:selected.name==='Notification'?720:selected.name==='Form'?Math.max(360,designScale>0?designHeights[designScale]*6+180:360):selected.name==='Calendar'?420:selected.name==='DatePicker'?380:selected.name==='ColorPicker'?620:selected.name==='DataTable'?340:['Menu','ContextMenu'].includes(selected.name)?420:260;const dpr=Math.min(devicePixelRatio||1,3);
 if(designScale>=7&&['DropdownButton','Badge','Tag','Attachment','Progress','Settings','Tabs','DescriptionList','Icon','Toolbar','Questionnaire','Speech'].includes(selected.name))height=Math.max(height,designHeights[designScale]*(selected.name==='Questionnaire'?8:4)+140);
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
 wasm.gallery_description_options(Number($('description-columns').value),Number($('description-vertical').checked),Number($('description-plain').checked),Number($('description-size').value));
 wasm.gallery_status_options(Number($('status-right').checked),Number($('status-custom').checked));
 wasm.gallery_resizable_options(Number($('resizable-vertical').checked),Number($('resizable-disabled').checked));
 wasm.gallery_icon_options(Number($('icon-choice').value),Number($('icon-size').value),Number($('icon-rotate').checked),Number($('icon-color').checked));
 wasm.gallery_toolbar_options(Number($('toolbar-size').value),Number($('toolbar-grouped').checked),Number($('toolbar-input').checked));
 wasm.gallery_settings_options(Number($('settings-size').value),Number($('settings-variant').value));
 wasm.gallery_carousel_options(Number($('carousel-axis').value),Number($('carousel-loop').checked));
 wasm.gallery_command_palette_options(Number($('command-modal').checked),Number($('command-loading').checked),Number($('command-filterable').checked));
 wasm.gallery_text_view_motion(Number($('motion-duration').value),Number($('motion-stagger').value),Number($('motion-easing').value));
 wasm.gallery_text_view_options(Number($('text-view-format').value),Number($('text-view-scroll').checked),Number($('text-view-fade').checked));
 wasm.gallery_chart_options(Number($('chart-kind').value),Number($('chart-curve').value),Number($('chart-dots').checked),Number($('chart-labels').checked),Number($('chart-animate').checked));
 wasm.gallery_speech_options(Number($('speech-size').value),Number($('speech-unavailable').checked));
 wasm.gallery_questionnaire_options(Number($('questionnaire-size').value),Number($('questionnaire-numbers').checked),Number($('questionnaire-custom').checked));
 wasm.gallery_sidebar_options(Number($('sidebar-mode').value),Number($('sidebar-side').value),Number($('sidebar-collapsed').checked));
 wasm.gallery_textarea_options(Number($('textarea-rows').value),Number($('textarea-grow').checked),Number($('textarea-readonly').checked),Number($('textarea-nowrap').checked));
 wasm.gallery_title_options(Number($('title-decoration').value),Number($('title-close').checked),Number($('title-fullscreen').checked));
 wasm.gallery_typography_options(Number($('text-family').value),Number($('text-size').value),Number($('text-weight').value),Number($('text-italic').checked));
 wasm.gallery_effects_options(Number($('effect-angle').value),Number($('effect-opacity').value),Number($('effect-blur').value));
 wasm.gallery_plot_mode(Number($('plot-mode').value));
 wasm.gallery_virtual_mode(Number($('virtual-axis').value));
 wasm.gallery_design_options(($('design-size').hidden?0:Number($('design-size').value)),Number($('design-radius').value),Number($('design-spacing').value));
 wasm.gallery_control_options(Number($('control-size').value),Number($('choice-required').checked),Number($('choice-invalid').checked),Number($('choice-skip-tab').checked),Number($('switch-side').checked),Number($('switch-color').checked),Number($('toggle-variant').value),Number($('toggle-segmented').checked),Number($('toggle-selection').value));
 wasm.gallery_message_options(Number($('message-alignment').value),Number($('message-variant').value),Number($('message-inset').value),Number($('message-avatar').checked),Number($('message-scrollbar').checked));
 wasm.gallery_dropdown_options(...['variant','size','halves','action','anchor'].map(k=>Number($('dropdown-'+k).value)),Number($('dropdown-outline').checked),Number($('dropdown-selected').checked));
 wasm.gallery_tabs_options(Number($('tabs-mode').value),Number($('tabs-size').value),Number($('tabs-variant').value),Number($('tabs-manual').checked));
 wasm.gallery_progress_mode(Number($('progress-mode').value));
 wasm.gallery_progress_shape(Number($('progress-shape').value));
 wasm.gallery_shimmer_mode(Number($('shimmer-mode').value));
 wasm.gallery_notification_options(Number($('notification-anchor').value),Number($('notification-kind').value),Number($('notification-autohide').checked));
 wasm.gallery_tag_options(Number($('tag-variant').value),Number($('tag-color').value),Number($('tag-outline').checked),Number($('tag-pill').checked));
 wasm.gallery_marker_options(Number($('marker-variant').value),Number($('marker-loading').value),Number($('marker-alignment').value),Number($('marker-icon').checked));
 wasm.gallery_otp_options(Number($('otp-groups').value),Number($('otp-mask').checked));
 wasm.gallery_group_mode(Number($('group-mode').value));
 wasm.gallery_form_mode(Number($('form-mode').value));
 wasm.gallery_date_locale(Number($('date-locale').value));
 wasm.gallery_density(Number($('density').value));
 wasm.gallery_selection_mode(Number($('selection-mode').value));
 const frameReducedMotion=reducedMotion.matches;
 const now=performance.now();wasm.gallery_time(now/1000,Math.min((now-lastTime)/1000,.1),Number(frameReducedMotion));lastTime=now;
 wasm.gallery_theme_preset(themeChoice==='high-contrast'?-1:selectedPreset());
 if(!wasm.gallery_frame(selected.name==='Menu'&&$('menu-kind').value==='1'?47:selected.example?80:stories[selected.name],effectiveTheme(),Number($('disabled').checked),width,height))throw Error(`CUI layout failed (error ${wasm.gallery_error()})`);
 applyTouchPageFallback();
 let clips=0;
 for(let i=0;i<wasm.gallery_count();i++){
  const f=n=>wasm.gallery_command(i,n);const type=f(0),x=f(1),y=f(2),w=f(3),h=f(4);
  if(type===3){ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();clips++;continue;}
  if(type===4){if(clips){ctx.restore();clips--;}continue;}
  ctx.fillStyle=ctx.strokeStyle=`rgba(${f(5)},${f(6)},${f(7)},${f(8)/255})`;
  if(type===2){const size=f(11),weight=f(22),italic=!!f(23),monospace=!!f(24),familyId=wasm.gallery_command_family(i)>>>0;ctx.font=canvasFont(size,weight,italic,monospace,familyId);const baseline=textBaselineOffset(size,weight,italic,monospace,familyId);ctx.textBaseline=baseline===null?'top':'alphabetic';ctx.fillText(stringAt(wasm.gallery_command_text(i),wasm.gallery_command_length(i)),x,y+(baseline??0));}
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
 syncImeTarget();
 canvas.style.cursor=['default','pointer','text','ew-resize','ns-resize','move','not-allowed','crosshair','grab','grabbing','zoom-in','zoom-out','wait','help'][wasm.gallery_cursor()]||'default';
 syncSemantics();
 if(selected.name==='Checkbox')$('checkbox-state').value=String(wasm.gallery_checkbox_state(-1));
 if(selected.name==='Sidebar')$('sidebar-collapsed').checked=Boolean(wasm.gallery_sidebar_stats(0));
 if(selected.name==='Attachment')$('attachment-status').value=String(wasm.gallery_attachment_stats(9));
 $('table-selection').value=String(wasm.gallery_table_selection_mode(-1));
 window.cuiGallery.renderedReducedMotion=frameReducedMotion;
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
let semanticStory = null;
const roles=['','button','checkbox','switch','radio','slider','textbox','text','tab','group','tablist','tabpanel','list','listbox','option','menu','menuitem','dialog','alert','status','img','progressbar','tree','treeitem','menuitemcheckbox','menuitemradio','table','row','cell','columnheader','gridcell','grid','scrollbar','rowheader','rowgroup','alertdialog','link','listitem','log','toolbar','separator','region','radiogroup'];
const interactiveRoles=[1,2,3,4,5,6,8,14,16,23,24,25,29,30,32,33,36];
function syncSemantics(){
 const root=$('accessible'),seen=new Set(),entries=[];
 if(semanticStory!==selected.name){root.replaceChildren();semanticNodes.clear();semanticStory=selected.name;}
 for(let i=0;i<wasm.gallery_semantic_count();i++){
  const f=n=>wasm.gallery_semantic(i,n),role=roles[f(0)];if(!role)continue;
  const id=`cui-semantic-${wasm.gallery_semantic_id(i)}`;seen.add(id);
  let el=semanticNodes.get(id);
  if(!el){el=document.createElement(interactiveRoles.includes(f(0))&&!['treeitem','columnheader'].includes(role)?'button':'div');el.id=id;if(el.tagName==='BUTTON'){el.type='button';el.tabIndex=-1;}semanticNodes.set(id,el);}
  if(role==='text')el.removeAttribute('role');else el.setAttribute('role',role);
  const name=stringAt(wasm.gallery_semantic_name(i),wasm.gallery_semantic_length(i));
  if(role==='text')el.textContent=name;
  el.setAttribute('aria-label',name);
  if(f(33)&&el.tagName!=='BUTTON'){el.tabIndex=-1;el.setAttribute('aria-disabled',String(Boolean(f(5))));el.onfocus=()=>{if(!f(5)){wasm.gallery_focus(i);requestFrame();}};el.onkeydown=e=>{const key=keyIdentity(e.key);if(key&&!f(5)){e.preventDefault();wasm.gallery_keyboard_event(3,key,mods(e),Number(e.repeat),Number(e.isComposing),Number(role==='textbox'&&plainCharacterInput(e)));requestFrame();}};}else if(el.tagName!=='BUTTON'){el.removeAttribute('tabindex');el.onfocus=null;el.onkeydown=null;}
  const description=stringAt(wasm.gallery_semantic_description(i),wasm.gallery_semantic_description_length(i));
  if(description)el.setAttribute('aria-description',description);else el.removeAttribute('aria-description');
  if(role==='columnheader'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0,0,1);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0,0,1);requestFrame();};}
  if(el.tagName==='BUTTON'){if(el.textContent!==name)el.textContent=name;el.disabled=Boolean(f(5));el.onclick=e=>{e.stopPropagation();wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0,0,1);wasm.gallery_event(2,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0,0,1);requestFrame();};}
  else el.setAttribute('aria-label',name);
  if(['checkbox','switch','radio','menuitemcheckbox','menuitemradio'].includes(role))el.setAttribute('aria-checked',role==='checkbox'&&f(8)===2?'mixed':String(Boolean(f(6))));
  if(['checkbox','radio'].includes(role)){el.setAttribute('aria-required',String(Boolean(f(27))));el.setAttribute('aria-invalid',String(Boolean(f(28))));}
  if(role==='button'&&selected.name==='Toggle')el.setAttribute('aria-pressed',String(Boolean(f(6))));else el.removeAttribute('aria-pressed');
  el.setAttribute('aria-busy',String(Boolean(f(14))));
  if(['listbox','tree','grid'].includes(role))el.setAttribute('aria-multiselectable',String(Boolean(f(15))));
  if(role==='treeitem'){el.onclick=e=>{e.stopPropagation();if(f(5))return;wasm.gallery_event(1,f(1)+f(3)/2,f(2)+f(4)/2,0,0,0,0,1);requestFrame();};el.setAttribute('aria-level',String(f(30)||f(8)+1));el.setAttribute('aria-disabled',String(Boolean(f(5))));}
  if(['treeitem','radio','checkbox'].includes(role)){if(f(31))el.setAttribute('aria-posinset',String(f(31)));else el.removeAttribute('aria-posinset');if(f(32))el.setAttribute('aria-setsize',String(f(32)));else el.removeAttribute('aria-setsize');}
  if(role==='tab'||role==='option'||role==='treeitem')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(f(16))el.setAttribute('aria-orientation',f(16)===2?'vertical':'horizontal');else el.removeAttribute('aria-orientation');
  if(role==='separator'&&f(13)>f(12)){el.setAttribute('aria-valuenow',String(f(8)));el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));}
  for(const [field,attribute] of [[22,'aria-rowindex'],[23,'aria-colindex'],[24,'aria-rowcount'],[25,'aria-colcount']]){if(f(field)||(field>=24&&['grid','table'].includes(role)))el.setAttribute(attribute,String(f(field)));else el.removeAttribute(attribute);}
  if(role==='gridcell'||role==='columnheader'||role==='row'||role==='rowheader')el.setAttribute('aria-selected',String(Boolean(f(11))));
  if(role==='columnheader')el.setAttribute('aria-sort',f(8)===1?'ascending':f(8)===2?'descending':'none');
  if(role==='dialog'||role==='alertdialog')el.setAttribute('aria-modal',String(selected.name!=='Popover'));else el.removeAttribute('aria-modal');
  if(role==='textbox'){el.setAttribute('aria-readonly',String(Boolean(f(26))));el.setAttribute('aria-required',String(Boolean(f(27))));el.setAttribute('aria-invalid',String(Boolean(f(28))));el.setAttribute('aria-multiline',String(Boolean(f(29))));}
  if(role==='progressbar'){el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));if(f(14))el.removeAttribute('aria-valuenow');else el.setAttribute('aria-valuenow',String(f(8)));}
  if(role==='slider'||role==='scrollbar'){el.setAttribute('aria-valuenow',String(f(8)));el.setAttribute('aria-valuemin',String(f(12)));el.setAttribute('aria-valuemax',String(f(13)));}
  if(selected.name==='Accordion'||selected.name==='Collapsible'||((selected.name==='Select'||selected.name==='Menu'||selected.name==='DatePicker')&&f(0)===1))el.setAttribute('aria-expanded',String(Boolean(f(10))));else el.removeAttribute('aria-expanded');
  entries.push({el,parent:`cui-semantic-${wasm.gallery_semantic_parent(i)}`,controls:`cui-semantic-${wasm.gallery_semantic_controls(i)}`,labelledBy:`cui-semantic-${wasm.gallery_semantic_labelled_by(i)}`});
 }
 for(const [id,el] of semanticNodes)if(!seen.has(id)){el.remove();semanticNodes.delete(id);}
 const byId=new Map(entries.map(entry=>[entry.el.id,entry]));
 const depth=(entry,visited=new Set())=>{if(visited.has(entry.el.id))return 0;visited.add(entry.el.id);const parent=byId.get(entry.parent);return parent?1+depth(parent,visited):0;};
 entries.sort((a,b)=>depth(a)-depth(b));
 const positions=new Map();
 for(const {el,parent,controls,labelledBy} of entries){
  const container=seen.has(parent)?semanticNodes.get(parent):root;
  if(container===el||el.contains(container))throw Error(`semantic cycle: ${el.id} (${el.getAttribute('role')}) parent ${parent} (${container.getAttribute('role')}); logical ${byId.get(parent)?.parent}; existing ${Array.from({length:8},(_,i)=>{let node=container;for(let j=0;j<i;j++)node=node?.parentElement;return node?.id}).join('>')}`);
  const position=positions.get(container)||0,reference=container.children[position];
  if(reference!==el)container.insertBefore(el,reference||null);positions.set(container,position+1);
  if(seen.has(controls))el.setAttribute('aria-controls',controls);else el.removeAttribute('aria-controls');
  if(seen.has(labelledBy))el.setAttribute('aria-labelledby',labelledBy);else el.removeAttribute('aria-labelledby');
 }
 for(const {el} of entries)if(el.getAttribute('role')==='treeitem'){const toggle=Array.from(el.children).find(child=>child.getAttribute('role')==='button'&&['Expand','Collapse'].includes(child.textContent));if(toggle)el.setAttribute('aria-expanded',String(toggle.textContent==='Collapse'));else el.removeAttribute('aria-expanded');}
}
function fuzzyScore(value,query){
 const name=value.toLowerCase(),needle=query.trim().toLowerCase();if(!needle)return 0;
 let cursor=0,score=0,previous=-2;
 for(const character of needle){const found=name.indexOf(character,cursor);if(found<0)return -1;score+=found===previous+1?4:1;score+=found===0||' -_'.includes(name[found-1])?3:0;previous=found;cursor=found+1;}
 return score-(name.length-needle.length)*.04;
}
function navigation(){
 const query=$('search').value, nav=$('catalog');nav.replaceChildren();let category='';
 const results=catalog.components.map(item=>({item,score:fuzzyScore(item.name,query)})).filter(result=>result.score>=0);
 if(query.trim())results.sort((a,b)=>b.score-a.score||a.item.name.localeCompare(b.item.name));
 for(const {item} of results){
  if(!query.trim()&&item.category!==category){category=item.category;const h=document.createElement('h3');h.textContent=category.replaceAll('_',' ');nav.append(h);}
  const a=document.createElement('a');a.href=`#${encodeURIComponent(item.name)}`;a.classList.toggle('active',selected?.name===item.name);if(selected?.name===item.name)a.setAttribute('aria-current','page');
  const text=document.createElement('span');text.textContent=item.name;const dot=document.createElement('span');dot.className=`dot ${item.name in stories?'live':''}`;dot.title=item.name in stories?'Live browser story':'Browser story pending';a.append(text,dot);nav.append(a);
 }
 if(!results.length){const empty=document.createElement('p');empty.className='nav-empty';empty.textContent='No matching components';nav.append(empty);}
}
function docsNavigation(active){
 const nav=$('docs-nav');nav.replaceChildren();
 for(const category of ['Getting started','Guides','Components','Development']){
  const entries=documents.filter(doc=>doc.category===category);if(!entries.length)continue;
  const h=document.createElement('h3');h.textContent=category;nav.append(h);
  for(const doc of entries){const link=document.createElement('a');link.href=`#docs/${doc.slug}`;link.textContent=doc.title;link.classList.toggle('active',doc.slug===active);if(doc.slug===active)link.setAttribute('aria-current','page');nav.append(link);}
 }
}
function route(){
 const raw=location.hash.slice(1)||document.body.dataset.route||'home';
 if(!raw||raw==='home'){
  selected=null;$('home-page').hidden=false;$('docs-page').hidden=true;$('gallery-page').hidden=true;$('catalog').hidden=false;$('docs-nav').hidden=true;
  document.title='CUI · Portable C11 UI';shareRoute('', 'home',document.title,'Build portable C11 interfaces with CUI: themed components, explicit state, composable behavior, live WebAssembly examples and browsable documentation.');navigation();document.body.classList.remove('menu-open');return;
 }
 if(raw.startsWith('docs/')){
  const parts=raw.slice(5).split('/');let slug,anchor;
  try{slug=decodeURIComponent(parts[0]);anchor=decodeURIComponent(parts.slice(1).join('/'));}catch{slug='getting-started';anchor='';}
  const doc=documents.find(entry=>entry.slug===slug)||documents.find(entry=>entry.slug==='getting-started');
  selected=null;$('home-page').hidden=true;$('docs-page').hidden=false;$('gallery-page').hidden=true;$('catalog').hidden=true;$('docs-nav').hidden=false;
  $('docs-title').textContent=doc.title;$('docs-content').innerHTML=doc.html;$('docs-source').href=doc.source;
  for(const code of $('docs-content').querySelectorAll('pre>code')){
   const button=document.createElement('button');button.type='button';button.className='docs-copy';button.textContent='Copy';button.setAttribute('aria-label','Copy code block');
   button.addEventListener('click',()=>copyCode(button,code.textContent));code.parentElement.classList.add('copyable');code.parentElement.prepend(button);
  }
  document.title=`${doc.title} · CUI`;shareRoute(`docs/${doc.slug}/`,raw,document.title,($('docs-content').querySelector('p')?.textContent||doc.title).trim().slice(0,180));docsNavigation(doc.slug);document.body.classList.remove('menu-open');
  if(anchor)requestAnimationFrame(()=>document.getElementById(`doc-${anchor}`)?.scrollIntoView());else requestAnimationFrame(()=>window.scrollTo(0,0));
  return;
 }
 $('home-page').hidden=true;$('docs-page').hidden=true;$('gallery-page').hidden=false;$('catalog').hidden=false;$('docs-nav').hidden=true;
 selectComponent();
}
let screenColorRequest=null;
function screenColorControls(message=''){
 const active=selected?.name==='ColorPicker',supported=isSecureContext&&typeof window.EyeDropper==='function';
 $('screen-color').hidden=!active;$('screen-color-status').hidden=!active;
 $('screen-color').disabled=!supported||$('disabled').checked||Boolean(screenColorRequest);
 $('screen-color-status').textContent=message||(!supported?'Screen picking is unavailable in this browser.':screenColorRequest?'Choose a pixel; Escape cancels.':'');
}
async function pickScreenColor(){
 if(selected?.name!=='ColorPicker'||$('screen-color').disabled)return;
 const controller=new AbortController();screenColorRequest=controller;screenColorControls();
 try{
  // Call open inside this trusted click, before any await or C frame scheduling.
  const result=await new window.EyeDropper().open({signal:controller.signal});
  if(screenColorRequest!==controller)return;
  if(!/^#[0-9a-f]{6}$/i.test(result?.sRGBHex||''))throw new Error('Invalid color');
  if(selected?.name!=='ColorPicker'||$('disabled').checked)return;
  wasm.gallery_screen_color(parseInt(result.sRGBHex.slice(1),16));requestFrame();
  screenColorControls(`Selected ${result.sRGBHex.toUpperCase()}.`);
 }catch(error){
  if(screenColorRequest===controller)screenColorControls(error?.name==='AbortError'?'Color picking cancelled.':'Unable to pick a screen color.');
 }finally{
  if(screenColorRequest===controller){screenColorRequest=null;const message=$('screen-color-status').textContent;screenColorControls(message);}
 }
}
function cancelScreenColor(){if(screenColorRequest){const request=screenColorRequest;screenColorRequest=null;request.abort();}screenColorControls();}
$('screen-color').addEventListener('click',pickScreenColor);
const exampleDescriptor={name:'Northstar workspace',example:true,status:'tested',public_header:'examples/composed_apps/app.h',reference_url:'https://github.com/mattmezza/cui/tree/main/examples/composed_apps',features:[],limitations:['Session-local demo state; sign-in and pricing actions are examples without a server.','Native Linux/X11 and portable headless interactions are validated; other native platforms remain unverified.'],usage:{composition:'Application model + immediate frame composition',state_ownership:'Retain composed_app until its draw lists finish rendering.',interaction:'Pass app_callback to composed_app_create for navigation, session, plan and save events.',usage:'Open Overview, try a plan CTA, use the account menu, then edit and save workspace settings.',docs:['docs/COMPOSED_APPS.md']}};
function selectComponent(){
 let name='Button';try{name=decodeURIComponent(location.hash.slice(1)||document.body.dataset.route)||'Button';}catch{/* Malformed percent escapes select the default component. */}selected=name==='examples/northstar'?exampleDescriptor:catalog.components.find(c=>c.name===name)||catalog.components.find(c=>c.name==='Button');
 document.title=`${selected.name} · CUI`;shareRoute(selected.example?'examples/northstar/':`components/${componentSlug(selected.name)}/`,selected.example?'examples/northstar':encodeURIComponent(selected.name),document.title,`Explore CUI ${selected.name}: its C11 API, interactive WebAssembly example, application-owned state and implementation scope. Complete component coverage remains in progress.`); $('name').textContent=selected.name; $('status').textContent=selected.status.replaceAll('_',' ');
 const live=selected.example||selected.name in stories;
 window.cuiGallery.renderedComponent=live?'':selected.name;
 if(modelStories.has(selected.name))$('status').textContent+=' / base model'; $('preview-card').hidden=!live; $('pending').hidden=live;
 $('description').textContent=live?`Explore ${selected.name.toLowerCase()} through the same CUI core, base behavior and theme used by native applications.`:`${selected.name} is part of the 78-family component catalog.`;
 if(modelStories.has(selected.name))$('description').textContent='Interactive demonstration of the implemented base model. The complete styled component remains pending.';
 $('pending-message').textContent=selected.status==='not_started'?'Implementation and browser story are pending. This entry stays visible to preserve the complete component scope.':'Repository implementation exists for part of this entry. A browser story is pending; see implementation scope and source for details.';
 $('snippet').textContent=(selected.example?'/* Retain app across frames. Include examples/composed_apps/app.h. */\ncomposed_app *app = composed_app_create(on_app_event, user);\n/* Between frames, before cui_begin_frame: */\ncomposed_app_set_theme(app, &theme);\n/* Between cui_begin_frame and cui_end_frame: */\ncomposed_app_scene(ui, app, width, height);\n/* At shutdown, after rendering the last draw list: */\ncomposed_app_destroy(app);':snippets[selected.name])||(live?`/* See the public ${selected.public_header || 'component'} API. */`:'/* No complete styled browser example yet. */');
 $('source').href=`https://github.com/mattmezza/cui/${selected.public_header?'blob/main/'+selected.public_header:'blob/main/docs/gpui-parity.json'}`;

 const usage=selected.usage;
 $('component-composition').textContent=usage?.composition||'Immediate frame composition';
 $('component-state').textContent=usage?.state_ownership||'Retain application values between frames.';
 $('component-events').textContent=usage?.interaction||'Use the linked guide for the example callbacks.';
 $('component-usage-note').textContent=usage?.usage||'';
 $('component-guides').replaceChildren();for(const path of usage?.docs||[]){const doc=documents.find(d=>d.source.endsWith('/'+path));if(doc){const a=document.createElement('a');a.href='#docs/'+doc.slug;a.textContent=doc.title;$('component-guides').append(a,document.createTextNode(' '));}}

 $('features').replaceChildren();for(const feature of selected.features){const row=document.createElement('tr'),name=document.createElement('td'),status=document.createElement('td');name.textContent=feature.name;status.textContent=feature.status.replaceAll('_',' ');status.className=feature.status;row.append(name,status);$('features').append(row);}
 $('limitations').replaceChildren();for(const message of [...selected.limitations,...(live?['Canvas text uses the browser system font; native FreeType / HarfBuzz rendering is separate.','Browser semantics are experimental; native IME candidate UI and a complete platform accessibility bridge vary by host.']:[])]){const li=document.createElement('li');li.textContent=message;$('limitations').append(li);}
 $('table-pin').hidden=selected.name!=='DataTable';$('table-pin').value='0';
 $('table-mode').hidden=selected.name!=='DataTable';$('table-mode').value='0';
 $('table-selection').hidden=selected.name!=='DataTable';$('table-selection').value='2';
 $('surface-variant').hidden=selected.name!=='Sheet';$('surface-variant').value='0';
 $('disclosure-mode').hidden=!['Accordion','Collapsible'].includes(selected.name);$('disclosure-mode').value='0';
 $('disclosure-multiple-control').hidden=selected.name!=='Accordion';$('disclosure-multiple').checked=false;
 for(const id of ['message-alignment','message-variant','message-inset','message-avatar-control'])$(id).hidden=selected.name!=='Message';
 $('checkbox-state').hidden=selected.name!=='Checkbox';
 for(const id of ['settings-size','settings-variant'])$(id).hidden=selected.name!=='Settings';
 for(const id of ['carousel-axis','carousel-loop-control'])$(id).hidden=selected.name!=='Carousel';
 for(const id of ['speech-size','speech-unavailable-control'])$(id).hidden=selected.name!=='Speech';
 for(const id of ['questionnaire-size','questionnaire-numbers-control','questionnaire-custom-control'])$(id).hidden=selected.name!=='Questionnaire';
 $('questionnaire-size').value='0';$('questionnaire-numbers').checked=false;$('questionnaire-custom').checked=false;
 $('speech-size').value='0';$('speech-unavailable').checked=false;
 for(const id of ['chart-kind','chart-curve','chart-dots-control','chart-labels-control','chart-animate-control'])$(id).hidden=selected.name!=='Chart';
 $('chart-kind').value='0';$('chart-curve').value='0';$('chart-dots').checked=false;$('chart-labels').checked=false;$('chart-animate').checked=false;
 for(const id of ['text-view-format','text-view-scroll-control','text-view-fade-control','motion-duration','motion-stagger','motion-easing'])$(id).hidden=selected.name!=='TextView';
 $('motion-duration').value='0.35';$('motion-stagger').value='0';$('motion-easing').value='0';$('text-view-format').value='0';$('text-view-scroll').checked=false;$('text-view-fade').checked=false;
 for(const id of ['command-modal-control','command-loading-control','command-filterable-control'])$(id).hidden=selected.name!=='Command';
 $('control-size').hidden=!['Button','Checkbox','Radio','Switch','Toggle','Badge','Progress','Form','Input Group','OtpInput','Tag','Attachment','Select','Combobox','Textarea','Input','NumberInput'].includes(selected.name);
 $('control-size').value='0';
 $('control-size').disabled=false;
 $('plot-mode').hidden=selected.name!=='Plot';$('plot-mode').value='0';
 $('virtual-axis').hidden=selected.name!=='VirtualList';$('virtual-axis').value='0';
 for(const id of ['effect-angle','effect-blur','effect-opacity','text-family','text-size','text-weight','text-italic-control'])$(id).hidden=selected.name!=='Theme';$('effect-angle').value='35';$('effect-blur').value='0';$('effect-opacity').value='1';
 $('design-size').hidden=!['Button','Checkbox','Radio','Switch','Toggle','Input','List','Select','Combobox','Menu','Textarea','NumberInput','Form','Input Group','OtpInput','DropdownButton','Badge','Tag','Attachment','Progress','Settings','Tabs','DescriptionList','Icon','Toolbar','Questionnaire','Speech'].includes(selected.name);$('design-size').value='3';$('control-size').hidden=$('control-size').hidden||!$('design-size').hidden;
 for(const id of ['design-radius','design-spacing'])$(id).hidden=!['Button','Checkbox','Radio','Switch','Toggle','Input','Theme'].includes(selected.name);
 $('design-radius').value='-1';$('design-spacing').value='-1';
 for(const id of ['choice-required-control','choice-invalid-control','choice-skip-tab-control'])$(id).hidden=!['Checkbox','Radio'].includes(selected.name);
 for(const id of ['switch-side-control','switch-color-control'])$(id).hidden=selected.name!=='Switch';
 for(const id of ['toggle-variant','toggle-segmented-control','toggle-selection'])$(id).hidden=selected.name!=='Toggle';
 for(const id of ['image-state','image-fit','image-grayscale-control','image-opacity-control'])$(id).hidden=selected.name!=='Image';
 for(const id of ['attachment-status','attachment-vertical-control'])$(id).hidden=selected.name!=='Attachment';
 for(const id of ['description-columns','description-size','description-vertical-control','description-plain-control'])$(id).hidden=selected.name!=='DescriptionList';
 $('description-columns').value='3';$('description-size').value='0';$('description-vertical').checked=false;$('description-plain').checked=false;
 for(const id of ['status-right-control','status-custom-control'])$(id).hidden=selected.name!=='StatusBar';
 $('status-right').checked=true;$('status-custom').checked=false;
 for(const id of ['resizable-vertical-control','resizable-disabled-control'])$(id).hidden=selected.name!=='Resizable';
 $('date-locale').hidden=!['Calendar','DatePicker'].includes(selected.name);$('date-locale').value='-1';
 $('resizable-vertical').checked=false;$('resizable-disabled').checked=false;
 for(const id of ['icon-choice','icon-size','icon-rotate-control','icon-color-control'])$(id).hidden=selected.name!=='Icon';
 $('icon-choice').value='0';$('icon-size').value='3';$('icon-rotate').checked=false;$('icon-color').checked=false;
 for(const id of ['toolbar-size','toolbar-grouped-control','toolbar-input-control'])$(id).hidden=selected.name!=='Toolbar';
 $('toolbar-size').value='2';$('toolbar-grouped').checked=true;$('toolbar-input').checked=true;
 for(const id of ['title-decoration','title-close-control','title-fullscreen-control'])$(id).hidden=selected.name!=='TitleBar';
 $('title-decoration').value='0';$('title-close').checked=false;$('title-fullscreen').checked=false;
 for(const id of ['sidebar-mode','sidebar-side','sidebar-collapsed-control'])$(id).hidden=selected.name!=='Sidebar';
 $('sidebar-mode').value='0';$('sidebar-side').value='0';$('sidebar-collapsed').checked=false;
 for(const id of ['textarea-rows','textarea-grow-control','textarea-readonly-control','textarea-nowrap-control'])$(id).hidden=selected.name!=='Textarea';
 $('textarea-rows').value='4';$('textarea-grow').checked=false;$('textarea-readonly').checked=false;$('textarea-nowrap').checked=false;
 $('message-scrollbar-control').hidden=selected.name!=='MessageScroller';
 $('message-alignment').value='0';$('message-variant').value='1';$('message-inset').value='0';$('message-avatar').checked=true;$('message-scrollbar').checked=true;
 for(const id of ['dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline-control','dropdown-selected-control'])$(id).hidden=selected.name!=='DropdownButton';
 for(const k of ['variant','size','halves','action','anchor'])$('dropdown-'+k).value='0';$('dropdown-outline').checked=false;$('dropdown-selected').checked=false;
 for(const id of ['tabs-mode','tabs-size','tabs-variant','tabs-manual-control'])$(id).hidden=selected.name!=='Tabs';
 $('tabs-mode').value='0';$('tabs-size').value='0';$('tabs-variant').value='1';$('tabs-manual').checked=false;
 for(const id of ['settings-size','tabs-size','description-size','icon-size','toolbar-size','questionnaire-size','speech-size','dropdown-size'])$(id).hidden=true;
 cancelScreenColor();
 for(const id of ['progress-mode','progress-shape']){$(id).hidden=selected.name!=='Progress';$(id).value='0';}
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
const imeTarget=document.createElement('textarea');
imeTarget.id='cui-ime-input';imeTarget.tabIndex=-1;imeTarget.setAttribute('aria-label','Canvas text input');
Object.assign(imeTarget.style,{position:'fixed',opacity:'0',width:'1px',height:'1px',padding:'0',border:'0',pointerEvents:'none'});
document.body.append(imeTarget);
let composing=false,preeditText='';
function compositionEvent(phase,text='',anchor=0,cursor=anchor){
 if(!wasm?.gallery_composition)return;
 const bytes=encoder.encode(text),pointer=bytes.length?wasm.malloc(bytes.length):0;
 if(bytes.length)new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);
 wasm.gallery_composition(phase,pointer,bytes.length,anchor,cursor);
 if(pointer)wasm.free(pointer);requestFrame();
}
function syncImeTarget(){
 if(!wasm?.gallery_ime_rect)return;
 const focused=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).some(i=>wasm.gallery_semantic(i,0)===6&&wasm.gallery_semantic(i,7)&&!wasm.gallery_semantic(i,5));
 if(focused&&(document.activeElement===canvas||document.activeElement===imeTarget)){
  const rect=canvas.getBoundingClientRect(),x=wasm.gallery_ime_rect(0),y=wasm.gallery_ime_rect(1);
  if(x>=0){imeTarget.style.left=`${rect.left+x}px`;imeTarget.style.top=`${rect.top+y}px`;}
  if(document.activeElement!==imeTarget)imeTarget.focus({preventScroll:true});
 }else if(document.activeElement===imeTarget){compositionEvent(3);composing=false;canvas.focus({preventScroll:true});}
}
for(const target of [canvas,imeTarget]){
 target.addEventListener('compositionstart',()=>{composing=true;preeditText='';compositionEvent(0);});
 target.addEventListener('compositionupdate',e=>{
  preeditText=e.data;compositionEvent(1,e.data,imeTarget.selectionStart,imeTarget.selectionEnd);
 });
 target.addEventListener('compositionend',e=>{
  compositionEvent(e.data?2:3,e.data||'');composing=false;imeTarget.value='';
 });
}
// Native selection is updated after compositionupdate. Read the resulting
// UTF-16 range on input/selectionchange rather than relying on event ordering.
imeTarget.addEventListener('input',()=>{
 if(composing)compositionEvent(1,preeditText,imeTarget.selectionStart,imeTarget.selectionEnd);
 else imeTarget.value='';
});
document.addEventListener('selectionchange',()=>{
 if(composing&&document.activeElement===imeTarget)
  compositionEvent(1,preeditText,imeTarget.selectionStart,imeTarget.selectionEnd);
});
imeTarget.addEventListener('beforeinput',e=>{
 if(e.isComposing||composing||e.inputType==='insertCompositionText')return;
 if(e.inputType==='insertFromComposition'){e.preventDefault();return;}
 if(e.inputType==='insertText'&&e.data){
  e.preventDefault();const bytes=encoder.encode(e.data),pointer=wasm.malloc(bytes.length);
  new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);requestFrame();
 }
});
imeTarget.addEventListener('blur',e=>{
 if(e.relatedTarget===canvas)return;
 compositionEvent(3);composing=false;wasm?.gallery_focus(0xffffffff);requestFrame();
});
const keys={Tab:1,Enter:2,' ':3,Escape:4,ArrowLeft:5,ArrowRight:6,ArrowUp:7,ArrowDown:8,Home:9,End:10,Backspace:11,Delete:12,PageUp:19,PageDown:20};
function keyIdentity(key){return keys[key]||(key.length===1?wasm.gallery_key_identity(key.charCodeAt(0)):0);}
function plainCharacterInput(e){return e.key.length===1&&!e.ctrlKey&&!e.metaKey&&!e.altKey&&!e.isComposing;}
function prefersCharacterInput(e){return plainCharacterInput(e)&&Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).some(i=>wasm.gallery_semantic(i,0)===6&&wasm.gallery_semantic(i,7));}
function mods(e){return(e.shiftKey?1:0)|(e.ctrlKey?2:0)|(e.altKey?4:0)|(e.metaKey?8:0);}
const pointerButton=e=>e.button===2?1:e.button===1?2:0;
let suppressCompatibilityMouse=false;
const pageTouches=new Map();
function applyTouchPageFallback(){
 for(const [id,t] of pageTouches){
  const dx=t.x-t.originX,dy=t.y-t.originY;
  if(!t.owned&&!t.blocked&&pageTouches.size===1&&Math.abs(dy)>=8&&Math.abs(dy)>Math.abs(dx)&&wasm.gallery_touch_page_available(id)){
   t.owned=true;wasm.gallery_touch(7,0,0,id,1);requestFrame();
   window.scrollBy(0,-(t.y-t.ackY));
  }
  t.ackY=t.y;
 }
}

for(const [event,type] of [['pointermove',0],['pointerdown',1],['pointerup',2]])canvas.addEventListener(event,e=>{
 if(!wasm)return;
 const rect=canvas.getBoundingClientRect();
 if(e.pointerType==='touch'){
  if(type===1){
   if(pageTouches.size>=16){for(const t of pageTouches.values())t.blocked=true;}
   else {const blocked=pageTouches.size!==0;for(const t of pageTouches.values())t.blocked=true;
    const hostOnly=Array.from(pageTouches.values()).some(t=>t.owned);
    pageTouches.set(e.pointerId,{originX:e.clientX,originY:e.clientY,x:e.clientX,y:e.clientY,ackY:e.clientY,owned:hostOnly,blocked});
    if(hostOnly){canvas.setPointerCapture(e.pointerId);suppressCompatibilityMouse=true;requestFrame();return;}
   }
  }else {const t=pageTouches.get(e.pointerId);if(t){
   if(t.owned){if(type===0&&!t.blocked&&pageTouches.size===1)window.scrollBy(0,t.y-e.clientY);
    t.x=e.clientX;t.y=e.clientY;if(type===2)pageTouches.delete(e.pointerId);requestFrame();return;}
   t.x=e.clientX;t.y=e.clientY;if(type===2)pageTouches.delete(e.pointerId);
  }}
 }
 if(type===1){canvas.focus({preventScroll:true});canvas.setPointerCapture(e.pointerId);suppressCompatibilityMouse=e.pointerType!=='mouse';}
 if(type!==0&&e.pointerType==='mouse')return; // MouseEvent.detail carries the click count; PointerEvent.detail is zero.
 if(e.pointerType==='touch'||e.pointerType==='pen')wasm.gallery_touch(type,e.clientX-rect.left,e.clientY-rect.top,e.pointerId,e.pointerType==='touch'?1:2);
 else wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,pointerButton(e),type===0?0:1);requestFrame();
});
for(const [event,type] of [['mousedown',1],['mouseup',2]])canvas.addEventListener(event,e=>{
 if(!wasm||suppressCompatibilityMouse)return;
 const rect=canvas.getBoundingClientRect();
 wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,pointerButton(e),Math.max(1,e.detail));requestFrame();
});
let plotMouseDrag=false;
canvas.addEventListener('mousedown',e=>{if(selected.name==='Plot'&&e.button===0&&e.shiftKey)plotMouseDrag=true;});
canvas.addEventListener('pointerup',e=>{if(e.pointerType==='mouse')plotMouseDrag=false;});
canvas.addEventListener('mouseup',()=>{plotMouseDrag=false;});
function cancelPlotMouse(e){if(e.pointerType==='mouse'&&plotMouseDrag){plotMouseDrag=false;wasm?.gallery_event(7,0,0,0,0,0,0,0);requestFrame();}}
canvas.addEventListener('lostpointercapture',cancelPlotMouse);
canvas.addEventListener('pointercancel',cancelPlotMouse);
canvas.addEventListener('wheel',e=>{
 if(!wasm)return;const rect=canvas.getBoundingClientRect(),x=e.clientX-rect.left,y=e.clientY-rect.top;
 const scale=e.deltaMode===1?32:e.deltaMode===2?canvas.clientHeight:1;
 if(selected.name==='Plot'&&e.ctrlKey&&!$('disabled').checked&&!wasm.gallery_modal()){
  const plot=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).find(i=>wasm.gallery_semantic(i,0)===20&&!wasm.gallery_semantic(i,5)&&x>=wasm.gallery_semantic(i,1)&&y>=wasm.gallery_semantic(i,2)&&x<wasm.gallery_semantic(i,1)+wasm.gallery_semantic(i,3)&&y<wasm.gallery_semantic(i,2)+wasm.gallery_semantic(i,4));
  if(plot!==undefined){e.preventDefault();wasm.gallery_plot_wheel(x,y,e.deltaY*scale);requestFrame();return;}
 }

 let dx=e.deltaX*scale,dy=e.deltaY*scale;
 if(e.shiftKey&&dx===0){dx=dy;dy=0;}
 const consumes=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).some(i=>{const f=n=>wasm.gallery_semantic(i,n);if(!f(17)||f(5)||x<f(1)||y<f(2)||x>=f(1)+f(3)||y>=f(2)+f(4))return false;return(dx>0&&f(18)<f(20))||(dx<0&&f(18)>0)||(dy>0&&f(19)<f(21))||(dy<0&&f(19)>0);});
 if(consumes){e.preventDefault();if(selected.example||['Scrollable','Sidebar','VirtualList','DataTable','Tabs'].includes(selected.name))wasm.gallery_scroll_pixels(x,y,-dx,-dy);else wasm.gallery_scroll(x,y,-dx/32,-dy/32);requestFrame();}
},{passive:false});
canvas.addEventListener('pointerleave',()=>{wasm?.gallery_event(0,-100,-100,0,0,0,0,0);requestFrame();});
canvas.addEventListener('contextmenu',e=>e.preventDefault());
canvas.addEventListener('click',()=>{suppressCompatibilityMouse=false;});
canvas.style.touchAction='none';
function cancelPageTouches(){for(const [id,t] of pageTouches)if(!t.owned)wasm?.gallery_touch(7,0,0,id,1);pageTouches.clear();requestFrame();}
window.addEventListener('blur',cancelPageTouches);window.addEventListener('hashchange',cancelPageTouches);
canvas.addEventListener('lostpointercapture',e=>{const t=pageTouches.get(e.pointerId);if(!t)return;pageTouches.delete(e.pointerId);if(!t.owned)wasm?.gallery_touch(7,0,0,e.pointerId,1);requestFrame();});
canvas.addEventListener('pointercancel',e=>{const t=pageTouches.get(e.pointerId);pageTouches.delete(e.pointerId);if(!t?.owned)wasm?.gallery_touch(7,0,0,e.pointerId,e.pointerType==='touch'?1:2);requestFrame();});
canvas.addEventListener('blur',e=>{if(e.relatedTarget===imeTarget)return;wasm?.gallery_event(7,0,0,0,0,0,0,0);wasm?.gallery_event(0,-100,-100,0,0,0,0,0);wasm?.gallery_focus(0xffffffff);requestFrame();});
for(const target of [canvas,imeTarget])for(const [event,type] of [['keydown',3],['keyup',4]])target.addEventListener(event,e=>{
 if(!wasm)return;const key=keyIdentity(e.key);
 if(e.key==='Tab'&&!composing&&!e.isComposing){
  const enabled=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).filter(i=>!wasm.gallery_semantic(i,5)&&wasm.gallery_semantic(i,9)===0&&(interactiveRoles.includes(wasm.gallery_semantic(i,0))||wasm.gallery_semantic(i,33)));
  const focused=enabled.findIndex(i=>wasm.gallery_semantic(i,7));
  const focusedHandlesTab=focused>=0&&wasm.gallery_semantic(enabled[focused],35);
  if(!focusedHandlesTab&&!wasm.gallery_modal()&&(!enabled.length||(e.shiftKey&&focused===0)||(!e.shiftKey&&focused===enabled.length-1&&focused>=0)))return;
 }
 if(key){if(!composing&&!e.isComposing)e.preventDefault();wasm.gallery_keyboard_event(type,key,mods(e),Number(e.repeat),Number(e.isComposing||composing),Number(prefersCharacterInput(e)));}
 if(type===3 && e.key.length===1 && !composing && !e.isComposing && !e.ctrlKey&&!e.metaKey&&!e.altKey && (selected.name==='Input'||selected.name==='Form'||selected.name==='Input Group'||selected.name==='OtpInput'||selected.name==='Editor'||selected.name==='NumberInput'||selected.name==='Combobox'||selected.name==='TimeField'||selected.name==='Toolbar'||selected.name==='Textarea'||selected.name==='Settings'||selected.name==='Command'||selected.name==='Questionnaire'||['Tree','List','Select','Menu','ContextMenu','DropdownButton'].includes(selected.name))){
  e.preventDefault();const bytes=encoder.encode(e.key),pointer=wasm.malloc(bytes.length);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);
 }requestFrame();
});
reducedMotion.addEventListener('change',requestFrame);
systemDark.addEventListener('change',()=>{if(themeChoice==='system')syncTheme();});
$('search').addEventListener('input',navigation);window.addEventListener('hashchange',()=>{if(catalog&&documents)route();});window.addEventListener('popstate',()=>{if(catalog&&documents)route();});
window.addEventListener('keydown',event=>{
 const editing=event.target instanceof HTMLElement&&(event.target.matches('input,textarea,select')||event.target.isContentEditable||event.target.closest('[role="textbox"]'))||(event.target===canvas&&wasm&&prefersCharacterInput(event));
 if(event.key==='/'&&!editing&&!event.ctrlKey&&!event.metaKey&&!event.altKey){event.preventDefault();document.body.classList.add('menu-open');$('catalog').hidden=false;$('docs-nav').hidden=true;$('search').focus();$('search').select();}
 if(event.key==='Enter'&&document.activeElement===$('search')){const first=$('catalog').querySelector('a');if(first){event.preventDefault();location.hash=first.getAttribute('href');$('search').value='';navigation();$('search').blur();}}
 if(event.key==='Escape'&&document.activeElement===$('search')){$('search').value='';navigation();$('search').blur();}
});
async function copyCode(button,code){
 const original=button.textContent;
 try{await navigator.clipboard.writeText(code);button.textContent='Copied';}
 catch{button.textContent='Copy unavailable';}
 setTimeout(()=>button.textContent=original,1800);
}
$('copy-quickstart').addEventListener('click',()=>copyCode($('copy-quickstart'),$('quickstart-code').textContent));
$('copy-snippet').addEventListener('click',()=>copyCode($('copy-snippet'),$('snippet').textContent));
$('theme').addEventListener('change',()=>{const mode=$('theme').value;themeChoice=mode==='3'?'system':mode==='2'?'high-contrast':mode==='1'?darkDefault:lightDefault;syncTheme();});
$('palette-trigger').addEventListener('click',()=>$('palette-menu').hidden?openPalette():closePalette(true));
$('palette-menu').addEventListener('keydown',event=>{
 const options=[...$('palette-menu').querySelectorAll('[role="option"]')];const index=options.indexOf(document.activeElement);
 if(event.key==='Escape'){event.preventDefault();closePalette(true);return;}
 if(event.key==='ArrowDown'||event.key==='ArrowUp'||event.key==='Home'||event.key==='End'){
  event.preventDefault();const next=event.key==='Home'?0:event.key==='End'?options.length-1:(index+(event.key==='ArrowDown'?1:-1)+options.length)%options.length;
  options.forEach(option=>option.tabIndex=-1);options[next].tabIndex=0;options[next].focus();return;
 }
 if(event.key==='Tab')closePalette();
});
document.addEventListener('pointerdown',event=>{if(!event.target.closest('.palette-picker'))closePalette();});
for(const id of ['image-state','image-fit','image-grayscale','image-opacity','attachment-status','attachment-vertical'])$(id).addEventListener('change',requestFrame);
for(const id of ['description-columns','description-size','description-vertical','description-plain','status-right','status-custom','resizable-vertical','resizable-disabled','icon-choice','icon-size','icon-rotate','icon-color'])$(id).addEventListener('change',requestFrame);
for(const id of ['toolbar-size','toolbar-grouped','toolbar-input'])$(id).addEventListener('change',requestFrame);
for(const id of ['title-decoration','title-close','title-fullscreen'])$(id).addEventListener('change',requestFrame);
for(const id of ['motion-duration','motion-stagger','motion-easing','questionnaire-size','questionnaire-numbers','questionnaire-custom','speech-size','speech-unavailable','chart-kind','chart-curve','chart-dots','chart-labels','chart-animate','text-view-format','text-view-scroll','text-view-fade','settings-size','settings-variant','carousel-axis','carousel-loop','command-modal','command-loading','command-filterable','sidebar-mode','sidebar-side','sidebar-collapsed','textarea-rows','textarea-grow','textarea-readonly','textarea-nowrap'])$(id).addEventListener('change',requestFrame);
for(const id of ['control-size','choice-required','choice-invalid','choice-skip-tab','switch-side','switch-color','toggle-variant','toggle-segmented','toggle-selection'])$(id).addEventListener('change',requestFrame);
for(const id of ['design-size','design-radius','design-spacing'])$(id).addEventListener('change',requestFrame);
$('checkbox-state').addEventListener('change',()=>{wasm.gallery_checkbox_state(Number($('checkbox-state').value));requestFrame();});
$('selection-mode').addEventListener('change',requestFrame);
$('menu-kind').addEventListener('change',requestFrame);
$('input-mode').addEventListener('change',requestFrame);
for(const id of ['message-alignment','message-variant','message-inset','message-avatar','message-scrollbar','dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline','dropdown-selected','tabs-mode','tabs-size','tabs-variant','tabs-manual','progress-shape','progress-mode','shimmer-mode','disclosure-mode','disclosure-multiple','notification-anchor','notification-kind','notification-autohide','tag-variant','tag-color','tag-outline','tag-pill','marker-variant','marker-loading','marker-alignment','marker-icon'])$(id).addEventListener('change',requestFrame);
$('otp-groups').addEventListener('change',requestFrame);
$('otp-mask').addEventListener('change',requestFrame);
$('group-mode').addEventListener('change',requestFrame);
$('form-mode').addEventListener('change',requestFrame);
$('table-pin').addEventListener('change',requestFrame);
 $('table-mode').addEventListener('change',requestFrame);
 $('table-selection').addEventListener('change',requestFrame);
$('surface-variant').addEventListener('change',requestFrame);
$('date-locale').addEventListener('change',requestFrame);$('density').addEventListener('change',requestFrame);
$('disabled').addEventListener('change',()=>{cancelScreenColor();requestFrame();});$('menu').onclick=()=>document.body.classList.toggle('menu-open');new ResizeObserver(requestFrame).observe(canvas);
try{
 const [response,data,docs,presets]=await Promise.all([fetch('./gallery.wasm?v=__CUI_WASM_VERSION__'),fetch('./catalog.json',{cache:'no-cache'}).then(r=>r.json()),fetch('./docs.json',{cache:'no-cache'}).then(r=>{if(!r.ok)throw Error('Cannot fetch documentation');return r.json();}),fetch('./themes.json',{cache:'no-cache'}).then(r=>{if(!r.ok)throw Error('Cannot fetch themes');return r.json();})]);if(!response.ok)throw Error('Cannot fetch WebAssembly module');catalog=data;documents=docs;themes=presets;
 const module=await WebAssembly.compile(await response.arrayBuffer());
 const imports={wasi_snapshot_preview1:wasiImports(),host:{measure:(ptr,len,size)=>{ctx.font=canvasFont(size);return ctx.measureText(stringAt(ptr,len)).width;},measure_styled:(ptr,len,size,weight,italic,monospace,familyId)=>{ctx.font=canvasFont(size,weight,!!italic,!!monospace,familyId>>>0);return ctx.measureText(stringAt(ptr,len)).width;},font_metrics:(size,weight,italic,monospace,familyId,pointer)=>{const metrics=canvasFontMetrics(size,weight,!!italic,!!monospace,familyId>>>0);if(!metrics)return 0;new Float32Array(wasm.memory.buffer,pointer,3).set(metrics);return 1;}}};
 const instance=await WebAssembly.instantiate(module,imports);wasm=instance.exports;wasm._initialize();if(!wasm.gallery_init())throw Error('CUI initialization failed');
 let saved;try{saved=localStorage.getItem('cui.theme');}catch{}themeChoice=saved==='system'||saved==='high-contrast'||themes.some(theme=>theme.id===saved)?saved:'system';buildPaletteMenu();syncTheme();
 window.cuiGallery={exports:wasm,stories,catalog,documents,themes,render,get themeChoice(){return themeChoice;}};$('home-story-count').textContent=String(Object.keys(stories).length);route();
}catch(error){fatal(error);}

for(const id of ['effect-angle','effect-blur','effect-opacity','text-family','text-size','text-weight','text-italic'])$(id).addEventListener('change',requestFrame);

$('plot-mode').addEventListener('change',requestFrame);
$('virtual-axis').addEventListener('change',requestFrame);
