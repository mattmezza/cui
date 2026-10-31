use cui_builders::*;
use std::{cell::Cell, rc::Rc};
#[test]
fn owned_root_shadows_clear_reset_geometry_and_input() {
    let mut app = App::headless(360., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let button = app.child(root, Kind::Button, "target").unwrap();
    app.text(button, "Go").unwrap();
    let layer = BoxShadow::css(0., 2., 8., -1., 0x10203040, ShadowKind::Drop).unwrap();
    assert_eq!(layer.sigma, 4.);
    let r = CornerRadii {
        top_left: 3.,
        top_right: 4.,
        bottom_left: 5.,
        bottom_right: 6.,
    };
    let mut source = vec![
        layer,
        BoxShadow {
            kind: ShadowKind::Inset,
            ..layer
        },
    ];
    let style = ShadowStyle::new(&source, r).unwrap();
    source[0].sigma = 200.;
    assert_eq!(style.layers()[0].sigma, 4.);
    app.box_shadows(button, &style).unwrap();
    app.step(0.016).unwrap();
    let before = app.bounds(button).unwrap();
    let calls = Rc::new(Cell::new(0));
    let c = calls.clone();
    app.on_click(button, move || c.set(c.get() + 1)).unwrap();
    app.pointer(
        Phase::Down,
        before.x + before.width / 2.,
        before.y + before.height / 2.,
    )
    .unwrap();
    app.pointer(
        Phase::Up,
        before.x + before.width / 2.,
        before.y + before.height / 2.,
    )
    .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(calls.get(), 1);
    app.box_shadows(button, &ShadowStyle::new(&[], r).unwrap())
        .unwrap();
    app.step(0.016).unwrap();
    app.shadow_preset(button, ShadowPreset::MD, r).unwrap();
    app.step(0.016).unwrap();
    let after = app.bounds(button).unwrap();
    assert_eq!(
        (before.x, before.y, before.width, before.height),
        (after.x, after.y, after.width, after.height)
    );
    app.reset_box_shadows(button).unwrap();
    app.step(0.016).unwrap();
    assert!(ShadowStyle::new(&[layer; 9], r).is_err());
    assert!(ShadowStyle::new(
        &[BoxShadow {
            sigma: f64::INFINITY,
            ..layer
        }],
        r
    )
    .is_err());
    assert!(BoxShadow::css(1e300, 0., 0., 0., 0, ShadowKind::Drop).is_err());
    assert!(BoxShadow::css(0., 0., 513., 0., 0, ShadowKind::Drop).is_err());
    let unsupported = app.child(root, Kind::Checkbox, "no").unwrap();
    assert_eq!(app.box_shadows(unsupported, &style).err(), Some(Error(3)));
    app.remove(button).unwrap();
    assert_eq!(app.reset_box_shadows(button).err(), Some(Error(4)));
}

#[test]
fn optional_old_library_defaults() {
    if std::env::var("CUI_TEST_OLD_SHADOW_BRIDGE").ok().as_deref() != Some("1") {
        return;
    }
    let mut app = App::headless(300., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let button = app.child(root, Kind::Button, "plain").unwrap();
    app.text(button, "Old defaults").unwrap();
    app.step(0.016).unwrap();
    let empty = ShadowStyle::new(&[], CornerRadii::default()).unwrap();
    assert_eq!(app.box_shadows(button, &empty).err(), Some(Error(9)));
    assert_eq!(
        BoxShadow::css(0., 0., 1., 0., 0, ShadowKind::Drop).err(),
        Some(Error(9))
    );
    app.step(0.016).unwrap();
}
