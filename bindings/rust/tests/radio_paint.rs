use cui_builders::*;
use std::{cell::RefCell, rc::Rc};

#[test]
fn paint_geometry_validation_and_queued_selection() {
    let mut app = App::headless(480., 400.).unwrap();
    let supported = app.supports_radio_paint();
    assert_eq!(
        supported,
        std::env::var("CUI_TEST_RADIO_PAINT").as_deref() != Ok("0")
    );
    let root = app.root(Kind::Column, "root").unwrap();
    let radio = app.child(root, Kind::Radio, "radio").unwrap();
    app.text(radio, "Standalone").unwrap();
    let group = app.child(root, Kind::RadioGroup, "group").unwrap();
    let one = app.child(group, Kind::Item, "one").unwrap();
    app.text(one, "Standard").unwrap();
    let two = app.child(group, Kind::Item, "two").unwrap();
    app.text(two, "Priority").unwrap();
    app.select(group, one).unwrap();
    let events = Rc::new(RefCell::new(Vec::new()));
    let capture = events.clone();
    app.on_change_event(group, move |e| capture.borrow_mut().push(e.selected))
        .unwrap();
    app.step(0.016).unwrap();
    if !supported {
        for node in [radio, group, two] {
            app.radio_paint(node, RadioPaintLayer::Base, None).unwrap();
            app.radio_paint(node, RadioPaintLayer::Base, Some(&RadioPaint::default()))
                .unwrap();
            let nonempty = RadioPaint {
                root_padding: Some(4.),
                ..RadioPaint::default()
            };
            assert_eq!(
                app.radio_paint(node, RadioPaintLayer::Base, Some(&nonempty))
                    .err(),
                Some(Error(9))
            );
        }
        app.step(0.016).unwrap();
        assert_eq!(app.selected(group).unwrap(), Some(one));
        assert_eq!(app.dispatch().unwrap(), 0);
        return;
    }
    let before = app.bounds(two).unwrap();
    let geometry = RadioPaint {
        root_padding: Some(12.),
        indicator_extent: Some(32.),
        ..RadioPaint::default()
    };
    app.radio_paint(two, RadioPaintLayer::Base, Some(&geometry))
        .unwrap();
    app.radio_paint(
        group,
        RadioPaintLayer::Base,
        Some(&RadioPaint {
            root_gap: Some(10.),
            ..RadioPaint::default()
        }),
    )
    .unwrap();
    app.step(0.016).unwrap();
    let after = app.bounds(two).unwrap();
    assert!(after.height > before.height);
    assert!(after.width > before.width);
    for paint in [
        RadioPaint {
            indicator_extent: Some(3.),
            ..RadioPaint::default()
        },
        RadioPaint {
            root_padding: Some(f32::NAN),
            ..RadioPaint::default()
        },
        RadioPaint {
            label_weight: Some(4),
            ..RadioPaint::default()
        },
        RadioPaint {
            root_border_width: Some(65.),
            ..RadioPaint::default()
        },
    ] {
        assert_eq!(
            app.radio_paint(two, RadioPaintLayer::Base, Some(&paint))
                .err(),
            Some(Error(3))
        );
    }
    assert_eq!(
        app.radio_paint(two, RadioPaintLayer::Hovered, Some(&geometry))
            .err(),
        Some(Error(3))
    );
    assert_eq!(
        app.radio_paint(group, RadioPaintLayer::Checked, None).err(),
        Some(Error(3))
    );
    assert_eq!(
        app.radio_paint(
            radio,
            RadioPaintLayer::Base,
            Some(&RadioPaint {
                description_size: Some(12.),
                ..RadioPaint::default()
            })
        )
        .err(),
        Some(Error(3))
    );
    assert_eq!(
        app.radio_paint(root, RadioPaintLayer::Base, None).err(),
        Some(Error(8))
    );
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(two).unwrap().height, after.height);
    let b = app.bounds(two).unwrap();
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
    }
    app.step(0.016).unwrap();
    assert_eq!(app.selected(group).unwrap(), Some(two));
    let zero = RadioPaint {
        root_background_rgba: Some(0),
        root_border_width: Some(0.),
        ..RadioPaint::default()
    };
    app.radio_paint(two, RadioPaintLayer::Checked, Some(&zero))
        .unwrap();
    app.radio_paint(two, RadioPaintLayer::Hovered, Some(&zero))
        .unwrap();
    app.radio_paint(two, RadioPaintLayer::Hovered, None)
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(two).unwrap().height, after.height);
    app.radio_paint(two, RadioPaintLayer::Base, Some(&RadioPaint::default()))
        .unwrap();
    assert!(events.borrow().is_empty());
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*events.borrow(), &[Some(two)]);
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(two).unwrap().height, before.height);
    assert_eq!(app.dispatch().unwrap(), 0);
    app.build_child(root, Kind::Radio, "builder")
        .unwrap()
        .radio_paint(RadioPaintLayer::Base, Some(&zero))
        .unwrap()
        .finish();
    app.remove(two).unwrap();
    assert_eq!(
        app.radio_paint(two, RadioPaintLayer::Base, None).err(),
        Some(Error(4))
    );
}
