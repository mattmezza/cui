// Package sdk embeds the generated, pinned CUI source snapshot for module-local builds.
package sdk

import (
	"crypto/sha256"
	"embed"
	"encoding/hex"
	"errors"
	"io/fs"
	"strings"
)

//go:embed all:source SOURCE_PROVENANCE.json
var embedded embed.FS

func Files() fs.FS {
	files, err := fs.Sub(embedded, "source")
	if err != nil {
		panic(err)
	}
	return files
}
func Digest() (string, error) {
	files := Files()
	manifest, err := fs.ReadFile(files, "SOURCE_FILES.sha256")
	if err != nil {
		return "", err
	}
	for _, line := range strings.Split(strings.TrimSpace(string(manifest)), "\n") {
		parts := strings.SplitN(line, "  ", 2)
		if len(parts) != 2 {
			return "", errors.New("invalid embedded CUI source manifest")
		}
		content, err := fs.ReadFile(files, parts[1])
		if err != nil {
			return "", err
		}
		digest := sha256.Sum256(content)
		if hex.EncodeToString(digest[:]) != parts[0] {
			return "", errors.New("embedded CUI source failed integrity verification")
		}
	}
	hash := sha256.Sum256(manifest)
	return hex.EncodeToString(hash[:]), nil
}
