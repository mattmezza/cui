#!/usr/bin/env python3
"""Check local prerelease version coherence; never contacts a registry."""
import argparse
import ast
import json
from pathlib import Path
import re
import tempfile
import tomllib
import unittest


def literal_assignments(source):
    result = {}
    for node in ast.walk(ast.parse(source)):
        if isinstance(node, ast.Assign) and isinstance(node.value, ast.Constant):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    result[target.id] = node.value.value
    return result


def check(root, tag, registries=False):
    errors = []
    match = re.fullmatch(r"v(\d+\.\d+\.\d+)-alpha\.(\d+)", tag)
    if not match:
        return ["expected prerelease tag vMAJOR.MINOR.PATCH-alpha.N"]
    base, alpha = match.groups()
    semver, python_version = tag[1:], base + "a" + alpha
    read = lambda path: (root / path).read_text()
    cmake = re.search(r"project\(cui\s+VERSION\s+(\S+)", read("CMakeLists.txt"))
    python = tomllib.loads(read("bindings/python/pyproject.toml"))["project"]
    node = json.loads(read("bindings/node/package.json"))
    rust = tomllib.loads(read("bindings/rust/Cargo.toml"))["package"]
    rust_lock = tomllib.loads(read("bindings/rust/Cargo.lock"))
    rust_locked = [entry.get("version") for entry in rust_lock.get("package", [])
                   if entry.get("name") == rust.get("name")]
    backend = read("bindings/python/packaging/wheel_backend.py")
    packaging = literal_assignments(read("scripts/package_bindings.py"))
    public_version = re.search(r'^#define CUI_VERSION "([^"\n]+)"', read("include/cui/cui.h"), re.MULTILINE)
    expected = {
        "Public C version": (public_version.group(1) if public_version else None, semver),
        "CMake base": (cmake.group(1) if cmake else None, base),
        "Python project": (python.get("version"), python_version),
        "Python wheel basename": (literal_assignments(backend).get("NAME"), "cui_bridge-" + python_version),
        "Python wheel METADATA": (re.search(r"(?<!-)Version: ([^\\\n]+)\\n", backend).group(1), python_version),
        "npm": (node.get("version"), semver),
        "Rust": (rust.get("version"), semver),
        "Rust lockfile": (rust_locked, [semver]),
        "Go proxy": (packaging.get("version"), tag),
        "Go module": (packaging.get("module"), read("bindings/go/go.mod").splitlines()[0].removeprefix("module ")),
    }
    for label, (actual, wanted) in expected.items():
        if actual != wanted:
            errors.append(f"{label}: {actual!r} != {wanted!r}")
    if registries:
        if node.get("private"):
            errors.append("npm manifest is private; registry publication is disabled")
        if rust.get("publish") is False:
            errors.append("Cargo publish=false; registry publication is disabled")
    return errors


class ParserTests(unittest.TestCase):
    def test_literals_do_not_execute_code(self):
        self.assertEqual(literal_assignments('version = "v1"\nmodule = "m"\nraise RuntimeError()'),
                         {"version": "v1", "module": "m"})

    def test_real_metadata_and_mismatch(self):
        root = Path(__file__).resolve().parents[1]
        tag = "v" + json.loads((root / "bindings/node/package.json").read_text())["version"]
        self.assertEqual(check(root, tag), [])
        self.assertTrue(check(root, "v999.0.0-alpha.1"))
        self.assertTrue(check(root, "invalid"))
        with tempfile.TemporaryDirectory() as temporary:
            staged = Path(temporary)
            for relative in ("include/cui/cui.h", "CMakeLists.txt", "bindings/python/pyproject.toml",
                             "bindings/python/packaging/wheel_backend.py", "bindings/node/package.json",
                             "bindings/rust/Cargo.toml", "bindings/rust/Cargo.lock",
                             "bindings/go/go.mod", "scripts/package_bindings.py"):
                target = staged / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text((root / relative).read_text())
            public_header = staged / "include/cui/cui.h"
            original_header = public_header.read_text()
            public_header.write_text(original_header.replace('#define CUI_VERSION "' + tag[1:] + '"',
                                                              '#define CUI_VERSION "0.1.0-dev"'))
            self.assertTrue(any(e.startswith("Public C version:") for e in check(staged, tag)))
            public_header.write_text(original_header)
            lockfile = staged / "bindings/rust/Cargo.lock"
            original_lock = lockfile.read_text()
            lockfile.write_text(original_lock.replace('version = "' + tag[1:] + '"',
                                                       'version = "0.0.0"'))
            self.assertTrue(any(e.startswith("Rust lockfile:") for e in check(staged, tag)))
            lockfile.write_text(original_lock)
            manifest = staged / "bindings/node/package.json"
            metadata = json.loads(manifest.read_text())
            metadata["private"] = True
            manifest.write_text(json.dumps(metadata))
            self.assertTrue(any("npm manifest is private" in e for e in check(staged, tag, True)))
            metadata["version"] = "999.0.0-alpha.1"
            manifest.write_text(json.dumps(metadata))
            self.assertTrue(any(e.startswith("npm:") for e in check(staged, tag)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--tag", default="v0.1.0-alpha.2")
    parser.add_argument("--registries", action="store_true", help="also reject explicit npm/Cargo publication gates; does not prove registry ownership or OIDC readiness")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(ParserTests)
        return 0 if unittest.TextTestRunner().run(suite).wasSuccessful() else 1
    try:
        errors = check(args.root.resolve(), args.tag, args.registries)
    except (OSError, ValueError, KeyError, AttributeError, SyntaxError) as exc:
        errors = [f"cannot validate metadata: {exc}"]
    for error in errors:
        print("FAIL:", error)
    if not errors:
        print(f"PASS: local versions agree with {args.tag}; registry ownership/authentication and artifact contents are not checked")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
