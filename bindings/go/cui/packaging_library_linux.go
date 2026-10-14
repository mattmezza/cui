//go:build linux && cui_dynamic

package cui

import (
	"crypto/sha256"
	"debug/elf"
	"embed"
	"encoding/json"
	"errors"
	"fmt"
	"github.com/mattmezza/cui/bindings/go/sdk"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"syscall"
)

//go:embed _native/*
var packagedFiles embed.FS

type packageReceipt struct {
	Architecture string `json:"architecture"`
	Platform     string `json:"platform"`
	SourceDigest string `json:"source_digest"`
	ABIVersion   int    `json:"abi_version"`
	Native       bool   `json:"native_requested"`
	BuildType    string `json:"build_type"`
	LibrarySHA   string `json:"library_sha256"`
}

func hashBytes(data []byte) string { return fmt.Sprintf("%x", sha256.Sum256(data)) }
func validLibrary(path string, receipt packageReceipt) bool {
	data, err := os.ReadFile(path)
	if err != nil || hashBytes(data) != receipt.LibrarySHA {
		return false
	}
	binary, err := elf.Open(path)
	if err != nil {
		return false
	}
	defer binary.Close()
	machine := map[string]elf.Machine{"amd64": elf.EM_X86_64, "arm64": elf.EM_AARCH64}[runtime.GOARCH]
	return machine != elf.EM_NONE && binary.Machine == machine && binary.Type == elf.ET_DYN
}
func atomicFile(path string, data []byte) error {
	file, err := os.CreateTemp(filepath.Dir(path), "cui-write-*.tmp")
	if err != nil {
		return err
	}
	temporary := file.Name()
	defer os.Remove(temporary)
	if _, err = file.Write(data); err != nil {
		file.Close()
		return err
	}
	if err = file.Close(); err != nil {
		return err
	}
	return os.Rename(temporary, path)
}
func packagedLibrary(native bool) (string, error) {
	sourceDigest, err := sdk.Digest()
	if err != nil {
		return "", err
	}
	buildType := os.Getenv("CUI_PACKAGE_BUILD_TYPE")
	if buildType == "" {
		buildType = "Release"
	}
	if buildType != "Release" && buildType != "Debug" {
		return "", errors.New("CUI_PACKAGE_BUILD_TYPE must be Release or Debug")
	}
	expected := packageReceipt{Architecture: runtime.GOARCH, Platform: runtime.GOOS, SourceDigest: sourceDigest,
		ABIVersion: 1, Native: native, BuildType: buildType}
	embedded, readErr := packagedFiles.ReadFile("_native/libcui_bridge.so")
	if readErr == nil {
		metadata, err := packagedFiles.ReadFile("_native/build-info.json")
		if err != nil {
			return "", errors.New("packaged CUI build metadata missing")
		}
		if json.Unmarshal(metadata, &expected) != nil || expected.Architecture != runtime.GOARCH ||
			expected.Platform != runtime.GOOS || expected.ABIVersion != 1 || expected.SourceDigest != sourceDigest ||
			expected.LibrarySHA != hashBytes(embedded) {
			return "", errors.New("packaged CUI bridge provenance/platform/architecture/ABI integrity mismatch")
		}
		if native && !expected.Native {
			return "", errors.New("packaged CUI bridge lacks SDL; select a native artifact or source module")
		}
	}
	encoded, _ := json.Marshal(expected)
	cache, err := os.UserCacheDir()
	if err != nil {
		return "", err
	}
	directory := filepath.Join(cache, "cui-bridge", hashBytes(encoded))
	if err := os.MkdirAll(directory, 0700); err != nil {
		return "", err
	}
	descriptor, err := syscall.Open(filepath.Join(directory, "build.lock"), syscall.O_RDWR|syscall.O_CREAT|syscall.O_NOFOLLOW, 0600)
	if err != nil {
		return "", err
	}
	defer syscall.Close(descriptor)
	if err := syscall.Flock(descriptor, syscall.LOCK_EX); err != nil {
		return "", err
	}
	defer syscall.Flock(descriptor, syscall.LOCK_UN)
	library := filepath.Join(directory, "libcui_bridge.so")
	metadataPath := filepath.Join(directory, "build-info.json")
	var existing packageReceipt
	metadata, _ := os.ReadFile(metadataPath)
	if json.Unmarshal(metadata, &existing) == nil && existing.Architecture == expected.Architecture &&
		existing.Platform == expected.Platform && existing.SourceDigest == expected.SourceDigest &&
		existing.ABIVersion == expected.ABIVersion && existing.Native == expected.Native &&
		existing.BuildType == expected.BuildType && validLibrary(library, existing) {
		return library, nil
	}
	if readErr == nil {
		if err := atomicFile(library, embedded); err != nil {
			return "", err
		}
	} else {
		temporary, err := os.MkdirTemp(directory, "source-build-")
		if err != nil {
			return "", err
		}
		defer os.RemoveAll(temporary)
		source := filepath.Join(temporary, "source")
		if err := fs.WalkDir(sdk.Files(), ".", func(path string, entry fs.DirEntry, walkErr error) error {
			if walkErr != nil {
				return walkErr
			}
			target := filepath.Join(source, filepath.FromSlash(path))
			if entry.IsDir() {
				return os.MkdirAll(target, 0700)
			}
			data, err := fs.ReadFile(sdk.Files(), path)
			if err != nil {
				return err
			}
			return os.WriteFile(target, data, 0600)
		}); err != nil {
			return "", err
		}
		backend := "OFF"
		if native {
			backend = "ON"
		}
		build := filepath.Join(temporary, "build")
		for _, args := range [][]string{{"-S", source, "-B", build, "-DCUI_BUILD_NATIVE=" + backend, "-DCMAKE_BUILD_TYPE=" + buildType},
			{"--build", build, "--target", "cui_bridge", "-j2"}} {
			output, err := exec.Command("cmake", args...).CombinedOutput()
			if err != nil {
				return "", fmt.Errorf("CUI source package requires CMake/C11 toolchain and requested system backend dependencies: %w\n%s", err, output)
			}
		}
		compiled, err := os.ReadFile(filepath.Join(build, "libcui_bridge.so"))
		if err != nil {
			return "", err
		}
		expected.LibrarySHA = hashBytes(compiled)
		if err := atomicFile(library, compiled); err != nil {
			return "", err
		}
	}
	if !validLibrary(library, expected) {
		return "", errors.New("built CUI bridge failed ELF architecture/integrity validation")
	}
	metadata, _ = json.Marshal(expected)
	if err := atomicFile(metadataPath, metadata); err != nil {
		return "", err
	}
	return library, nil
}
