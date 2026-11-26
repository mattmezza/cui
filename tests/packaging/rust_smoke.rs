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
    assert!(app.supports_virtual_list_rows(), "installed bridge lacks VirtualList live rows");
    assert!(app.supports_disclosure_presentation(), "installed bridge lacks disclosure presentation");
    assert!(app.supports_disclosure_heading(), "installed bridge lacks disclosure headings");
    assert!(app.supports_disclosure_divider(), "installed bridge lacks disclosure dividers");
    assert!(app.supports_carousel_basis(), "installed bridge lacks fractional Carousel slides");
    assert!(app.supports_carousel_navigation(), "installed bridge lacks Carousel navigation");
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
    app.step(0.016)?;
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
    println!("installed Rust bridge smoke passed");
    Ok(())
}
