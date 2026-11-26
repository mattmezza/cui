//! A file card with an ordinary Inspect button and a sibling summary.
//! This example runs headlessly unless given a TTF font as its first argument.
use cui_builders::{App, AttachmentSlot, AttachmentStatus, Error, Kind, Phase, Result};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = if let Some(font) = &font {
        App::sdl(500., 240., "Project files", font)?
    } else {
        App::headless(500., 240.)?
    };
    if !app.supports_attachment_slots() {
        return Err(Error::ATTACHMENT_SLOTS_UNAVAILABLE);
    }
    let root = app
        .build_root(Kind::Column, "files")?
        .gap_units(3.)?
        .finish();
    app.build_child(root, Kind::Label, "heading")?
        .text("Project files")?;
    let card = app
        .build_child(root, Kind::Attachment, "report")?
        .text("Quarterly report.pdf")?
        .detail_text("1.4 MB")?
        .visual_size(380., 0.)?
        .attachment_status(AttachmentStatus::Complete)?
        .finish();
    let preview = app.create_attachment_slot(card, AttachmentSlot::Media, "preview")?;
    app.build_child(preview, Kind::Label, "format")?
        .text("PDF")?;
    let metadata = app.create_attachment_slot(card, AttachmentSlot::Metadata, "metadata")?;
    app.build_child(metadata, Kind::Label, "owner")?
        .text("Finance team")?;
    let actions = app
        .build_attachment_slot(
            card,
            AttachmentSlot::Actions {
                preferred_width: 100.,
            },
            "actions",
        )?
        .gap_px(4.)?
        .finish();
    let pending = Rc::new(Cell::new(0));
    let inspect = app
        .build_child(actions, Kind::Button, "inspect")?
        .text("Inspect")?
        .on_click({
            let pending = Rc::clone(&pending);
            move || pending.set(pending.get() + 1)
        })?
        .finish();
    let summary = app
        .build_child(root, Kind::Label, "summary")?
        .text("Select Inspect for file details.")?
        .finish();
    let message = "Quarterly report.pdf · Finance team · 1.4 MB";
    let update = |app: &mut App| -> Result<()> {
        if pending.replace(0) > 0 {
            app.text(summary, message)?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(update);
    }
    app.step(0.016)?;
    let b = app.bounds(inspect)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)?;
    }
    app.step(0.016)?;
    assert_eq!(app.dispatch()?, 1);
    assert_eq!(pending.get(), 1);
    update(&mut app)?;
    assert_eq!(app.editable_text(summary)?, message);
    println!("Inspect updated the file summary once.");
    Ok(())
}
