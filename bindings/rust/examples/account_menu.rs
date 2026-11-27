// Deterministic headless account menu: cargo run --example account_menu
use cui_builders::*;
use std::{cell::Cell, rc::Rc};
fn main() {
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
    if a.supports_popover_options() {
        a.popover_options(menu, Some(options)).unwrap();
    }
    let accepted = Rc::new(Cell::new(false));
    let c = accepted.clone();
    a.on_click(close, move || c.set(true)).unwrap();
    let opening = Rc::new(Cell::new(false));
    let o = opening.clone();
    a.on_click(trigger, move || o.set(true)).unwrap();
    a.step(0.016).unwrap();
    let b = a.bounds(trigger).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
    a.dispatch().unwrap();
    if opening.replace(false) {
        a.open(menu, true).unwrap();
    }
    a.step(0.016).unwrap();
    a.step(0.016).unwrap();
    let b = a.bounds(close).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
    a.dispatch().unwrap();
    assert!(accepted.get());
    a.open(menu, false).unwrap();
    a.open(menu, true).unwrap();
    a.step(0.016).unwrap();
    a.key(true, Key::Escape, 0, false).unwrap();
    a.step(0.016).unwrap();
    assert!(!a.is_open(menu).unwrap());
    println!("Account menu: Button and Escape dismissal complete");
}
