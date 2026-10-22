"""Headless opt-in Stepper visual authoring with owned icon bytes."""
import cui_bridge as c
with c.App(width=800,height=400) as app:
    first=c.item("first","First").image_rgba(bytes([255,255,255,255]),1,1)
    steps=c.stepper("steps").stepper_visual(True,False,"sm").design_size("lg").children(first,c.item("second","Second"))
    app.mount(c.column().children(steps));app.step()
    assert app.numeric_part_bounds(steps,2)[2]>0
