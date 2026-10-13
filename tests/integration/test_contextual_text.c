/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include "cui/cui_textarea.h"
#include "cui/cui_editor.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void assert_comparison(bool equal, const char *message) {
    if (!equal) fprintf(stderr,"FAIL: %s\n",message);
    assert(equal);
}
static const char *font_path(const char *env, const char *const *paths, size_t count) {
    const char *requested = getenv(env);
    if (requested) {
        cui_font *font = cui_font_open(requested);
        assert(font);
        cui_font_close(font);
        return requested;
    }
    for (size_t i = 0; i < count; ++i) {
        cui_font *font = cui_font_open(paths[i]);
        if (font) {
            cui_font_close(font);
            return paths[i];
        }
    }
    return NULL;
}
typedef struct {
    uint64_t hash;
    unsigned glyphs, pixels;
} raster;
static void glyph(void *user, int x, int y, int width, int height, int pitch,
                  const unsigned char *data) {
    raster *r = user;
    ++r->glyphs;
    r->hash = r->hash * UINT64_C(1099511628211) + (unsigned)x;
    r->hash = r->hash * UINT64_C(1099511628211) + (unsigned)y;
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) {
            r->hash = r->hash * UINT64_C(1099511628211) + data[row * pitch + col];
            r->pixels += data[row * pitch + col] != 0;
        }
}
static cui_text_layout *layout(cui_font *font, cui_str text, float size, size_t start,
                               size_t count) {
    cui_text_layout *result = NULL;
    assert(cui_font_layout_line(font, text, size, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, start,
                                count, NULL, &result) == CUI_TEXT_LAYOUT_OK);
    return result;
}
static void caret_audit(cui_context *ui, cui_font *font) {
    char buffer[64] = "אבג";
    cui_str text = cui_string(buffer);
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    assert(edit);
    assert(cui_edit_select_atomic(edit, 0, 0) == CUI_EDIT_OK);
    cui_textarea_line rows[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, rows, 16));
    cui_theme theme = cui_theme_light();
    cui_textarea_options options = {.width = 280, .no_wrap = true};
    for (unsigned frame = 0; frame < 2; ++frame) {
        if (frame)
            cui_focus(ui, 50);
        assert(cui_begin_frame(ui, 600, 400, .016f));
        assert(
            cui_textarea(ui, 50, &theme, cui_string("RTL editor"), edit, &state, options).error ==
            CUI_TEXTAREA_OK);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_text_layout *line = layout(font, text, state.font_size, 0, SIZE_MAX);
    cui_text_layout_view view;
    assert(cui_text_layout_get(line, &view));
    cui_text_caret caret;
    size_t count;
    assert(cui_text_layout_carets(line, 0, &caret, 1, &count) == CUI_TEXT_LAYOUT_OK && count == 1);
    float editor_x = cui_textarea_row_x(ui, edit, &state, 0, 0);
    printf("RTL byte 0 caret: editor %.3f, shaped %.3f\n", editor_x, caret.x);
    assert_comparison(fabsf(editor_x - caret.x) < .01f,
               "Textarea byte0 caret uses logical prefix width instead of RTL visual edge");
    cui_rect ime;
    assert(cui_text_input_rect(ui, &ime));
    assert(fabsf(ime.x - state.viewport.x - editor_x + state.scroll_x) < .01f);
    cui_input_hit_geometry hit = {
        .ui = ui, .font_size = state.font_size, .viewport = {0, 0, 280, 20}};
    size_t input_byte = cui_base_input_hit_test(&hit, text, 0, 0);
    size_t textarea_byte = cui_textarea_row_offset(ui, edit, &state, 0, 0);
    assert(cui_text_layout_hit_test(line, 0, 0, CUI_TEXT_UPSTREAM, &caret) == CUI_TEXT_LAYOUT_OK);
    printf("RTL left edge: Input byte %zu, Textarea byte %zu, shaped byte %zu\n", input_byte,
           textarea_byte, caret.byte_offset);
    assert_comparison(input_byte == caret.byte_offset && textarea_byte == caret.byte_offset,
               "Textarea visual left hit maps to logical start instead of RTL end");
    assert(cui_edit_select_atomic(edit, 0, 2) == CUI_EDIT_OK);
    theme.selection = (cui_color){17, 33, 49, 255};
    assert(cui_begin_frame(ui, 600, 400, .016f));
    assert(cui_textarea(ui, 50, &theme, cui_string("RTL editor"), edit, &state, options).error ==
           CUI_TEXTAREA_OK);
    cui_draw_list painted = cui_end_frame(ui);
    const cui_draw_command *highlight = NULL;
    for (size_t i = 0; i < painted.count; ++i)
        if (painted.commands[i].type == CUI_DRAW_RECT &&
            !memcmp(&painted.commands[i].color, &theme.selection, sizeof theme.selection))
            highlight = &painted.commands[i];
    assert(highlight);
    cui_text_selection_rect selected;
    assert(cui_text_layout_selection(line, 0, 2, &selected, 1, &count) == CUI_TEXT_LAYOUT_OK &&
           count == 1);
    float painted_x = highlight->bounds.x - state.viewport.x + state.scroll_x;
    printf("RTL first-character selection: widget x%.3f, shaped x%.3f\n", painted_x,
           selected.bounds.x);
    assert_comparison(
        fabsf(painted_x - selected.bounds.x) < .01f,
        "Textarea selection highlight occupies logical prefix side rather than selected RTL glyph");
    cui_software *software=cui_software_create(600,400,font);assert(software);
    assert(cui_software_render(software,&painted,(cui_color){255,255,255,255}));
    const unsigned char *pixels=cui_software_pixels(software,NULL,NULL);
    unsigned selected_pixels=0;
    for(unsigned y=0;y<400;y++) for(unsigned x=0;x<600;x++) {
        const unsigned char *pixel=pixels+((size_t)y*600+x)*4;
        if(pixel[0]==17 && pixel[1]==33 && pixel[2]==49) {
            assert((float)x>=highlight->bounds.x-1 && (float)x<highlight->bounds.x+highlight->bounds.w+1);
            selected_pixels++;
        }
    }
    assert(selected_pixels);cui_software_destroy(software);
    cui_text_layout_destroy(line);
    cui_edit_destroy(edit);
}
static void wrap_audit(cui_context *ui, cui_font *font) {
    cui_str paragraph = cui_string("אבג 123 !");
    float width = cui_font_measure(font, cui_string("123 !"), 24).width + 1;
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 80, (cui_box){.width = width, .fixed_width = true});
    cui_text(ui, paragraph, 24, (cui_color){20, 40, 60, 255});
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    const cui_draw_command *wrapped = NULL;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 5 &&
            !memcmp(draw.commands[i].text.data, "123 !", 5))
            wrapped = &draw.commands[i];
    assert(wrapped && cui_last_error(ui) == CUI_OK);
    cui_text_layout *contextual = layout(font, paragraph, 24, 4, 5);
    cui_text_layout *standalone = layout(font, wrapped->text, 24, 0, SIZE_MAX);
    cui_text_layout_view expected, actual;
    assert(cui_text_layout_get(contextual, &expected) && cui_text_layout_get(standalone, &actual));
    raster renderer = {0}, independent = {0}, reference = {0};
    /* SDL/GPU/Sokol all invoke this same native draw entry point per command. */
    assert(cui_font_draw_context_line(font, wrapped->text, wrapped->text_source, wrapped->font_size, wrapped->text_style, 0, 0,
                                glyph, &renderer));
    assert(cui_text_layout_draw(standalone, 0, 0, glyph, &independent));
    assert(cui_text_layout_draw(contextual, 0, 0, glyph, &reference));
    assert(renderer.hash != independent.hash && renderer.pixels && reference.pixels);
    printf("Core wrapped '123 !': standalone base %u, paragraph base %u, raster %s\n",
           actual.paragraph_level, expected.paragraph_level,
           renderer.hash == reference.hash ? "equal" : "different");
    assert_comparison(
        renderer.hash == reference.hash,
        "Core native wrapped command resets paragraph bidi context and repositions punctuation");
    cui_text_layout_destroy(contextual);
    cui_text_layout_destroy(standalone);
}


static size_t highlight(void *user,cui_str language,cui_str text,size_t start,size_t end,cui_color *color) {
    (void)user;(void)language;
    *color=(start&2)?(cui_color){220,20,30,255}:(cui_color){20,220,30,255};
    size_t next=start+1;
    while(next<end && ((unsigned char)text.data[next]&0xc0)==0x80) next++;
    return next;
}
typedef struct { raster pixels; unsigned red,green; } colored_raster;
static void colored_glyph(void *user,int x,int y,int w,int h,int pitch,const unsigned char *data,
                           cui_color color) {
    colored_raster *r=user;glyph(&r->pixels,x,y,w,h,pitch,data);
    r->red+=color.r==220;r->green+=color.g==220;
}
static void editor_case(cui_context *ui,cui_font *font,const char *sample) {
    char buffer[128];strcpy(buffer,sample);
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof buffer,
                              .multiline=true,.history_bytes=1024,.max_history_steps=8});assert(edit);
    assert(cui_edit_select_atomic(edit,0,0)==CUI_EDIT_OK);
    cui_textarea_line rows[16];cui_editor_state state;
    assert(cui_editor_state_init(&state,edit,rows,16));
    cui_theme theme=cui_theme_light();
    cui_editor_options options={.textarea={.width=280,.no_wrap=true},.highlight=highlight};
    for(unsigned frame=0;frame<2;frame++) {
        if(frame)cui_focus(ui,90);
        assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_editor(ui,90,&theme,cui_string("Code"),&state,options).error==CUI_TEXTAREA_OK);
        cui_draw_list draw=cui_end_frame(ui);const cui_draw_command *text=NULL;
        for(size_t i=0;i<draw.count;i++) if(draw.commands[i].type==CUI_DRAW_TEXT &&
                    draw.commands[i].text_color_count) text=&draw.commands[i];
        assert(text && text->text_source.paragraph.length==strlen(sample));
        cui_text_layout *line=layout(font,cui_string(sample),text->font_size,0,SIZE_MAX);
        raster reference={0};colored_raster actual={0};
        assert(cui_text_layout_draw(line,0,0,glyph,&reference));
        assert(cui_font_draw_context_line_colored(font,text->text_source,text->font_size,
                text->text_style,0,0,text->color,text->text_colors,text->text_color_count,
                colored_glyph,&actual));
        assert(reference.hash==actual.pixels.hash && actual.pixels.pixels && actual.red && actual.green);
        cui_software *software=cui_software_create(600,400,font);assert(software);
        assert(cui_software_render(software,&draw,(cui_color){255,255,255,255}));
        const unsigned char *pixels=cui_software_pixels(software,NULL,NULL);
        unsigned red_pixels=0,green_pixels=0;
        for(size_t pixel=0;pixel<600*400;pixel++) {
            red_pixels+=pixels[pixel*4]>pixels[pixel*4+1]*2;
            green_pixels+=pixels[pixel*4+1]>pixels[pixel*4]*2;
        }
        assert(red_pixels && green_pixels);cui_software_destroy(software);
        if(frame) {
            cui_text_caret caret;size_t count;
            assert(cui_text_layout_carets(line,0,&caret,1,&count)==CUI_TEXT_LAYOUT_OK);
            cui_rect ime;assert(cui_text_input_rect(ui,&ime));
            assert(fabsf(ime.x-state.view.viewport.x-caret.x+state.view.scroll_x)<.01f);
        }
        cui_text_layout_destroy(line);
    }
    cui_edit_destroy(edit);
}
static void ime_case(cui_context *ui,cui_font *font) {
    char buffer[64]="אבג";
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof buffer,
        .multiline=true,.history_bytes=1024,.max_history_steps=8});assert(edit);
    cui_textarea_line rows[16];cui_textarea_state state;assert(cui_textarea_state_init(&state,rows,16));
    cui_theme theme=cui_theme_light();cui_textarea_options options={.width=280,.no_wrap=true};
    assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_textarea(ui,91,&theme,cui_string("IME"),edit,&state,options).error==CUI_TEXTAREA_OK);
    cui_end_frame(ui);cui_focus(ui,91);assert(cui_edit_select_atomic(edit,0,0)==CUI_EDIT_OK);
    for(unsigned pass=0;pass<2;pass++) {
        cui_event update={.type=CUI_EVENT_COMPOSITION_UPDATE,.text_data="ד",.text_length=2,
                           .composition_anchor=2,.composition_cursor=2};
        assert(cui_queue_event(ui,update));assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_textarea(ui,91,&theme,cui_string("IME"),edit,&state,options).error==CUI_TEXTAREA_OK);
        cui_end_frame(ui);assert(!strcmp(buffer,"אבג") && state.composition->active);
        cui_text_layout *line=layout(font,cui_string("דאבג"),state.font_size,0,SIZE_MAX);
        cui_text_caret carets[2];size_t count;
        assert(cui_text_layout_carets(line,2,carets,2,&count)==CUI_TEXT_LAYOUT_OK && count);
        cui_rect ime;assert(cui_text_input_rect(ui,&ime));
        assert(fabsf(ime.x-state.viewport.x-carets[0].x+state.scroll_x)<.01f);
        cui_text_layout_destroy(line);
        update.type=pass?CUI_EVENT_COMPOSITION_COMMIT:CUI_EVENT_COMPOSITION_CANCEL;
        assert(cui_queue_event(ui,update));assert(cui_begin_frame(ui,600,400,.016f));
        cui_textarea_result result=cui_textarea(ui,91,&theme,cui_string("IME"),edit,&state,options);
        cui_end_frame(ui);assert(result.error==CUI_TEXTAREA_OK && result.changed==(pass!=0));
        assert(!state.composition->active);
    }
    assert(!strcmp(buffer,"דאבג") && cui_edit_undo(edit)==CUI_EDIT_OK && !strcmp(buffer,"אבג"));
    cui_edit_destroy(edit);
}


static void affinity_case(cui_context *ui,cui_font *font) {
    char buffer[64]="abc אבג";
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof buffer,.multiline=true});assert(edit);
    assert(cui_edit_select_atomic(edit,0,0)==CUI_EDIT_OK);
    cui_textarea_line rows[8];cui_textarea_state state;assert(cui_textarea_state_init(&state,rows,8));
    cui_theme theme=cui_theme_light();cui_textarea_options options={.width=280,.no_wrap=true};
    for(unsigned frame=0;frame<2;frame++) {
        if(frame)cui_focus(ui,94);
        assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_textarea(ui,94,&theme,cui_string("Affinity"),edit,&state,options).error==CUI_TEXTAREA_OK);
        cui_end_frame(ui);
    }
    cui_text_layout *line=layout(font,cui_string(buffer),state.font_size,0,SIZE_MAX);
    cui_text_caret edges[2];size_t count;
    assert(cui_text_layout_carets(line,4,edges,2,&count)==CUI_TEXT_LAYOUT_OK && count==2 && edges[0].x!=edges[1].x);
    for(size_t e=0;e<count;e++) {
        cui_text_caret chosen;
        assert(cui_text_layout_hit_test(line,edges[e].x,0,CUI_TEXT_DOWNSTREAM,&chosen)==CUI_TEXT_LAYOUT_OK);
        assert(chosen.byte_offset==4);
        cui_event pointer={.type=CUI_EVENT_POINTER_DOWN,.x=state.viewport.x+edges[e].x-state.scroll_x,
                            .y=state.viewport.y+2};
        assert(cui_queue_event(ui,pointer));pointer.type=CUI_EVENT_POINTER_UP;assert(cui_queue_event(ui,pointer));
        for(unsigned idle=0;idle<2;idle++) {
            assert(cui_begin_frame(ui,600,400,.016f));
            assert(cui_textarea(ui,94,&theme,cui_string("Affinity"),edit,&state,options).error==CUI_TEXTAREA_OK);
            cui_end_frame(ui);assert(cui_edit_get_state(edit).cursor==4);
            cui_rect ime;assert(cui_text_input_rect(ui,&ime));
            assert(fabsf(ime.x-state.viewport.x+state.scroll_x-chosen.x)<.01f);
        }
    }
    cui_text_layout_destroy(line);cui_edit_destroy(edit);
}

static void editor_ime_case(cui_context *ui,cui_font *font) {
    char buffer[64]="אבג";
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof buffer,
        .multiline=true,.history_bytes=1024,.max_history_steps=8});assert(edit);
    assert(cui_edit_select_atomic(edit,0,0)==CUI_EDIT_OK);
    cui_textarea_line rows[16];cui_editor_state editor;assert(cui_editor_state_init(&editor,edit,rows,16));
    cui_theme theme=cui_theme_light();cui_editor_options options={.textarea={.width=280,.no_wrap=true,.rows=3},
                                                               .line_numbers=true,.highlight=highlight};
    assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_editor(ui,95,&theme,cui_string("IME code"),&editor,options).error==CUI_TEXTAREA_OK);
    cui_end_frame(ui);cui_focus(ui,95);
    for(unsigned pass=0;pass<2;pass++) {
        cui_event update={.type=CUI_EVENT_COMPOSITION_UPDATE,.text_data="ד\nד",.text_length=5,
                           .composition_anchor=5,.composition_cursor=5};
        assert(cui_queue_event(ui,update));assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_editor(ui,95,&theme,cui_string("IME code"),&editor,options).error==CUI_TEXTAREA_OK);
        cui_end_frame(ui);assert(!strcmp(buffer,"אבג") && editor.view.count==2 &&
                                        rows[1].logical_line==1 && rows[1].paragraph_start==3);
        cui_text_layout *line=layout(font,cui_string("דאבג"),editor.view.font_size,0,SIZE_MAX);
        cui_text_caret carets[2];size_t count;
        assert(cui_text_layout_carets(line,2,carets,2,&count)==CUI_TEXT_LAYOUT_OK && count);
        cui_rect ime;assert(cui_text_input_rect(ui,&ime));
        assert(fabsf(ime.x-editor.view.viewport.x-carets[0].x+editor.view.scroll_x)<.01f);
        assert(fabsf(ime.y-editor.view.viewport.y-editor.view.line_height+editor.view.scroll_y)<.01f);
        cui_text_layout_destroy(line);
        update.type=pass?CUI_EVENT_COMPOSITION_COMMIT:CUI_EVENT_COMPOSITION_CANCEL;
        assert(cui_queue_event(ui,update));assert(cui_begin_frame(ui,600,400,.016f));
        cui_textarea_result result=cui_editor(ui,95,&theme,cui_string("IME code"),&editor,options);
        cui_end_frame(ui);assert(result.error==CUI_TEXTAREA_OK && result.changed==(pass!=0));
    }
    assert(!strcmp(buffer,"ד\nדאבג") && cui_edit_undo(edit)==CUI_EDIT_OK && !strcmp(buffer,"אבג"));
    cui_edit_destroy(edit);
}
typedef struct { cui_font *font; unsigned measured,geometry; } counted_host;
static cui_size counted_measure(void *user,cui_str text,float size,cui_text_style style) {
    counted_host *host=user;host->measured++;
    return cui_font_measure_styled(host->font,text,size,style);
}
static cui_text_layout_status counted_geometry(void *user,cui_text_line_source source,float size,
                                               cui_text_style style,cui_text_geometry *out) {
    counted_host *host=user;host->geometry++;
    return cui_font_text_geometry_line(host->font,source,size,style,out);
}
static void large_case(cui_font *font) {
    const char *paragraph="abc אבג 123 !\n";size_t length=strlen(paragraph),repeats=200000/length+1;
    size_t bytes=length*repeats;char *buffer=malloc(bytes+32);assert(buffer);
    for(size_t i=0;i<repeats;i++) memcpy(buffer+i*length,paragraph,length);
    buffer[bytes]=0;
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=bytes+32,.multiline=true,
                          .history_bytes=bytes*4,.max_history_steps=8});assert(edit);
    cui_textarea_line *rows=calloc(repeats+4,sizeof *rows);assert(rows);
    cui_textarea_state state;assert(cui_textarea_state_init(&state,rows,repeats+4));
    counted_host host={font,0,0};
    cui_context *ui=cui_create((cui_config){.measure_user=&host,.measure_styled=counted_measure,
                                           .text_geometry_line=counted_geometry});assert(ui);
    cui_theme theme=cui_theme_light();cui_textarea_options options={.width=280,.rows=4,.no_wrap=true};
    for(unsigned frame=0;frame<3;frame++) {
        if(frame) state.scroll_y=state.maximum_y;
        unsigned before=host.measured,geometry=host.geometry;
        assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_textarea(ui,92,&theme,cui_string("Large"),edit,&state,options).error==CUI_TEXTAREA_OK);
        cui_end_frame(ui);assert(state.count==repeats+1);
        if(frame) assert(host.measured-before<100 && host.geometry-geometry<=8);
    }
    assert(bytes>200000 && cui_last_error(ui)==CUI_OK);
    cui_focus(ui,92);
    cui_event update={.type=CUI_EVENT_COMPOSITION_UPDATE,.text_data="ד",.text_length=2,
                       .composition_anchor=2,.composition_cursor=2};
    assert(cui_queue_event(ui,update));assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_textarea(ui,92,&theme,cui_string("Large"),edit,&state,options).error==CUI_TEXTAREA_OK);
    cui_end_frame(ui);assert(state.composition->active && strlen(buffer)==bytes);
    unsigned before=host.measured;
    assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_textarea(ui,92,&theme,cui_string("Large"),edit,&state,options).error==CUI_TEXTAREA_OK);
    cui_end_frame(ui);assert(host.measured-before<100);
    update.type=CUI_EVENT_COMPOSITION_COMMIT;assert(cui_queue_event(ui,update));
    assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_textarea(ui,92,&theme,cui_string("Large"),edit,&state,options).changed);
    cui_end_frame(ui);assert(strlen(buffer)==bytes+2 && cui_edit_undo(edit)==CUI_EDIT_OK && strlen(buffer)==bytes);
    cui_destroy(ui);
    host.measured=host.geometry=0;
    ui=cui_create((cui_config){.measure_user=&host,.measure_styled=counted_measure,
                               .text_geometry_line=counted_geometry});assert(ui);
    cui_editor_state editor;assert(cui_editor_state_init(&editor,edit,rows,repeats+4));
    cui_editor_options editor_options={.textarea=options,.line_numbers=true};
    for(unsigned frame=0;frame<3;frame++) {
        if(frame)editor.view.scroll_y=editor.view.maximum_y;
        unsigned before_count=host.measured;
        assert(cui_begin_frame(ui,600,400,.016f));
        assert(cui_editor(ui,96,&theme,cui_string("Large code"),&editor,editor_options).error==CUI_TEXTAREA_OK);
        cui_end_frame(ui);assert(editor.gutter_line_count==repeats+1);
        if(frame)assert(host.measured-before_count<100);
    }
    cui_destroy(ui);cui_edit_destroy(edit);free(rows);free(buffer);
}
static void separator_case(cui_context *ui,cui_font *font) {
    cui_str paragraph=cui_string("אבג\xe2\x80\xa8" "123 !");
    cui_text_line_source source={paragraph,9,14};cui_text_geometry geometry={0};
    assert(cui_font_text_geometry_line(font,source,24,(cui_text_style){0},&geometry)==CUI_TEXT_LAYOUT_OK);
    cui_text_layout *reference=layout(font,paragraph,24,4,5);
    raster actual={0},expected={0};
    assert(cui_font_draw_context_line(font,cui_string("123 !"),source,24,(cui_text_style){0},0,0,glyph,&actual));
    assert(cui_text_layout_draw(reference,0,0,glyph,&expected));
    assert(actual.hash==expected.hash && actual.pixels);
    cui_release_text_geometry(&geometry);cui_text_layout_destroy(reference);
    char buffer[64];memcpy(buffer,paragraph.data,paragraph.length);buffer[paragraph.length]=0;
    cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof buffer,.multiline=true});assert(edit);
    cui_textarea_line rows[8];cui_textarea_state state;assert(cui_textarea_state_init(&state,rows,8));
    cui_theme theme=cui_theme_light();assert(cui_begin_frame(ui,600,400,.016f));
    assert(cui_textarea(ui,93,&theme,cui_string("Soft break"),edit,&state,
                        (cui_textarea_options){.width=280,.no_wrap=true,.rows=3}).error==CUI_TEXTAREA_OK);
    cui_draw_list list=cui_end_frame(ui);assert(state.count==2 && rows[1].start==9 &&
                   rows[1].paragraph_start==0 && rows[1].paragraph_end==14);
    unsigned seen=0;
    for(size_t i=0;i<list.count;i++) if(list.commands[i].type==CUI_DRAW_TEXT) {
        assert(list.commands[i].text_source.paragraph.length==14);seen++;
    }
    assert(seen==2);cui_edit_destroy(edit);
}

static void session_lifetime(const char *primary,cui_font *fallback) {
    cui_font *font=cui_font_open(primary);assert(font && cui_font_set_fallbacks(font,&fallback,1));
    char text[]="אבג 123 !";cui_text_line_source source={cui_string(text),7,12};
    cui_text_geometry geometry={0};
    assert(cui_font_text_geometry_line(font,source,24,(cui_text_style){0},&geometry)==CUI_TEXT_LAYOUT_OK);
    cui_text_geometry preserved=geometry;source.byte_start=1;
    assert(cui_font_text_geometry_line(font,source,24,(cui_text_style){0},&geometry)==CUI_TEXT_LAYOUT_INVALID);
    assert(geometry.session==preserved.session);
    cui_text_caret before[2],after[2];size_t n,m;
    assert(geometry.ops->carets(geometry.session,7,before,2,&n)==CUI_TEXT_LAYOUT_OK);
    memset(text,'x',sizeof text-1);assert(cui_font_set_fallbacks(font,NULL,0));cui_font_close(font);
    assert(geometry.ops->carets(geometry.session,7,after,2,&m)==CUI_TEXT_LAYOUT_OK && n==m);
    for(size_t i=0;i<n;i++)assert(before[i].byte_offset==after[i].byte_offset && before[i].x==after[i].x);
    raster pixels={0};assert(cui_text_layout_draw(geometry.session,0,0,glyph,&pixels) && pixels.pixels);
    cui_release_text_geometry(&geometry);cui_release_text_geometry(&geometry);
}
int main(void) {
    const char *const primary_paths[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf"};
    const char *const hebrew_paths[] = {"/usr/share/fonts/noto/NotoSansHebrew-Regular.ttf"};
    const char *primary = font_path("CUI_TEST_FONT",primary_paths,1);
    const char *hebrew = font_path("CUI_TEST_HEBREW_FONT",hebrew_paths,1);
    if (!primary || !hebrew) return 77;
    cui_font *font = cui_font_open(primary), *fallback = cui_font_open(hebrew);
    assert(font && fallback && cui_font_set_fallbacks(font,&fallback,1));
    cui_text_geometry probe = {0};
    cui_text_line_source source = {cui_string("אבג 123 !"),7,12};
    cui_text_layout_status status = cui_font_text_geometry_line(font,source,24,(cui_text_style){0},&probe);
    if (status == CUI_TEXT_LAYOUT_UNAVAILABLE || status == CUI_TEXT_LAYOUT_MISSING_GLYPH) {
        cui_font_close(font);cui_font_close(fallback);return 77;
    }
    assert(status == CUI_TEXT_LAYOUT_OK);
    cui_release_text_geometry(&probe);
    cui_context *ui = cui_create((cui_config){.width=600,.height=400,.measure=cui_font_measure,
        .measure_styled=cui_font_measure_styled,.font_metrics=cui_font_get_metrics,.measure_user=font,
        .text_geometry=cui_font_text_geometry,.text_geometry_line=cui_font_text_geometry_line});
    assert(ui);
    session_lifetime(primary,fallback);
    caret_audit(ui,font);
    wrap_audit(ui,font);
    editor_case(ui,font,"אבג 123 !");
    ime_case(ui,font);
    affinity_case(ui,font);
    editor_ime_case(ui,font);
    separator_case(ui,font);
    large_case(font);
    cui_font *arabic=cui_font_open("/usr/share/fonts/noto/NotoSansArabic-Regular.ttf");
    if(arabic) {
        cui_font *chain[]={fallback,arabic};assert(cui_font_set_fallbacks(font,chain,2));
        editor_case(ui,font,"السَّلَامُ");
    } else puts("SKIP: Arabic contextual coloring fixture unavailable");
    cui_destroy(ui); cui_font_close(font); cui_font_close(fallback); cui_font_close(arabic);
    return 0;
}
