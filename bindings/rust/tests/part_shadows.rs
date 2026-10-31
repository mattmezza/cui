use cui_builders::*;
use std::{cell::Cell, rc::Rc};
#[test]
fn semantic_parts() {
    let mut app = App::headless(360., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let check = app.child(root, Kind::Checkbox, "check").unwrap();
    app.text(check, "Check").unwrap();
    let style = ShadowStyle::new(
        &[BoxShadow::css(0., 1., 8., 0., 0x00000080, ShadowKind::Drop).unwrap()],
        CornerRadii::default(),
    )
    .unwrap();
    app.part_box_shadows(check, ShadowPart::Main, &style)
        .unwrap();
    app.step(0.016).unwrap();
    let before = app.bounds(check).unwrap();
    let calls = Rc::new(Cell::new(0));
    let captured = calls.clone();
    app.on_change(check, move || captured.set(captured.get() + 1))
        .unwrap();
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
    let info = app.shadow_target(check, ShadowPart::Main).unwrap();
    assert!(info.supported());
    assert_eq!(info.canonical_part, Some(ShadowPart::Indicator));
    let sw = app.child(root, Kind::Switch, "sw").unwrap();
    app.part_box_shadows(sw, ShadowPart::Track, &style)
        .unwrap()
        .part_box_shadows(sw, ShadowPart::Thumb, &style)
        .unwrap();
    let slider = app.child(root, Kind::Slider, "slider").unwrap();
    assert!(!app
        .shadow_target(slider, ShadowPart::Thumb)
        .unwrap()
        .supported());
    assert_eq!(
        app.part_box_shadows(slider, ShadowPart::Thumb, &style)
            .err(),
        Some(Error(3))
    );
    app.part_shadow_preset(
        check,
        ShadowPart::Root,
        ShadowPreset::SM,
        CornerRadii::default(),
    )
    .unwrap();
    app.reset_part_box_shadows(check, ShadowPart::Indicator)
        .unwrap();
    app.step(0.016).unwrap();
    let after = app.bounds(check).unwrap();
    assert_eq!(
        (after.x, after.y, after.width, after.height),
        (before.x, before.y, before.width, before.height)
    );
    let group = app.child(root, Kind::RadioGroup, "g").unwrap();
    let item = app.child(group, Kind::Item, "a").unwrap();
    app.text(item, "A").unwrap();
    app.part_box_shadows(item, ShadowPart::Main, &style)
        .unwrap();
    app.step(0.016).unwrap();
    assert!(app
        .shadow_target(item, ShadowPart::Indicator)
        .unwrap()
        .supported());
    let tabs = app.child(root, Kind::Tabs, "tabs").unwrap();
    let wrong = app.child(tabs, Kind::Item, "wrong").unwrap();
    assert_eq!(
        app.part_box_shadows(wrong, ShadowPart::Indicator, &style)
            .err(),
        Some(Error(3))
    );
    app.remove(check).unwrap();
    assert_eq!(
        app.reset_part_box_shadows(check, ShadowPart::Main).err(),
        Some(Error(4))
    );
}
#[test]
fn optional_old_library_defaults() {
    if std::env::var("CUI_TEST_OLD_SHADOW_BRIDGE").ok().as_deref() != Some("1") {
        return;
    }
    let mut app = App::headless(300., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let check = app.child(root, Kind::Checkbox, "plain").unwrap();
    app.step(0.016).unwrap();
    let empty = ShadowStyle::new(&[], CornerRadii::default()).unwrap();
    assert_eq!(
        app.part_box_shadows(check, ShadowPart::Main, &empty).err(),
        Some(Error(9))
    );
    app.step(0.016).unwrap();
}
