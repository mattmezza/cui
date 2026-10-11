/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define GLYPH_CACHE_SLOTS 256
#define GLYPH_CACHE_BYTES (4u*1024u*1024u)
typedef struct { SDL_Texture *texture; unsigned char *coverage; int w,h; uint64_t hash,used; } glyph_entry;
struct cui_sdl3 { SDL_Window *window; SDL_Renderer *renderer; cui_font *font; cui_color ink; bool quit,glyph_ok; glyph_entry glyphs[GLYPH_CACHE_SLOTS]; size_t glyph_bytes; uint64_t clock,hits,misses; char *clipboard; };
cui_sdl3 *cui_sdl3_create(const char *title,int w,int h,cui_font *font) {
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO))return NULL;
    cui_sdl3 *s=calloc(1,sizeof(*s));if(!s){SDL_QuitSubSystem(SDL_INIT_VIDEO);return NULL;}
    s->font=font;s->window=SDL_CreateWindow(title,w,h,SDL_WINDOW_RESIZABLE);
    if(s->window)s->renderer=SDL_CreateRenderer(s->window,NULL);
    if(!s->renderer){cui_sdl3_destroy(s);return NULL;}
    SDL_SetRenderDrawBlendMode(s->renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderVSync(s->renderer,1);SDL_StartTextInput(s->window);return s;
}
void cui_sdl3_destroy(cui_sdl3 *s){if(!s)return;for(size_t i=0;i<GLYPH_CACHE_SLOTS;i++){SDL_DestroyTexture(s->glyphs[i].texture);free(s->glyphs[i].coverage);}SDL_free(s->clipboard);SDL_DestroyRenderer(s->renderer);SDL_DestroyWindow(s->window);free(s);SDL_QuitSubSystem(SDL_INIT_VIDEO);}
static cui_key key(SDL_Keycode k){switch(k){case SDLK_TAB:return CUI_KEY_TAB;case SDLK_RETURN:return CUI_KEY_ENTER;case SDLK_SPACE:return CUI_KEY_SPACE;case SDLK_ESCAPE:return CUI_KEY_ESCAPE;case SDLK_LEFT:return CUI_KEY_LEFT;case SDLK_RIGHT:return CUI_KEY_RIGHT;case SDLK_UP:return CUI_KEY_UP;case SDLK_DOWN:return CUI_KEY_DOWN;case SDLK_HOME:return CUI_KEY_HOME;case SDLK_END:return CUI_KEY_END;case SDLK_BACKSPACE:return CUI_KEY_BACKSPACE;case SDLK_DELETE:return CUI_KEY_DELETE;case SDLK_A:return CUI_KEY_A;default:return CUI_KEY_NONE;}}
bool cui_sdl3_poll(cui_sdl3 *s,cui_context *ui){
    SDL_Event e;while(SDL_PollEvent(&e)){cui_event v={0};bool send=true;
    switch(e.type){
    case SDL_EVENT_QUIT:case SDL_EVENT_WINDOW_CLOSE_REQUESTED:s->quit=true;send=false;break;
    case SDL_EVENT_MOUSE_MOTION:v.type=CUI_EVENT_POINTER_MOVE;v.x=e.motion.x;v.y=e.motion.y;break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:case SDL_EVENT_MOUSE_BUTTON_UP:if(e.button.button!=SDL_BUTTON_LEFT){send=false;break;}v.type=e.type==SDL_EVENT_MOUSE_BUTTON_DOWN?CUI_EVENT_POINTER_DOWN:CUI_EVENT_POINTER_UP;v.x=e.button.x;v.y=e.button.y;break;
    case SDL_EVENT_MOUSE_WHEEL:v.type=CUI_EVENT_SCROLL;v.x=e.wheel.mouse_x;v.y=e.wheel.mouse_y;v.dx=e.wheel.x;v.dy=e.wheel.y;break;
    case SDL_EVENT_KEY_DOWN:case SDL_EVENT_KEY_UP:v.type=e.type==SDL_EVENT_KEY_DOWN?CUI_EVENT_KEY_DOWN:CUI_EVENT_KEY_UP;v.key=key(e.key.key);v.repeat=e.key.repeat;v.modifiers=((e.key.mod&SDL_KMOD_SHIFT)?CUI_MOD_SHIFT:0)|((e.key.mod&SDL_KMOD_CTRL)?CUI_MOD_CTRL:0)|((e.key.mod&SDL_KMOD_ALT)?CUI_MOD_ALT:0)|((e.key.mod&SDL_KMOD_GUI)?CUI_MOD_META:0);break;
    case SDL_EVENT_TEXT_INPUT:v.type=CUI_EVENT_TEXT;SDL_strlcpy(v.text,e.text.text,sizeof(v.text));break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:v.type=CUI_EVENT_CANCEL;break;
    case SDL_EVENT_WINDOW_RESIZED:case SDL_EVENT_WINDOW_EXPOSED:cui_invalidate(ui);send=false;break;
    default:send=false;break;
    }if(send)cui_queue_event(ui,v);}
    return !s->quit;
}
void cui_sdl3_size(cui_sdl3 *s,float *w,float *h){int x,y;SDL_GetWindowSize(s->window,&x,&y);*w=(float)x;*h=(float)y;SDL_SetRenderLogicalPresentation(s->renderer,x,y,SDL_LOGICAL_PRESENTATION_STRETCH);}
static float inset(float radius,float y,float height){radius=fminf(fmaxf(0,radius),height/2);float d=radius-fminf(y,height-y);return d>0?radius-sqrtf(fmaxf(0,radius*radius-d*d)):0;}
static bool rect(cui_sdl3 *s,const cui_draw_command *c){
    SDL_SetRenderDrawColor(s->renderer,c->color.r,c->color.g,c->color.b,c->color.a);
    float rad=fminf(c->radius,c->bounds.w/2);
    if(rad<=0 && c->type==CUI_DRAW_RECT){SDL_FRect r={c->bounds.x,c->bounds.y,c->bounds.w,c->bounds.h};return SDL_RenderFillRect(s->renderer,&r);}
    for(int y=0;y<(int)ceilf(c->bounds.h);y++){float h=fminf(1,c->bounds.h-y),off=inset(rad,y+h/2,c->bounds.h);SDL_FRect r={c->bounds.x+off,c->bounds.y+y,c->bounds.w-2*off,h};
        if(c->type==CUI_DRAW_BORDER && y+h/2>=c->stroke && y+h/2<c->bounds.h-c->stroke && c->bounds.w>2*c->stroke){float inner=c->stroke+inset(fmaxf(0,rad-c->stroke),y+h/2-c->stroke,c->bounds.h-2*c->stroke);float edge=fmaxf(0,inner-off);r.w=edge;if(!SDL_RenderFillRect(s->renderer,&r))return false;r.x=c->bounds.x+c->bounds.w-inner;}
        if(!SDL_RenderFillRect(s->renderer,&r))return false;
    }return true;
}
static void evict(cui_sdl3 *s,glyph_entry *e){if(e->coverage)s->glyph_bytes-=(size_t)e->w*e->h;SDL_DestroyTexture(e->texture);free(e->coverage);memset(e,0,sizeof(*e));}
static glyph_entry *oldest(cui_sdl3 *s){glyph_entry *best=&s->glyphs[0];for(size_t i=0;i<GLYPH_CACHE_SLOTS;i++){glyph_entry *e=&s->glyphs[i];if(!e->texture)return e;if(e->used<best->used)best=e;}return best;}
static SDL_Texture *cached_glyph(cui_sdl3 *s,int w,int h,int pitch,const unsigned char *data){
    if(w<0 || h<0 || (size_t)w>GLYPH_CACHE_BYTES/(size_t)h){SDL_SetError("CUI glyph exceeds the bounded cache memory budget");return NULL;}
    size_t bytes=(size_t)w*h;uint64_t hash=UINT64_C(14695981039346656037);
    for(int j=0;j<h;j++)for(int i=0;i<w;i++){hash^=data[j*pitch+i];hash*=UINT64_C(1099511628211);}
    for(size_t i=0;i<GLYPH_CACHE_SLOTS;i++){
        glyph_entry *e=&s->glyphs[i];if(e->texture && e->w==w && e->h==h && e->hash==hash){
            bool same=true;for(int j=0;j<h && same;j++)same=memcmp(e->coverage+(size_t)j*w,data+j*pitch,(size_t)w)==0;
            if(same){e->used=++s->clock;s->hits++;return e->texture;}
        }
    }
    s->misses++;
    while(s->glyph_bytes+bytes>GLYPH_CACHE_BYTES){glyph_entry *e=oldest(s);if(!e->texture){ /* Find the oldest populated entry when empty slots remain. */
            for(size_t i=0;i<GLYPH_CACHE_SLOTS;i++)if(s->glyphs[i].texture && (!e->texture || s->glyphs[i].used<e->used))e=&s->glyphs[i];
        }evict(s,e);}
    glyph_entry *e=oldest(s);if(e->texture)evict(s,e);
    SDL_Surface *surface=SDL_CreateSurface(w,h,SDL_PIXELFORMAT_RGBA32);if(!surface)return NULL;
    for(int j=0;j<h;j++){unsigned char *p=(unsigned char*)surface->pixels+j*surface->pitch;for(int i=0;i<w;i++){p[i*4]=255;p[i*4+1]=255;p[i*4+2]=255;p[i*4+3]=data[j*pitch+i];}}
    SDL_Texture *t=SDL_CreateTextureFromSurface(s->renderer,surface);SDL_DestroySurface(surface);if(!t)return NULL;
    unsigned char *copy=malloc(bytes);if(!copy){SDL_DestroyTexture(t);SDL_SetError("CUI glyph coverage allocation failed");return NULL;}
    for(int j=0;j<h;j++)memcpy(copy+(size_t)j*w,data+j*pitch,(size_t)w);
    if(!SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND)){SDL_DestroyTexture(t);free(copy);return NULL;}
    *e=(glyph_entry){t,copy,w,h,hash,++s->clock};s->glyph_bytes+=bytes;return t;
}
static void glyph(void *user,int x,int y,int w,int h,int pitch,const unsigned char *data){
    cui_sdl3 *s=user;if(!w || !h || !data || !s->glyph_ok)return;
    SDL_Texture *t=cached_glyph(s,w,h,pitch,data);if(!t){s->glyph_ok=false;return;}
    SDL_FRect r={(float)x,(float)y,(float)w,(float)h};
    if(!SDL_SetTextureColorMod(t,s->ink.r,s->ink.g,s->ink.b) || !SDL_SetTextureAlphaMod(t,s->ink.a) || !SDL_RenderTexture(s->renderer,t,NULL,&r))s->glyph_ok=false;
}
bool cui_sdl3_render_capture(cui_sdl3 *s,const cui_draw_list *list,cui_color clear,const char *path){
    if(!s||!list || (!list->commands && list->count) || !isfinite(list->width) || !isfinite(list->height) || list->width<0 || list->height<0 || list->width>16384 || list->height>16384)return false;
    SDL_SetRenderClipRect(s->renderer,NULL);SDL_SetRenderDrawColor(s->renderer,clear.r,clear.g,clear.b,clear.a);if(!SDL_RenderClear(s->renderer))return false;
    SDL_Rect stack[64],clip={0,0,(int)ceilf(list->width),(int)ceilf(list->height)};size_t depth=0;
    for(size_t i=0;i<list->count;i++){const cui_draw_command *c=&list->commands[i];
    if(!isfinite(c->bounds.x)||!isfinite(c->bounds.y)||!isfinite(c->bounds.w)||!isfinite(c->bounds.h)||!isfinite(c->radius)||!isfinite(c->stroke)||fabsf(c->bounds.x)>1e6f||fabsf(c->bounds.y)>1e6f||fabsf(c->bounds.w)>16384||fabsf(c->bounds.h)>16384)return false;
    switch(c->type){
    case CUI_DRAW_RECT:case CUI_DRAW_BORDER:if(!rect(s,c))return false;break;
    case CUI_DRAW_TEXT:s->ink=c->color;s->glyph_ok=true;if(!s->font || !cui_font_draw(s->font,c->text,c->font_size,c->bounds.x,c->bounds.y,glyph,s) || !s->glyph_ok)return false;break;
    case CUI_DRAW_CLIP_PUSH:{if(depth==64)return false;stack[depth++]=clip;SDL_Rect next={(int)floorf(c->bounds.x),(int)floorf(c->bounds.y),(int)ceilf(c->bounds.w),(int)ceilf(c->bounds.h)};if(!SDL_GetRectIntersection(&clip,&next,&clip))clip=(SDL_Rect){0,0,0,0};SDL_SetRenderClipRect(s->renderer,&clip);break;}
    case CUI_DRAW_CLIP_POP:if(!depth)return false;clip=stack[--depth];SDL_SetRenderClipRect(s->renderer,depth?&clip:NULL);break;
    default:return false;}}
    if(depth)return false;
    if(path && !cui_sdl3_save_bmp(s,path))return false;
    return SDL_RenderPresent(s->renderer);
}
bool cui_sdl3_render(cui_sdl3 *s,const cui_draw_list *list,cui_color clear){return cui_sdl3_render_capture(s,list,clear,NULL);}
bool cui_sdl3_save_bmp(cui_sdl3 *s,const char *path){SDL_Surface *surface=SDL_RenderReadPixels(s->renderer,NULL);if(!surface)return false;bool ok=SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);return ok;}
const char *cui_sdl3_driver(cui_sdl3 *s){return s?SDL_GetRendererName(s->renderer):"unavailable";}
const char *cui_sdl3_video_driver(void){return SDL_GetCurrentVideoDriver();}
const char *cui_sdl3_error(void){return SDL_GetError();}
cui_sdl3_cache_stats cui_sdl3_glyph_cache_stats(cui_sdl3 *s){cui_sdl3_cache_stats stats={0};if(!s)return stats;stats.coverage_bytes=s->glyph_bytes;stats.hits=s->hits;stats.misses=s->misses;for(size_t i=0;i<GLYPH_CACHE_SLOTS;i++)stats.entries+=s->glyphs[i].texture!=NULL;return stats;}
static bool clipboard_set(void *user,cui_str text){
    if(!user || (!text.data && text.length) || text.length>16u*1024u*1024u || (text.length && memchr(text.data,0,text.length)))return false;
    char *copy=malloc(text.length+1);if(!copy)return false;if(text.length)memcpy(copy,text.data,text.length);copy[text.length]=0;bool ok=SDL_SetClipboardText(copy);free(copy);return ok;
}
static cui_str clipboard_get(void *user){cui_sdl3 *s=user;if(!s)return(cui_str){NULL,0};SDL_free(s->clipboard);s->clipboard=SDL_GetClipboardText();return(cui_str){s->clipboard,s->clipboard?strlen(s->clipboard):0};}
cui_platform cui_sdl3_clipboard_interface(cui_sdl3 *s){return(cui_platform){.user=s,.capabilities=s?CUI_PLATFORM_CLIPBOARD:0,.clipboard_set=clipboard_set,.clipboard_get=clipboard_get};}
void cui_sdl3_wait(unsigned ms){SDL_Delay(ms);}
static bool render(void *s,const cui_draw_list *l,cui_color c){return cui_sdl3_render(s,l,c);}
static void destroy(void *s){cui_sdl3_destroy(s);}
cui_renderer cui_sdl3_interface(cui_sdl3 *s){const char *name=cui_sdl3_driver(s);return(cui_renderer){s,name,(s && s->font?CUI_RENDER_TEXT:0)|CUI_RENDER_ROUNDED|CUI_RENDER_CLIP|(s && strcmp(name,"software")?CUI_RENDER_ACCELERATED:0),render,destroy};}
