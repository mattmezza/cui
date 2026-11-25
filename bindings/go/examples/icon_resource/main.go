// SPDX-License-Identifier: MIT
package main

import (
	"fmt"
	"log"

	"github.com/mattmezza/cui/bindings/go/cui"
)

const mark = `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16">
  <path d="M2 8l4 4 8-8" fill="none" stroke="black" stroke-width="2"/>
</svg>`

func run() (err error) {
	app, err := cui.New(cui.Options{})
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := app.Close(); err == nil {
			err = closeErr
		}
	}()

	const path = "profile/verified.svg"
	icon, err := app.NewIconResource(cui.IconPath(path), cui.IconLimits{MaxVersions: 2})
	if err != nil {
		return err
	}
	if err = icon.PutAsset(path, mark); err != nil {
		return err
	}
	status, err := icon.Prepare(16, 16)
	if err != nil {
		return err
	}
	if status != cui.IconOK {
		return fmt.Errorf("artwork preparation failed with status %d", status)
	}
	bitmap, err := icon.Bitmap()
	if err != nil {
		return err
	}
	if bitmap.Width != 16 || bitmap.Height != 16 {
		return fmt.Errorf("unexpected artwork dimensions %dx%d", bitmap.Width, bitmap.Height)
	}
	for i := 3; i < len(bitmap.RGBA); i += 4 {
		if bitmap.RGBA[i] != 0 {
			fmt.Printf("Prepared %s as a %dx%d coverage mask\n", path, bitmap.Width, bitmap.Height)
			return icon.Close()
		}
	}
	return fmt.Errorf("no visible coverage in %s", path)
}

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}
