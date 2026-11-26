//! Navigate document results. Pass a TrueType font path to open an SDL window.
use cui_builders::{
    App, DesignSize, Kind, Node, PaginationConfig, PaginationOptions, Phase, Result,
};
use std::{cell::Cell, rc::Rc};

const PAGE_COUNT: u32 = 24;

fn show_page(app: &mut App, summary: Node, documents: &[Node], page: u32) -> Result<()> {
    app.text(summary, &format!("Page {page} of {PAGE_COUNT}"))?;
    for (index, document) in documents.iter().enumerate() {
        let number = (page as usize - 1) * documents.len() + index + 1;
        app.text(*document, &format!("Document {number:02}"))?;
    }
    Ok(())
}

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = match font.as_deref() {
        Some(font) => App::sdl(440., 320., "Document results", font)?,
        None => App::headless(440., 320.)?,
    };
    let root = app
        .build_root(Kind::Column, "results")?
        .gap_units(2.)?
        .finish();
    let title = app.child(root, Kind::Label, "title")?;
    app.text(title, "Documents")?;
    let summary = app.child(root, Kind::Label, "summary")?;
    let documents = [
        app.child(root, Kind::Label, "first")?,
        app.child(root, Kind::Label, "second")?,
        app.child(root, Kind::Label, "third")?,
    ];
    show_page(&mut app, summary, &documents, 1)?;

    let pager = PaginationOptions::default()
        .configure(PaginationConfig {
            chevrons: true,
            jump_menu: true,
            visible_pages: 5,
            available_width: 320.,
            group: "Results pages".into(),
            ..Default::default()
        })?
        .design_size(DesignSize::Base)
        .custom_size(52.5)?
        .mount(&mut app, root, "pages")?;
    app.total(pager, PAGE_COUNT)?;

    let pending_page = Rc::new(Cell::new(None));
    let captured_page = pending_page.clone();
    app.on_change_event(pager, move |event| {
        captured_page.set(Some(event.value as u32))
    })?;
    if font.is_some() {
        return app.run_with(|app| {
            if let Some(page) = pending_page.take() {
                show_page(app, summary, &documents, page)?;
            }
            Ok(())
        });
    }

    app.step(0.016)?;
    let next = app.numeric_part_bounds(pager, 3)?;
    assert!(next.width > 0.);
    assert_eq!(next.height, 52.5);
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, next.x + next.width / 2., next.y + next.height / 2.)?;
    }
    assert_eq!(app.frame(0.016)?, 1);
    let page = pending_page.take().expect("page change callback");
    assert_eq!(page, 2);
    show_page(&mut app, summary, &documents, page)?;
    let text = app.editable_text(summary)?;
    let first = app.editable_text(documents[0])?;
    assert_eq!((&*text, &*first), ("Page 2 of 24", "Document 04"));
    println!("{text} {first}");
    Ok(())
}
