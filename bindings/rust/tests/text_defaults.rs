use cui_builders::*;
#[test]
fn scoped_defaults_and_optional_provider() {
    let mut a = App::headless(640., 480.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let row = a.child(root, Kind::Row, "row").unwrap();
    let label = a.child(row, Kind::Label, "label").unwrap();
    a.text(label, "Inherited label").unwrap();
    let icon = a.child(row, Kind::Icon, "icon").unwrap();
    a.text(icon, "check").unwrap();
    a.child(row, Kind::Spinner, "busy").unwrap();
    let supported = a.supports_text_defaults();
    if let Ok(expected) = std::env::var("CUI_EXPECT_TEXT_DEFAULTS") {
        assert_eq!(supported, expected == "1");
    }
    if !supported {
        assert_eq!(
            a.text_defaults(row, TextDefaults::default()).err(),
            Some(Error::TEXT_DEFAULTS_UNAVAILABLE)
        );
        a.step(0.016).unwrap();
        return;
    }
    a.text_defaults(
        root,
        TextDefaults {
            font_size: 24.,
            color_rgba: Some(0),
        },
    )
    .unwrap();
    a.step(0.016).unwrap();
    let before = a.bounds(label).unwrap();
    for size in [-1., 4097., f32::NAN, f32::INFINITY] {
        assert_eq!(
            a.text_defaults(
                row,
                TextDefaults {
                    font_size: size,
                    ..Default::default()
                }
            )
            .err(),
            Some(Error(3))
        );
    }
    assert!(a.text_defaults(label, TextDefaults::default()).is_err());
    a.text_defaults(
        row,
        TextDefaults {
            font_size: 40.,
            ..Default::default()
        },
    )
    .unwrap();
    a.step(0.016).unwrap();
    assert!(a.bounds(label).unwrap().height > before.height);
    a.text_defaults(row, TextDefaults::default()).unwrap();
    a.step(0.016).unwrap();
    assert_eq!(a.bounds(label).unwrap().height, before.height);
}
