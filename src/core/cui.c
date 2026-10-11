/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <stdlib.h>
#include <string.h>
#define CLAY_IMPLEMENTATION
#include "clay.h"
struct cui_context { Clay_Context *clay; void *arena; cui_config config; cui_draw_command *commands; size_t count; cui_error error; bool in_frame; };
static Clay_Dimensions measure(Clay_StringSlice s,Clay_TextElementConfig *c,void *u) { cui_context *ui=u; if(ui->config.measure) { cui_size z=ui->config.measure(ui->config.measure_user,(cui_str){s.chars,(size_t)s.length},c->fontSize);return (Clay_Dimensions){z.width,z.height}; } return (Clay_Dimensions){s.length*c->fontSize*0.6f,c->fontSize*1.3f}; }
static void error(Clay_ErrorData e) { ((cui_context *)e.userData)->error=CUI_ERROR_LAYOUT; }
cui_str cui_string(const char *s) { return (cui_str){s,s?strlen(s):0}; }
cui_context *cui_create(cui_config config) { cui_context *ui=calloc(1,sizeof(*ui));if(!ui)return NULL;ui->config=config;if(!ui->config.max_commands)ui->config.max_commands=4096;ui->commands=calloc(ui->config.max_commands,sizeof(*ui->commands));size_t n=Clay_MinMemorySize();ui->arena=malloc(n);if(!ui->commands||!ui->arena){cui_destroy(ui);return NULL;}ui->clay=Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(n,ui->arena),(Clay_Dimensions){config.width,config.height},(Clay_ErrorHandler){error,ui});Clay_SetMeasureTextFunction(measure,ui);return ui; }
void cui_destroy(cui_context *ui) { if(ui){free(ui->commands);free(ui->arena);free(ui);} }
bool cui_begin_frame(cui_context *ui,float w,float h,float dt) { (void)dt;if(ui->in_frame){ui->error=CUI_ERROR_LIFECYCLE;return false;}Clay_SetCurrentContext(ui->clay);Clay_SetLayoutDimensions((Clay_Dimensions){w,h});Clay_BeginLayout();ui->config.width=w;ui->config.height=h;ui->in_frame=true;return true; }
static Clay_SizingAxis axis(float n) { return n<0?(Clay_SizingAxis)CLAY_SIZING_GROW():n>0?(Clay_SizingAxis)CLAY_SIZING_FIXED(n):(Clay_SizingAxis)CLAY_SIZING_FIT(); }
void cui_begin_box(cui_context *ui,cui_id id,cui_box b) { (void)ui;Clay__OpenElementWithId((Clay_ElementId){.id=(uint32_t)id});Clay__ConfigureOpenElement((Clay_ElementDeclaration){.layout={.sizing={axis(b.width),axis(b.height)},.padding=CLAY_PADDING_ALL((uint16_t)b.padding),.childGap=(uint16_t)b.gap,.layoutDirection=b.row?CLAY_LEFT_TO_RIGHT:CLAY_TOP_TO_BOTTOM},.backgroundColor={b.background.r,b.background.g,b.background.b,b.background.a},.cornerRadius=CLAY_CORNER_RADIUS(b.radius)}); }
void cui_end_box(cui_context *ui) { (void)ui;Clay__CloseElement(); }
void cui_text(cui_context *ui,cui_str t,float size,cui_color c) { (void)ui;Clay__OpenTextElement((Clay_String){false,(int32_t)t.length,t.data},(Clay_TextElementConfig){.fontSize=(uint16_t)size,.textColor={c.r,c.g,c.b,c.a}}); }
cui_id cui_id_from(cui_context *ui,const char *s) { (void)ui;uint64_t h=14695981039346656037ULL;while(*s){h^=(unsigned char)*s++;h*=1099511628211ULL;}return h; }
cui_draw_list cui_end_frame(cui_context *ui) { Clay_RenderCommandArray a=Clay_EndLayout(0);ui->count=0;for(int32_t i=0;i<a.length;i++){Clay_RenderCommand *c=&a.internalArray[i];if(ui->count==ui->config.max_commands){ui->error=CUI_ERROR_CAPACITY;break;}cui_draw_command d={.bounds={c->boundingBox.x,c->boundingBox.y,c->boundingBox.width,c->boundingBox.height}};if(c->commandType==CLAY_RENDER_COMMAND_TYPE_RECTANGLE){d.type=CUI_DRAW_RECT;Clay_Color z=c->renderData.rectangle.backgroundColor;d.color=(cui_color){z.r,z.g,z.b,z.a};d.radius=c->renderData.rectangle.cornerRadius.topLeft;}else if(c->commandType==CLAY_RENDER_COMMAND_TYPE_TEXT){d.type=CUI_DRAW_TEXT;Clay_TextRenderData t=c->renderData.text;d.text=(cui_str){t.stringContents.chars,(size_t)t.stringContents.length};d.font_size=t.fontSize;d.color=(cui_color){t.textColor.r,t.textColor.g,t.textColor.b,t.textColor.a};}else continue;ui->commands[ui->count++]=d;}ui->in_frame=false;return (cui_draw_list){ui->commands,ui->count,ui->config.width,ui->config.height}; }
cui_error cui_last_error(cui_context *ui){return ui->error;}
