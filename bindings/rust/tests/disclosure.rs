use cui_builders::{
    App, DesignSize, DisclosureCard, DisclosureConfig, DisclosureCursor, DisclosureTriggerStyle,
    Error, Kind, Node, Phase,
};
use std::{cell::RefCell, rc::Rc};

fn click(app: &mut App, trigger: Node) {
    let bounds = app.bounds(trigger).unwrap();
    assert!(bounds.width > 0. && bounds.height > 0.);
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, bounds.x + 20., bounds.y + 12.).unwrap();
    }
    app.step(0.016).unwrap();
}

#[test]
fn retained_disclosure_presentation() {
    let mut app = App::headless(540., 320.).unwrap();
    let supported = std::env::var("CUI_TEST_LEGACY_DISCLOSURE").as_deref() != Ok("1");
    assert_eq!(app.supports_disclosure_presentation(), supported);
    let root = app.root(Kind::Column, "root").unwrap();
    if !supported {
        let fold = app.child(root, Kind::Collapsible, "fold").unwrap();
        app.text(fold, "Advanced").unwrap();
        let label = app.child(fold, Kind::Label, "cache").unwrap();
        app.text(label, "Cache preferences").unwrap();
        assert_eq!(
            app.configure_disclosure(fold, None).err(),
            Some(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)
        );
        assert_eq!(
            app.set_disclosure_trigger_style(fold, None).err(),
            Some(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)
        );
        app.step(0.016).unwrap();
        click(&mut app, fold);
        assert!(app.is_checked(fold).unwrap());
        return;
    }

    let name = String::from("plus");
    let group = app
        .build_child(root, Kind::Accordion, "settings")
        .unwrap()
        .disclosure_options(&DisclosureConfig {
            design_size: DesignSize::Lg,
            width: 320.,
            joined: true,
            bordered: true,
            single: true,
            allow_collapse: true,
            card: Some(DisclosureCard {
                padding: 8.,
                radius: 10.,
                clip: true,
                ..Default::default()
            }),
            collapsed_icon: &name,
            expanded_icon: "minus",
            ..Default::default()
        })
        .unwrap()
        .finish();
    drop(name); // The provider owns the icon name bytes after submission.
    let changes = Rc::new(RefCell::new(Vec::new()));
    let mut items = Vec::new();
    for (key, title) in [("account", "Account"), ("editor", "Editor")] {
        let item = app.child(group, Kind::Item, key).unwrap();
        app.text(item, title).unwrap();
        let body = app
            .child(item, Kind::Label, &format!("{key}-body"))
            .unwrap();
        app.text(body, "Workspace preferences").unwrap();
        let captured = changes.clone();
        app.on_change_event(item, move |e| {
            captured.borrow_mut().push((e.node, e.checked))
        })
        .unwrap();
        items.push(item);
    }
    let [account, editor] = items[..] else {
        unreachable!()
    };
    app.set_disclosure_trigger_style(
        editor,
        Some(&DisclosureTriggerStyle {
            hover_rgba: Some(0),
            cursor: Some(DisclosureCursor::Pointer),
            ..Default::default()
        }),
    )
    .unwrap();
    app.checked(account, true).unwrap();
    app.frame(0.016).unwrap();
    changes.borrow_mut().clear();
    assert_eq!(app.bounds(group).unwrap().width, 320.);
    assert!(app.configure_disclosure(editor, None).is_err());
    assert!(app
        .configure_disclosure(
            group,
            Some(&DisclosureConfig {
                collapsed_icon: "absent-catalog-icon",
                ..Default::default()
            })
        )
        .is_err());
    click(&mut app, editor);
    assert!(changes.borrow().is_empty());
    app.configure_disclosure(group, None).unwrap();
    app.set_disclosure_trigger_style(editor, None).unwrap();
    assert_eq!(app.dispatch().unwrap(), 2);
    assert_eq!(&*changes.borrow(), &[(account, false), (editor, true)]);
    assert!(app.is_checked(editor).unwrap());
    assert_eq!(app.dispatch().unwrap(), 0);
    app.remove(editor).unwrap();
    assert!(app.set_disclosure_trigger_style(editor, None).is_err());
}
