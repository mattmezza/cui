use cui_builders::*;
use std::{cell::RefCell, rc::Rc};
#[test]
fn optional_motion_preserves_selection_and_events() {
    let mut app = App::headless(320., 240.).unwrap();
    let supported = app.supports_radio_motion();
    let expected = std::env::var("CUI_TEST_RADIO_MOTION").as_deref() != Ok("0");
    assert_eq!(supported, expected);
    let root = app.root(Kind::Column, "root").unwrap();
    let group = app
        .build_child(root, Kind::RadioGroup, "delivery")
        .unwrap()
        .animate_selection(supported)
        .unwrap()
        .finish();
    let one = app.child(group, Kind::Item, "standard").unwrap();
    app.text(one, "Standard").unwrap();
    let two = app.child(group, Kind::Item, "priority").unwrap();
    app.text(two, "Priority").unwrap();
    let radio = app.child(root, Kind::Radio, "radio").unwrap();
    app.animate_selection(radio, supported).unwrap();
    app.text(radio, "Receive updates").unwrap();
    assert!(!app.is_checked(radio).unwrap());
    if !supported {
        assert_eq!(app.animate_selection(group, true).err(), Some(Error(9)));
        assert_eq!(app.animate_selection(radio, true).err(), Some(Error(9)));
    }
    assert_eq!(app.animate_selection(one, false).err(), Some(Error(8)));
    assert_eq!(app.animate_selection(root, true).err(), Some(Error(8)));
    app.select(group, one).unwrap();
    let changes = Rc::new(RefCell::new(Vec::new()));
    for node in [group, radio] {
        let changes = changes.clone();
        app.on_change_event(node, move |e| {
            changes.borrow_mut().push((e.node, e.selected, e.checked));
        })
        .unwrap();
    }
    app.step(0.016).unwrap();
    for target in [two, radio] {
        let b = app.bounds(target).unwrap();
        assert!(b.width > 0. && b.height > 0.);
        for phase in [Phase::Down, Phase::Up] {
            app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
                .unwrap();
        }
        app.step(0.016).unwrap();
    }
    assert_eq!(app.selected(group).unwrap(), Some(two));
    assert!(app.is_checked(radio).unwrap());
    assert!(changes.borrow().is_empty());
    app.animate_selection(group, supported).unwrap();
    assert_eq!(app.dispatch().unwrap(), 2);
    let expected = [(group, Some(two), false), (radio, None, true)];
    assert_eq!(&*changes.borrow(), &expected);
    app.animate_selection(group, false).unwrap();
    app.animate_selection(radio, false).unwrap();
    app.step(0.25).unwrap();
    assert_eq!(app.selected(group).unwrap(), Some(two));
    assert_eq!(app.dispatch().unwrap(), 0);
    assert_eq!(&*changes.borrow(), &expected);
    let mut other = App::headless(100., 100.).unwrap();
    assert_eq!(other.animate_selection(group, false).err(), Some(Error(3)));
    let nested = app.child(root, Kind::Column, "nested").unwrap();
    let nested_radio = app.child(nested, Kind::Radio, "nested-radio").unwrap();
    app.remove(nested).unwrap();
    assert_eq!(
        app.animate_selection(nested_radio, false).err(),
        Some(Error(4))
    );
    let replacement = app.child(root, Kind::Checkbox, "replacement").unwrap();
    assert_eq!(
        app.animate_selection(replacement, false).err(),
        Some(Error(8))
    );
    app.remove(group).unwrap();
    assert_eq!(app.animate_selection(group, false).err(), Some(Error(4)));
}

#[test]
fn optional_item_content_keeps_one_radio_target() {
    let mut app = App::headless(320., 240.).unwrap();
    let supported = app.supports_radio_item_content();
    let expected = std::env::var("CUI_TEST_RADIO_MOTION").as_deref() != Ok("0");
    assert_eq!(supported, expected);
    let root = app.root(Kind::Column, "root").unwrap();
    let group = app.child(root, Kind::RadioGroup, "delivery").unwrap();
    let standard = app.child(group, Kind::Item, "standard").unwrap();
    app.text(standard, "Standard").unwrap();
    app.select(group, standard).unwrap();
    let priority = app
        .build_child(group, Kind::Item, "priority")
        .unwrap()
        .text("Priority")
        .unwrap()
        .detail_text("Tracked delivery")
        .unwrap()
        .finish();
    let column = app.child(priority, Kind::Column, "decoration").unwrap();
    let tag = app.child(column, Kind::Tag, "recommended").unwrap();
    app.text(tag, "Recommended").unwrap();
    let control = app.child(column, Kind::Button, "invalid-child").unwrap();
    assert_eq!(app.decorate_content(group, false).err(), Some(Error(8)));
    assert_eq!(
        app.decorate_content(priority, true).err(),
        Some(Error(if supported { 8 } else { 9 }))
    );
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(tag).err(), Some(Error(8)));
    app.remove(control).unwrap();
    app.decorate_content(priority, supported).unwrap();

    let changes = Rc::new(RefCell::new(Vec::new()));
    let events = changes.clone();
    app.on_change_event(group, move |event| {
        events.borrow_mut().push((event.node, event.selected))
    })
    .unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(if supported { tag } else { priority }).unwrap();
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(
            phase,
            bounds.x + bounds.width / 2.,
            bounds.y + bounds.height / 2.,
        )
        .unwrap();
    }
    app.step(0.016).unwrap();
    assert_eq!(app.selected(group).unwrap(), Some(priority));
    assert!(changes.borrow().is_empty());
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*changes.borrow(), &[(group, Some(priority))]);
    app.decorate_content(priority, false).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(tag).err(), Some(Error(8)));
    if supported {
        app.decorate_content(priority, true).unwrap();
        app.step(0.016).unwrap();
        assert!(app.bounds(tag).unwrap().width > 0.);
    }
    assert_eq!(app.dispatch().unwrap(), 0);
    app.remove(group).unwrap();
    assert_eq!(app.decorate_content(priority, false).err(), Some(Error(4)));
}

#[test]
fn optional_tooltips_preserve_selection_events() {
    let mut app = App::headless(320., 240.).unwrap();
    let supported = app.supports_radio_tooltip();
    assert_eq!(
        supported,
        std::env::var("CUI_TEST_RADIO_MOTION").as_deref() != Ok("0")
    );
    let root = app.root(Kind::Column, "root").unwrap();
    let radio = app.child(root, Kind::Radio, "alternate").unwrap();
    app.text(radio, "Save a copy").unwrap();
    let group = app.child(root, Kind::RadioGroup, "delivery").unwrap();
    let standard = app.child(group, Kind::Item, "standard").unwrap();
    app.text(standard, "Standard").unwrap();
    let priority = app
        .build_child(group, Kind::Item, "priority")
        .unwrap()
        .text("Priority")
        .unwrap()
        .radio_tooltip(supported.then_some("Suivre le colis"), 0)
        .unwrap()
        .finish();
    let tabs = app.child(root, Kind::Tabs, "tabs").unwrap();
    let tab = app.child(tabs, Kind::Item, "tab").unwrap();
    assert_eq!(app.radio_tooltip(tab, None, 0).err(), Some(Error(8)));
    assert_eq!(app.radio_tooltip(group, None, 0).err(), Some(Error(8)));
    app.select(group, standard).unwrap();
    if supported {
        app.radio_tooltip(radio, Some("Local file"), 350).unwrap();
        assert_eq!(
            app.radio_tooltip(radio, Some(&"é".repeat(129)), 350).err(),
            Some(Error(5))
        );
        assert_eq!(
            app.radio_tooltip(radio, Some("Invalid\0hint"), 0).err(),
            Some(Error(3))
        );
        assert_eq!(
            app.radio_tooltip(radio, Some("Invalid delay"), 10001).err(),
            Some(Error(3))
        );
    } else {
        assert_eq!(
            app.radio_tooltip(radio, Some("Local file"), 350).err(),
            Some(Error(9))
        );
        assert_eq!(
            app.radio_tooltip(priority, Some("Tracking"), 350).err(),
            Some(Error(9))
        );
    }
    let changes = Rc::new(RefCell::new(Vec::new()));
    let events = changes.clone();
    app.on_change_event(group, move |event| events.borrow_mut().push(event.selected))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(priority).unwrap();
    let (x, y) = (b.x + b.width / 2., b.y + b.height / 2.);
    app.pointer(Phase::Move, x, y).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 0);
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.selected(group).unwrap(), Some(priority));
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*changes.borrow(), &[Some(priority)]);
    if supported {
        app.radio_tooltip(priority, Some("Track shipment"), 350)
            .unwrap();
    }
    app.radio_tooltip(priority, None, u32::MAX).unwrap();
    app.radio_tooltip(radio, Some(""), u32::MAX).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 0);
    app.remove(group).unwrap();
    assert_eq!(app.radio_tooltip(priority, None, 0).err(), Some(Error(4)));
}
