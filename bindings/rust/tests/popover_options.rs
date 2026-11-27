use cui_builders::*;
use std::{cell::Cell, rc::Rc};
#[test]
fn provider_focus_and_escape() {
    let mut a = App::headless(480., 320.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let trigger = a.child(root, Kind::Button, "account").unwrap();
    a.text(trigger, "Account").unwrap();
    let menu = a.child(root, Kind::Popover, "menu").unwrap();
    a.anchor(menu, trigger).unwrap();
    let close = a.child(menu, Kind::Button, "close").unwrap();
    a.text(close, "Close").unwrap();
    let options = PopoverOptions {
        anchor: PopoverAnchor::TopRight,
        width: 180.,
        height: 120.,
        margin: 8.,
        hide_close: true,
        focus_content: true,
        ..Default::default()
    };
    let supported = a.supports_popover_options();
    assert_eq!(
        supported,
        std::env::var("CUI_POPOVER_UNSUPPORTED").as_deref() != Ok("1")
    );
    if !supported {
        assert_eq!(a.popover_options(menu, Some(options)).err(), Some(Error(8)));
        a.open(menu, true).unwrap();
        a.step(0.016).unwrap();
        return;
    }
    a.popover_options(menu, Some(options)).unwrap();
    let accepted = Rc::new(Cell::new(false));
    let c = accepted.clone();
    a.on_click(close, move || c.set(true)).unwrap();
    a.open(menu, true).unwrap();
    a.step(0.016).unwrap();
    a.step(0.016).unwrap();
    let b = a.bounds(close).unwrap();
    assert!(b.width > 0. && b.x >= 8. && b.y >= 8. && b.x + b.width <= 472.);
    a.key(true, Key::Enter, 0, false).unwrap();
    a.key(false, Key::Enter, 0, false).unwrap();
    a.step(0.016).unwrap();
    a.dispatch().unwrap();
    assert!(accepted.get());
    a.key(true, Key::Escape, 0, false).unwrap();
    a.step(0.016).unwrap();
    assert!(!a.is_open(menu).unwrap());
    a.popover_options(menu, None).unwrap();
}
