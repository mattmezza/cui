//go:build linux && cui_native && !cui_dynamic

package cui

/*
#cgo pkg-config: sdl3 freetype2 harfbuzz
#cgo CFLAGS: -DCUI_BRIDGE_WITH_SDL=1
*/
import "C"

const staticNative = true
