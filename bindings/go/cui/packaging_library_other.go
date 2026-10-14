//go:build !linux

package cui

import "errors"

func packagedLibrary(native bool) (string, error) {
	return "", errors.New("alpha packaged/source bridge currently validates Linux only; use explicit library override on other platforms")
}
