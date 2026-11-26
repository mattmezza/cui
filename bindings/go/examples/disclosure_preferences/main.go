// Workspace preferences. Pass -font FONT.ttf for the native SDL window.
package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for a native SDL window")
	flag.Parse()
	app, err := cui.New(cui.Options{
		Title: "Workspace preferences", Width: 480, Height: 320,
		Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()

	status := cui.Label("Open a section to edit your preferences.")
	account := cui.Item("account", "Account").Children(cui.Label("Personal details"))
	editor := cui.Item("editor", "Editor").
		Children(cui.Label("Auto-save is enabled.")).
		OnChange(func(event cui.Event) {
			if event.Checked {
				must(status.SetText("Editing workspace preferences."))
			}
		})
	sections := cui.Accordion("sections").Children(account, editor)
	advanced := cui.Collapsible("advanced").Text("Advanced").Children(cui.Label("Keep local cache"))
	supported, err := app.SupportsDisclosurePresentation()
	must(err)
	if supported {
		must(sections.ConfigureDisclosure(&cui.DisclosureConfig{
			Width: 360, DesignSize: cui.LG, Joined: true, Bordered: true, Single: true,
			AllowCollapse: true, CollapsedIcon: "plus", ExpandedIcon: "minus",
		}))
		must(advanced.ConfigureDisclosure(&cui.DisclosureConfig{Width: 360, Borderless: true}))
		color, cursor := uint32(0x4879e02b), cui.DisclosureCursorPointer
		must(editor.SetDisclosureTriggerStyle(&cui.DisclosureTriggerStyle{HoverRGBA: &color, Cursor: &cursor}))
	}
	headings, err := app.SupportsDisclosureHeading()
	must(err)
	if headings {
		must(sections.SetDisclosureHeading(cui.DisclosureHeadingNormal, &cui.DisclosureHeading{
			Typography: &cui.DisclosureTypography{FontSize: 17, LineHeight: 24, Weight: cui.DisclosureWeightMedium, Wrap: true},
		}))
		must(sections.SetDisclosureHeading(cui.DisclosureHeadingExpanded, &cui.DisclosureHeading{
			Typography: &cui.DisclosureTypography{FontSize: 19, LineHeight: 26, Weight: cui.DisclosureWeightBold, Wrap: true},
		}))
	}
	titleStates, err := app.SupportsDisclosureTitleState()
	must(err)
	if titleStates {
		must(sections.SetDisclosureTitleState(cui.DisclosureTitleHovered, &cui.DisclosureTitleStyle{
			Typography: &cui.DisclosureTypography{FontSize: 17, LineHeight: 24, Weight: cui.DisclosureWeightSemibold, Wrap: true},
		}))
		focus := uint32(0x275dccff)
		must(sections.SetDisclosureTitleState(cui.DisclosureTitleFocusVisible, &cui.DisclosureTitleStyle{TitleRGBA: &focus}))
	}
	dividers, err := app.SupportsDisclosureDivider()
	must(err)
	if dividers {
		must(sections.SetDisclosureDivider(&cui.DisclosureDivider{
			Thickness: 2, InsetLeft: 12, InsetRight: 12, ColorRGBA: 0x4d80c0aa,
		}))
	}
	must(app.Mount(cui.Column("workspace").GapUnits(3).Children(
		cui.Label("Workspace preferences"), sections, advanced, status,
	)))
	if *font != "" {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	bounds, err := app.Bounds(editor)
	must(err)
	must(app.QueuePointer(cui.PointerDown, bounds.X+20, bounds.Y+12))
	must(app.QueuePointer(cui.PointerUp, bounds.X+20, bounds.Y+12))
	must(app.Step(.016))
	events, err := app.Dispatch()
	must(err)
	summary, err := app.Text(status)
	must(err)
	if events != 1 || summary != "Editing workspace preferences." {
		panic(fmt.Sprintf("unexpected preferences UI: %d events, %q", events, summary))
	}
	fmt.Println(summary)
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
