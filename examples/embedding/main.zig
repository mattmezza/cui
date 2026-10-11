const std = @import("std");
const c = @cImport({
    @cInclude("cui/cui.h");
});

pub fn main() !void {
    var config = std.mem.zeroes(c.cui_config);
    config.width = 320;
    config.height = 200;
    const ui = c.cui_create(config) orelse return error.AllocationFailed;
    defer c.cui_destroy(ui);
    if (!c.cui_begin_frame(ui, 320, 200, 0)) return error.FrameFailed;
    var box = std.mem.zeroes(c.cui_box);
    box.width = -1;
    box.height = -1;
    box.padding = 16;
    c.cui_begin_box(ui, c.cui_id_from(ui, "zig-host"), box);
    c.cui_text(ui, c.cui_string("C ABI from Zig"), 18, .{ .r = 230, .g = 230, .b = 230, .a = 255 });
    c.cui_end_box(ui);
    const list = c.cui_end_frame(ui);
    if (list.count == 0) return error.EmptyDrawList;
    std.debug.print("Zig host consumed {d} C draw commands\n", .{list.count});
}
