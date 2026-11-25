use cui_builders::{App, DataTableLoadMore, Error, Kind, Node, Phase};

const HEADERS: &[&str] = &["Subject"];
const ROWS: &[&[&str]] = &[
    &["One"],
    &["Two"],
    &["Three"],
    &["Four"],
    &["Five"],
    &["Six"],
    &["Seven"],
    &["Eight"],
];
const KEYS: &[u32] = &[11, 22, 33, 44, 55, 66, 77, u32::MAX];

fn positional_table(app: &mut App) -> Node {
    let root = app.root(Kind::Column, "root").unwrap();
    app.build_child(root, Kind::DataTable, "inbox")
        .unwrap()
        .data_table(HEADERS, ROWS)
        .unwrap()
        .visual_size(320., 160.)
        .unwrap()
        .finish()
}

fn click(app: &mut App, table: Node, row: u32) {
    let r = app.data_table_cell_bounds(table, row, 0).unwrap();
    let (x, y) = (r.x + r.width / 2., r.y + r.height / 2.);
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
}

#[test]
fn demand_is_separate_from_selection_and_preserves_the_visible_row() {
    let request = {
        let mut app = App::headless(480., 320.).unwrap();
        assert!(app.supports_data_table_paging());
        let table = positional_table(&mut app);
        app.data_table_keyed(table, HEADERS, ROWS, KEYS).unwrap();
        app.step(0.016).unwrap();
        assert!(app.poll_data_table_load_more(table).unwrap().is_none());
        app.configure_data_table_load_more(table, true, 32).unwrap();
        app.step(0.016).unwrap();
        let early = app.poll_data_table_load_more(table).unwrap().unwrap();
        assert!(early.visible_end < early.row_count);
        app.configure_data_table_load_more(table, false, 0).unwrap();
        let r = app.data_table_cell_bounds(table, 1, 0).unwrap();
        app.scroll(r.x + 10., r.y + 10., 0., -20.).unwrap();
        app.step(0.016).unwrap();
        assert!(app.poll_data_table_load_more(table).unwrap().is_none());

        app.configure_data_table_load_more(table, true, 0).unwrap();
        app.step(0.016).unwrap();
        app.configure_data_table_load_more(table, true, 0).unwrap();
        assert_eq!(
            app.configure_data_table_load_more(table, false, 33).err(),
            Some(Error(3))
        );
        let selected = app.data_table_cell_bounds(table, 6, 0).unwrap();
        click(&mut app, table, 6);
        app.step(0.016).unwrap();
        let more = app.poll_data_table_load_more(table).unwrap().unwrap();
        assert!(more.frame_sequence > early.frame_sequence);
        assert!(more.visible_begin < more.visible_end);
        assert_eq!(
            (
                more.row_count,
                more.visible_end,
                more.last_row_key,
                more.capacity
            ),
            (8, 8, u32::MAX, 32)
        );
        assert!(app.poll_data_table_load_more(table).unwrap().is_none());
        let change = app.poll_value_notification_v1().unwrap().unwrap();
        let cui_builders::ValueNotificationV1::TableChange(_, change) = change else {
            panic!("expected a selection notification, got {change:?}");
        };
        assert_eq!(change.row, KEYS[6]);

        let rows: Vec<_> = ROWS.iter().copied().chain([&["Nine"][..]]).collect();
        let keys: Vec<_> = KEYS.iter().copied().chain([99]).collect();
        app.configure_data_table_load_more(table, false, 0).unwrap();
        app.data_table_keyed(table, HEADERS, &rows, &keys).unwrap();
        app.step(0.016).unwrap();
        assert_eq!(
            app.data_table_cell_bounds(table, 6, 0).unwrap().y,
            selected.y
        );
        assert_eq!(app.data_table_state(table).unwrap().row, KEYS[6]);
        assert!(app.poll_value_notification_v1().unwrap().is_none());
        assert!(app.poll_data_table_load_more(table).unwrap().is_none());
        app.remove(table).unwrap();
        assert_eq!(app.poll_data_table_load_more(table).err(), Some(Error(4)));
        more
    };
    assert_eq!(request.last_row_key, u32::MAX);
}

#[test]
fn empty_table_requests_an_initial_page_and_requires_ownership_and_columns() {
    let mut app = App::headless(400., 280.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app.child(root, Kind::DataTable, "inbox").unwrap();
    assert!(app.configure_data_table_load_more(table, true, 0).is_err());
    assert!(app.configure_data_table_load_more(root, true, 0).is_err());
    let mut other = App::headless(400., 280.).unwrap();
    assert_eq!(other.poll_data_table_load_more(table).err(), Some(Error(3)));
    app.data_table_keyed(table, HEADERS, &[], &[]).unwrap();
    app.configure_data_table_load_more(table, true, 0).unwrap();
    assert!(app.poll_data_table_load_more(table).unwrap().is_none());
    app.step(0.016).unwrap();
    let initial = app.poll_data_table_load_more(table).unwrap().unwrap();
    assert_eq!(
        initial,
        DataTableLoadMore {
            frame_sequence: 1,
            row_count: 0,
            visible_begin: 0,
            visible_end: 0,
            last_row_key: 0,
            capacity: 32,
        }
    );
}

#[test]
fn incompatible_provider_leaves_positional_tables_usable() {
    if !matches!(
        std::env::var("CUI_DATA_TABLE_PAGING_FIXTURE").as_deref(),
        Ok("legacy" | "partial" | "size")
    ) {
        return;
    }
    let mut app = App::headless(480., 320.).unwrap();
    assert!(!app.supports_data_table_paging());
    let table = positional_table(&mut app);
    assert_eq!(
        app.configure_data_table_load_more(table, true, 0).err(),
        Some(Error(9))
    );
    assert_eq!(app.poll_data_table_load_more(table).err(), Some(Error(9)));
    app.step(0.016).unwrap();
    click(&mut app, table, 0);
    app.frame(0.016).unwrap();
    assert_eq!(app.data_table_state(table).unwrap().row, 1);
    assert_eq!(app.data_table_cell(table, 0, 0).unwrap(), "One");
}

#[test]
fn copied_record_is_validated() {
    let Ok(mode) = std::env::var("CUI_DATA_TABLE_PAGING_FIXTURE") else {
        return;
    };
    let mut app = App::headless(480., 320.).unwrap();
    assert!(app.supports_data_table_paging());
    let table = positional_table(&mut app);
    if mode == "explicit_capacity" {
        app.data_table_keyed_capacity(table, HEADERS, ROWS, KEYS, 48)
            .unwrap();
    }
    app.configure_data_table_load_more(table, true, 0).unwrap();
    app.step(0.016).unwrap();
    let observed = app.poll_data_table_load_more(table);
    if mode == "exact" {
        assert_eq!(
            observed,
            Ok(Some(DataTableLoadMore {
                frame_sequence: u64::MAX - 1,
                row_count: 8,
                visible_begin: 4,
                visible_end: 8,
                last_row_key: u32::MAX,
                capacity: 32,
            }))
        );
    } else {
        assert_eq!(observed, Err(Error(8)));
    }
}
