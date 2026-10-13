/* SPDX-License-Identifier: MIT */
/* Private allocation invariants: this fixture links Core only. */
#include "../../src/core/cui.c"
#include <assert.h>
#include <stdio.h>
typedef struct {
    long double alignment;
    uint64_t tag;
    size_t bytes;
    unsigned *calls;
} parts_header;
static unsigned char expected[1024];
static bool paint_parts(const cui_prepaint_info *info, const void *copied,
                        cui_prepaint_writer *writer) {
    const parts_header *header = copied;
    assert((uintptr_t)copied % _Alignof(parts_header) == 0);
    assert(header->alignment == 1.25L && header->tag == UINT64_C(0x123456789abcdef0));
    assert(header->bytes == sizeof expected);
    assert(!memcmp((const unsigned char *)copied + sizeof *header, expected, header->bytes));
    ++*header->calls;
    cui_draw_command command = {
        .type = CUI_DRAW_RECT, .bounds = info->bounds, .color = {7, 8, 9, 255}};
    return cui_prepaint_emit(writer, &command, false);
}
static bool paint_empty(const cui_prepaint_info *info, const void *copied,
                        cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    assert(!copied);
    return true;
}
static void open_owner(cui_context *ui) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 40});
}
int main(void) {
    for (size_t i = 0; i < sizeof expected; ++i)
        expected[i] = (unsigned char)('a' + i % 26);
    cui_context *ui =
        cui_create((cui_config){.max_elements = 64, .max_commands = 64, .frame_bytes = 4096});
    assert(ui);
    unsigned calls = 0;
    for (int frame = 0; frame < 2; ++frame) {
        open_owner(ui);
        parts_header header = {1.25L, UINT64_C(0x123456789abcdef0), sizeof expected, &calls};
        unsigned char tail[sizeof expected];
        memcpy(tail, expected, sizeof tail);
        cui_prepaint_options options = {.callback = paint_parts,
                                        .payload = &header,
                                        .payload_bytes = sizeof header,
                                        .command_budget = 3};
        size_t used = ui->string_used, count = ui->prepaint_count;
        size_t reserved = ui->prepaint_reserved;
        assert(!cui_set_prepaint_parts(ui, options, NULL, 1));
        assert(ui->string_used == used && ui->prepaint_count == count &&
               ui->prepaint_reserved == reserved);
        cui_prepaint_options overflow = options;
        overflow.payload_bytes = SIZE_MAX;
        assert(!cui_set_prepaint_parts(ui, overflow, tail, 1));
        assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
        assert(ui->string_used == used && ui->prepaint_count == count &&
               ui->prepaint_reserved == reserved);
        cui_clear_error(ui);
        cui_prepaint_options budget = options;
        budget.command_budget = 65;
        assert(!cui_set_prepaint_parts(ui, budget, tail, sizeof tail));
        assert(ui->string_used == used && ui->prepaint_count == count &&
               ui->prepaint_reserved == reserved);
        cui_clear_error(ui);
        /* Failed registration owns no arena bytes, node slot or command budget.
         * The whole failed frame remains invalid; a fresh frame can paint. */
        assert(cui_set_prepaint_parts(ui, options, tail, sizeof tail));
        assert(ui->prepaints[count].options.payload_bytes == sizeof header + sizeof tail);
        assert(ui->prepaint_count == count + 1 && ui->prepaint_reserved == reserved + 3);
        memset(&header, 0, sizeof header);
        memset(tail, 'x', sizeof tail);
        cui_end_box(ui);
        assert(cui_end_frame(ui).count == 0);
    }
    assert(calls == 0);
    open_owner(ui);
    parts_header header = {1.25L, UINT64_C(0x123456789abcdef0), sizeof expected, &calls};
    unsigned char tail[sizeof expected];
    memcpy(tail, expected, sizeof tail);
    assert(cui_set_prepaint_parts(ui,
                                  (cui_prepaint_options){.callback = paint_parts,
                                                         .payload = &header,
                                                         .payload_bytes = sizeof header,
                                                         .command_budget = 3},
                                  tail, sizeof tail));
    memset(&header, 0, sizeof header);
    memset(tail, 'x', sizeof tail);
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 3 && calls == 1 && cui_last_error(ui) == CUI_OK);
    open_owner(ui);
    assert(cui_set_prepaint_parts(
        ui, (cui_prepaint_options){.callback = paint_empty, .command_budget = 3}, NULL, 0));
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    ui = cui_create((cui_config){.max_elements = 64, .max_commands = 64, .frame_bytes = 512});
    assert(ui);
    open_owner(ui);
    size_t used = ui->string_used;
    header = (parts_header){1.25L, UINT64_C(0x123456789abcdef0), sizeof expected, &calls};
    assert(!cui_set_prepaint_parts(ui,
                                   (cui_prepaint_options){.callback = paint_parts,
                                                          .payload = &header,
                                                          .payload_bytes = sizeof header,
                                                          .command_budget = 3},
                                   expected, sizeof expected));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY && ui->string_used == used &&
           ui->prepaint_count == 0 && ui->prepaint_reserved == 0);
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0 && calls == 1);
    cui_destroy(ui);
    puts("prepaint parts owned lifetime alignment and atomic capacity tests passed");
    return 0;
}
