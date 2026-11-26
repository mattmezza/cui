// SPDX-License-Identifier: MIT
use cui_builders::{
    App, AttachmentOptions, AttachmentSlot, AttachmentStatus, AttachmentTextStyle,
    AttachmentTextWeight, AttachmentTypography, DesignSize, Error, Kind,
};

#[test]
fn retained_attachment_slots() {
    use cui_builders::Phase;
    use std::{cell::Cell, rc::Rc};

    let mut app = App::headless(480., 300.).unwrap();
    let current = std::env::var("CUI_TEST_LEGACY_ATTACHMENT").as_deref() != Ok("1");
    assert_eq!(app.supports_attachment_slots(), current);
    let root = app.root(Kind::Column, "root").unwrap();
    let card = app
        .build_child(root, Kind::Attachment, "file")
        .unwrap()
        .text("Report.pdf")
        .unwrap()
        .visual_size(340., 0.)
        .unwrap()
        .attachment_status(AttachmentStatus::Complete)
        .unwrap()
        .finish();

    if !current {
        assert_eq!(
            app.create_attachment_slot(card, AttachmentSlot::Media, "media")
                .err(),
            Some(Error::ATTACHMENT_SLOTS_UNAVAILABLE)
        );
        let sibling = app.child(root, Kind::Button, "media").unwrap();
        app.text(sibling, "Ordinary button").unwrap();
        app.step(0.016).unwrap();
        assert!(app.bounds(card).unwrap().width > 0.);
        assert!(app.bounds(sibling).unwrap().width > 0.);
        return;
    }

    assert_eq!(
        app.create_attachment_slot(root, AttachmentSlot::Media, "invalid")
            .err(),
        Some(Error(8))
    );
    for preferred_width in [0., -1., 4097., f32::NAN, f32::INFINITY] {
        assert_eq!(
            app.create_attachment_slot(
                card,
                AttachmentSlot::Actions { preferred_width },
                "invalid"
            )
            .err(),
            Some(Error(3))
        );
    }
    let media = app
        .create_attachment_slot(card, AttachmentSlot::Media, "media")
        .unwrap();
    assert_eq!(
        app.create_attachment_slot(card, AttachmentSlot::Media, "duplicate")
            .err(),
        Some(Error(8))
    );
    let overlay = app
        .create_attachment_slot(card, AttachmentSlot::Overlay, "overlay")
        .unwrap();
    let metadata = app
        .create_attachment_slot(card, AttachmentSlot::Metadata, "metadata")
        .unwrap();
    let actions = app
        .build_attachment_slot(
            card,
            AttachmentSlot::Actions {
                preferred_width: 100.,
            },
            "actions",
        )
        .unwrap()
        .gap_px(8.)
        .unwrap()
        .finish();
    let fallback = app.child(media, Kind::Label, "PDF").unwrap();
    let logo = app.child(overlay, Kind::Label, "stamp").unwrap();
    let author = app.child(metadata, Kind::Label, "author").unwrap();
    app.text(fallback, "PDF")
        .unwrap()
        .text(logo, "1")
        .unwrap()
        .text(author, "Finance")
        .unwrap();
    let clicks = Rc::new(Cell::new(0));
    let capture = Rc::new(());
    let weak = Rc::downgrade(&capture);
    let inspect = app
        .build_child(actions, Kind::Button, "inspect")
        .unwrap()
        .text("Inspect")
        .unwrap()
        .on_click({
            let clicks = Rc::clone(&clicks);
            move || {
                let _ = &capture;
                clicks.set(clicks.get() + 1);
            }
        })
        .unwrap()
        .finish();
    let outside = app
        .build_child(root, Kind::Button, "outside")
        .unwrap()
        .text("Outside")
        .unwrap()
        .on_click({
            let clicks = Rc::clone(&clicks);
            move || clicks.set(clicks.get() + 10)
        })
        .unwrap()
        .finish();
    let queue_click = |app: &mut App, node| {
        let b = app.bounds(node).unwrap();
        for phase in [Phase::Down, Phase::Up] {
            app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
                .unwrap();
        }
    };
    app.step(0.016).unwrap();
    let preview = app.bounds(media).unwrap();
    let layered = app.bounds(overlay).unwrap();
    assert_eq!(
        (preview.x, preview.y, preview.width, preview.height),
        (layered.x, layered.y, layered.width, layered.height)
    );
    assert_eq!(app.bounds(actions).unwrap().width, 100.);

    queue_click(&mut app, inspect);
    app.component_disabled(actions, true).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 0);
    app.component_disabled(actions, false).unwrap();
    app.step(0.016).unwrap();
    queue_click(&mut app, inspect);
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(clicks.get(), 1);

    queue_click(&mut app, inspect);
    app.step(0.016).unwrap();
    assert!(weak.upgrade().is_some());
    app.remove(card).unwrap();
    assert!(weak.upgrade().is_none());
    assert_eq!(app.bounds(actions).err(), Some(Error(4)));
    assert_eq!(app.bounds(inspect).err(), Some(Error(4)));
    assert_eq!(app.dispatch().unwrap(), 0);
    app.step(0.016).unwrap();
    queue_click(&mut app, outside);
    app.step(0.016).unwrap();
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(clicks.get(), 11);
}

#[test]
fn retained_attachment_presentation() {
    let mut app = App::headless(430., 300.).unwrap();
    let want = std::env::var("CUI_TEST_LEGACY_ATTACHMENT").as_deref() != Ok("1");
    assert_eq!(app.supports_attachment_options(), want);
    assert_eq!(app.supports_attachment_preview(), want);
    let root = app.root(Kind::Column, "root").unwrap();
    let file = app
        .build_child(root, Kind::Attachment, "file")
        .unwrap()
        .text("Quarterly-report-with-a-very-long-name.pdf")
        .unwrap()
        .detail_text("Uploading")
        .unwrap()
        .visual_size(200., 0.)
        .unwrap()
        .attachment_status(AttachmentStatus::Uploading)
        .unwrap()
        .value(0.4)
        .unwrap();

    if !want {
        let node = file.finish();
        assert_eq!(
            app.configure_attachment(node, Some(&AttachmentOptions::default()))
                .err(),
            Some(Error::ATTACHMENT_OPTIONS_UNAVAILABLE)
        );
        assert_eq!(
            app.clear_attachment_preview(node).err(),
            Some(Error::ATTACHMENT_OPTIONS_UNAVAILABLE)
        );
        app.step(0.016).unwrap();
        assert!(app.bounds(node).unwrap().width > 0.);
        assert_eq!(app.current_value(node).unwrap(), 0.4);
        return;
    }

    let mut pixels = [255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0];
    let options = AttachmentOptions {
        vertical: true,
        media_aspect: 2.,
        title_status: Some(AttachmentStatus::Failed),
        title: AttachmentTextStyle {
            font_size: 17.,
            color_rgba: Some(0x5c39adff),
            typography: Some(AttachmentTypography {
                weight: AttachmentTextWeight::Semibold,
                ..Default::default()
            }),
        },
        ..Default::default()
    };
    let node = file
        .design_size(DesignSize::Xl)
        .unwrap()
        .attachment_options(&options)
        .unwrap()
        .attachment_preview_rgba(&pixels, 2, 1, 12)
        .unwrap()
        .finish();
    pixels.fill(0);
    app.step(0.016).unwrap();
    let wide = app.bounds(node).unwrap();
    assert_eq!(wide.width, 200.);
    assert!(wide.height > 100.);
    let broken = AttachmentOptions {
        media_aspect: f32::NAN,
        ..options
    };
    assert_eq!(
        app.configure_attachment(node, Some(&broken)).err(),
        Some(Error(3))
    );
    assert_eq!(app.text(node, "line\nbreak").err(), Some(Error(3)));
    assert_eq!(
        app.set_attachment_preview_rgba(node, &pixels[..4], 2, 1, 12)
            .err(),
        Some(Error(3))
    );
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(node).unwrap().height, wide.height);
    let flat = AttachmentOptions {
        media_aspect: 4.,
        ..options
    };
    app.configure_attachment(node, Some(&flat)).unwrap();
    app.set_attachment_preview_rgba(node, &pixels, 2, 1, 12)
        .unwrap();
    app.step(0.016).unwrap();
    let short = app.bounds(node).unwrap();
    assert!(short.height < wide.height - 20.);
    app.clear_attachment_preview(node).unwrap();
    app.step(0.016).unwrap();
    assert!(app.bounds(node).unwrap().height < short.height - 20.);
    app.configure_attachment(node, None).unwrap();
    app.text(node, "line\nbreak").unwrap();
    assert_eq!(app.current_value(node).unwrap(), 0.4);
    app.remove(node).unwrap();
    assert!(app.clear_attachment_preview(node).is_err());
}

#[test]
fn retained_attachment_actions() {
    use cui_builders::{
        AttachmentAction, AttachmentActionPart, AttachmentActionSource, AttachmentActions,
        ComponentNotificationV1, Key, Phase, ValueNotificationV1,
    };
    let mut app = App::headless(430., 300.).unwrap();
    let want = std::env::var("CUI_TEST_LEGACY_ATTACHMENT").as_deref() != Ok("1");
    assert_eq!(app.supports_attachment_actions(), want);
    let root = app.root(Kind::Column, "root").unwrap();
    let file = app
        .build_child(root, Kind::Attachment, "file")
        .unwrap()
        .text("Report.pdf")
        .unwrap()
        .detail_text("Ready to review")
        .unwrap()
        .visual_size(220., 0.)
        .unwrap()
        .attachment_status(AttachmentStatus::Complete)
        .unwrap()
        .finish();
    let button = app
        .build_child(root, Kind::Button, "refresh")
        .unwrap()
        .text("Refresh")
        .unwrap()
        .finish();
    let click = |app: &mut App, node| {
        let b = app.bounds(node).unwrap();
        for phase in [Phase::Down, Phase::Up] {
            app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
                .unwrap();
        }
    };
    let key = |app: &mut App, code| {
        app.key(true, code, 0, false).unwrap();
        app.key(false, code, 0, false).unwrap();
    };
    if !want {
        assert_eq!(
            app.set_attachment_actions(file, AttachmentActions::ALL)
                .err(),
            Some(Error::ATTACHMENT_ACTIONS_UNAVAILABLE)
        );
    } else {
        app.set_attachment_actions(file, AttachmentActions::ALL)
            .unwrap();
    }
    app.step(0.016).unwrap();
    click(&mut app, button);
    app.step(0.016).unwrap();
    if !want {
        assert_eq!(
            app.poll_component_notification_v1().err(),
            Some(Error::ATTACHMENT_ACTIONS_UNAVAILABLE)
        );
        assert!(matches!(
            app.poll_value_notification_v1().unwrap(),
            Some(ValueNotificationV1::Activation(meta)) if meta.node == Some(button)
        ));
        return;
    }
    assert!(matches!(
        app.poll_component_notification_v1().unwrap(),
        Some(ComponentNotificationV1::Value(ValueNotificationV1::Activation(meta)))
            if meta.node == Some(button)
    ));
    click(&mut app, file);
    app.step(0.016).unwrap();
    let Some(ComponentNotificationV1::AttachmentAction(opened)) =
        app.poll_component_notification_v1().unwrap()
    else {
        panic!("expected typed Open")
    };
    assert_eq!(opened.action, AttachmentAction::Open);
    assert_eq!(opened.part, Some(AttachmentActionPart::Card));
    assert_eq!(opened.source, Some(AttachmentActionSource::Pointer));
    assert_eq!(opened.event_index, Some(1));
    assert_eq!(opened.metadata.node, Some(file));
    assert!(opened.metadata.frame > 0);
    key(&mut app, Key::Enter);
    app.step(0.016).unwrap();
    assert!(matches!(
        app.poll_value_notification_v1().unwrap(),
        Some(ValueNotificationV1::Legacy(meta)) if meta.legacy.value == 1.
    ));
    key(&mut app, Key::Tab);
    app.step(0.016).unwrap();
    key(&mut app, Key::Enter);
    app.step(0.016).unwrap();
    let Some(ComponentNotificationV1::AttachmentAction(discard)) =
        app.poll_component_notification_v1().unwrap()
    else {
        panic!("expected typed Remove")
    };
    assert_eq!(discard.action, AttachmentAction::Remove);
    assert_eq!(discard.part, Some(AttachmentActionPart::Remove));
    assert_eq!(discard.source, Some(AttachmentActionSource::Keyboard));
    assert_eq!(discard.event_index, Some(0));
    app.set_attachment_actions(file, AttachmentActions::default())
        .unwrap();
    click(&mut app, file);
    app.step(0.016).unwrap();
    assert!(app.poll_component_notification_v1().unwrap().is_none());
    app.remove(file).unwrap();
    assert_eq!(opened.metadata.handle, discard.metadata.handle);
    assert!(opened.metadata.frame < discard.metadata.frame);
    assert_eq!(opened.event_index, Some(1));
}
