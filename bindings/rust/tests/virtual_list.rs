// SPDX-License-Identifier: MIT
use cui_builders::{App, Error, Kind, Node, VirtualListRow};
use std::{cell::Cell, rc::Rc};

fn expect_row(app: &App, list: Node, item: Node, offset: f64, top: f32, extent: f32) {
    let list_rect = app.bounds(list).unwrap();
    let row = app.virtual_list_item_bounds(list, item).unwrap();
    assert_eq!(app.current_value(list).unwrap(), offset);
    assert_eq!(row.y - list_rect.y, top);
    assert_eq!(row.height, extent);
}

#[test]
fn live_rows_preserve_position_and_input_evidence() {
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let list = app
        .build_child(root, Kind::VirtualList, "inbox")
        .unwrap()
        .visual_size(220., 96.)
        .unwrap()
        .finish();
    let other_list = app.child(root, Kind::VirtualList, "other").unwrap();
    let other_item = app.child(other_list, Kind::Item, "other-item").unwrap();
    let items: Vec<_> = (0..10)
        .map(|i| {
            let label = format!("Message {i}");
            let item = app.child(list, Kind::Item, &label).unwrap();
            app.text(item, &label).unwrap();
            item
        })
        .collect();
    let mut rows: Vec<_> = items
        .iter()
        .map(|item| VirtualListRow {
            item: *item,
            extent: 0.,
        })
        .collect();
    let change = Rc::new(Cell::new(0.));
    let seen = change.clone();
    app.on_change_event(list, move |event| seen.set(event.value))
        .unwrap();
    app.value(list, 109.5).unwrap();
    app.step(0.016).unwrap();
    expect_row(&app, list, items[3], 109.5, -13.5, 32.);
    let supported = app.supports_virtual_list_rows();
    assert_ne!(
        supported,
        std::env::var("CUI_TEST_LEGACY_VIRTUAL_LIST").as_deref() == Ok("1")
    );
    if !supported {
        assert_eq!(
            app.preserve_virtual_list_anchor(list, true).err(),
            Some(Error::VIRTUAL_LIST_ROWS_UNAVAILABLE)
        );
        assert_eq!(
            app.set_virtual_list_rows(list, &rows).err(),
            Some(Error::VIRTUAL_LIST_ROWS_UNAVAILABLE)
        );
        return;
    }

    app.preserve_virtual_list_anchor(list, true).unwrap();
    let new_item = app.child(list, Kind::Item, "new").unwrap();
    app.text(new_item, "New message").unwrap();
    rows.insert(
        0,
        VirtualListRow {
            item: new_item,
            extent: 48.5,
        },
    );
    let mut invalid = rows.clone();
    invalid[10].item = other_item;
    assert_eq!(
        app.set_virtual_list_rows(list, &invalid).err(),
        Some(Error(3))
    );
    invalid[10] = invalid[0];
    assert_eq!(
        app.set_virtual_list_rows(list, &invalid).err(),
        Some(Error(3))
    );
    invalid[10] = rows[10];
    invalid[0].extent = f32::NAN;
    assert_eq!(
        app.set_virtual_list_rows(list, &invalid).err(),
        Some(Error(3))
    );
    assert_eq!(
        app.set_virtual_list_rows(list, &rows[1..]).err(),
        Some(Error(3))
    );
    let mut separate = App::headless(100., 100.).unwrap();
    let foreign = separate.root(Kind::Column, "foreign").unwrap();
    invalid[10].item = foreign;
    invalid[0].extent = 0.;
    assert_eq!(
        app.set_virtual_list_rows(list, &invalid).err(),
        Some(Error(3))
    );

    app.step(0.016).unwrap();
    expect_row(&app, list, items[3], 109.5, -13.5, 32.);
    app.set_virtual_list_rows(list, &rows).unwrap();
    rows[0].extent = 62.5;
    assert_eq!(app.frame(0.016).unwrap(), 0);
    expect_row(&app, list, items[3], 158., -13.5, 32.);
    app.set_virtual_list_rows(list, &rows).unwrap();
    assert_eq!(app.frame(0.016).unwrap(), 0);
    expect_row(&app, list, items[3], 172., -13.5, 32.);
    let bounds = app.bounds(list).unwrap();
    app.scroll(bounds.x + 20., bounds.y + 20., 0., -1.).unwrap();
    assert_eq!(app.frame(0.016).unwrap(), 1);
    expect_row(&app, list, items[4], 204., -13.5, 32.);
    assert_eq!(change.get(), 204.);

    app.remove(items[4]).unwrap();
    rows.remove(5);
    let mut invalid = rows.clone();
    invalid[0].item = items[4];
    assert_eq!(
        app.set_virtual_list_rows(list, &invalid).err(),
        Some(Error(4))
    );
    app.set_virtual_list_rows(list, &rows).unwrap();
    app.step(0.016).unwrap();
    expect_row(&app, list, items[5], 190.5, 0., 32.);
    app.value(list, 7.25).unwrap();
    rows[0].extent = 80.;
    app.set_virtual_list_rows(list, &rows).unwrap();
    app.step(0.016).unwrap();
    expect_row(&app, list, new_item, 7.25, -7.25, 80.);
}

#[test]
fn horizontal_axis_uses_widths_and_preserves_scalar_offset() {
    use cui_builders::VirtualListAxis;
    let mut app = App::headless(400., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let supported = app.supports_virtual_list_axis();
    if let Ok(expected) = std::env::var("CUI_EXPECT_VIRTUAL_AXIS") {
        assert_eq!(supported, expected == "1");
    }
    let builder = app
        .build_child(root, Kind::VirtualList, "rail")
        .unwrap()
        .visual_size(96., 80.)
        .unwrap();
    let list = if supported {
        builder
            .virtual_list_axis(VirtualListAxis::Horizontal)
            .unwrap()
            .finish()
    } else {
        builder.finish()
    };
    if !supported {
        assert_eq!(
            app.virtual_list_axis(list, VirtualListAxis::Horizontal)
                .err(),
            Some(Error::VIRTUAL_LIST_AXIS_UNAVAILABLE)
        );
        return;
    }
    let items: Vec<_> = (0..4)
        .map(|i| app.child(list, Kind::Item, &format!("item-{i}")).unwrap())
        .collect();
    let rows: Vec<_> = items
        .iter()
        .zip([48., 64., 80., 0.])
        .map(|(&item, extent)| VirtualListRow { item, extent })
        .collect();
    app.set_virtual_list_rows(list, &rows).unwrap();
    app.value(list, 20.).unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(list).unwrap();
    let first = app.virtual_list_item_bounds(list, items[0]).unwrap();
    let second = app.virtual_list_item_bounds(list, items[1]).unwrap();
    assert_eq!(first.x - bounds.x, -20.);
    assert_eq!(first.width, 48.);
    assert_eq!(second.x - first.x, 48.);
    assert_eq!(second.width, 64.);
    assert_eq!(first.y, second.y);
    app.virtual_list_axis(list, VirtualListAxis::Vertical)
        .unwrap();
    app.step(0.016).unwrap();
    expect_row(&app, list, items[0], 20., -20., 48.);
}
