//! A headless review panel using real input and the shared event queue.
use cui_builders::{
    App, AttachmentAction, AttachmentActions, AttachmentStatus, ComponentNotificationV1, Kind,
    Node, Phase, Result,
};

fn main() -> Result<()> {
    let mut app = App::headless(420., 280.)?;
    let root = app
        .build_root(Kind::Column, "review")?
        .gap_units(3.)?
        .finish();
    let file = app
        .build_child(root, Kind::Attachment, "report")?
        .text("Report.pdf")?
        .detail_text("Ready for review")?
        .visual_size(220., 0.)?
        .attachment_status(AttachmentStatus::Complete)?
        .attachment_actions(AttachmentActions::ALL)?
        .finish();
    let status = app
        .build_child(root, Kind::Label, "status")?
        .text("Select a file action")?
        .finish();
    app.step(0.016)?;
    let bounds = app.bounds(file)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(
            phase,
            bounds.x + bounds.width / 2.,
            bounds.y + bounds.height / 2.,
        )?;
    }
    app.step(0.016)?;
    while let Some(notice) = app.poll_component_notification_v1()? {
        if let ComponentNotificationV1::AttachmentAction(action) = notice {
            apply_file_action(&mut app, file, status, action.action)?;
        }
    }
    let text = app.editable_text(status)?;
    assert_eq!(text, "Preview requested for Report.pdf");
    println!("{text}");
    Ok(())
}

fn apply_file_action(
    app: &mut App,
    file: Node,
    status: Node,
    action: AttachmentAction,
) -> Result<()> {
    let message = match action {
        AttachmentAction::Open => "Preview requested for Report.pdf",
        AttachmentAction::Remove => {
            app.remove(file)?;
            "Removed Report.pdf from the review"
        }
        AttachmentAction::Retry => {
            app.attachment_status(file, AttachmentStatus::Uploading)?
                .value(file, 0.)?
                .detail_text(file, "Retry queued")?;
            "Upload retry requested"
        }
    };
    app.text(status, message)?;
    Ok(())
}
