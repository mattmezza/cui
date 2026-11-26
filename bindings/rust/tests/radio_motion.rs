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
