//! Asset uploads. Pass a TrueType font path to open the native SDL window.
use cui_builders::{
    App, AttachmentOptions, AttachmentStatus, AttachmentTextStyle, AttachmentTextWeight,
    AttachmentTypography, DesignSize, Kind, Phase, Result,
};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = match font.as_deref() {
        Some(font) => App::sdl(440., 380., "Asset uploads", font)?,
        None => App::headless(440., 380.)?,
    };
    let root = app
        .build_root(Kind::Column, "uploads")?
        .gap_units(3.)?
        .finish();
    app.build_child(root, Kind::Label, "title")?
        .text("Asset uploads")?;
    let card = app
        .build_child(root, Kind::Attachment, "hero")?
        .text("Workspace landing page.png")?
        .detail_text("Uploading")?
        .visual_size(240., 0.)?
        .attachment_status(AttachmentStatus::Uploading)?
        .value(0.4)?
        .finish();
    if app.supports_attachment_options() {
        app.design_size(card, DesignSize::Lg)?
            .configure_attachment(
                card,
                Some(&AttachmentOptions {
                    vertical: true,
                    media: true,
                    media_aspect: 16. / 9.,
                    title: AttachmentTextStyle {
                        typography: Some(AttachmentTypography {
                            weight: AttachmentTextWeight::Semibold,
                            ..Default::default()
                        }),
                        ..Default::default()
                    },
                    ..Default::default()
                }),
            )?;
    }
    if app.supports_attachment_preview() {
        let rgba = [
            38, 84, 166, 255, 87, 187, 170, 255, 108, 125, 207, 255, 239, 219, 173, 255,
        ];
        app.set_attachment_preview_rgba(card, &rgba, 2, 2, 8)?;
    }

    let completed = Rc::new(Cell::new(false));
    let click = completed.clone();
    let button = app
        .build_child(root, Kind::Button, "complete")?
        .text("Complete upload")?
        .on_click(move || click.set(true))?
        .finish();
    let apply = |app: &mut App| -> Result<()> {
        if completed.replace(false) {
            app.attachment_status(card, AttachmentStatus::Complete)?
                .value(card, 1.)?
                .detail_text(card, "Ready to publish")?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(apply);
    }
    app.step(0.016)?;
    let bounds = app.bounds(button)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(
            phase,
            bounds.x + bounds.width / 2.,
            bounds.y + bounds.height / 2.,
        )?;
    }
    assert_eq!(app.frame(0.016)?, 1);
    apply(&mut app)?;
    assert_eq!(app.current_value(card)?, 1.);
    println!("Upload complete");
    Ok(())
}
