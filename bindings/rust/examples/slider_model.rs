//! Complete windowless authored UI, including controlled owner state and rejection.
use cui_builders::*;
use std::{cell::Cell, rc::Rc};
fn main() -> Result<()> {
    let args = std::env::args().collect::<Vec<_>>();
    let single = args.iter().any(|s| s == "--single");
    let vertical = args.iter().any(|s| s == "--vertical");
    let accepting = Rc::new(Cell::new(!args.iter().any(|s| s == "--reject")));
    let owner = Rc::new(Cell::new(if single {
        SliderValue::Single { end: 60. }
    } else {
        SliderValue::Range {
            start: 20.,
            end: 60.,
        }
    }));
    let mut app = App::headless(480., 400.)?;
    app.enable_slider_requests(64)?;
    let root = app.root(Kind::Column, "root")?;
    let heading = app.child(root, Kind::Label, "heading")?;
    app.text(heading, "Controlled Rust selection")?;
    let row = app.child(root, Kind::Row, "row")?;
    let nested = app.child(row, Kind::Column, "nested")?;
    let config = SliderConfig {
        source_generation: 0xfedcba9876543210,
        orientation: if vertical {
            SliderOrientation::Vertical
        } else {
            SliderOrientation::Horizontal
        },
        ..Default::default()
    };
    let slider = app.slider_model(nested, "selection", config, owner.get())?;
    let toggle = app.child(root, Kind::Button, "policy")?;
    app.text(toggle, "Toggle acceptance")?;
    let status = app.child(root, Kind::Label, "status")?;
    app.text(status, "Ready")?;
    let policy = accepting.clone();
    app.on_click(toggle, move || policy.set(!policy.get()))?;
    let confirmed = owner.clone();
    let policy = accepting.clone();
    app.on_slider_request(slider, move |ctx, request| {
        if request.phase == SliderPhase::Update && policy.get() {
            ctx.accept(&request.receipt, request.requested)?;
            confirmed.set(request.requested);
        }
        ctx.text(
            status,
            &format!(
                "phase={:?} requested={:?} owner={:?}",
                request.phase,
                request.requested,
                confirmed.get()
            ),
        )
    })?;
    app.step(0.016)?;
    let b = app
        .slider_part_bounds(slider, SliderPart::EndThumb)?
        .ok_or(Error(8))?;
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)?;
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)?;
    app.frame(0.016)?;
    let before = owner.get();
    app.key(true, if vertical { Key::Up } else { Key::Right }, 0, false)?;
    app.frame(0.016)?;
    assert_eq!(app.slider_value(slider)?, owner.get());
    if accepting.get() {
        assert_ne!(owner.get(), before)
    } else {
        assert_eq!(owner.get(), before)
    }
    println!("PASS Rust-authored controlled UI: {:?}", owner.get());
    Ok(())
}
