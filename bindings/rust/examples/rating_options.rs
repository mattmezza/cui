//! Delivery feedback. Supply a TrueType font path to open an SDL window.
use cui_builders::{App, DesignSize, IconLimits, IconSource, Kind, Phase, RatingConfig, Result};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = match font.as_deref() {
        Some(font) => App::sdl(480., 280., "Delivery feedback", font)?,
        None => App::headless(480., 280.)?,
    };
    let root = app
        .build_root(Kind::Column, "feedback")?
        .gap_units(2.)?
        .finish();
    let title = app.child(root, Kind::Label, "title")?;
    app.text(title, "How was your delivery?")?;
    let supported = app.supports_rating_options();
    let (rating, initial) = if supported {
        let heart =
            app.create_icon_resource(&IconSource::Name("heart".into()), IconLimits::default())?;
        let rating = app.configured_rating(
            root,
            "rating",
            &RatingConfig {
                half_steps: true,
                hover_preview: true,
                clearable: true,
                design_size: DesignSize::Lg,
                fill_rgba: Some(0xd53975ff),
                outline_icon: Some(heart),
                fill_icon: Some(heart),
                ..RatingConfig::default()
            },
        )?;
        (rating, 2.5)
    } else {
        (app.child(root, Kind::Rating, "rating")?, 2.)
    };
    app.text(rating, "Rate this delivery")?
        .value(rating, initial)?;
    let summary = app.child(root, Kind::Label, "summary")?;
    app.text(summary, &format!("Your rating is {initial:.1} of 5"))?;
    let pending = Rc::new(Cell::new(None));
    let captured = pending.clone();
    app.on_change_event(rating, move |event| captured.set(Some(event.value)))?;
    let update_summary = |app: &mut App| -> Result<()> {
        if let Some(value) = pending.take() {
            app.text(summary, &format!("Your rating is {value:.1} of 5"))?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(update_summary);
    }
    app.step(0.016)?;
    let mark = app.numeric_part_bounds(rating, 13)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, mark.x + mark.width / 4., mark.y + mark.height / 2.)?;
    }
    assert_eq!(app.frame(0.016)?, 1);
    update_summary(&mut app)?;
    let text = app.editable_text(summary)?;
    assert_eq!(
        text,
        if supported {
            "Your rating is 3.5 of 5"
        } else {
            "Your rating is 4.0 of 5"
        }
    );
    println!("{text}");
    Ok(())
}
