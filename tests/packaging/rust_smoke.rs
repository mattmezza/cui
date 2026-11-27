use cui_builders::*;
use std::{cell::Cell, env, rc::Rc};
fn main() -> Result<()> {
    let mut app = if env::var("CUI_PACKAGE_SMOKE_BACKEND").as_deref() == Ok("sdl") {
        App::sdl(480.,240.,"CUI Installed Rust Smoke", &env::var("CUI_PACKAGE_SMOKE_FONT").unwrap())?
    } else { App::headless(480.,240.)? };
    assert!(app.supports_radio_tab_index(), "installed bridge lacks Radio ordering");
    assert!(app.supports_radio_paint(), "installed bridge lacks Radio paint");
    assert!(app.supports_rating_options(), "installed bridge lacks Rating options");
    assert!(app.supports_stepper_fill(), "installed bridge lacks Stepper Fill");
    assert!(app.supports_notification_action(), "installed bridge lacks Notification actions");
    assert!(app.supports_notification_auto_hide(), "installed bridge lacks Notification auto-hide");
    assert!(app.supports_stepper_centered_fill(), "installed bridge lacks Stepper Centered Fill");
    assert!(app.supports_dialog_composition(), "installed bridge lacks Dialog composition");
    assert!(app.supports_popover_secondary_trigger(), "installed bridge lacks Popover right-click");
    assert!(app.supports_stepper_wrapped_fill(), "installed bridge lacks Stepper Wrapped Fill");
    assert!(app.supports_virtual_list_axis(), "installed bridge lacks horizontal VirtualList");
    assert!(app.supports_kbd_style(), "installed bridge lacks Kbd styling");
    assert!(app.supports_responsive_notifications(), "installed bridge lacks responsive notifications");
    assert!(app.supports_virtual_list_rows(), "installed bridge lacks VirtualList live rows");
    assert!(app.supports_disclosure_presentation(), "installed bridge lacks disclosure presentation");
    assert!(app.supports_disclosure_heading(), "installed bridge lacks disclosure headings");
    assert!(app.supports_disclosure_divider(), "installed bridge lacks disclosure dividers");
    assert!(app.supports_carousel_basis(), "installed bridge lacks fractional Carousel slides");
    assert!(app.supports_carousel_navigation(), "installed bridge lacks Carousel navigation");
    assert!(app.supports_kbd_stroke(), "installed bridge lacks typed Kbd");
    assert!(app.supports_spinner_presentation(), "installed bridge lacks rotating Spinner");
    assert!(app.supports_spinner_icon_resources(), "installed bridge lacks shared Spinner artwork");
    let root = app.build_root(Kind::Column,"root")?.finish();
    let clicked = Rc::new(Cell::new(0));
    let callback = clicked.clone();
    let button = app.build_child(root,Kind::Button,"save")?.text("Save ✓")?
        .on_click(move || callback.set(callback.get()+1))?.finish();
    let slides = app.build_child(root, Kind::Carousel, "slides")?.visual_size(240., 100.)?.finish();
    let overview = app.build_child(slides, Kind::Item, "overview")?.text("Overview")?
        .carousel_basis(0.5)?.finish();
    let activity = app.build_child(slides, Kind::Item, "activity")?.text("Activity")?
        .carousel_basis(0.5)?.finish();
    app.build_child(slides, Kind::Item, "settings")?.text("Settings")?
        .carousel_basis(0.5)?.finish();
    let shortcut_style = KbdStrokeOptions {
        modifiers: KbdModifiers::CTRL,
        outline: true,
        accessible_name: "Save shortcut".into(),
        ..Default::default()
    };
    let hint = app.child_kbd_stroke(root, "shortcut", "ß", &shortcut_style)?;
    let reference = app.child_kbd_stroke(root, "reference", "sS", &shortcut_style)?;
    let spinner = app.rotating_spinner(root, "spinner", SpinnerConfig::default())?;
    let shared_spinner = app.rotating_spinner(root, "shared-spinner", SpinnerConfig::default())?;
    let artwork = app.create_icon_resource(
        &IconSource::svg(r#"<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v8H2z"/></svg>"#),
        IconLimits::default(),
    )?;
    for node in [spinner, shared_spinner] {
        app.set_spinner_icon_resource(node, Some(artwork))?;
        assert_eq!(app.spinner_icon_resource(node)?, Some(artwork));
    }
    app.step(0.016)?;
    let expanded = app.bounds(hint)?;
    let expected = app.bounds(reference)?;
    assert!(expanded.width > 0. && expanded.height > 0.);
    assert_eq!((expanded.width, expanded.height), (expected.width, expected.height));
    assert_eq!(app.bounds(activity)?.x - app.bounds(overview)?.x, 128.);
    let bounds = app.bounds(button)?;
    assert!(bounds.width > 0. && bounds.height > 0.);
    app.pointer(Phase::Down,bounds.x+bounds.width/2.,bounds.y+bounds.height/2.)?;
    app.pointer(Phase::Up,bounds.x+bounds.width/2.,bounds.y+bounds.height/2.)?;
    app.step(0.016)?;
    assert_eq!(app.dispatch()?,1);
    assert_eq!(clicked.get(),1);
    let changes = Rc::new(Cell::new(None));
    let observed = changes.clone();
    app.on_change_event(slides, move |event| observed.set(event.selected))?;
    assert_eq!(app.carousel_adjacent(slides, true)?, Some(activity));
    assert_eq!(app.selected(slides)?, Some(overview));
    assert_eq!(app.carousel_step(slides, true)?, Some(activity));
    assert_eq!(changes.get(), None);
    assert_eq!(app.dispatch()?, 1);
    assert_eq!(changes.get(), Some(activity));
    let first = app.spinner_presentation(spinner)?.expect("installed Spinner animation sample");
    app.step(0.2)?;
    let next = app.spinner_presentation(spinner)?.expect("installed Spinner animation sample");
    let bounds = app.bounds(spinner)?;
    assert!(bounds.width > 0. && bounds.height > 0.);
    assert!(next.core_time > first.core_time);
    assert_ne!(next.phase, first.phase);
    assert!(app.release_icon_resource(artwork).is_err());
    for node in [spinner, shared_spinner] {
        app.set_spinner_icon_resource(node, None)?;
        assert_eq!(app.spinner_icon_resource(node)?, None);
    }
    app.release_icon_resource(artwork)?;
    println!("installed Rust bridge smoke passed");
    Ok(())
}
