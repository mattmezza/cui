use cui_builders::{App, DesignSize, Error, IconLimits, IconSource, Kind, Phase, RatingConfig};
use std::{cell::RefCell, rc::Rc, sync::Mutex};

static UI: Mutex<()> = Mutex::new(());

#[test]
fn optional_provider_keeps_integer_rating() {
    let _guard = UI.lock().unwrap();
    let mut app = App::headless(420., 280.).unwrap();
    let supported = std::env::var("CUI_TEST_UNSUPPORTED_RATING").as_deref() != Ok("1");
    assert_eq!(app.supports_rating_options(), supported);
    if supported {
        return;
    }
    let root = app.root(Kind::Column, "root").unwrap();
    assert_eq!(
        app.configured_rating(root, "rating", &RatingConfig::default())
            .err(),
        Some(Error(9))
    );
    let rating = app.child(root, Kind::Rating, "rating").unwrap();
    let values = Rc::new(RefCell::new(Vec::new()));
    let captured = values.clone();
    app.on_change_event(rating, move |event| captured.borrow_mut().push(event.value))
        .unwrap();
    app.value(rating, 2.5).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(&*values.borrow(), &[3.]);
    assert_eq!(app.rating_config(rating).unwrap(), None);
    assert_eq!(app.reset_rating(rating).err(), Some(Error(9)));
}

#[test]
fn round_trip_masks_and_fractional_change() {
    let _guard = UI.lock().unwrap();
    if std::env::var("CUI_TEST_UNSUPPORTED_RATING").as_deref() == Ok("1") {
        return;
    }
    let mut app = App::headless(520., 320.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let mask = app
        .create_icon_resource(&IconSource::Name("heart".into()), IconLimits::default())
        .unwrap();
    let config = RatingConfig {
        half_steps: true,
        hover_preview: true,
        step_down_filled: true,
        clearable: true,
        design_size: DesignSize::Lg,
        width: 270.,
        fill_rgba: Some(0xd53975ff),
        outline_rgba: Some(0),
        outline_icon: Some(mask),
        fill_icon: Some(mask),
        ..RatingConfig::default()
    };
    let rating = app.configured_rating(root, "rating", &config).unwrap();
    app.text(rating, "Delivery")
        .unwrap()
        .value(rating, 1.5)
        .unwrap();
    assert_eq!(app.rating_config(rating).unwrap(), Some(config));
    assert_eq!(app.icon_info(mask).unwrap().references, 2);
    assert!(app.release_icon_resource(mask).is_err());

    let bad = RatingConfig {
        width: f32::NAN,
        ..config
    };
    assert_eq!(app.configure_rating(rating, &bad).err(), Some(Error(3)));
    let mut other = App::headless(320., 280.).unwrap();
    let foreign = other
        .create_icon_resource(&IconSource::Empty, IconLimits::default())
        .unwrap();
    let bad = RatingConfig {
        fill_icon: Some(foreign),
        ..config
    };
    assert_eq!(app.configure_rating(rating, &bad).err(), Some(Error(3)));
    assert_eq!(app.rating_config(rating).unwrap(), Some(config));
    other.release_icon_resource(foreign).unwrap();
    drop(other);

    let values = Rc::new(RefCell::new(Vec::new()));
    let captured = values.clone();
    app.on_change_event(rating, move |event| captured.borrow_mut().push(event.value))
        .unwrap();
    assert_eq!(app.frame(0.016).unwrap(), 0);
    let mark = app.numeric_part_bounds(rating, 13).unwrap();
    let x = mark.x + mark.width / 4.;
    let y = mark.y + mark.height / 2.;
    app.pointer(Phase::Move, x, y).unwrap();
    assert_eq!(app.frame(0.016).unwrap(), 0);
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, x, y).unwrap();
    }
    assert_eq!(app.frame(0.016).unwrap(), 1);
    assert_eq!(app.dispatch().unwrap(), 0);
    assert_eq!(&*values.borrow(), &[3.5]);
    app.reset_rating(rating).unwrap();
    assert_eq!(app.rating_config(rating).unwrap(), None);
    app.release_icon_resource(mask).unwrap();
    assert_eq!(app.frame(0.016).unwrap(), 1);
    assert_eq!(&*values.borrow(), &[3.5, 4.]);
    app.remove(rating).unwrap();
    assert!(app.rating_config(rating).is_err());

    let another = app
        .build_child(root, Kind::Rating, "builder")
        .unwrap()
        .rating_options(&RatingConfig::default())
        .unwrap()
        .finish();
    assert_eq!(
        app.rating_config(another).unwrap(),
        Some(RatingConfig::default())
    );
}
