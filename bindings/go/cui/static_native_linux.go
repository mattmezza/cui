//go:build linux && cui_native && !cui_dynamic

package cui

/*
#cgo pkg-config: sdl3 freetype2 harfbuzz
#cgo CFLAGS: -DCUI_BRIDGE_WITH_SDL=1 -DCUI_TEXT_BIDI -I${SRCDIR}/../sdk/source/third_party/sheenbidi/Headers -I${SRCDIR}/../sdk/source/third_party/sheenbidi/Source
*/
import "C"

const staticNative = true
