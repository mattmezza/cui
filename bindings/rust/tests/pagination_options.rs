use cui_builders::{App, DesignSize, Error, Key, Kind, PaginationConfig, PaginationOptions, Phase};
use std::{cell::RefCell, rc::Rc, sync::Mutex};
static UI: Mutex<()> = Mutex::new(());
fn click(app: &mut App, pager: cui_builders::Node, part: u32) {
    let b = app.numeric_part_bounds(pager, part).unwrap();
    assert!(b.width > 0. && b.height > 0.);
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
    }
    app.step(0.016).unwrap();
}
#[test]
fn wrapping_navigation_menu_and_reset() {
    let _serial = UI.lock().unwrap();
    let config = PaginationConfig {
        chevrons: true,
        jump_menu: true,
        visible_pages: 5,
        available_width: 210.,
        group: "Pages de résultats".into(),
        previous: "Précédente".into(),
        ..Default::default()
    };
    let options = PaginationOptions::default()
        .configure(config.clone())
        .unwrap()
        .design_size(DesignSize::Xl);
    assert_eq!(options.get_config(), Some(&config));
    let mut app = App::headless(320., 480.).unwrap();
    assert!(app.supports_pagination_options());
    let root = app.root(Kind::Column, "root").unwrap();
    let pager = options.mount(&mut app, root, "pages").unwrap();
    app.total(pager, 20).unwrap().value(pager, 3.).unwrap();
    let changes = Rc::new(RefCell::new(Vec::new()));
    let captured = changes.clone();
    app.on_change_event(pager, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.pagination_config(pager).unwrap(), Some(config.clone()));
    let previous = app.numeric_part_bounds(pager, 2).unwrap();
    let next = app.numeric_part_bounds(pager, 3).unwrap();
    assert_eq!((previous.height, next.height), (48., 48.));
    assert!(previous.y < next.y);
    assert!(app.numeric_part_bounds(pager, 1).is_err());
    assert_eq!(app.dispatch().unwrap(), 0);
    click(&mut app, pager, 3);
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*changes.borrow(), &[4.]);
    assert_eq!(app.dispatch().unwrap(), 0);
    click(&mut app, pager, 6);
    assert_eq!(app.dispatch().unwrap(), 0);
    app.key(true, Key::Down, 0, false).unwrap();
    app.step(0.016).unwrap();
    app.key(true, Key::Enter, 0, false).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*changes.borrow(), &[4., 7.]);
    let bad = PaginationConfig {
        available_width: f32::NAN,
        ..Default::default()
    };
    assert_eq!(app.configure_pagination(pager, &bad).err(), Some(Error(3)));
    assert_eq!(app.pagination_config(pager).unwrap(), Some(config));
    app.configure_pagination(
        pager,
        &PaginationConfig {
            counter: true,
            icon_only: true,
            ..Default::default()
        },
    )
    .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 0);
    app.reset_pagination(pager)
        .unwrap()
        .design_size(pager, DesignSize::Inherit)
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.pagination_config(pager).unwrap(), None);
    assert!(app.numeric_part_bounds(pager, 1).unwrap().width > 0.);
    assert_eq!(app.dispatch().unwrap(), 0);
    assert!(app
        .configure_pagination(root, &PaginationConfig::default())
        .is_err());
    app.remove(pager).unwrap();
    assert!(app.pagination_config(pager).is_err());
}
#[test]
fn mounted_builder_and_foreign_handles() {
    let _serial = UI.lock().unwrap();
    let mut app = App::headless(440., 320.).unwrap();
    let mut other = App::headless(440., 320.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let foreign = other.root(Kind::Column, "other").unwrap();
    let options = PaginationOptions::default()
        .configure(PaginationConfig::default())
        .unwrap();
    assert!(options.mount(&mut app, foreign, "pages").is_err());
    let node = app
        .build_child(root, Kind::Pagination, "pages")
        .unwrap()
        .pagination_options(&PaginationConfig::default())
        .unwrap()
        .finish();
    assert!(other
        .configure_pagination(node, &PaginationConfig::default())
        .is_err());
    assert!(other.pagination_config(node).is_err());
    assert!(other.reset_pagination(node).is_err());
    assert_eq!(
        app.pagination_config(node).unwrap(),
        Some(PaginationConfig::default())
    );
}

#[test]
fn eager_validation_and_owned_options() {
    for config in [
        PaginationConfig {
            visible_pages: 4,
            ..Default::default()
        },
        PaginationConfig {
            visible_pages: 33,
            ..Default::default()
        },
        PaginationConfig {
            available_width: f32::INFINITY,
            ..Default::default()
        },
        PaginationConfig {
            available_width: -1.,
            ..Default::default()
        },
        PaginationConfig {
            next: "é".repeat(64),
            ..Default::default()
        },
        PaginationConfig {
            next: "Page\u{2028}Next".into(),
            ..Default::default()
        },
        PaginationConfig {
            next: "Page\0Next".into(),
            ..Default::default()
        },
    ] {
        assert!(PaginationOptions::default().configure(config).is_err());
    }
    let mut config = PaginationConfig {
        next: "a".repeat(127),
        visible_pages: 32,
        ..Default::default()
    };
    let options = PaginationOptions::default()
        .configure(config.clone())
        .unwrap();
    config.next.clear();
    assert_eq!(options.get_config().unwrap().next.len(), 127);
    assert!(options.reset().get_config().is_none());
}
#[test]
fn optional_provider_preserves_legacy() {
    let unsupported = std::env::var("CUI_TEST_UNSUPPORTED_PAGINATION").as_deref() == Ok("1");
    let _serial = UI.lock().unwrap();
    let mut app = App::headless(320., 480.).unwrap();
    assert_eq!(app.supports_pagination_options(), !unsupported);
    let root = app.root(Kind::Column, "root").unwrap();
    if unsupported {
        for options in [
            PaginationOptions::default()
                .configure(PaginationConfig::default())
                .unwrap(),
            PaginationOptions::default().design_size(DesignSize::Xl),
        ] {
            assert_eq!(options.mount(&mut app, root, "pages").err(), Some(Error(9)));
        }
    }
    let pager = app
        .build_child(root, Kind::Pagination, "pages")
        .unwrap()
        .finish();
    app.total(pager, 20).unwrap();
    assert_eq!(app.pagination_config(pager).unwrap(), None);
    if unsupported {
        assert_eq!(
            app.configure_pagination(pager, &PaginationConfig::default())
                .err(),
            Some(Error(9))
        );
        assert_eq!(app.reset_pagination(pager).err(), Some(Error(9)));
    }
    let changes = Rc::new(RefCell::new(Vec::new()));
    let captured = changes.clone();
    app.on_change_event(pager, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    app.step(0.016).unwrap();
    click(&mut app, pager, 3);
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(&*changes.borrow(), &[2.]);
}
