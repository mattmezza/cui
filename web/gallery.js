/* SPDX-License-Identifier: MIT */
import {createSecureInputBridge} from "./secure_input.js";
import {createCuiShadowRenderer,cuiShadowWasmRasterizer} from "./modules/cui_shadow.js";
import {createCuiMeshRenderer} from "./modules/cui_mesh.js";
import {paintGradientText} from "./modules/cui_text_paint.js";
import {selectedUsage} from "./modules/cui_usage.js";
import {resizableUsage} from "./modules/cui_resizable_usage.js";
import {stepperUsage} from "./modules/cui_stepper_usage.js";
import {dropdownUsage} from "./modules/cui_dropdown_usage.js";
let shadowRenderer;
const meshRenderer=createCuiMeshRenderer();
let iconCatalog=null,apiIndex=null;
const $ = id => document.getElementById(id);
// Freeze the asset root before shareable paths change the current document URL.
const siteAssetRoot=new URL('.',document.baseURI);
const siteBaseElement=document.querySelector('base')||document.head.appendChild(document.createElement('base'));
siteBaseElement.href=siteAssetRoot.href;
const stepperControls=document.createElement('span');
stepperControls.id='stepper-controls';stepperControls.hidden=true;
function stepperControl(id,label,options,value){
 const wrapper=document.createElement('label');wrapper.textContent=label+' ';
 const input=document.createElement('select');input.id='stepper-'+id;input.setAttribute('aria-label','Stepper '+label);
 for(const [v,text] of options){const option=document.createElement('option');option.value=v;option.textContent=text;input.append(option);}
 input.value=value;input.addEventListener('change',requestFrame);wrapper.append(input);stepperControls.append(wrapper,' ');return input;
}
stepperControl('mode','Rendering',[[0,'Legacy buttons'],[1,'Indicators and connectors']],0);
stepperControl('axis','Orientation',[[1,'Vertical'],[0,'Horizontal']],1);
stepperControl('density','Source density',[[0,'MD (24px)'],[1,'XS (8px, no glyph)'],[2,'SM (18px)'],[3,'LG (32px)']],0);
stepperControl('icons','Custom icons',[[0,'Ordinals'],[1,'RGBA bitmap masks']],0);
stepperControl('design','Design scale',[[0,'Source density'],...['xs','sm','base','md','lg','xl','2xl','3xl','4xl','5xl','6xl','7xl','8xl','9xl'].map((s,i)=>[i+1,s])],0);
$('control-size').after(stepperControls);
function stepperValues(){return ['mode','axis','density','icons','design'].map(k=>Number($('stepper-'+k).value));}
function resetStepperControls(){for(const [k,v] of Object.entries({mode:0,axis:1,density:0,icons:0,design:0}))$('stepper-'+k).value=String(v);}
const popoverControls=document.createElement('span');
popoverControls.id='popover-controls';popoverControls.hidden=true;
const popoverAnchors=['TOP_LEFT','TOP_CENTER','TOP_RIGHT','BOTTOM_LEFT','BOTTOM_CENTER','BOTTOM_RIGHT','LEFT_CENTER','RIGHT_CENTER'];
function popoverControl(id,label,options,value){
 const wrapper=document.createElement('label');wrapper.textContent=label+' ';
 const input=document.createElement(options?'select':'input');input.id='popover-'+id;
 input.setAttribute('aria-label','Popover '+label);
 if(options)for(const [v,text] of options){const option=document.createElement('option');option.value=v;option.textContent=text;input.append(option);}
 else input.type='checkbox';
 if(options)input.value=value;else input.checked=!!value;
 input.addEventListener('change',requestFrame);wrapper.append(input);popoverControls.append(wrapper,' ');return input;
}
popoverControl('mode','Positioning',[[0,'Legacy placement'],[1,'Source anchors (opt in)']],0);
popoverControl('anchor','Popup anchor',popoverAnchors.map((name,i)=>[i,name.toLowerCase().replaceAll('_',' ')]),0);
popoverControl('custom-offset','Custom signed gap',null,false);
popoverControl('offset','Gap',[-24,-8,0,8,24].map(n=>[n,String(n)]),8);
popoverControl('arrow','Arrow',null,true);
popoverControl('rem','Reference rem',[[0,'Default (16px)'],[16,'16px'],[24,'24px'],[32,'32px']],16);
popoverControl('edge','Edge clamp',null,false);
popoverControl('opacity','Opacity',[[1,'100%'],[.5,'50%']],1);
popoverControl('inert','Inert ancestor',null,false);
popoverControl('modal','Modal above card',null,false);
$('surface-variant').after(popoverControls);
function popoverValues(){return [Number($('popover-mode').value),Number($('popover-anchor').value),Number($('popover-custom-offset').checked),Number($('popover-offset').value),Number($('popover-arrow').checked),Number($('popover-rem').value),Number($('popover-edge').checked),Number($('popover-opacity').value),Number($('popover-inert').checked),Number($('popover-modal').checked)];}
function popoverUsage(){
 const [mode,anchor,custom,offset,arrow,rem,edge,opacity,inert,modal]=popoverValues();
 const position=mode?`    cui_popover_position position = {
        .mode = CUI_POPOVER_POSITION_ANCHOR, .anchor = CUI_POPOVER_${popoverAnchors[anchor]},
        .reference_rem = ${rem}, .custom_offset = ${!!custom}, .offset = ${offset}, .arrow = ${!!arrow}};
    /* Reference rem comes from app/host root sizing, never the theme font. */
    cui_begin_box(ui, cui_id_from(ui, "account-stage"),
        (cui_box){.width = width - 48, .height = height - 48,
          .align_x = CUI_ALIGN_${edge?'END':'CENTER'}, .align_y = CUI_ALIGN_${edge?'END':'CENTER'},
          .custom_opacity = true, .opacity = ${Number(opacity).toFixed(1)}f, .inert = ${!!inert}});`:'';
 const begin=mode?'cui_popover_begin(ui, theme, card, cui_string("Alex Morgan"),\n            cui_string("Maintainer"), options, position)':'cui_surface_begin(ui, theme, card, cui_string("Alex Morgan"),\n            cui_string("Maintainer"), options)';
 return `#include <cui/cui_surface.h>
#include <cui/cui_components.h>
/* Retain card and layer across frames. Call init once before drawing. */
void account_init(cui_context *ui, cui_surface_state *card, cui_surface_state *layer) {
    cui_surface_init(card, cui_id_from(ui, "account"), CUI_SURFACE_POPOVER);
    cui_surface_init(layer, cui_id_from(ui, "account-modal"), CUI_SURFACE_DIALOG);
}
${modal&&mode?`/* Call from an application action after opening the card. */
void account_modal_open(cui_context *ui, cui_surface_state *layer) {
    cui_surface_open(ui, layer, 0);
}
`:''}void account_draw(cui_context *ui, const cui_theme *theme,
        cui_surface_state *card, cui_surface_state *layer, float width, float height) {
    (void)layer;
${position}
    cui_id trigger = cui_id_from(ui, "account-trigger");
    if (cui_button(ui, trigger, theme, cui_string("Open account card"), false))
        cui_surface_open(ui, card, trigger);
    cui_surface_options options = {.viewport = {0, 0, width, height},
        .anchor = cui_bounds(ui, trigger), .width = width > 500 ? 340 : width - 48,
        .height = 190, .margin = 12, .side = CUI_POPUP_BELOW,
        .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE};
    if (${begin}) {
        cui_label(ui, theme, cui_string("Profile content"));
        cui_surface_end(ui);
    }
${mode?'    cui_end_box(ui);\n':''}${modal&&mode?`    cui_surface_options modal = {.viewport = {0, 0, width, height},
        .width = width > 500 ? 260 : width - 72, .height = 120, .layer = 200};
    if (cui_surface_begin(ui, theme, layer, cui_string("Layering modal"),
            cui_string("The account card remains below"), modal))
        cui_surface_end(ui);
`:''}}
/* Source anchors clamp without flipping or shrinking. Arrows are decoration;
 * keep the mesh-capable host and state alive through draw-list consumption.
 * Open the card before selecting inert/modal demonstration controls. */`;
}
function componentSlug(name){return name.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'');}
function shareRoute(path,raw,title,description){
 document.body.dataset.route=raw;
 if(!raw.startsWith('docs/'))document.querySelector('link[rel=alternate][type="text/markdown"]')?.remove();
 const address=new URL(path,siteAssetRoot);address.hash=raw;
 if(location.href!==address.href)history.replaceState(null,'',address.href);
 const canonical=new URL(path,document.body.dataset.siteUrl||'https://matteo.merola.co/cui/').href;
 document.querySelector('link[rel=canonical]')?.setAttribute('href',canonical);
 for(const [selector,value] of [['meta[name=description]',description],['meta[property="og:title"]',title],['meta[property="og:description"]',description],['meta[property="og:url"]',canonical],['meta[name="twitter:title"]',title],['meta[name="twitter:description"]',description]])document.querySelector(selector)?.setAttribute('content',value);
}
const questionnaireProgressControl=document.createElement('select');
questionnaireProgressControl.id='questionnaire-progress';questionnaireProgressControl.hidden=true;
questionnaireProgressControl.setAttribute('aria-label','Questionnaire progress presentation');
for(const [value,label] of [['0','Default progress'],['1','Localized progress'],['2','Short visible count'],['3','Decorative progress'],['4','Text metrics']]){
 const option=document.createElement('option');option.value=value;option.textContent=label;questionnaireProgressControl.append(option);
}
$('questionnaire-size').after(questionnaireProgressControl);
questionnaireProgressControl.addEventListener('change',requestFrame);
const questionnairePresentationControl=document.createElement('select');
questionnairePresentationControl.id='questionnaire-presentation';questionnairePresentationControl.hidden=true;
questionnairePresentationControl.setAttribute('aria-label','Questionnaire named text presentation');
for(const [value,label] of [['0','Default text'],['1','Named text parts'],['2','Localized token text'],['3','Refined text metrics'],['4','Decorative error content']]){
 const option=document.createElement('option');option.value=value;option.textContent=label;questionnairePresentationControl.append(option);
}
questionnaireProgressControl.after(questionnairePresentationControl);
questionnairePresentationControl.addEventListener('change',requestFrame);
function questionnairePresentationUsage(mode){
 if(!mode)return null;
 const custom=mode===4?`static void error_child(cui_context *ui, cui_id id,
        const cui_questionnaire_text_view *view, void *user) {
    (void)user;
    cui_begin_box(ui, id, (cui_box){.width = -1, .gap = 4});
    cui_begin_box(ui, 0, (cui_box){.width = 88, .height = 3, .background = view->color});
    cui_end_box(ui);
    cui_text_ex(ui, view->text, view->font_size, view->color, view->line_height, view->style);
    cui_end_box(ui);
}
`:'';
 const policy=mode>=2?'CUI_QUESTIONNAIRE_TEXT_TOKENS':'CUI_QUESTIONNAIRE_TEXT_LEGACY';
 const choiceRefine=mode===3?', .text = {.fields = CUI_QUESTIONNAIRE_TEXT_SIZE | CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT | CUI_QUESTIONNAIRE_TEXT_STYLE, .font_size = 17, .line_height = 26, .style = {.weight = CUI_TEXT_WEIGHT_NORMAL}}':'';
 const refine=mode===3?`    text.text = (cui_questionnaire_text_refinement){
        .fields = CUI_QUESTIONNAIRE_TEXT_SIZE | CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT |
                  CUI_QUESTIONNAIRE_TEXT_STYLE,
        .font_size = 17, .line_height = 26, .style = {.weight = CUI_TEXT_WEIGHT_NORMAL}};`:'';
 const localized=mode>=2?`        text.fields = CUI_QUESTIONNAIRE_TEXT_VISIBLE;
        text.visible = cui_string("Wählen Sie eine Richtung für Ihr nächstes Projekt. Die Beschreibung bleibt auch auf einem schmalen Bildschirm vollständig lesbar.");`:'';
 const error=mode===4?`        text.fields = CUI_QUESTIONNAIRE_TEXT_VISIBLE;
        text.visible = cui_string("Bitte prüfen Sie Ihre Auswahl.");
        text.render_child = error_child;`:'';
 return `#include <cui/cui_questionnaire_presentation.h>
/* Model/editor bytes survive frames; callbacks and stack descriptors are borrowed
 * only during submission. Reserve these IDs for this composition. */
${custom}
typedef struct { const cui_theme *theme; const cui_questionnaire_state *state; } choice_content;
static void named_choice(cui_context *ui, cui_id id,
        const cui_questionnaire_choice_view *view, void *user) {
    const choice_content *content = user;
    const cui_questionnaire_choice *choice = &content->state->items[view->item].choices[view->choice];
    cui_text(ui, choice->label, content->theme->font_size, content->theme->text);
    (void)cui_questionnaire_choice_description_ex(ui, id, content->theme, choice->description,
        (cui_questionnaire_options){.size = view->size, .design_size = view->design_size,
            .density = view->design_size ? content->theme->font_size / 16 : 0},
        (cui_questionnaire_text_options){.policy = ${policy}${choiceRefine}});
}
void draw_named_question(cui_context *ui, const cui_theme *theme,
        cui_questionnaire_state *state, cui_questionnaire_input_state *input) {
    if (!state || !state->initialized || state->current >= state->count) return;
    size_t item = state->current;
    const cui_questionnaire_item *definition = &state->items[item];
    if (definition->choice_count > 3) return; /* This sample reserves three choice IDs. */
    choice_content content = {theme, state};
    cui_questionnaire_labels labels = {.fields = CUI_QUESTIONNAIRE_LABEL_REQUIRED,
        .required = cui_string("Bitte wählen Sie eine Antwort.")};
    cui_questionnaire_options base = {.labels = &labels,
        .design_size = (cui_design_size)${Number($('design-size').value)},
        .render_content = named_choice, .render_user = &content};
    cui_questionnaire_text_options text = {.policy = ${policy}};
${refine}
    if (!cui_questionnaire_begin(ui, 100, theme, state, base)) return;
    if (cui_questionnaire_item_begin(ui, 101, state, item, base)) {
        (void)cui_questionnaire_title_ex(ui, 102, theme, state, item, base, text);
${localized}
        (void)cui_questionnaire_description_ex(ui, 103, theme, state, item, base, text);
        if (cui_questionnaire_choices_begin(ui, 104, state, item, base)) {
            for (size_t i = 0; i < definition->choice_count; ++i)
                (void)cui_questionnaire_choice_control(ui, 200 + i, theme, state, item, i, base);
            cui_questionnaire_part_end(ui);
        }
        if (definition->input && input)
            (void)cui_questionnaire_input_control(ui, 105, theme, state, item, input, NULL, base);
        text.fields = 0;
${error}
        (void)cui_questionnaire_error_ex(ui, 106, theme, state, item, base, text);
        cui_questionnaire_part_end(ui);
    }
    if (cui_questionnaire_actions_begin(ui, 107, state, base)) {
        (void)cui_questionnaire_action_control(ui, 108, theme, state,
            CUI_QUESTIONNAIRE_NEXT, (cui_str){0}, base);
        (void)cui_questionnaire_action_control(ui, 109, theme, state,
            CUI_QUESTIONNAIRE_FINISH, (cui_str){0}, base);
        cui_questionnaire_part_end(ui);
    }
    cui_questionnaire_end(ui);
}`;
}
function questionnaireProgressUsage(mode){
 if(!mode)return null;
 const custom=mode===3?`static void progress_child(cui_context *ui, cui_id id,
        const cui_questionnaire_progress_view *view, void *user) {
    progress_text *storage = user;
    cui_begin_box(ui, id, (cui_box){.row = true, .gap = 8, .align_y = CUI_ALIGN_CENTER});
    cui_text_ex(ui, cui_string(storage->visible), view->font_size,
                view->color, view->line_height, view->style);
    cui_color track = view->color; track.a = 32;
    cui_begin_box(ui, 0, (cui_box){.width = 96, .height = 4, .background = track});
    cui_begin_box(ui, 0, (cui_box){.width = view->total ? 96.f * (float)view->current / (float)view->total : 0,
                                  .height = 4, .background = view->color});
    cui_end_box(ui); cui_end_box(ui); cui_end_box(ui);
}
`:'';
 const refinement=mode===2?`    p.fields |= CUI_QUESTIONNAIRE_PROGRESS_VISIBLE;
    p.visible = (cui_str){storage->visible, (size_t)b};`:mode===3?`    p.render_child = progress_child;
    p.render_user = storage;`:mode===4?`    p.text = (cui_questionnaire_text_refinement){
        .fields = CUI_QUESTIONNAIRE_TEXT_SIZE | CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT |
                  CUI_QUESTIONNAIRE_TEXT_STYLE,
        .font_size = 18, .line_height = 28, .style = {.weight = CUI_TEXT_WEIGHT_NORMAL}};`:'';
 return `#include <cui/cui_questionnaire_progress.h>
#include <stdio.h>
/* Retain storage through completed-frame draw/semantic consumption. */
typedef struct { char label[96], visible[48]; } progress_text;
${custom}
bool draw_progress(cui_context *ui, cui_id id, const cui_theme *theme,
                   const cui_questionnaire_state *state, progress_text *storage) {
    size_t position = cui_questionnaire_position(state);
    size_t current = position == SIZE_MAX ? 0 : position + 1;
    size_t total = cui_questionnaire_total(state);
    int a = snprintf(storage->label, sizeof storage->label, "Frage %zu von %zu", current, total);
    int b = snprintf(storage->visible, sizeof storage->visible, "%zu / %zu", current, total);
    if (a < 0 || (size_t)a >= sizeof storage->label || b < 0 || (size_t)b >= sizeof storage->visible)
        return false;
    cui_questionnaire_progress_options p = {
        .fields = CUI_QUESTIONNAIRE_PROGRESS_LABEL, .label = {storage->label, (size_t)a}};
${refinement}
    return cui_questionnaire_progress_ex(ui, id, theme, state,
                                          (cui_questionnaire_options){0}, p);
}`;
}
const canvas = $('preview'), ctx = canvas.getContext('2d');
const iconLayoutControl=document.createElement('select');iconLayoutControl.id='icon-layout';
iconLayoutControl.hidden=true;iconLayoutControl.setAttribute('aria-label','Icon box styling');
for(const [value,label] of [['0','Default icon box'],['1','Padding 4px'],['2','Padding 24px floor'],['3','Maximum width 8px']]){
 const option=document.createElement('option');option.value=value;option.textContent=label;iconLayoutControl.append(option);
}
$('icon-size').after(iconLayoutControl);iconLayoutControl.addEventListener('change',requestFrame);

for(const [id,label,choices] of [
 ['spinner-size','Spinner size',[['12','Extra small · 12px'],['14','Small · 14px'],['16','Medium · 16px'],['24','Large · 24px'],['64','Custom · 64px']]],
 ['spinner-icon','Spinner icon',[['0','Loader'],['1','Loader circle'],['2','Arrow right']]],
 ['spinner-color','Spinner color',[['0','Theme foreground'],['1','Custom pink'],['2','Transparent']]],
 ['spinner-easing','Spinner easing',[['0','Source ease in out'],['1','Linear']]],
]){
 const control=document.createElement('select');control.id=id;control.hidden=true;control.setAttribute('aria-label',label);
 for(const [value,name] of choices){const option=document.createElement('option');option.value=value;option.textContent=name;control.append(option);}
 if(id==='spinner-size')control.value='16';
 $('text-weight').after(control);control.addEventListener('change',requestFrame);
}

const textPaintControl=document.createElement('select');
textPaintControl.id='text-paint';textPaintControl.setAttribute('aria-label','Typography paint');
textPaintControl.hidden=true;
for(const [value,label] of [['0','Solid text'],['1','Linear gradient text']]){
 const option=document.createElement('option');option.value=value;option.textContent=label;textPaintControl.append(option);
}
$('text-weight').after(textPaintControl);
const gradientSnippet=`cui_text_gradient_stop stops[] = {
    {0, {244, 40, 90, 255}}, {1, {30, 90, 250, 255}},
};
cui_text_paint paint = {.kind = CUI_TEXT_PAINT_LINEAR, .x1 = 1,
    .anchor = CUI_TEXT_ANCHOR_BLOCK, .stops = stops, .stop_count = 2};
/* Inside an active frame; Core copies the stops. Check the return value. */
bool ok = cui_text_paragraph_painted(ui, cui_string("WiiW: Build with CUI"),
    24, 30, (cui_text_style){0}, true, CUI_TEXT_ALIGN_LEFT, paint);`;
function typographyUsage(){
 if(selected?.name==='Theme'&&textPaintControl.value==='1')$('snippet').textContent=gradientSnippet;
 else if(selected?.name==='Theme')$('snippet').textContent=snippets.Theme;
}

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
const stories = {Button:0,Checkbox:1,Switch:2,Toggle:3,Radio:4,Slider:5,Input:6,Label:7,Badge:8,Progress:9,Theme:11,Alert:12,Avatar:13,Bubble:14,Empty:15,GroupBox:16,Kbd:17,Skeleton:18,Spinner:19,Accordion:20,Collapsible:21,Tabs:22,Tooltip:23,HoverCard:24,Editor:25,VirtualList:26,Dock:27,Plot:28,Calendar:29,List:30,Select:31,Menu:32,NumberInput:33,Pagination:34,Rating:35,Stepper:36,Combobox:37,'Focus Trap':38,Clipboard:39,DatePicker:40,TimeField:41,ColorPicker:42,Tree:43,Scrollable:44,DataTable:45,Table:46,Dialog:48,AlertDialog:49,Sheet:50,Popover:51,DropdownButton:52,Form:53,'Input Group':54,OtpInput:55,Tag:56,Marker:57,Notification:58,Shimmer:59,Message:60,MessageScroller:61,Image:62,Attachment:63,DescriptionList:64,StatusBar:65,Resizable:66,Icon:67,Toolbar:68,TitleBar:69,Sidebar:70,Textarea:71,Settings:72,Carousel:73,Command:74,'Root View':75,TextView:76,Chart:77,Speech:78,Questionnaire:79,Video:81,VideoPlayer:82};
const modelStories = new Set(['Focus Trap']);
function dateTimeSnippet(seconds,twelve,clear){
 const names=['EN_US','EN_GB','IT','FR','DE','ES','JA'],locale=Number($('date-locale').value);
 const localeExpression=locale<0?'NULL':`cui_date_locale_builtin(CUI_DATE_LOCALE_${names[locale]})`;
 return `#include "cui/cui_date_time_picker.h"

/* One owner UI context; retain state/value, and submit once per frame. */
void render_date_time(cui_context *ui, const cui_theme *theme,
                      float width, float height, bool disabled) {
    static cui_date_time_picker_state state;
    static cui_date_time_value value = {{2026, 10, 12}, {9, 30, 0}};
    static bool initialized;
    if (!initialized) {
        cui_date_time_picker_init(&state, cui_id_from(ui, "date-time-popup"), value.date);
        initialized = true;
    }
    const cui_date_time_preset presets[] = {
        {.id = cui_id_from(ui, "launch"), .label = cui_string("Launch day"),
         .date = {2026, 11, 5}},
        {.id = cui_id_from(ui, "review"), .label = cui_string("Timed review"),
         .date = {2026, 12, 18}, .has_time = true, .time = {23, 15, 30}},
    };
    cui_date_time_picker(ui, cui_id_from(ui, "date-picker"), theme,
        cui_string("Date and time"), &state, &value,
        (cui_date_time_picker_options){
            .picker = {.calendar = {.first_weekday = 1, .cell_size = 28,
                .locale = ${localeExpression}},
                .viewport = {0, 0, width, height}, .disabled = disabled,
                .width = width > 360 ? 280 : width - 48},
            .calendar = {.presentation = {.month_toggle = true, .neighboring_days = true}},
            .precision = ${seconds?'CUI_DATE_TIME_SECOND':'CUI_DATE_TIME_MINUTE'},
            .twelve_hour = ${twelve}, .default_time = {9, 30, 0},
            .presets = presets, .preset_count = 2,
            .clear_id = cui_id_from(ui, "clear-date"), .clear_label = cui_string("Clear date"),
            .cleanable = ${clear},
            .delete_policy = ${clear?'CUI_DATE_PICKER_DELETE_SCOPE':'CUI_DATE_PICKER_DELETE_OFF'}});
    /* Single civil date/time: no timezone/range. Empty date has year == 0.
       Same-day selection confirms; changed day stays open for time editing.
       Popup requires enough viewport space. Bindings and full source events differ. */
}`;}
function dateActionsSnippet(clear){return `#include "cui/cui_date_picker_actions.h"

/* Retain state/value across frames; call once per frame on the owner thread. */
void render_date_actions(cui_context *ui, const cui_theme *theme,
                         float width, float height, bool disabled) {
    static cui_date_picker_actions_state state;
    static cui_date_picker_value value = {.dates = {.start = {2026, 10, 12}}};
    static bool initialized;
    if (!initialized) {
        cui_date_picker_actions_init(&state, cui_id_from(ui, "date-popup"),
                                      value.dates.start);
        initialized = true;
    }
    const cui_date_picker_preset presets[] = {
        {.id = cui_id_from(ui, "launch"), .label = cui_string("Launch day"),
         .value = {.dates = {.start = {2026, 11, 5}}}},
        {.id = cui_id_from(ui, "review"), .label = cui_string("Review day"),
         .value = {.dates = {.start = {2026, 12, 18}}}},
    };
    cui_date_picker_actions(ui, cui_id_from(ui, "date-picker"), theme,
        cui_string("Date"), &state, &value,
        (cui_date_picker_actions_options){
            .picker = {.calendar = {.first_weekday = 1, .cell_size = 28,
                .locale = cui_date_locale_builtin(CUI_DATE_LOCALE_EN_US)},
                .viewport = {0, 0, width, height}, .disabled = disabled,
                .width = width > 360 ? 280 : width - 48},
            .calendar = {.presentation = {.month_toggle = true,
                                           .neighboring_days = true}},
            .presets = presets, .preset_count = 2,
            .clear_id = cui_id_from(ui, "clear-date"),
            .clear_label = cui_string("Clear date"), .cleanable = ${clear},
            .delete_policy = ${clear?'CUI_DATE_PICKER_DELETE_SCOPE':'CUI_DATE_PICKER_DELETE_OFF'}});
    /* Consume value.dates in application code. Empty dates have year == 0.
       Date-only sample: no time editor. Popup needs enough viewport space.
       For an open modal popup, dismiss before clicking its outside clear button. */
}`;}
const snippets = {
 Button:'cui_button_ex(ui, id, &theme, label,\n    (cui_button_options){\n        .variant = CUI_BUTTON_OUTLINE,\n        .size = CUI_CONTROL_SMALL\n    });',
 Checkbox:'cui_checkbox_ex(ui, id, &theme, label, &state,\n    (cui_checkbox_options){.size = CUI_CONTROL_SMALL,\n                           .required = true});',
 Switch:'cui_switch_ex(ui, id, &theme, label, &enabled,\n    &spring, (cui_switch_options){.now_seconds = now});',
 Toggle:'cui_toggle_group_ex(ui, group_id, &theme,\n    items, selected, 3, options);',
 Radio:'cui_radio_group_item choices[] = {\n    {cui_id_from(ui, "standard"), cui_string("Standard"), false},\n    {cui_id_from(ui, "priority"), cui_string("Priority"), false}};\ncui_radio_group(ui, id, &theme, choices, 2, &selected, &navigation,\n    (cui_radio_group_options){.name = cui_string("Delivery"),\n        .vertical = true, .design_size = CUI_DESIGN_BASE});',
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
 Label:'cui_label_source(ui, &theme, (cui_label_options){\n    .primary = cui_string("Workspace"),\n    .secondary = cui_string("Ready to collaborate"), .has_secondary = true,\n    .highlight = cui_string("workspace"),\n    .semantic_id = cui_id_from(ui, "workspace-label")});',
 Badge:'// Define draw_inbox to compose your button, avatar or other content.\nvoid draw_inbox(cui_context *, void *);\ncui_badge_options badge = cui_badge_defaults();\nbadge.count = 125; // Displays 99+; zero hides the marker.\ncui_badge_ex(ui, id, &theme, badge, draw_inbox, user);',
 Progress:'cui_progress_options options = cui_progress_defaults();\noptions.motion = &motion; // retain zero-initialized state\noptions.now_seconds = now_seconds;\ncui_progress_ex(ui, id, &theme, progress / 100.0f, 300, options);',
 Alert:'cui_alert(ui, id, &theme, (cui_alert_options){\n    .kind = CUI_ALERT_INFO,\n    .title = cui_string("Heads up"),\n    .message = cui_string("Your changes were saved"),\n    .width = 360});',
 Bubble:'cui_begin_bubble(ui, id, &theme, CUI_BUBBLE_FILLED, 240);\ncui_label(ui, &theme, cui_string("Message"));\ncui_end_bubble(ui);',
 Empty:'const cui_empty_slot title = {.text = cui_string("No projects yet")};\nconst cui_empty_slot description = {.text = cui_string("Create your first project.")};\nconst cui_empty_header header = {.title = &title, .description = &description};\ncui_empty_ex(ui, id, &theme, (cui_empty_options){\n    .header = &header, .accessible_name = cui_string("Empty workspace")});\n/* content.children accepts a borrowed callback that submits native controls;\n   see the Empty guide for a complete action-button example. */',
 GroupBox:'cui_begin_group_box(ui, id, &theme, cui_string("Preferences"), 320);\ncui_label(ui, &theme, cui_string("Your settings"));\ncui_end_group_box(ui);',
 Skeleton:'cui_skeleton(ui, id, &theme, 240, 20, 6,\n    cui_loading_phase(now_seconds, 1.2f), false, reduced_motion);',
 Spinner:'/* Retain one zero-initialized motion per spinner between frames. */\ncui_loading_motion motion = {0};\ncui_spinner_options options = cui_spinner_options_default();\noptions.now_seconds = now_seconds;\noptions.reduced_motion = reduced_motion;\ncui_loading_motion_result result;\ndouble next_wake = 0; /* Host schedules this deadline. */\nif (cui_spinner_ex(ui, id, &theme, &motion, options, &result))\n    next_wake = result.next_wake_seconds; /* Same clock domain. */\n(void)next_wake; /* Pass to the host scheduler; zero means idle. */',
 Tooltip:'/* Paint trigger before calling the tooltip. */\ncui_tooltip(ui, id, &theme, &hover, cui_string("Helpful detail"),\n    (cui_hover_options){.trigger = trigger_id, .viewport = viewport});',
 HoverCard:'/* Paint trigger before opening the card. */\nif (cui_begin_hover_card(ui, id, &theme, &hover, cui_string("Profile"),\n    (cui_hover_options){.trigger = trigger_id, .viewport = viewport})) {\n    cui_label(ui, &theme, cui_string("Alex Morgan"));\n    cui_end_hover_card(ui);\n}',
 'Focus Trap':'if (cui_begin_focus_scope(ui, scope_id, true)) {\n    /* Paint focusable children here. */\n    cui_end_focus_scope(ui);\n}',
 Clipboard:'/* Install clipboard_set/get on cui_platform first. */\nif (cui_button(ui, id, &theme, cui_string("Copy"), false))\n    platform.clipboard_set(platform.user, cui_string("Copied text"));',
 Shimmer:'cui_shimmer_text(ui, id, &theme, &shimmer,\n    cui_string("Loading content"), cui_shimmer_options_default());',
 Message:'cui_message_body body = {\n    .slot = {.text = cui_string("Can you review the release notes?")},\n    .typed_bubble = true, .bubble = CUI_BUBBLE_SECONDARY};\ncui_message_options options = cui_message_options_default();\noptions.width = 360;\noptions.max_bubble_width = 280;\noptions.header.text = cui_string("Alex Morgan · 09:30");\noptions.body = &body;\noptions.body_count = 1;\noptions.footer.text = cui_string("Delivered · 09:30");\ncui_message(ui, id, &theme, options);\n/* Supply slot render callbacks for portraits and reaction controls. */',
 MessageScroller:'cui_message_scroller(ui, id, &theme, scroll_state,\n    draw_message_row, user, cui_message_scroller_options_default());',

 Editor:'/* Retain edit, state, query and workspace; call find bar every frame. */\ncui_editor_find_bar(ui, search_id, id, &theme, &state, &search, 640, false);\ncui_editor(ui, id, &theme, cui_string("C code"), &state, cui_editor_default());',
 VirtualList:'/* Retain list and state; paint_row composes each visible item. */\ncui_virtual_list_draw(ui, id, &theme, list, &state,\n    (cui_virtual_widget_options){.scroll = {\n        .width = 640, .height = 320}, .overscan = 1},\n    paint_row, user);',
 Dock:'/* Retain dock and view; panels hold your content callbacks. */\ncui_dock_view_render(ui, id, &theme, dock, view, panels, count,\n    (cui_dock_view_options){.width = 640, .height = 400,\n        .tab_height = 32, .allow_reorder = true});',
 Plot:'/* Build pixels with cui_plot_raster; retain pixels and axis labels. */\ncui_plot_view(ui, id, &theme,\n    &(cui_plot_view_options){.bitmap = bitmap, .axis = &axis,\n        .name = cui_string("Trend")});',
 Calendar:'/* Supply today as a host-local civil date when desired. */\ncui_calendar_ex(ui, id, &theme, &state, &selection, options,\n    (cui_calendar_presentation_options){\n        .month_toggle = true, .neighboring_days = true});',
 DatePicker:'cui_date_picker_ex(ui, id, &theme, label, &state, &selection, options,\n    (cui_date_picker_calendar_options){.presentation = {\n        .month_toggle = true, .neighboring_days = true}});',
 ColorPicker:'cui_color_picker(ui, id, &theme,\n    &state, &color, options);',
 TimeField:'cui_time_field(ui, id, &theme, label,\n    &state, &time, options);',
 Combobox:'cui_combobox(ui, id, &theme, label, query,\n    items, 4, scratch, 4, &selection,\n    &popup, options);',
 NumberInput:'cui_number_input(ui, id, &theme, label,\n    &state, &value, (cui_number_options){\n        .size = CUI_NUMBER_SMALL, .step = 1});',
 Pagination:'cui_pagination(ui, id, &theme,\n    &page, total_pages, true, false);',
 Rating:'cui_rating(ui, id, &theme, label,\n    &rating, options);',
 Stepper:'/* The callback validates and updates your owner model for next submission. */\nvoid on_step_request(void *, const cui_stepper_navigation_request *);\ncui_stepper_navigation(ui, id, &theme, steps, 3,\n    current_step, true, false,\n    (cui_stepper_navigation_handler){on_step_request, user});',
 List:'cui_list(ui, id, &theme, items, 3,\n    &selection, &state, options);',
 Select:'cui_select(ui, id, &theme, label, placeholder,\n    items, 3, &selection, &popup, options);',
 Menu:'cui_menu_tree(ui, id, &theme, label, entries,\n    count, &action, &state, options);',
 DropdownButton:'cui_dropdown_button_result result = cui_dropdown_button(\n    ui, container, action_id, menu_id, &theme,\n    label, menu_name, entries, count, &menu_action,\n    &menu_state, options);',
 ContextMenu:'/* Call after painting the receiving area. */\ncui_context_menu(ui, target, &theme, label,\n    entries, count, &action, &state, options);',
 Accordion:'cui_accordion(ui, id, &theme, items, 3,\n    expanded, options, draw_panel, user);',
 Collapsible:'cui_collapsible(ui, id, &theme, label,\n    &open, false, draw_panel, user);',
 Tabs:'cui_tabs(ui, id, &theme, items, 3,\n    &selection, options, draw_panel, user);',
 Avatar:"/* Existing photo storage/cache remains live through rendering. */\nstatic unsigned char pixels[96 * 96 * 4];\nstatic cui_avatar_image circle;\nif (!circle.storage)\n    cui_avatar_image_init(&circle, pixels, sizeof pixels, 96);\ncui_avatar_ex(ui, id, &theme, cui_string(\"Taylor Reed\"),\n    (cui_avatar_options){.design_size = CUI_DESIGN_MD,\n        .identity_colors = true, .custom_identity_hue = true, .identity_hue = 2,\n        .image = photo, .image_state = &circle});\n/* A valid photo wins; missing pixels show named initials with the palette. */\ncui_avatar_ex(ui, cui_id_from(ui, \"anonymous\"), &theme, cui_string(\"\"),\n    (cui_avatar_options){.design_size = CUI_DESIGN_SM,\n        .anonymous_icon = true, .placeholder_name = cui_string(\"circle-check\")});\n/* For custom SVG pass .placeholder = &retained_icon_state.\n * Keep that state/mask versions alive through rendering; icons are decorative.\n * INHERIT keeps .size behavior; explicit variants and rem scale together,\n * preserving the text/diameter ratio when diameter reaches the 512px bound. */",
 Kbd:'cui_kbd_ex(ui, id, &theme,\n    (cui_kbd_stroke){.modifiers = CUI_MOD_CTRL | CUI_MOD_SHIFT,\n                     .key = CUI_STR("k")},\n    (cui_kbd_options){.outline = true});',
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
if (cui_questionnaire_begin_with_keyboard(ui, root_id, &theme, &state, options,
        &(cui_questionnaire_keyboard_options){.shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF})) {
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
 Video:`/* The host owns decoding, playback and frame lifetime. */
cui_video_surface_options surface = {
    .geometry = {.width = 320, .height = 180, .fit = CUI_IMAGE_CONTAIN},
    .opacity = 1, .description = cui_string("Video surface")
};
if (decoded_frame)
    cui_video_surface(ui, id, &confirmed_view.timeline, &decoded_frame->frame, surface);
/* Decoded RGBA pixels stay live and immutable until draw-list consumption. */`,
 VideoPlayer:`/* Called inside the host's begin_frame/end_frame composition.
   Retain controls across frames. confirmed_view is acknowledged host state;
   decoded_frame is NULL while loading, otherwise matching source_id/generation.
   The host supplies a queue_video_request callback and video_host queue. */
static cui_video_controls_state controls = {0};
cui_video_player_options options = cui_video_player_defaults();
options.controls_options.on_request = queue_video_request;
options.controls_options.user = video_host;
cui_video_player(ui, id, &theme, &controls, confirmed_view,
                 decoded_frame, options);
/* Queue copied typed requests without mutating controls/view during callbacks.
   After ALL draw-list rendering completes, apply supported host requests and
   acknowledge success in the next confirmed_view. Keep decoded RGBA8 pixels
   unchanged until rendering completes; increment bitmap.revision on edits.
   The host decodes/schedules frames and rejects stale source/generation requests. */`,
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
function checkboxPreviewHeight(designScale){
 const density=Number($('density').value)?0.875:1;
 const fonts=[14,12,14,16,16,18,20,24,30,36,48,60,72,96,128];
 const indicators=[16,12,14,16,16,18,20,24,30,36,48,60,72,96,128];
 const font=fonts[designScale]*density,indicator=indicators[designScale]*density;
 const textWidth=Math.max(canvas.clientWidth-48-indicator-8*density,8*font);
 const labelLines=Math.ceil(15*font*.7/textWidth),detailLines=Math.ceil(38*font*.875*.7/textWidth);
 return Math.max(260,Math.ceil((labelLines+detailLines*.875)*font*1.5+indicator*.125+4*density+96));
}
function attachmentPreviewHeight(designScale){
 const density=Number($('density').value)?0.875:1;
 const fonts=[16,12,14,16,16,18,20,24,30,36,48,60,72,96,128];
 const controls=[64,20,24,32,32,40,48,56,64,80,104,128,152,200,264];
 const padding=[12,4,8,10,10,12,16,20,24,28,32,40,48,56,64];
 const gaps=[12,4,4,6,6,8,8,12,12,16,20,24,28,32,40];
 const font=fonts[designScale]*density,pad=padding[designScale]*density;
 const cardWidth=Math.max(260,16.25*font),media=controls[designScale]*density;
 const metadata=1.25*(font+font*0.875)+2;
 const card=$('attachment-vertical').checked
  ?cardWidth-2*pad+metadata+gaps[designScale]*density+2*pad
  :Math.max(media,metadata);
 return Math.max(420,Math.ceil(card+6+8+48));
}
function buttonPreviewHeight(width, controlHeight, scale){
 const columns=scale?(width<600?1:2):3;
 const rows=Math.ceil(12/columns)+3;
 const spacing=Number($('design-spacing').value);
 const gap=spacing<0?16:spacing*4;
 // Preview padding, primary control, counter label, variant rows, and their gaps.
 return Math.max(260,48+(rows+1)*controlHeight+26+(rows+1)*gap+16);
}
function descriptionPreviewHeight(width, scale, density){
 if(!scale)return 360;
 const fonts=[0,12,14,16,16,18,20,24,30,36,48,60,72,96,128];
 const heights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264];
 const lines=[0,16,20,24,24,28,28,32,36,40,48,60,72,96,128];
 const paddings=[0,4,8,10,10,12,16,20,24,28,32,40,48,56,64];
 const font=fonts[scale]*density,px=paddings[scale]*density,py=(heights[scale]-lines[scale])*density/2;
 const available=Math.max(1,width-48),requested=Number($('description-columns').value);
 const columns=Math.max(1,Math.min(requested,Math.floor(available/(font*8+32))));
 const vertical=Boolean($('description-vertical').checked)||font*6+120>available;
 const cell=Math.max(1,available/columns-2*px-2);
 const lineCount=(value,size,space)=>{
  ctx.font=canvasFont(size,400,false,false,0);
  let lines=1,run='';
  for(const word of value.split(' ')){
   const next=run?`${run} ${word}`:word;
   if(run&&ctx.measureText(next).width>space){lines++;run=word}else run=next;
  }
  return lines;
 };
 const item=(label,value)=>{
  const labelSpace=vertical?cell:Math.max(1,Math.min(120,cell/2)-2*px);
  const valueSpace=vertical?cell:Math.max(1,cell-Math.min(120,cell/2)-2*px);
  const labelHeight=lineCount(label,font*.875,labelSpace)*font*.875*1.618;
  const valueHeight=lineCount(value,font,valueSpace)*font*1.618;
  return (vertical?labelHeight+valueHeight:Math.max(labelHeight,valueHeight))+4*py+2;
 };
 const entries=[['Owner','Ada Lovelace'],['Team','Core UI'],['Status','In progress'],['Created','12 October 2026']];
 const row=(a,b)=>columns===1?item(...entries[a])+item(...entries[b]):Math.max(item(...entries[a]),item(...entries[b]));
 return Math.max(360,48+row(0,1)+8+row(2,3)+32);
}
function settingsPreviewHeight(scale,density){
 if(!scale)return 640;
 const fonts=[0,12,14,16,16,18,20,24,30,36,48,60,72,96,128];
 const heights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264];
 const gaps=[0,4,4,6,6,8,8,12,12,16,20,24,28,32,40];
 const font=fonts[scale]*density;
 // Search, two page entries, selected header, group heading, first setting and scroll room.
 return Math.max(640,48+2*heights[scale]*density+12*font+8*gaps[scale]*density);
}
// The decoder is a host service, never an HTML control overlay. Its bounded
// staging copy is completed before CUI composition and Canvas consumption.
const videoHost=(()=>{
 let active=null,serial=0,decodedFrames=0;
 const snapshot=()=>active?{active:true,decodedFrames,session:active.session,generation:active.generation,
  time:active.media.currentTime,duration:active.media.duration,paused:active.media.paused,
  rate:active.media.playbackRate,frames:active.frames,checksum:active.checksum,
  requests:active.requests.slice(),error:active.error,source:active.media.currentSrc,asset:active.asset,
  seeking:active.media.seeking,muted:active.media.muted,volume:active.media.volume,audioDecodedBytes:active.media.webkitAudioDecodedByteCount??null,loop:active.media.loop}: {active:false,decodedFrames};
 const valid=s=>active===s;
 function status(s,message='',failed=false){
  if(!valid(s))return;
  if(message){s.error=message;s.failed=failed;}
  $('video-host-status').textContent=s.error||(s.mode==='Video'?'Bare video surface; the browser host automatically plays this muted sample.':'VideoPlayer with sound; play to hear the quiet sample tone. Skip ±2 seconds, rate 1× / 1.5×.');
 }
 function sync(){
  const s=active;if(!s)return;
  const m=s.media,duration=Number.isFinite(m.duration)?m.duration:0;
  if(!m.seeking&&!s.scrubbing&&!m.paused&&m.currentTime+.5<s.lastTime){s.generation++;s.ticket++;cancelFrame(s);}
  s.lastTime=m.currentTime;
  const ready=m.readyState>=2;
  const state=s.failed?3:!duration?1:m.seeking&&!s.scrubbing?2:ready?0:2;
  wasm.gallery_video_sync(s.session,s.generation,Math.max(0,m.currentTime),duration,m.playbackRate,
   Number(!m.paused&&!m.ended),state,1|8|(m.seekable.length&&m.seekable.end(m.seekable.length-1)>0?2:0)|(document.fullscreenEnabled?4:0),Number(document.fullscreenElement===$('video-player-stage')));
 }
 function cancelFrame(s){
  if(s.frameCallback!==null){if(s.media.cancelVideoFrameCallback)s.media.cancelVideoFrameCallback(s.frameCallback);else cancelAnimationFrame(s.frameCallback);s.frameCallback=null;}
 }
 function capture(s,ticket=s.ticket){
  if(!valid(s)||ticket!==s.ticket||s.media.seeking||s.media.readyState<2)return;
  if(s.media.videoWidth!==320||s.media.videoHeight!==180){status(s,'Sample dimensions must be 320 × 180.',true);requestFrame();return;}
  try{
   sync();s.context.drawImage(s.media,0,0,320,180);
   const bytes=s.context.getImageData(0,0,320,180).data;
   const pointer=wasm.gallery_video_buffer(s.session,s.generation);
   if(!pointer||!valid(s)||ticket!==s.ticket)return;
   new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);
   if(wasm.gallery_video_commit(s.session,s.generation,s.media.currentTime)){
    s.frames++;decodedFrames++;let hash=2166136261;for(let i=0;i<bytes.length;i+=97)hash=Math.imul(hash^bytes[i],16777619);s.checksum=hash>>>0;requestFrame();
   }
  }catch(error){status(s,`Frame transfer failed: ${error.message}`,true);requestFrame();}
 }
 function schedule(s){
  if(!valid(s)||s.media.paused||s.frameCallback!==null||s.media.seeking)return;
  const ticket=s.ticket;
  const frame=()=>{s.frameCallback=null;if(!valid(s)||ticket!==s.ticket)return;capture(s,ticket);schedule(s);};
  s.frameCallback=s.media.requestVideoFrameCallback?s.media.requestVideoFrameCallback(frame):requestAnimationFrame(frame);
 }
 function seek(s,time,terminal){
  cancelFrame(s);s.ticket++;
  if(terminal){s.generation++;s.scrubbing=false;}
  s.media.currentTime=Math.max(0,Math.min(s.media.duration,time));sync();
 }
 function stop(){
  const s=active;if(!s)return;active=null;cancelFrame(s);
  for(const [event,fn] of s.listeners)s.media.removeEventListener(event,fn);
  if(document.fullscreenElement===$('video-player-stage'))void document.exitFullscreen().catch(()=>{});
  s.abort.abort();s.media.pause();s.media.removeAttribute('src');s.media.load();
  if(s.url)URL.revokeObjectURL(s.url);
  wasm.gallery_video_session(0);$('video-host-status').hidden=true;
 }
 function start(mode){
  if(active?.mode===mode)return;
  if(active)stop();
  const media=document.createElement('video'),decoder=document.createElement('canvas');
  decoder.width=320;decoder.height=180;media.muted=mode==='Video';media.volume=1;media.loop=true;media.playsInline=true;media.preload='auto';
  const s={mode,media,decoder,context:decoder.getContext('2d',{willReadFrequently:true}),session:++serial,
   generation:1,ticket:1,frames:0,checksum:0,frameCallback:null,listeners:[],requests:[],error:'',failed:false,scrubbing:false,seekOrigin:0,lastTime:0,operation:0,abort:new AbortController(),url:null,asset:''};
  active=s;wasm.gallery_video_session(s.session);$('video-host-status').hidden=false;status(s);
  const on=(event,fn)=>{s.listeners.push([event,fn]);media.addEventListener(event,fn);};
  for(const event of ['loadedmetadata','loadeddata','canplay','timeupdate','ratechange','waiting','playing','pause','ended'])on(event,()=>{
   if(!valid(s))return;if(event==='pause'||event==='ended')cancelFrame(s);
   sync();if(event==='loadeddata'){capture(s);if(mode==='Video')void media.play().catch(error=>{if(valid(s))status(s,`Autoplay rejected: ${error.message}`);});}schedule(s);requestFrame();
  });
  on('seeked',()=>{if(valid(s)){capture(s);schedule(s);requestFrame();}});
  const sources=[['video/webm; codecs="vp8"','./assets/video/sample.webm'],['video/mp4','./assets/video/sample.mp4']];
  const available=sources.filter(([type])=>media.canPlayType(type));let source=0;
  async function loadSource(){
   const candidate=available[source];
   if(!candidate){status(s,'This browser advertises neither VP8 WebM nor H.264 MP4 decoding.',true);sync();requestFrame();return;}
   try{
    // Fully buffered small fixture: Pages need not support byte ranges. Never
    // accept an unbounded body, even when Content-Length is absent or untrusted.
    const response=await fetch(candidate[1],{signal:s.abort.signal});
    if(!response.ok||!response.body)throw Error(`Sample fetch failed (${response.status})`);
    const limit=512*1024,reader=response.body.getReader(),chunks=[];let size=0;
    try{for(;;){const {done,value}=await reader.read();if(done)break;
     size+=value.byteLength;if(size>limit)throw Error('Encoded sample exceeds 512 KiB limit');chunks.push(value);
    }}catch(error){await reader.cancel();throw error;}finally{reader.releaseLock();}
    if(!valid(s))return;
    if(s.url)URL.revokeObjectURL(s.url);
    s.url=URL.createObjectURL(new Blob(chunks,{type:candidate[0].split(';')[0]}));
    s.asset=candidate[1];media.src=s.url;media.load();
   }catch(error){if(valid(s)&&error.name!=='AbortError'){status(s,error.message,true);sync();requestFrame();}}
  }
  on('error',()=>{if(!valid(s))return;
   if(source+1<available.length){source++;s.ticket++;s.generation++;cancelFrame(s);s.error='';void loadSource();}
   else{status(s,`Sample cannot be decoded (media error ${media.error?.code||'unknown'}).`,true);sync();requestFrame();}
  });
  void loadSource();
  sync();
 }
 function drain(){
  const s=active;if(!s)return;
  const requests=Array.from({length:wasm.gallery_video_request_count()},(_,i)=>Array.from({length:5},(_,f)=>wasm.gallery_video_request(i,f)));
  const overflow=wasm.gallery_video_overflow();
  wasm.gallery_video_drain(); // Never reenter CUI from a request callback.
  if(overflow){
   if(s.scrubbing)seek(s,s.seekOrigin,true);
   else{s.generation++;s.ticket++;cancelFrame(s);}
   status(s,'Video request limit: batch exceeds 32 requests; entire batch rejected.');sync();requestFrame();return;
  }
  const generation=s.generation;
  for(const [kind,phase,source,epoch,value] of requests){
   if(!valid(s)||source!==s.session||epoch!==generation)continue;
   s.requests.push({kind,phase,value});if(s.requests.length>32)s.requests.shift();s.error='';status(s);
   const operation=++s.operation,ticket=s.ticket;
   const current=()=>valid(s)&&operation===s.operation&&ticket===s.ticket;
   try{
    if(kind===0){if(value){s.media.play().catch(error=>{if(current()){status(s,`Play rejected: ${error.message}`);sync();requestFrame();}});}else s.media.pause();}
    else if(kind===1){if(phase===0){s.seekOrigin=s.media.currentTime;s.scrubbing=true;}else seek(s,phase===3?s.seekOrigin:value,phase===2||phase===3);}
    else if(kind===2)seek(s,s.media.currentTime+value,true);
    else if(kind===3){const operation=value?$('video-player-stage').requestFullscreen?.():document.exitFullscreen?.();if(!operation)status(s,'Fullscreen is unavailable.');else operation.catch(error=>{if(current()){status(s,`Fullscreen rejected: ${error.message}`);requestFrame();}});}
    else if(kind===4)s.media.playbackRate=value;
   }catch(error){status(s,`Media request rejected: ${error.message}`);}
  }
  sync();if(requests.length)requestFrame();
 }
 document.addEventListener('fullscreenchange',()=>{if(active){sync();requestFrame();}});
 window.addEventListener('keydown',event=>{
  if(event.key==='Escape'&&document.fullscreenElement===$('video-player-stage')){
   event.preventDefault();event.stopPropagation();
   void document.exitFullscreen().catch(error=>{if(active)status(active,`Fullscreen exit rejected: ${error.message}`);});
  }
 },true);
 return {start,stop,sync,drain,snapshot};
})();

function render(){
 clearTimeout(wakeTimer);
 if(!selected || (!selected.example && !(selected.name in stories)))return;
 const designScale=($('design-size').hidden?0:Number($('design-size').value)),designHeights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264];
 const gaps=[0,4,4,6,6,8,8,12,12,16,20,24,28,32,40];
 const avatarDiameter=selected.name==='Avatar'?wasm.gallery_avatar_metric(designScale,Number($('avatar-rem').value),0):0;
 canvas.parentElement.style.overflowX=selected.name==='Avatar'?'auto':'';
 canvas.style.minWidth=selected.name==='Avatar'?`${avatarDiameter+48}px`:selected.name==='OtpInput'&&designScale>0?`${6*designHeights[designScale]+7*gaps[designScale]+48}px`:'0';
 const width=canvas.clientWidth;let height=selected.name==='Popover'&&$('popover-mode').value==='1'?520:selected.name==='Avatar'?avatarDiameter+390:['Video','VideoPlayer'].includes(selected.name)?(document.fullscreenElement===$('video-player-stage')?innerHeight:Math.min(640,Math.max(160,width-48))*180/320+(selected.name==='VideoPlayer'?(width-48<350?200:144):48)):selected.example?840:selected.name==='Button'?buttonPreviewHeight(width,designScale?designHeights[designScale]*($('density').value==='1'?.875:1):40,designScale):selected.name==='Checkbox'?checkboxPreviewHeight(designScale):selected.name==='Alert'?540:selected.name==='Bubble'?540:selected.name==='Empty'?320:selected.name==='Toolbar'?460:selected.name==='Badge'?400:selected.name==='Editor'?560:selected.name==='Plot'?440:selected.name==='Dock'?520:selected.name==='VirtualList'?360:selected.name==='Message'?500:selected.name==='MessageScroller'?550:selected.name==='Sidebar'?500:selected.name==='Textarea'?360:selected.name==='Carousel'?420:selected.name==='Command'?480:selected.name==='Root View'?520:selected.name==='TextView'?640:selected.name==='Chart'?440:selected.name==='Speech'?400:selected.name==='Questionnaire'?560:selected.name==='Settings'?settingsPreviewHeight(designScale,($('density').value==='1'?.875:1)):selected.name==='Image'?320:selected.name==='Attachment'?attachmentPreviewHeight(designScale):selected.name==='DescriptionList'?descriptionPreviewHeight(width,designScale,($('density').value==='1'?.875:1)):selected.name==='Resizable'?($('resizable-nested').checked?440:360):selected.name==='Theme'?1150:selected.name==='Accordion'?650:selected.name==='Collapsible'?380:selected.name==='Notification'?720:selected.name==='Form'?Math.max(360,designScale>0?designHeights[designScale]*6+180:360):selected.name==='Stepper'?Math.max(520,$('stepper-mode').value==='1'?8*designHeights[Number($('stepper-design').value)]+240:520):selected.name==='Calendar'?560:selected.name==='DatePicker'?(wasm.gallery_control_stats(9)?680:520):selected.name==='ColorPicker'?820:selected.name==='DataTable'?420:selected.name==='Table'?Math.max(260,designScale>0?4*designHeights[designScale]*($('density').value==='1'?.875:1)+72:260):['Menu','ContextMenu'].includes(selected.name)?420:260;const dpr=Math.min(devicePixelRatio||1,3);
 if(designScale>=7&&['DropdownButton','Badge','Tag','Attachment','Progress','Settings','Tabs','DescriptionList','Icon','Toolbar','Questionnaire','Speech'].includes(selected.name))height=Math.max(height,designHeights[designScale]*(selected.name==='Questionnaire'?8:4)+140);
 canvas.style.height=`${height}px`;
 ctx.setTransform(dpr,0,0,dpr,0,0);
 wasm.gallery_scrollbar_options(Number($('scrollbar-policy').value),Number($('scrollbar-width').value),Number($('scrollbar-radius').value),Number($('scrollbar-track').checked));
 wasm.gallery_table_pin(Number($('table-pin').value));
 wasm.gallery_table_mode(Number($('table-mode').value));
 wasm.gallery_table_selection_mode(Number($('table-selection').value));
 wasm.gallery_surface_variant(Number($('surface-variant').value));
 wasm.gallery_popover_options(...popoverValues());
 for(const input of popoverControls.querySelectorAll('select,input'))if(input.id!=='popover-mode')input.disabled=$('popover-mode').value==='0';
 wasm.gallery_button_collection(Number($('button-collection').value));
 wasm.gallery_input_mode(Number($('input-mode').value));
 wasm.gallery_disclosure_options(Number($('disclosure-mode').value),Number($('disclosure-multiple').checked));
 wasm.gallery_image_options(Number($('image-state').value),Number($('image-fit').value),Number($('image-grayscale').checked),Number($('image-opacity').checked));
 wasm.gallery_attachment_options(Number($('attachment-status').value),Number($('attachment-vertical').checked));
 wasm.gallery_description_options(Number($('description-columns').value),Number($('description-vertical').checked),Number($('description-plain').checked),Number($('description-size').value));
 wasm.gallery_status_options(Number($('status-right').checked),Number($('status-custom').checked));
 $('resizable-vertical-control').hidden=selected.name!=='Resizable'||$('resizable-nested').checked;
 wasm.gallery_resizable_nested_options(Number($('resizable-nested').checked));
 wasm.gallery_stepper_options(...stepperValues());
 wasm.gallery_resizable_options(Number($('resizable-vertical').checked),Number($('resizable-disabled').checked));
 wasm.gallery_icon_options(Number($('icon-choice').value),Number($('icon-size').value),Number($('icon-rotate').checked),Number($('icon-color').checked),Number(iconLayoutControl.value));
 wasm.gallery_toolbar_options(Number($('toolbar-size').value),Number($('toolbar-grouped').checked),Number($('toolbar-input').checked));
 wasm.gallery_settings_options(Number($('settings-size').value),Number($('settings-variant').value));
 wasm.gallery_carousel_options(Number($('carousel-axis').value),Number($('carousel-loop').checked));
 wasm.gallery_command_palette_options(Number($('command-modal').checked),Number($('command-loading').checked),Number($('command-filterable').checked));
 wasm.gallery_text_view_motion(Number($('motion-duration').value),Number($('motion-stagger').value),Number($('motion-easing').value));
 wasm.gallery_text_view_options(Number($('text-view-format').value),Number($('text-view-scroll').checked),Number($('text-view-fade').checked));
 wasm.gallery_chart_options(Number($('chart-kind').value),Number($('chart-curve').value),Number($('chart-dots').checked),Number($('chart-labels').checked),Number($('chart-animate').checked));
 wasm.gallery_chart_content(Number($('chart-custom-rows').checked));
 wasm.gallery_chart_appearance(Number($('chart-tooltip-appearance').value));
 wasm.gallery_speech_options(Number($('speech-size').value),Number($('speech-unavailable').checked));
 wasm.gallery_questionnaire_options(Number($('questionnaire-size').value),$('questionnaire-shortcuts').checked?Number($('questionnaire-numbers').checked):-1,Number($('questionnaire-custom').checked));
 wasm.gallery_questionnaire_progress_mode(Number($('questionnaire-progress').value));
 wasm.gallery_questionnaire_presentation_mode(Number(questionnairePresentationControl.value));
 wasm.gallery_sidebar_options(Number($('sidebar-mode').value),Number($('sidebar-side').value),Number($('sidebar-collapsed').checked));
 wasm.gallery_textarea_options(Number($('textarea-rows').value),Number($('textarea-grow').checked),Number($('textarea-readonly').checked),Number($('textarea-nowrap').checked));
 wasm.gallery_title_options(Number($('title-decoration').value),Number($('title-close').checked),Number($('title-fullscreen').checked));
 wasm.gallery_typography_options(Number($('text-family').value),Number($('text-size').value),Number($('text-weight').value),Number($('text-italic').checked),Number(textPaintControl.value));
 wasm.gallery_effects_options(Number($('effect-angle').value),Number($('effect-opacity').value),Number($('effect-blur').value));
 wasm.gallery_plot_mode(Number($('plot-mode').value));
 wasm.gallery_virtual_mode(Number($('virtual-axis').value));
 wasm.gallery_design_options(($('design-size').hidden?0:Number($('design-size').value)),Number($('design-radius').value),Number($('design-spacing').value));
 wasm.gallery_control_options(Number($('control-size').value),Number($('choice-required').checked),Number($('choice-invalid').checked),Number($('choice-skip-tab').checked),Number($('switch-side').checked),Number($('switch-color').checked),Number($('toggle-variant').value),Number($('toggle-segmented').checked),Number($('toggle-selection').value));
 wasm.gallery_avatar_options(Number($('avatar-source').value),Number($('avatar-palette').checked),Number($('avatar-hue').value),Number($('avatar-placeholder').value),Number($('avatar-rem').value));
 wasm.gallery_message_options(Number($('message-alignment').value),Number($('message-variant').value),Number($('message-inset').value),Number($('message-avatar').checked),Number($('message-scrollbar').checked));
 $('dropdown-shadow').disabled=$('dropdown-paint').value==='0'||$('dropdown-variant').value!=='11';
 wasm.gallery_dropdown_metadata_options(...['names','order','skip','toggle'].map(k=>Number($('dropdown-'+k).value)));
 wasm.gallery_dropdown_style_options(Number($('dropdown-paint').value),Number($('dropdown-shadow').checked));
 wasm.gallery_dropdown_options(...['variant','size','halves','action','anchor'].map(k=>Number($('dropdown-'+k).value)),Number($('dropdown-outline').checked),Number($('dropdown-selected').checked));
 wasm.gallery_tabs_options(Number($('tabs-mode').value),Number($('tabs-size').value),Number($('tabs-variant').value),Number($('tabs-manual').checked));
 wasm.gallery_progress_mode(Number($('progress-mode').value));
 wasm.gallery_progress_shape(Number($('progress-shape').value));
 wasm.gallery_spinner_options(Number($('spinner-size').value),Number($('spinner-icon').value),Number($('spinner-color').value),Number($('spinner-easing').value));
 wasm.gallery_shimmer_mode(Number($('shimmer-mode').value));
 wasm.gallery_notification_options(Number($('notification-anchor').value),Number($('notification-kind').value),Number($('notification-autohide').checked));
 wasm.gallery_tag_options(Number($('tag-variant').value),Number($('tag-color').value),Number($('tag-outline').checked),Number($('tag-pill').checked));
 wasm.gallery_marker_options(Number($('marker-variant').value),Number($('marker-loading').value),Number($('marker-alignment').value),Number($('marker-icon').checked));
 wasm.gallery_otp_options(Number($('otp-groups').value),Number($('otp-mask').checked));
 wasm.gallery_group_mode(Number($('group-mode').value));
 wasm.gallery_form_mode(Number($('form-mode').value));
 wasm.gallery_date_locale(Number($('date-locale').value));
 const today=new Date();wasm.gallery_calendar_today(today.getFullYear(),today.getMonth()+1,today.getDate());
 wasm.gallery_density(Number($('density').value));
 wasm.gallery_selection_mode(Number($('selection-mode').value));
 const frameReducedMotion=reducedMotion.matches;
 const now=performance.now();wasm.gallery_time(now/1000,Math.min((now-lastTime)/1000,.1),Number(frameReducedMotion));lastTime=now;
 wasm.gallery_theme_preset(themeChoice==='high-contrast'?-1:selectedPreset());
 if(['Video','VideoPlayer'].includes(selected.name))videoHost.sync();
 if(!wasm.gallery_frame(selected.name==='Menu'&&$('menu-kind').value==='1'?47:selected.example?80:stories[selected.name],effectiveTheme(),Number($('disabled').checked),width,height))throw Error(`CUI layout failed (error ${wasm.gallery_error()})`);
 applyTouchPageFallback();
 const commandCount=wasm.gallery_count();let meshVertices=0;
 if(commandCount>131072)throw Error('CUI mesh command capacity');
 const commands=Array.from({length:commandCount},(_,i)=>{
  const type=wasm.gallery_command(i,0);
  if(type===6){
   const n=wasm.gallery_command(i,71);
   if(!Number.isSafeInteger(n)||n<=0||n%3||n>262144-meshVertices)throw Error('CUI mesh vertex capacity');
   meshVertices+=n;
   return {type:'mesh',mesh:{vertexCount:n,originX:wasm.gallery_command(i,72),originY:wasm.gallery_command(i,73),scale:wasm.gallery_command(i,74),color:Array.from({length:4},(_,j)=>wasm.gallery_command(i,75+j)),vertices:Array.from({length:n*4},(_,j)=>wasm.gallery_command(i,79+j))}};
  }
  return(type===7||type===8)?{type:type===8?'insetShadow':'shadow',values:Array.from({length:16},(_,field)=>wasm.gallery_shadow_value(i,field))}
   :type===3?{type:'clipPush',bounds:[1,2,3,4].map(field=>wasm.gallery_command(i,field))}
   :type===4?{type:'clipPop'}:{type:'other'};
 });
 const prepared=shadowRenderer.prepare(commands,ctx,width,height);
 const meshes=meshRenderer.prepare(commands,ctx,width,height);
 // Canvas dimension assignment also clears pixels: defer it until Core and
 // every owned shadow raster have passed preflight.
 const physicalWidth=Math.round(width*dpr),physicalHeight=Math.round(height*dpr);
 if(canvas.width!==physicalWidth)canvas.width=physicalWidth;
 if(canvas.height!==physicalHeight)canvas.height=physicalHeight;
 ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
 const clips=[];
 for(let i=0;i<wasm.gallery_count();i++){
  const f=n=>wasm.gallery_command(i,n);const type=f(0),x=f(1),y=f(2),w=f(3),h=f(4);
  if(type===3){ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();clips.push([x,y,w,h]);continue;}
  if(type===4){if(clips.length){ctx.restore();clips.pop();}continue;}
  if(type===6||type===7||type===8){
   // Prepared physical shadow/mesh masks own clipping; a fractional browser
   // clip would incorrectly trim covered boundary pixels.
   for(let n=0;n<clips.length;n++)ctx.restore();
   (type===6?meshes:prepared).draw(i,ctx,{unclipped:true});
   for(const bounds of clips){ctx.save();ctx.beginPath();ctx.rect(...bounds);ctx.clip();}
   continue;
  }
  ctx.fillStyle=ctx.strokeStyle=`rgba(${f(5)},${f(6)},${f(7)},${f(8)/255})`;
  if(type===2){const size=f(11),weight=f(22),italic=!!f(23),monospace=!!f(24),familyId=wasm.gallery_command_family(i)>>>0;ctx.font=canvasFont(size,weight,italic,monospace,familyId);const baseline=textBaselineOffset(size,weight,italic,monospace,familyId);ctx.textBaseline=baseline===null?'top':'alphabetic';const text=stringAt(wasm.gallery_command_text(i),wasm.gallery_command_length(i));if(f(25)===1){const paint={x0:f(26),y0:f(27),x1:f(28),y1:f(29),stops:Array.from({length:f(30)},(_,s)=>Array.from({length:5},(_,v)=>f(31+s*5+v)))};paintGradientText(ctx,text,x,y+(baseline??0),paint);}else if(f(25)===0)ctx.fillText(text,x,y+(baseline??0));else throw Error('Unsupported CUI text paint');}
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
 while(clips.length){ctx.restore();clips.pop();}
 syncImeTarget();
 canvas.style.cursor=['default','pointer','text','ew-resize','ns-resize','move','not-allowed','crosshair','grab','grabbing','zoom-in','zoom-out','wait','help'][wasm.gallery_cursor()]||'default';
 syncSemantics();
 if(['Video','VideoPlayer'].includes(selected.name))videoHost.drain();
 if(selected.name==='Button'||selected.name==='Input'){
  const mode=Number($(selected.name==='Button'?'button-collection':'input-mode').value);
  $('snippet').textContent=selectedUsage(selected.name,mode,Number($('design-size').value),$('disabled').checked)||snippets[selected.name];
 }
 if(selected.name==='Resizable')$('snippet').textContent=resizableUsage($('resizable-nested').checked);
 if(selected.name==='Stepper') {
  for(const k of ['density','icons','design'])$('stepper-'+k).parentElement.hidden=$('stepper-mode').value==='0';
  $('snippet').textContent=stepperUsage(...stepperValues(),$('disabled').checked);
 }
 if(selected.name==='DropdownButton')$('snippet').textContent=dropdownUsage(Number($('dropdown-paint').value),$('dropdown-shadow').checked,...['names','order','skip','toggle'].map(k=>Number($('dropdown-'+k).value)));
 if(selected.name==='DatePicker')$('snippet').textContent=wasm.gallery_control_stats(9)?dateTimeSnippet(Boolean(wasm.gallery_control_stats(10)),Boolean(wasm.gallery_control_stats(11)),Boolean(wasm.gallery_control_stats(13))):wasm.gallery_control_stats(7)?dateActionsSnippet(Boolean(wasm.gallery_control_stats(8))):snippets.DatePicker;
 if(selected.name==='Questionnaire')$('snippet').textContent=questionnairePresentationUsage(Number(questionnairePresentationControl.value))||questionnaireProgressUsage(Number($('questionnaire-progress').value))||snippets.Questionnaire;
 if(selected.name==='Popover'){
  if($('popover-modal').checked&&!wasm.gallery_surface_open(0))$('popover-modal').checked=false;
  $('snippet').textContent=popoverUsage();
 }
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
 let hasScrollablePanel=false;
 if(semanticStory!==selected.name){root.replaceChildren();semanticNodes.clear();semanticStory=selected.name;}
 for(let i=0;i<wasm.gallery_semantic_count();i++){
  const f=n=>wasm.gallery_semantic(i,n),role=roles[f(0)];
  if(f(17)&&(f(20)>0||f(21)>0))hasScrollablePanel=true;
  if(!role)continue;
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
 for(const id of ['scrollbar-policy','scrollbar-width','scrollbar-radius','scrollbar-track-control'])$(id).hidden=!hasScrollablePanel;
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
 const results=[...catalog.components,...(catalog.extensions||[])].map(item=>({item,score:fuzzyScore(item.name,query)})).filter(result=>result.score>=0);
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
 const query=$('search').value.trim().toLowerCase();
 if(query){
  const words=query.split(/\s+/);
  const results=documents.filter(doc=>words.every(word=>(doc.title+' '+doc.description+' '+doc.keywords+' '+(doc.symbols||[]).join(' ')).toLowerCase().includes(word))).sort((a,b)=>Number(a.historical)-Number(b.historical));
  for(const doc of results){const link=document.createElement('a');link.href='#docs/'+doc.slug;link.textContent=doc.title+(doc.historical?' (historical)':'');nav.append(link);}
  if(!results.length){const empty=document.createElement('p');empty.textContent='No matching documentation';nav.append(empty);}
  return;
 }
 for(const category of ['Getting started','Guides','Components','Development']){
  const entries=documents.filter(doc=>doc.category===category);if(!entries.length)continue;
  const h=document.createElement('h3');h.textContent=category;nav.append(h);
  for(const doc of entries){const link=document.createElement('a');link.href=`#docs/${doc.slug}`;link.textContent=doc.title;link.classList.toggle('active',doc.slug===active);if(doc.slug===active)link.setAttribute('aria-current','page');nav.append(link);}
 }
}

async function loadComponentApi(component){
 const header=component.public_header?.split('/').pop();
 $('component-header').href=header?'./headers/'+header:'./api-index.json';
 $('component-declarations').textContent='Loading matching header declarations…';
 try{if(!apiIndex){const response=await fetch('./api-index.json');if(!response.ok)throw Error('Cannot load API index');apiIndex=(await response.json()).symbols;}
  if(selected?.name===component.name)$('component-declarations').textContent=apiIndex.filter(symbol=>symbol.header==='./headers/'+header).map(symbol=>symbol.declaration).join('\n\n')||'See the downloadable header for types and declarations.';
 }catch(error){if(selected?.name===component.name)$('component-declarations').textContent=error.message;}
}
async function loadIconCatalog(){
 if(!iconCatalog){try{const response=await fetch('./icons.json');if(!response.ok)throw Error('Icon catalog unavailable');iconCatalog=(await response.json()).icons;}catch(error){$('icon-catalog-count').textContent=error.message;return;}}
 renderIconCatalog();
}
function renderIconCatalog(){
 if(!iconCatalog)return;
 const query=$('icon-catalog-search').value.trim().toLowerCase(),embedded=$('icon-catalog-embedded').checked;
 const matches=iconCatalog.filter(icon=>(!embedded||icon.embedded)&&icon.name.includes(query));
 $('icon-catalog-count').textContent=`${matches.length} matching icons; showing ${Math.min(60,matches.length)}. Narrow the search for more.`;
 const results=$('icon-catalog-results');results.replaceChildren();
 for(const icon of matches.slice(0,60)){
  const row=document.createElement('div');row.className='icon-catalog-row';
  const name=document.createElement('code');name.textContent=icon.name;
  const status=document.createElement('span');status.textContent=icon.embedded?'Embedded by default':'Requires loader or full-assets build';
  const copy=document.createElement('button');copy.type='button';copy.textContent='Copy C';copy.setAttribute('aria-label',`Copy ${icon.name} C example`);
  copy.addEventListener('click',()=>copyCode(copy,`/* Active frame; stable nonzero id and theme supplied by your app.${icon.embedded?'':' Requires a host loader or CUI_ICON_ALL_ASSETS=1.'} */\ncui_icon_error error = cui_icon_named(ui, id, &theme, cui_string("${icon.name}"),\n    (cui_icon_options){0});\nif (error != CUI_ICON_OK) { /* Report error or render your fallback. */ }`));
  row.append(name,status,copy);results.append(row);
 }
}
$('icon-catalog-search').addEventListener('input',renderIconCatalog);
$('icon-catalog-embedded').addEventListener('change',renderIconCatalog);

function route(){
 if(!['Video','VideoPlayer'].includes(location.hash.slice(1)||document.body.dataset.route))videoHost.stop();
 const raw=location.hash.slice(1)||document.body.dataset.route||'home';
 if(!raw||raw==='home'){
  stepperControls.hidden=true;resetStepperControls();selected=null;$('home-page').hidden=false;$('docs-page').hidden=true;$('gallery-page').hidden=true;$('catalog').hidden=false;$('docs-nav').hidden=true;
  document.title='CUI · Portable C11 UI';shareRoute('', 'home',document.title,'Build portable C11 interfaces with CUI: themed components, explicit state, composable behavior, live WebAssembly examples and browsable documentation.');navigation();document.body.classList.remove('menu-open');return;
 }
 if(raw.startsWith('docs/')){
  const parts=raw.slice(5).split('/');let slug,anchor;
  try{slug=decodeURIComponent(parts[0]);anchor=decodeURIComponent(parts.slice(1).join('/'));}catch{slug='getting-started';anchor='';}
  const doc=documents.find(entry=>entry.slug===slug)||documents.find(entry=>entry.slug==='getting-started');
  stepperControls.hidden=true;resetStepperControls();selected=null;$('home-page').hidden=true;$('docs-page').hidden=false;$('gallery-page').hidden=true;$('catalog').hidden=true;$('docs-nav').hidden=false;
  $('docs-title').textContent=doc.title;$('docs-content').innerHTML=doc.html;$('docs-source').href=doc.source;
  $('docs-markdown').href=doc.markdown||'./llms.txt';
  $('docs-historical').hidden=!doc.historical;
  let alternate=document.querySelector('link[rel=alternate][type="text/markdown"]');
  if(!alternate){alternate=document.createElement('link');alternate.rel='alternate';alternate.type='text/markdown';document.head.append(alternate);}
  alternate.href=doc.markdown||'./llms.txt';
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
 let name='Button';try{name=decodeURIComponent(location.hash.slice(1)||document.body.dataset.route)||'Button';}catch{/* Malformed percent escapes select the default component. */}selected=name==='examples/northstar'?exampleDescriptor:[...catalog.components,...(catalog.extensions||[])].find(c=>c.name===name)||catalog.components.find(c=>c.name==='Button');
 if(['Video','VideoPlayer'].includes(selected.name))videoHost.start(selected.name);else videoHost.stop();
 document.title=`${selected.name} · CUI`;shareRoute(selected.example?'examples/northstar/':`components/${componentSlug(selected.name)}/`,selected.example?'examples/northstar':encodeURIComponent(selected.name),document.title,`Explore CUI ${selected.name}: its C11 API, interactive WebAssembly example, application-owned state and implementation scope. Complete component coverage remains in progress.`); $('name').textContent=selected.name; $('status').textContent=selected.status.replaceAll('_',' ');
 const live=selected.example||selected.name in stories;
 window.cuiGallery.renderedComponent=live?'':selected.name;
 if(modelStories.has(selected.name))$('status').textContent+=' / base model'; $('preview-card').hidden=!live; $('pending').hidden=live;
 $('description').textContent=live?`Explore ${selected.name.toLowerCase()} through the same CUI core, base behavior and theme used by native applications.`:`${selected.name} is part of the 78-family component catalog.`;
 if(modelStories.has(selected.name))$('description').textContent='Interactive demonstration of the implemented base model. The complete styled component remains pending.';
 $('pending-message').textContent=selected.status==='not_started'?'Implementation and browser story are pending. This entry stays visible to preserve the complete component scope.':'Repository implementation exists for part of this entry. A browser story is pending; see implementation scope and source for details.';
 $('snippet').textContent=(selected.example?'/* Retain app across frames. Include examples/composed_apps/app.h. */\ncomposed_app *app = composed_app_create(on_app_event, user);\n/* Between frames, before cui_begin_frame: */\ncomposed_app_set_theme(app, &theme);\n/* Between cui_begin_frame and cui_end_frame: */\ncomposed_app_scene(ui, app, width, height);\n/* At shutdown, after rendering the last draw list: */\ncomposed_app_destroy(app);':snippets[selected.name])||(live?`/* See the public ${selected.public_header || 'component'} API. */`:'/* No complete styled browser example yet. */');
 $('source').href=`https://github.com/mattmezza/cui/${selected.public_header?'blob/main/'+selected.public_header:'blob/main/docs/gpui-parity.json'}`;

 stepperControls.hidden=selected.name!=='Stepper';resetStepperControls();
 for(const id of ['spinner-size','spinner-icon','spinner-color','spinner-easing'])$(id).hidden=selected.name!=='Spinner';
 $('icon-catalog-panel').hidden=selected.name!=='Icon';if(selected.name==='Icon')loadIconCatalog();
 loadComponentApi(selected);
 const usage=selected.usage;
 $('component-composition').textContent=usage?.composition||'Immediate frame composition';
 $('component-state').textContent=usage?.state_ownership||'Retain application values between frames.';
 $('component-events').textContent=usage?.interaction||'Use the linked guide for the example callbacks.';
 $('component-usage-note').textContent=usage?.usage||'';
 $('component-guides').replaceChildren();for(const path of new Set([...(usage?.docs||[]),...(selected.name==='Theme'?['docs/TEXT_GRADIENT.md']:selected.name==='Spinner'?['docs/SPINNER.md']:[])])){const doc=documents.find(d=>d.source.endsWith('/'+path));if(doc){const a=document.createElement('a');a.href='#docs/'+doc.slug;a.textContent=doc.title;$('component-guides').append(a,document.createTextNode(' '));}}

 $('features').replaceChildren();for(const feature of selected.features){const row=document.createElement('tr'),name=document.createElement('td'),status=document.createElement('td');name.textContent=feature.name;status.textContent=feature.status.replaceAll('_',' ');status.className=feature.status;row.append(name,status);$('features').append(row);}
 $('limitations').replaceChildren();for(const message of [...selected.limitations,...(live?['Canvas text uses the browser system font; native FreeType / HarfBuzz rendering is separate.','Browser semantics are experimental; native IME candidate UI and a complete platform accessibility bridge vary by host.']:[])]){const li=document.createElement('li');li.textContent=message;$('limitations').append(li);}
 $('table-pin').hidden=selected.name!=='DataTable';$('table-pin').value='0';
 $('table-mode').hidden=selected.name!=='DataTable';$('table-mode').value='0';
 $('table-selection').hidden=selected.name!=='DataTable';$('table-selection').value='2';
 $('surface-variant').hidden=selected.name!=='Sheet';$('surface-variant').value='0';
 popoverControls.hidden=selected.name!=='Popover';
 for(const [id,value] of [['mode','0'],['anchor','0'],['offset','8'],['rem','16'],['opacity','1']])$('popover-'+id).value=value;
 for(const id of ['custom-offset','edge','inert','modal'])$('popover-'+id).checked=false;
 $('popover-arrow').checked=true;
 $('disclosure-mode').hidden=!['Accordion','Collapsible'].includes(selected.name);$('disclosure-mode').value='0';
 $('disclosure-multiple-control').hidden=selected.name!=='Accordion';$('disclosure-multiple').checked=false;
 for(const id of ['avatar-source','avatar-palette-control','avatar-hue','avatar-placeholder','avatar-rem'])$(id).hidden=selected.name!=='Avatar';
 $('avatar-source').value='1';$('avatar-palette').checked=true;$('avatar-hue').value='-1';$('avatar-placeholder').value='0';$('avatar-rem').value='16';
 for(const id of ['message-alignment','message-variant','message-inset','message-avatar-control'])$(id).hidden=selected.name!=='Message';
 $('checkbox-state').hidden=selected.name!=='Checkbox';
 for(const id of ['settings-size','settings-variant'])$(id).hidden=selected.name!=='Settings';
 for(const id of ['carousel-axis','carousel-loop-control'])$(id).hidden=selected.name!=='Carousel';
 for(const id of ['speech-size','speech-unavailable-control'])$(id).hidden=selected.name!=='Speech';
 for(const id of ['questionnaire-presentation','questionnaire-progress','questionnaire-size','questionnaire-shortcuts-control','questionnaire-numbers-control','questionnaire-custom-control'])$(id).hidden=selected.name!=='Questionnaire';
 questionnairePresentationControl.value='0';$('questionnaire-progress').value='0';$('questionnaire-size').value='0';$('questionnaire-shortcuts').checked=true;$('questionnaire-numbers').checked=false;$('questionnaire-custom').checked=false;
 $('speech-size').value='0';$('speech-unavailable').checked=false;
 for(const id of ['chart-kind','chart-curve','chart-dots-control','chart-labels-control','chart-animate-control','chart-custom-rows-control','chart-tooltip-appearance'])$(id).hidden=selected.name!=='Chart';
 $('chart-kind').value='0';$('chart-curve').value='0';$('chart-dots').checked=false;$('chart-labels').checked=false;$('chart-animate').checked=false;$('chart-custom-rows').checked=false;$('chart-tooltip-appearance').value='0';
 for(const id of ['text-view-format','text-view-scroll-control','text-view-fade-control','motion-duration','motion-stagger','motion-easing'])$(id).hidden=selected.name!=='TextView';
 $('motion-duration').value='0.35';$('motion-stagger').value='0';$('motion-easing').value='0';$('text-view-format').value='0';$('text-view-scroll').checked=false;$('text-view-fade').checked=false;
 for(const id of ['command-modal-control','command-loading-control','command-filterable-control'])$(id).hidden=selected.name!=='Command';
 $('control-size').hidden=!['Button','Checkbox','Radio','Switch','Toggle','Badge','Progress','Form','Input Group','OtpInput','Tag','Attachment','Select','Combobox','Textarea','Input','NumberInput'].includes(selected.name);
 $('control-size').value='0';
 $('control-size').disabled=false;
 $('plot-mode').hidden=selected.name!=='Plot';$('plot-mode').value='0';
 $('virtual-axis').hidden=selected.name!=='VirtualList';$('virtual-axis').value='0';
 for(const id of ['effect-angle','effect-blur','effect-opacity','text-family','text-size','text-weight','text-italic-control','text-paint'])$(id).hidden=selected.name!=='Theme';textPaintControl.value='0';$('effect-angle').value='35';$('effect-blur').value='0';$('effect-opacity').value='1';
 $('design-size').hidden=!['Button','Checkbox','Radio','Switch','Toggle','Input','List','Select','Combobox','Menu','Textarea','NumberInput','Form','Input Group','OtpInput','DropdownButton','Badge','Tag','Attachment','Progress','Settings','Tabs','DescriptionList','Icon','Toolbar','Questionnaire','Speech','Table','Avatar'].includes(selected.name);$('design-size').value='3';$('control-size').hidden=$('control-size').hidden||!$('design-size').hidden;
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
 for(const id of ['resizable-nested-control','resizable-vertical-control','resizable-disabled-control'])$(id).hidden=selected.name!=='Resizable';
 $('date-locale').hidden=!['Calendar','DatePicker'].includes(selected.name);$('date-locale').value='-1';
 $('resizable-nested').checked=false;$('resizable-vertical').checked=false;$('resizable-disabled').checked=false;
 for(const id of ['icon-choice','icon-size','icon-rotate-control','icon-color-control','icon-layout'])$(id).hidden=selected.name!=='Icon';
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
 for(const id of ['dropdown-names','dropdown-order','dropdown-skip','dropdown-toggle','dropdown-paint','dropdown-shadow-control','dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline-control','dropdown-selected-control'])$(id).hidden=selected.name!=='DropdownButton';
 for(const k of ['names','order','skip','toggle'])$('dropdown-'+k).value='0';
 $('dropdown-paint').value='1';$('dropdown-shadow').checked=false;
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
 $('button-collection').hidden=selected.name!=='Button';$('button-collection').value='0';
 $('input-mode').hidden=selected.name!=='Input';$('input-mode').value='0';
 $('menu-kind').hidden=selected.name!=='Menu';$('menu-kind').value='0';
 $('selection-mode').hidden=!['List','Select','Combobox','DataTable'].includes(selected.name);$('selection-mode').value='0';
 $('disabled').checked=false;document.body.classList.remove('menu-open');navigation();requestFrame();
}
let imeTarget=null, composing=false;
const boundImeTargets=new WeakSet();
function withUtf8(bytes, callback){
 const pointer=bytes.length?wasm.malloc(bytes.length):0;
 if(bytes.length&&!pointer)throw Error('CUI input allocation failed');
 try {if(bytes.length)new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);callback(pointer,bytes.length);}
 finally {if(pointer)wasm.free(pointer);}
}
const secureInput=createSecureInputBridge({parent:document.body,id:'cui-ime-input',
 text:bytes=>withUtf8(bytes,(pointer,length)=>wasm.gallery_text(pointer,length)),
 composition:(kind,bytes,anchor,cursor,owner)=>{
  composing=kind==='start'||kind==='update';
  const [high,low]=owner.split(':').map(Number);
  withUtf8(bytes,(pointer,length)=>wasm.gallery_composition_bytes(
   {start:0,update:1,commit:2,cancel:3}[kind],pointer,length,anchor,cursor,low,high));
 },
 key:event=>{handleCanvasKey(event,3);return event.defaultPrevented;},
 focusLost:()=>{composing=false;wasm?.gallery_focus(0xffffffff);},changed:requestFrame});
function syncImeTarget(){
 if(!wasm?.gallery_text_config)return;
 const active=document.activeElement===canvas||document.activeElement===secureInput.element;
 if(!active||!wasm.gallery_text_config(0)){secureInput.reset();imeTarget=null;composing=false;return;}
 const f=n=>wasm.gallery_text_config(n)>>>0;
 const rect=canvas.getBoundingClientRect();
 secureInput.sync({owner:`${f(2)}:${f(1)}`,generation:`${f(4)}:${f(3)}`,
  type:f(5),purpose:f(6),multiline:!!f(7),label:'Canvas text input'},
  {x:rect.left+Math.max(0,wasm.gallery_ime_rect(0)),y:rect.top+Math.max(0,wasm.gallery_ime_rect(1)),h:wasm.gallery_ime_rect(3)});
 imeTarget=secureInput.element;
 if(imeTarget&&!boundImeTargets.has(imeTarget)){
  boundImeTargets.add(imeTarget);imeTarget.tabIndex=-1;imeTarget.style.pointerEvents='none';
  imeTarget.addEventListener('compositionstart',()=>{composing=true;});
  imeTarget.addEventListener('compositionend',()=>{composing=false;});
  imeTarget.addEventListener('keyup',event=>handleCanvasKey(event,4));
 }
}
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

let hoverPointerCapture=null,pointerFocusSuspended=false;
function pointerFocusLoss(){
 hoverPointerCapture=null;plotMouseDrag=false;
 if(!pointerFocusSuspended){wasm?.gallery_event(7,0,0,0,0,0,0,0);pointerFocusSuspended=true;}
 pointerPresenceLeave();
}
canvas.addEventListener('focus',()=>{pointerFocusSuspended=false;});
window.addEventListener('focus',()=>{pointerFocusSuspended=false;});
function pointerPresenceLeave(){wasm?.gallery_event(15,0,0,0,0,0,0,0);requestFrame();}
function pointerOutsideCanvas(e,rect){return e.clientX<rect.left||e.clientY<rect.top||e.clientX>=rect.right||e.clientY>=rect.bottom;}
canvas.addEventListener('pointerenter',e=>{
 if(!wasm||e.pointerType==='touch')return;pointerFocusSuspended=false;const rect=canvas.getBoundingClientRect();
 wasm.gallery_event(14,0,0,0,0,0,0,0);
 wasm.gallery_event(0,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,0,0);requestFrame();
});
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
 if(type===1){pointerFocusSuspended=false;if(e.pointerType!=='touch')hoverPointerCapture=e.pointerId;canvas.focus({preventScroll:true});canvas.setPointerCapture(e.pointerId);suppressCompatibilityMouse=e.pointerType!=='mouse';}
 if(type===2&&hoverPointerCapture===e.pointerId)hoverPointerCapture=null;
 if(type!==0&&e.pointerType==='mouse')return; // MouseEvent.detail carries the click count; PointerEvent.detail is zero.
 if(e.pointerType==='touch'||e.pointerType==='pen')wasm.gallery_touch(type,e.clientX-rect.left,e.clientY-rect.top,e.pointerId,e.pointerType==='touch'?1:2);
 else wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,pointerButton(e),type===0?0:1);
 if(e.pointerType!=='touch'&&pointerOutsideCanvas(e,rect))pointerPresenceLeave();requestFrame();
});
for(const [event,type] of [['mousedown',1],['mouseup',2]])canvas.addEventListener(event,e=>{
 if(!wasm)return;
 // A touch tap may synthesize mouse events after the finalized text proxy has
 // taken focus. Suppress their browser default as well as duplicate CUI input:
 // the default mousedown would focus canvas again and clear the text owner.
 if(suppressCompatibilityMouse){e.preventDefault();return;}
 const rect=canvas.getBoundingClientRect();
 wasm.gallery_event(type,e.clientX-rect.left,e.clientY-rect.top,0,mods(e),0,pointerButton(e),Math.max(1,e.detail));
 if(pointerOutsideCanvas(e,rect))pointerPresenceLeave();requestFrame();
});
let plotMouseDrag=false;
canvas.addEventListener('mousedown',e=>{if(selected.name==='Plot'&&e.button===0&&e.shiftKey)plotMouseDrag=true;});
canvas.addEventListener('pointerup',e=>{if(e.pointerType==='mouse')plotMouseDrag=false;});
canvas.addEventListener('mouseup',()=>{plotMouseDrag=false;});
function cancelPlotMouse(e){if(e.pointerType!=='touch'&&(plotMouseDrag||hoverPointerCapture===e.pointerId)){plotMouseDrag=false;hoverPointerCapture=null;wasm?.gallery_event(7,0,0,0,0,0,0,0);requestFrame();}}
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
canvas.addEventListener('pointerleave',e=>{if(e.pointerType!=='touch')pointerPresenceLeave();});
canvas.addEventListener('contextmenu',e=>e.preventDefault());
canvas.addEventListener('click',()=>{suppressCompatibilityMouse=false;});
canvas.style.touchAction='none';
function cancelPageTouches(){for(const [id,t] of pageTouches)if(!t.owned)wasm?.gallery_touch(7,0,0,id,1);pageTouches.clear();requestFrame();}
window.addEventListener('blur',()=>{cancelPageTouches();pointerFocusLoss();secureInput.reset();imeTarget=null;composing=false;wasm?.gallery_focus(0xffffffff);requestFrame();});window.addEventListener('hashchange',cancelPageTouches);
canvas.addEventListener('lostpointercapture',e=>{const t=pageTouches.get(e.pointerId);if(!t)return;pageTouches.delete(e.pointerId);if(!t.owned)wasm?.gallery_touch(7,0,0,e.pointerId,1);requestFrame();});
canvas.addEventListener('pointercancel',e=>{const t=pageTouches.get(e.pointerId);pageTouches.delete(e.pointerId);if(!t?.owned)wasm?.gallery_touch(7,0,0,e.pointerId,e.pointerType==='touch'?1:2);requestFrame();});
canvas.addEventListener('blur',e=>{if(secureInput.element&&e.relatedTarget===secureInput.element)return;pointerFocusLoss();wasm?.gallery_focus(0xffffffff);requestFrame();});
function handleCanvasKey(e,type){
 if(!wasm)return;const key=keyIdentity(e.key);
 if(e.key==='Tab'&&!composing&&!e.isComposing){
  const enabled=Array.from({length:wasm.gallery_semantic_count()},(_,i)=>i).filter(i=>!wasm.gallery_semantic(i,5)&&!wasm.gallery_semantic(i,37)&&(wasm.gallery_semantic(i,36)||wasm.gallery_semantic(i,9)>=0)&&(interactiveRoles.includes(wasm.gallery_semantic(i,0))||wasm.gallery_semantic(i,33))).sort((a,b)=>wasm.gallery_tab_compare(a,b));
  const focused=enabled.findIndex(i=>wasm.gallery_semantic(i,7));
  const focusedHandlesTab=focused>=0&&wasm.gallery_semantic(enabled[focused],35);
  if(!focusedHandlesTab&&!wasm.gallery_modal()&&(!enabled.length||(e.shiftKey&&focused===0)||(!e.shiftKey&&focused===enabled.length-1&&focused>=0)))return;
 }
 if(key){if(!composing&&!e.isComposing)e.preventDefault();wasm.gallery_keyboard_event(type,key,mods(e),Number(e.repeat),Number(e.isComposing||composing),Number(prefersCharacterInput(e)));}
 if(type===3 && e.key.length===1 && !composing && !e.isComposing && !e.ctrlKey&&!e.metaKey&&!e.altKey && (prefersCharacterInput(e)||selected.name==='Input'||selected.name==='Form'||selected.name==='Input Group'||selected.name==='OtpInput'||selected.name==='Editor'||selected.name==='NumberInput'||selected.name==='Combobox'||selected.name==='TimeField'||selected.name==='Toolbar'||selected.name==='Textarea'||selected.name==='Settings'||selected.name==='Command'||selected.name==='Questionnaire'||['Tree','List','Select','Menu','ContextMenu','DropdownButton'].includes(selected.name))){
  e.preventDefault();const bytes=encoder.encode(e.key),pointer=wasm.malloc(bytes.length);new Uint8Array(wasm.memory.buffer,pointer,bytes.length).set(bytes);wasm.gallery_text(pointer,bytes.length);wasm.free(pointer);
 }requestFrame();
}
for(const [event,type] of [['keydown',3],['keyup',4]])canvas.addEventListener(event,e=>handleCanvasKey(e,type));
reducedMotion.addEventListener('change',requestFrame);
systemDark.addEventListener('change',()=>{if(themeChoice==='system')syncTheme();});
$('search').addEventListener('input',()=>{$('docs-page').hidden?navigation():docsNavigation();});window.addEventListener('hashchange',()=>{if(catalog&&documents)route();});window.addEventListener('popstate',()=>{if(catalog&&documents)route();});
window.addEventListener('keydown',event=>{
 const editing=event.target instanceof HTMLElement&&(event.target.matches('input,textarea,select')||event.target.isContentEditable||event.target.closest('[role="textbox"]'))||(event.target===canvas&&wasm&&prefersCharacterInput(event));
 if(event.key==='/'&&!editing&&!event.ctrlKey&&!event.metaKey&&!event.altKey){event.preventDefault();document.body.classList.add('menu-open');$('search').focus();$('search').select();}
 if(event.key==='Enter'&&document.activeElement===$('search')){const first=($('docs-page').hidden?$('catalog'):$('docs-nav')).querySelector('a');if(first){event.preventDefault();location.hash=first.getAttribute('href');$('search').value='';$('docs-page').hidden?navigation():docsNavigation();$('search').blur();}}
 if(event.key==='Escape'&&document.activeElement===$('search')){$('search').value='';$('docs-page').hidden?navigation():docsNavigation();$('search').blur();}
});
async function copyCode(button,code){
 const original=button.textContent;
 try{await navigator.clipboard.writeText(code);button.textContent='Copied';}
 catch{button.textContent='Copy unavailable';}
 setTimeout(()=>button.textContent=original,1800);
}
$('copy-quickstart').addEventListener('click',()=>copyCode($('copy-quickstart'),$('quickstart-code').textContent));
textPaintControl.addEventListener('change',()=>{typographyUsage();requestFrame();});
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
for(const id of ['description-columns','description-size','description-vertical','description-plain','status-right','status-custom','resizable-nested','resizable-vertical','resizable-disabled','icon-choice','icon-size','icon-rotate','icon-color'])$(id).addEventListener('change',requestFrame);
for(const id of ['toolbar-size','toolbar-grouped','toolbar-input'])$(id).addEventListener('change',requestFrame);
for(const id of ['title-decoration','title-close','title-fullscreen'])$(id).addEventListener('change',requestFrame);
for(const id of ['motion-duration','motion-stagger','motion-easing','questionnaire-size','questionnaire-shortcuts','questionnaire-numbers','questionnaire-custom','speech-size','speech-unavailable','chart-kind','chart-curve','chart-dots','chart-labels','chart-animate','chart-custom-rows','chart-tooltip-appearance','text-view-format','text-view-scroll','text-view-fade','settings-size','settings-variant','carousel-axis','carousel-loop','command-modal','command-loading','command-filterable','sidebar-mode','sidebar-side','sidebar-collapsed','textarea-rows','textarea-grow','textarea-readonly','textarea-nowrap'])$(id).addEventListener('change',requestFrame);
for(const id of ['button-collection','control-size','choice-required','choice-invalid','choice-skip-tab','switch-side','switch-color','toggle-variant','toggle-segmented','toggle-selection'])$(id).addEventListener('change',requestFrame);
for(const id of ['design-size','design-radius','design-spacing'])$(id).addEventListener('change',requestFrame);
$('checkbox-state').addEventListener('change',()=>{wasm.gallery_checkbox_state(Number($('checkbox-state').value));requestFrame();});
$('selection-mode').addEventListener('change',requestFrame);
$('menu-kind').addEventListener('change',requestFrame);
$('input-mode').addEventListener('change',requestFrame);
for(const id of ['avatar-source','avatar-palette','avatar-hue','avatar-placeholder','avatar-rem','message-alignment','message-variant','message-inset','message-avatar','message-scrollbar','dropdown-names','dropdown-order','dropdown-skip','dropdown-toggle','dropdown-paint','dropdown-shadow','dropdown-variant','dropdown-size','dropdown-halves','dropdown-action','dropdown-anchor','dropdown-outline','dropdown-selected','tabs-mode','tabs-size','tabs-variant','tabs-manual','progress-shape','progress-mode','shimmer-mode','disclosure-mode','disclosure-multiple','notification-anchor','notification-kind','notification-autohide','tag-variant','tag-color','tag-outline','tag-pill','marker-variant','marker-loading','marker-alignment','marker-icon'])$(id).addEventListener('change',requestFrame);
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
 const instance=await WebAssembly.instantiate(module,imports);wasm=instance.exports;wasm._initialize();shadowRenderer=createCuiShadowRenderer(cuiShadowWasmRasterizer(wasm));if(!wasm.gallery_init())throw Error('CUI initialization failed');
 let saved;try{saved=localStorage.getItem('cui.theme');}catch{}themeChoice=saved==='system'||saved==='high-contrast'||themes.some(theme=>theme.id===saved)?saved:'system';buildPaletteMenu();syncTheme();
 const regexOption=$('input-mode').querySelector('option[value="9"]');regexOption.disabled=!wasm.gallery_regex_available();regexOption.textContent=regexOption.disabled?'Unicode letters · regex (unavailable)':'Unicode letters · regex';
 window.cuiGallery={exports:wasm,stories,catalog,documents,themes,video:videoHost,render,shadowStats:()=>shadowRenderer.stats(),get themeChoice(){return themeChoice;}};$('home-story-count').textContent=String(Object.keys(stories).length);route();
}catch(error){fatal(error);}

for(const id of ['effect-angle','effect-blur','effect-opacity','text-family','text-size','text-weight','text-italic'])$(id).addEventListener('change',requestFrame);

$('plot-mode').addEventListener('change',requestFrame);
$('virtual-axis').addEventListener('change',requestFrame);

for(const id of ['scrollbar-policy','scrollbar-width','scrollbar-radius','scrollbar-track'])$(id).addEventListener('change',requestFrame);
