//go:build linux && !cui_dynamic

package cui

/*
#cgo CFLAGS: -Wpedantic -DCUI_GO_STATIC=1 -I${SRCDIR}/../sdk/source/third_party/clay
#cgo LDFLAGS: -lm
*/
import "C"
import "errors"

func packagedLibrary(native bool) (string, error) {
	if native && !staticNative {
		return "", errors.New("native backend requires go build -tags cui_native")
	}
	return "", nil
}
