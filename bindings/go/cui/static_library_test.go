//go:build linux && !cui_dynamic

package cui

import (
	"os"
	"strings"
	"testing"
)

func TestStaticPortableRejectsNativeWithoutBuildTag(t *testing.T) {
	if staticNative || os.Getenv("CUI_BRIDGE_LIBRARY") != "" {
		t.Skip("requires default portable static selection")
	}
	_, err := New(Options{Native: true, FontPath: "/missing/font.ttf"})
	if err == nil || !strings.Contains(err.Error(), "-tags cui_native") {
		t.Fatalf("expected native build-tag diagnostic, got %v", err)
	}
}
