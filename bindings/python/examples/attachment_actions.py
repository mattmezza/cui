"""Run with PYTHONPATH=bindings/python and CUI_BRIDGE_LIBRARY configured.

The app handles file action requests after the UI step.
"""
from cui_bridge import App, attachment, column, label


def main():
    with App(width=480, height=300) as app:
        card = (attachment("report", "Quarterly report.pdf")
                .detail("Upload failed").attachment_status("failed"))
        status = label("Select a file action")
        supported = app.supports_attachment_actions()
        if supported:
            card.attachment_actions(open=True, remove=True, retry=True)
        app.mount(column().children(card, status))
        app.step()
        app.step()
        if not supported:
            print("This bridge supports Attachment presentation only.")
            return

        for _ in range(3):
            app.queue_key("down", "tab")
            app.queue_key("up", "tab")
            app.step()
            app.queue_key("down", "enter")
            app.queue_key("up", "enter")
            app.step()
            for event in app.poll_component_notifications_v1():
                if event.kind == "attachment-action":
                    status.label(f"Requested {event.action} for Quarterly report.pdf")
                    print(event.action, event.part, event.source, event.frame_sequence)


if __name__ == "__main__":
    main()
