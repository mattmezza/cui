"""A Python-authored controlled interval UI; windowless by default, --font for SDL."""
import argparse
from cui_bridge import (App, button, column, label, row, slider_model, SliderConfig,
    SliderSingle, SliderRange, SliderOrientation, SliderPart, SliderPhase)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", help="explicit font path for the interactive native window")
    parser.add_argument("--reject", action="store_true", help="leave the owner unchanged")
    parser.add_argument("--single", action="store_true")
    parser.add_argument("--vertical", action="store_true")
    args = parser.parse_args()
    owner = SliderSingle(60) if args.single else SliderRange(20, 60)
    rejecting = args.reject
    config = SliderConfig(orientation=SliderOrientation.VERTICAL if args.vertical else
        SliderOrientation.HORIZONTAL, reduced_motion=True, source_generation=1)
    status = label("Owner: " + repr(owner), key="status")
    control = slider_model("selection", config, owner, "Selected interval")
    def request_changed(request):
        nonlocal owner
        # C has returned. External owner mutation and explicit acceptance are allowed.
        if not rejecting and request.phase == SliderPhase.UPDATE:
            app.accept_slider_request(request.receipt, request.requested)
            owner = request.requested
        status.label(f"{request.phase.name}: requested={request.requested!r}; owner={owner!r}")
        print(status._text)
    control.on_request(request_changed)
    def toggle(_event):
        nonlocal rejecting
        rejecting = not rejecting
        decision.label("Rejecting proposals" if rejecting else "Accepting UPDATE proposals")
    decision = button("decision").label("Rejecting proposals" if rejecting else
        "Accepting UPDATE proposals").on_click(toggle)
    options = dict(width=720, height=480, title="Python controlled interval")
    if args.font:
        options.update(backend="sdl", font_path=args.font)
    with App(**options) as app:
        app.mount(column("root").gap(8).children(label("Controlled Single / Range"),
            row("controls").children(column("slider-column").children(control)), decision, status))
        if args.font:
            app.run()  # Existing native after-frame trampoline catches Python exceptions.
        else:
            app.step(); app.step()
            x, y, width, height = control.part_bounds(SliderPart.END_THUMB)
            app.queue_pointer("down", x+width/2, y+height/2)
            app.queue_pointer("up", x+width/2, y+height/2)
            app.step(); app.dispatch()
            app.queue_key("down", "right")
            app.step(); app.dispatch()
            assert control.get_value() == owner
            assert owner.end == 60 if rejecting else owner.end > 60
            print("PASS Python-authored controlled UI: full pair, explicit accept/reject")


if __name__ == "__main__":
    main()
