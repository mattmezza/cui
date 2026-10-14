package main

import (
	"cui.local/bindings/go/cui"
	"errors"
	"flag"
	"fmt"
	"os"
)

func must(err error) {
	if err != nil {
		panic(err)
	}
}

func press(app *cui.App, node *cui.Node) error {
	r, err := app.Bounds(node)
	if err != nil {
		return err
	}
	if r.Width <= 0 || r.Height <= 0 {
		return errors.New("empty native hit target")
	}
	x, y := r.X+r.Width/2, r.Y+r.Height/2
	if err = app.QueuePointer(cui.PointerDown, x, y); err != nil {
		return err
	}
	if err = app.QueuePointer(cui.PointerUp, x, y); err != nil {
		return err
	}
	if err = app.Step(.016); err != nil {
		return err
	}
	_, err = app.Dispatch()
	return err
}

func run(headless bool, font string) error {
	if !headless && font == "" {
		return errors.New("native mode requires --font /absolute/path/to/font.ttf")
	}
	app, err := cui.New(cui.Options{Native: !headless, FontPath: font, Title: "CUI Go workspace", Width: 700, Height: 520})
	if err != nil {
		return err
	}
	defer app.Close()
	status := cui.Label("Ready").Key("status")
	name := cui.Input("name").Text("Ada")
	birthday := cui.DatePicker("birthday").Text("Birthday").Date(cui.Date{Year: 1990, Month: 5, Day: 17})
	confirm := cui.Dialog("confirm", "Changes saved", "Your profile is up to date")
	save := cui.Button("save").Text("Save changes").Variant(cui.Primary).Size(cui.MD)
	save.OnClick(func(cui.Event) {
		must(status.SetText("Saved ✓"))
		must(confirm.Open(true).Err())
	})
	notifications := cui.Checkbox("notifications").Text("Email notifications").Checked(true)
	account := cui.Item("account", "Account").Children(cui.Column("account-panel").GapUnits(2).Children(
		cui.Label("Name").Key("name-label"), name,
		cui.Label("Birthday").Key("birthday-label"), birthday, save,
	))
	preferences := cui.Item("preferences", "Preferences").Children(cui.Column("preferences-panel").GapUnits(2).Children(notifications))
	tabs := cui.Tabs("navigation").Children(account, preferences).Selected(account)
	if err := app.Mount(cui.Column("root").GapUnits(2).Children(
		cui.Label("Workspace").Key("heading"), tabs, status, confirm,
	)); err != nil {
		return err
	}
	if err := app.Step(.016); err != nil {
		return err
	}
	if !headless {
		return app.Run(1.0 / 60)
	}
	if err := press(app, name); err != nil {
		return err
	}
	if err := app.QueueText(" Lovelace"); err != nil {
		return err
	}
	if err := app.Step(.016); err != nil {
		return err
	}
	if _, err := app.Dispatch(); err != nil {
		return err
	}
	gotName, err := app.Text(name)
	if err != nil {
		return err
	}
	if gotName != "Ada Lovelace" {
		return fmt.Errorf("name edit = %q", gotName)
	}
	if err := press(app, preferences); err != nil {
		return err
	}
	selected, err := app.Selected(tabs)
	if err != nil {
		return err
	}
	if selected != preferences {
		return errors.New("Preferences navigation failed")
	}
	if err := press(app, notifications); err != nil {
		return err
	}
	checked, err := app.Checked(notifications)
	if err != nil {
		return err
	}
	if checked {
		return errors.New("notification checkbox did not change")
	}
	if err := press(app, account); err != nil {
		return err
	}
	selected, err = app.Selected(tabs)
	if err != nil {
		return err
	}
	if selected != account {
		return errors.New("Account navigation failed")
	}
	date, err := app.Date(birthday)
	if err != nil {
		return err
	}
	if date != (cui.Date{Year: 1990, Month: 5, Day: 17}) {
		return fmt.Errorf("birthday = %+v", date)
	}
	if err := press(app, save); err != nil {
		return err
	}
	if err := app.Step(.016); err != nil {
		return err
	}
	open, err := app.Open(confirm)
	if err != nil {
		return err
	}
	if !open {
		return errors.New("save dialog did not open")
	}
	r, err := app.Bounds(status)
	if err != nil || r.Width <= 0 {
		return errors.New("status label missing")
	}
	statusText, err := app.Text(status)
	if err != nil {
		return err
	}
	if statusText != "Saved ✓" {
		return fmt.Errorf("status = %q", statusText)
	}
	if status.Err() != nil {
		return status.Err()
	}
	fmt.Println("workspace headless: Account, Preferences, name, checkbox, birthday, save dialog and status passed")
	return nil
}

func main() {
	headless := flag.Bool("headless", false, "run deterministic headless UI validation")
	font := flag.String("font", "", "absolute TTF font path for native SDL mode")
	flag.Parse()
	if err := run(*headless, *font); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
