"""Headless retained Attachment with copied decoded preview pixels."""
from cui_bridge import App, AttachmentOptions, AttachmentTextStyle, attachment, column
with App(width=480, height=400) as app:
    card = attachment('report', 'Quarterly report.pdf').detail('2.4 MB').visual_size(280, 0)
    if app.supports_attachment_options():
        card.design_size("lg").attachment_options(AttachmentOptions(vertical=True, media=True, media_aspect=2,
            title=AttachmentTextStyle(font_size=18, weight=2)))
    if app.supports_attachment_preview():
        card.attachment_preview_rgba(bytes([36, 92, 182, 255] * 4), 2, 2)
    app.mount(column().children(card)); app.step()
    assert app.bounds(card)[2] == 280
