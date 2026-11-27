use cui_builders::*;
use std::{cell::Cell, rc::Rc, sync::Mutex};

// The bridge requires calls to separate apps to be serialized as well.
static BRIDGE: Mutex<()> = Mutex::new(());
#[test]
fn provider_focus_and_escape() {
    let _bridge = BRIDGE.lock().unwrap();
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

#[test]
fn secondary_trigger() {
    let _bridge = BRIDGE.lock().unwrap();
    let mut app = App::headless(480., 320.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let menu = app.child(root, Kind::Popover, "menu").unwrap();
    let trigger = app.child(root, Kind::Button, "account").unwrap();
    app.text(trigger, "Account").unwrap();
    app.anchor(menu, trigger).unwrap();
    let expected = [
        "CUI_POPOVER_UNSUPPORTED",
        "CUI_POPOVER_SECONDARY_UNSUPPORTED",
    ]
    .iter()
    .all(|key| std::env::var(key).as_deref() != Ok("1"));
    assert_eq!(app.supports_popover_secondary_trigger(), expected);
    let configured = app.popover_options(
        menu,
        Some(PopoverOptions {
            secondary_trigger: true,
            ..Default::default()
        }),
    );
    if !expected {
        assert_eq!(configured.err(), Some(Error(8)));
        return;
    }
    configured.unwrap();
    let clicks = Rc::new(Cell::new(0));
    let callback = clicks.clone();
    app.on_click(trigger, move || callback.set(callback.get() + 1))
        .unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(trigger).unwrap();
    for open in [true, false] {
        for phase in [Phase::Down, Phase::Up] {
            app.pointer_button(
                phase,
                PointerButton::Secondary,
                bounds.x + bounds.width / 2.,
                bounds.y + bounds.height / 2.,
            )
            .unwrap();
        }
        app.step(0.016).unwrap();
        app.step(0.016).unwrap();
        app.dispatch().unwrap();
        assert_eq!(app.is_open(menu).unwrap(), open);
    }
    assert_eq!(clicks.get(), 0);
}
