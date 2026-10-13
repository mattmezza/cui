# Semantic alerts, bubbles and table rows

Alerts select Info, Success, Warning or Error using `cui_alert_options.kind`.
The theme provides independent `info`, `success`, `warning` and `danger` colors;
the border and default SVG icon follow that color. Set `icon_name` to another
named SVG or supply the legacy `icon` text override. The gallery demonstrates
all four kinds, with a controlled, dismissible success alert.

Composable bubbles support `CUI_BUBBLE_INFO`, `CUI_BUBBLE_SUCCESS`,
`CUI_BUBBLE_WARNING` and `CUI_BUBBLE_ERROR`. Each uses a tinted theme color.
Compose arbitrary children between `cui_begin_bubble` and `cui_end_bubble`.

Static Table rows can provide their own background independently of header
semantics. This example alternates application-owned row colors:

```c
cui_table_begin(ui, table_id, &theme, 480);
for (unsigned row = 0; row < 4; row++) {
    cui_table_begin_row_ex(ui, table_id + row + 1, &theme,
        (cui_table_row_options){
            .height = 40, .header = row == 0, .custom_background = true,
            .background = row == 0 ? theme.accent
                : row % 2 ? theme.surface : theme.background});
    /* Compose your cells with cui_table_begin_cell / cui_table_end_cell. */
    cui_table_end_row(ui);
}
cui_table_end(ui);
```

Choose foreground colors appropriate to a custom background. The existing
`cui_table_begin_row` retains its previous default behavior. A DataTable has its
own options and row callback contract; this API applies to the composable Table.
The headless tests verify distinct semantic colors and actual row drawing in
both light and dark themes. These options do not imply complete component parity.
