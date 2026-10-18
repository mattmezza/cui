#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["wasmtime==49.0.0"]
# ///
"""Compile portable CUI suites for WASI Preview 1 and execute their assertions."""
from __future__ import annotations

import argparse
import importlib.metadata
import hashlib
import pathlib
import subprocess
import sys
import time

ZIG_VERSION = "0.16.0"
WASMTIME_VERSION = "49.0.0"
SOURCES = (
    "src/base/cui_boolean_events.c",
    "src/components/cui_styled_slider_events.c",
    "src/base/cui_surface_events.c",
    "src/components/cui_surface_events.c",
    "src/base/cui_button_group.c",
    "src/core/cui.c",
    "src/components/cui_text_overflow.c",
    "src/base/cui_radio_group.c", "src/components/cui_radio_group.c",
    "src/effects/cui_effects.c",
    "src/base/cui_base.c",
    "src/base/cui_models.c",
    "src/base/cui_edit.c", "src/base/cui_editor.c", "src/components/cui_editor.c",
    "src/base/cui_overlay.c",
    "src/base/cui_virtual.c", "src/base/cui_virtual_scroll.c", "src/components/cui_virtual.c",
    "src/base/cui_dock.c", "src/components/cui_dock.c",
    "src/base/cui_plot.c", "src/components/cui_plot.c", "src/components/cui_plot_motion.c",
    "src/theme/cui_theme.c", "src/theme/cui_theme_registry.c", "src/theme/cui_theme_motion.c",
    "src/components/cui_components.c", "src/components/cui_label.c",
    "src/base/cui_table.c",
    "src/components/cui_table.c",
    "src/base/cui_toolbar.c",
    "src/components/cui_toolbar.c",
    "src/base/cui_sidebar.c",
    "src/components/cui_sidebar.c",
    "src/base/cui_textarea.c",
    "src/components/cui_textarea.c",
    "src/base/cui_settings.c",
    "src/components/cui_settings.c",
    "src/base/cui_questionnaire.c",
    "src/components/cui_questionnaire.c",
    "src/base/cui_speech.c",
    "src/components/cui_speech.c",
    "src/base/cui_chart.c",
    "src/components/cui_chart.c",
    "src/base/cui_text_view.c",
    "src/components/cui_text_view.c",
    "src/base/cui_root.c",
    "src/components/cui_root.c",
    "src/base/cui_command.c",
    "src/components/cui_command.c",
    "src/base/cui_carousel.c",
    "src/components/cui_carousel.c",
    "src/base/cui_icon.c",
    "src/components/cui_icon.c",
    "src/base/cui_resizable.c",
    "src/components/cui_resizable.c",
    "src/base/cui_chrome.c",
    "src/components/cui_chrome.c",
    "src/base/cui_description.c",
    "src/components/cui_description.c",
    "src/base/cui_media.c",
    "src/base/cui_video.c",
    "src/components/cui_video_controls.c",
    "src/components/cui_media.c",
    "src/base/cui_message.c",
    "src/components/cui_message.c",
    "src/base/cui_status.c",
    "src/components/cui_status.c",
    "src/base/cui_otp.c",
    "src/components/cui_otp.c",
    "src/base/cui_form.c",
    "src/components/cui_form.c",
    "src/base/cui_surface.c",
    "src/components/cui_surface.c",
    "src/base/cui_scroll.c",
    "src/components/cui_scroll.c",
    "src/base/cui_tree.c",
    "src/components/cui_tree.c",
    "src/base/cui_picker.c",
    "src/components/cui_picker.c",
    "src/base/cui_numeric.c",
    "src/components/cui_numeric.c",
    "src/base/cui_content.c",
    "src/components/cui_content.c",
    "src/base/cui_disclosure.c",
    "src/components/cui_disclosure.c",
    "src/base/cui_choice.c",
    "src/components/cui_choice.c",
    "backends/software/cui_software.c",
)
SUITES = ("menu_events", "styled_slider_events", "combobox_events", "surface_events", "select_events", "editor_events", "textarea_events", "selection_events", "text_input_styled_events", "text_input_events", "slider_events", "boolean_styled_events", "boolean_events", "control_events", "component_events", "video_controls", "slider_controlled", "icon_style_source", "spinner_contract", "text_paint", "video", "responsive", "typography_tokens", "release_accessors", "unicode_wrap_source", "text_alignment_source", "contextual_wrap_source", "svg_transform_validation", "button_tab_order_source", "icon_accessibility_source", "label_source", "empty_source", "tab_order_source", "bootstrap", "core", "base", "components", "render", "effects", "edit", "editor", "overlay", "virtual", "virtual_styled", "dock", "dock_styled", "plot", "plot_styled", "content", "disclosure", "choice", "numeric", "picker", "tree", "input", "table", "scroll", "surface", "form", "otp", "status", "message", "media", "description", "chrome", "resizable", "icon", "toolbar", "sidebar", "textarea", "touch_textarea", "touch_page_host", "controlled_siblings", "settings", "carousel", "command", "root", "text_view", "chart", "speech", "questionnaire", "questionnaire_styled", "control_family_audit", "radio_group", "checkbox_motion", "checkbox_source", "contextual_text", "alert_bubble_slots", "pointer_focus_policy", "status_motion", "button_behavior", "advanced_family_audit", "table_selection_contracts", "core_prepaint_contract_fixture", "theme_registry", "prepaint", "plot_chart_owner_motion", "input_reference_contracts", "plot_chart_prepaint", "pointer_presence", "input_acceptance", "chart_source_point_axis", "input_normalization", "chart_radar_tooltip_title", "chart_typed_tooltip", "pointer_activation_blocker", "button_activation_blocker", "pointer_blocker_scroll_scope", "password_engine", "chart_tooltip_appearance", "button_group", "button_callback_events", "password_present", "password_start", "password_input", "secure_focus", "secure_focus_generation", "drop_shadow", "input_format_engine", "input_format_control", "input_number_engine", "input_number_control", "shadow_covering", "button_label_layout", "input_pointer_selection", "button_passive_lifecycle", "inset_shadow", "button_numeric", "shadow_presets", "input_matcher", "button_click_event", "notification_shadows", "popover_appearance", "button_artwork", "activation_snapshot", "tabs_source_shadows", "button_click_snapshot", "notification_source_motion")


def source_fingerprint(root: pathlib.Path) -> str:
    paths = list(SOURCES) + ["src/base/cui_icon_layout.h","src/core/cui_text_paint_internal.h", "src/core/cui_unicode_grapheme.h", "tests/unit/cui_edit_grapheme_tests.inc", "backends/software/cui_shadow.h", "backends/software/cui_bitmap.h", "src/core/cui_drop_shadow_bounds_internal.h", "tests/distribution/drop_shadow_effects.c", "tests/distribution/inset_shadow_effects.c"]
    paths += [f"tests/unit/test_{name}.c" for name in SUITES]
    paths += ["tests/unit/test_prepaint_parts.c", "tests/unit/test_layout_only_clips.c", "tests/unit/test_core_embed.c", "tests/distribution/single.c",
              "tests/distribution/implementation.c", "tests/distribution/consumer.c",
              "tests/distribution/effects.c", "tests/distribution/theme_registry.c", "tests/distribution/dock_model.c", "tests/distribution/virtual_model.c"]
    paths += [f"examples/{name}/main.c" for name in ("unstyled", "styled_settings")]
    paths += [str(path.relative_to(root)) for folder in ("include/cui", "dist")
              for path in (root / folder).glob("*.h")]
    paths += ["src/base/cui_textarea_geometry.h", "src/theme/cui_theme_json.h", "src/theme/cui_theme_equal.h", "src/theme/cui_theme_palette.inc", "src/theme/cui_theme_legacy_fields.inc"]
    paths += [f"examples/{name}/{file}" for name in ("responsive_dashboard", "typography")
              for file in ("app.c", "app.h", "test_app.c")]
    paths += ["web/touch_page_host.h", "examples/native_starter/app.c", "examples/native_starter/app.h", "examples/native_starter/test_app.c", "examples/system_theme/main.c", "tests/integration/test_system_theme_app.c"]
    paths += ["src/text/cui_bidi.c", "tests/unit/test_bidi.c", "tests/unit/test_radio_group_base.c", "tests/unit/test_accepted_activation.c", "tests/unit/test_button_group_base.c"]
    paths += [str(path.relative_to(root)) for path in (root / "third_party/sheenbidi").rglob("*") if path.is_file()]
    paths += ["third_party/clay/clay.h", "tests/integration/popover_appearance_fixture.h", "tests/unit/notification_shadow_fixture.h", "tests/unit/cui_edit_grapheme_tests.inc", "tests/unit/cui_edit_word_tests.inc"]
    digest = hashlib.sha256()
    for path in sorted(set(paths)):
        digest.update(path.encode() + b"\0")
        digest.update((root / path).read_bytes())
    return digest.hexdigest()


def command(args: list[str], root: pathlib.Path) -> None:
    result = subprocess.run(args, cwd=root, capture_output=True, text=True, timeout=180)
    if result.returncode:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(args)}\n"
            f"{result.stdout}{result.stderr}"
        )
    if result.stdout or result.stderr:
        print(result.stdout + result.stderr, end="", flush=True)


def run_module(path: pathlib.Path, engine: object, fuel: int) -> None:
    import wasmtime

    stdout = path.with_suffix(".stdout.txt")
    stderr = path.with_suffix(".stderr.txt")
    status: Exception | None = None
    with wasmtime.Store(engine) as store:
        store.set_fuel(fuel)
        wasi = wasmtime.WasiConfig()
        wasi.argv = [path.stem]
        wasi.stdout_file = str(stdout)
        wasi.stderr_file = str(stderr)
        # Deliberately grant no filesystem preopens or environment variables.
        store.set_wasi(wasi)
        with wasmtime.Linker(engine) as linker:
            linker.define_wasi()
            with wasmtime.Module.from_file(engine, str(path)) as module:
                instance = linker.instantiate(store, module)
                try:
                    instance.exports(store)["_start"](store)
                except wasmtime.ExitTrap as exc:
                    if exc.code:
                        status = exc
                except wasmtime.Trap as exc:
                    status = exc
    for output in (stdout, stderr):
        if output.exists():
            text = output.read_text(errors="replace").strip()
            if text:
                print(text, flush=True)
    if status:
        raise RuntimeError(f"{path.stem} trapped or exited unsuccessfully: {status}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=pathlib.Path, help="defaults to ROOT/build/wasi")
    parser.add_argument("--suite", action="append", help="run only this suite; repeat for multiple suites")
    parser.add_argument("--zig", default="zig", help="path to Zig 0.16.0")
    # Theme/variant matrices repeatedly construct full contexts; keep a finite
    # suite budget large enough for those allocation-heavy behavioral tests.
    # Core current-frame snapshot storage increased per-context initialization;
    # the Choice matrix exhausts 40B fuel in calloc with every assertion enabled.
    parser.add_argument("--fuel", type=int, default=80_000_000_000, help="per-suite instruction budget")
    args = parser.parse_args()
    root = args.root.resolve()
    build = (args.build_dir or root / "build/wasi").resolve()
    if args.fuel <= 0:
        parser.error("--fuel must be positive")
    build.mkdir(parents=True, exist_ok=True)
    actual_zig = subprocess.check_output([args.zig, "version"], text=True).strip()
    actual_wasmtime = importlib.metadata.version("wasmtime")
    if actual_zig != ZIG_VERSION or actual_wasmtime != WASMTIME_VERSION:
        raise RuntimeError(
            f"Pinned tools required: Zig {ZIG_VERSION}, wasmtime {WASMTIME_VERSION}; "
            f"found {actual_zig}, {actual_wasmtime}"
        )
    import wasmtime

    config = wasmtime.Config()
    config.consume_fuel = True
    with wasmtime.Engine(config) as engine:
        revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
        dirty = subprocess.run(["git", "diff", "--quiet", "HEAD"], cwd=root).returncode != 0
        fingerprint = source_fingerprint(root)
        print(f"CUI source revision: {revision}", flush=True)
        print(f"Portable source SHA256: {fingerprint}", flush=True)
        print("Tracked source modifications: " + ("present" if dirty else "none"), flush=True)
        print(f"Target: wasm32-wasi (WASI Preview 1), Zig {actual_zig}, wasmtime {actual_wasmtime}", flush=True)
        print("Optional adapters disabled: SDL, GPU, FreeType, HarfBuzz; no browser host tested.", flush=True)
        print(f"Per-suite instruction budget: {args.fuel:,}", flush=True)
        print("Assertions enabled. Virtual clock() compatibility uses wall time; benchmarks not invoked.", flush=True)
        flags = [args.zig, "cc", "-target", "wasm32-wasi", "-std=c11", "-O2", "-UNDEBUG",
                 "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(root / "include"),
                 "-I", str(root / "third_party/clay")]
        objects = []
        for source in SOURCES:
            obj = build / (source.replace("/", "_").replace(".c", ".o"))
            command(flags + ["-c", str(root / source), "-o", str(obj)], root)
            objects.append(str(obj))
        archive = build / "libcui-portable.a"
        command([args.zig, "ar", "rcs", str(archive), *objects], root)
        # WASI has no process CPU clock. The virtual suite contains an optional
        # benchmark branch using clock(), so compile its explicit wall-clock
        # compatibility library even though normal assertions do not run it.
        virtual_clock = ["-D_WASI_EMULATED_PROCESS_CLOCKS", "-lwasi-emulated-process-clocks"]
        # Compile the unmodified backend separately: upstream warning exceptions
        # must not weaken strict warnings on the CUI wrapper or its assertions.
        bidi_include = ["-I", str(root / "third_party/sheenbidi/Headers"),
                        "-I", str(root / "third_party/sheenbidi/Source")]
        bidi_object = build / "sheenbidi.o"
        command(flags + bidi_include + ["-DSB_CONFIG_UNITY", "-Wno-unused-parameter",
                "-Wno-unused-value", "-Wno-unused-function", "-Wno-implicit-fallthrough",
                "-c", str(root / "third_party/sheenbidi/Source/SheenBidi.c"),
                "-o", str(bidi_object)], root)
        targets = [(name, [f"tests/unit/test_{name}.c"] +
                    (["src/core/cui.c"] if name == "pointer_presence" else []),
                    name != "pointer_presence",
                    virtual_clock if name == "virtual" else []) for name in SUITES]
        targets += [("release_key_activation", ["tests/unit/test_release_key_activation.c", "src/core/cui.c"], False, [])]
        targets += [("release_key_overflow", ["tests/unit/test_release_key_overflow.c"], False, [])]
        targets += [("prepaint_parts_core_only", ["tests/unit/test_prepaint_parts.c"], False, [])]
        targets += [("layout_only_clips_core_only", ["tests/unit/test_layout_only_clips.c", "src/core/cui.c"], False, [])]
        targets += [("accepted_activation_core_only", ["tests/unit/test_accepted_activation.c", "src/core/cui.c"], False, [])]
        targets += [("button_group_base_only", ["tests/unit/test_button_group_base.c", "src/core/cui.c", "src/base/cui_button_group.c"], False, [])]
        targets += [("radio_group_base_only", ["tests/unit/test_radio_group_base.c", "src/core/cui.c", "src/base/cui_radio_group.c"], False, [])]
        targets += [("responsive_dashboard", ["examples/responsive_dashboard/app.c", "examples/responsive_dashboard/test_app.c"], True, [])]
        targets += [("typography_example", ["examples/typography/app.c", "examples/typography/test_app.c"], True, [])]
        targets += [("native_starter", ["examples/native_starter/app.c", "examples/native_starter/test_app.c"], True, [])]
        targets += [("system_theme_app", ["tests/integration/test_system_theme_app.c"], True, [])]
        targets += [("bidi", ["tests/unit/test_bidi.c", "src/text/cui_bidi.c"], False,
                     bidi_include + [str(bidi_object)])]
        targets += [("speech_base_only", ["tests/unit/test_speech.c", "src/base/cui_speech.c"], False, ["-DCUI_SPEECH_BASE_ONLY=1"])]
        targets += [("chart_base_only", ["tests/unit/test_chart.c", "src/base/cui_chart.c", "src/base/cui_plot.c"], False, ["-DCUI_CHART_BASE_ONLY=1"])]
        targets += [("text_view_base_only", ["tests/unit/test_text_view.c", "src/base/cui_text_view.c"], False, ["-DCUI_TEXT_VIEW_BASE_ONLY=1"])]
        targets += [("root_base_only", ["tests/unit/test_root.c", "src/base/cui_root.c", "src/core/cui.c", "src/base/cui_surface.c", "src/base/cui_overlay.c", "src/base/cui_models.c", "src/base/cui_status.c"], False, ["-DCUI_ROOT_BASE_ONLY=1"])]
        targets += [("command_base_only", ["tests/unit/test_command.c", "src/base/cui_command.c"], False, ["-DCUI_COMMAND_BASE_ONLY=1"])]
        targets += [("carousel_base_only", ["tests/unit/test_carousel.c", "src/base/cui_carousel.c"], False, ["-DCUI_CAROUSEL_BASE_ONLY=1"])]
        targets += [("sidebar_base_only", ["tests/unit/test_sidebar.c", "src/core/cui.c", "src/base/cui_sidebar.c"], False, ["-DCUI_SIDEBAR_BASE_ONLY=1"])]
        targets += [("numeric_base_only", ["tests/unit/test_numeric.c", "src/core/cui.c", "src/base/cui_edit.c", "src/base/cui_numeric.c"], False, ["-DCUI_NUMERIC_BASE_ONLY=1"])]
        targets += [("picker_base_only", ["tests/unit/test_picker.c", "src/core/cui.c", "src/base/cui_models.c", "src/base/cui_overlay.c", "src/base/cui_picker.c"], False, ["-DCUI_PICKER_BASE_ONLY=1"])]
        targets += [("tree_base_only", ["tests/unit/test_tree.c", "src/core/cui.c", "src/base/cui_tree.c"], False, ["-DCUI_TREE_BASE_ONLY=1"])]
        targets += [("input_base_only", ["tests/unit/test_input.c", "src/core/cui.c", "src/base/cui_base.c", "src/base/cui_edit.c"], False, ["-DCUI_INPUT_BASE_ONLY=1"])]
        targets += [("table_base_only", ["tests/unit/test_table.c", "src/core/cui.c", "src/base/cui_table.c", "src/base/cui_scroll.c"], False, ["-DCUI_TABLE_BASE_ONLY=1"])]
        targets += [("scroll_base_only", ["tests/unit/test_scroll.c", "src/core/cui.c", "src/base/cui_scroll.c"], False, ["-DCUI_SCROLL_BASE_ONLY=1"])]
        targets += [("toolbar_base_only", ["tests/unit/test_toolbar.c", "src/core/cui.c", "src/base/cui_toolbar.c", "src/base/cui_base.c", "src/base/cui_edit.c"], False, ["-DCUI_TOOLBAR_BASE_ONLY=1"])]
        targets += [("resizable_base_only", ["tests/unit/test_resizable.c", "src/core/cui.c", "src/base/cui_resizable.c"], False, ["-DCUI_RESIZABLE_BASE_ONLY=1"])]
        targets += [("media_base_only", ["tests/unit/test_media.c", "src/base/cui_media.c"], False, ["-DCUI_MEDIA_BASE_ONLY=1"])]
        targets += [("message_base_only", ["tests/unit/test_message.c", "src/base/cui_message.c", "src/base/cui_virtual.c", "src/base/cui_scroll.c", "src/core/cui.c"], False, ["-DCUI_MESSAGE_BASE_ONLY=1"])]
        targets += [("status_base_only", ["tests/unit/test_status.c", "src/base/cui_status.c"], False, ["-DCUI_STATUS_BASE_ONLY=1"])]
        targets += [("otp_base_only", ["tests/unit/test_otp.c", "src/core/cui.c", "src/base/cui_edit.c", "src/base/cui_otp.c"], False, ["-DCUI_OTP_BASE_ONLY=1"])]
        targets += [("form_base_only", ["tests/unit/test_form.c", "src/core/cui.c", "src/base/cui_form.c", "src/base/cui_base.c", "src/base/cui_edit.c"], False, ["-DCUI_FORM_BASE_ONLY=1"])]
        targets += [("surface_base_only", ["tests/unit/test_surface.c", "src/core/cui.c", "src/base/cui_models.c", "src/base/cui_overlay.c", "src/base/cui_surface.c"], False, ["-DCUI_SURFACE_BASE_ONLY=1"])]
        targets += [("drop_shadow_effects_distribution", ["tests/distribution/drop_shadow_effects.c"], False, ["-I", str(root / "dist")])]
        targets += [("inset_shadow_effects_distribution", ["tests/distribution/inset_shadow_effects.c"], False, ["-I", str(root / "dist")])]
        targets += [("implementation_embedding", ["tests/unit/test_core_embed.c"], False, [])]
        targets += [(f"example_{name}", [f"examples/{name}/main.c"], True, [])
                    for name in ("unstyled", "styled_settings")]
        distribution_flags = ["-I", str(root / "dist"), "-DCUI_DISTRIBUTION_CONTROLS=1"]
        targets += [("virtual_model_distribution", ["tests/distribution/virtual_model.c"], False, distribution_flags + virtual_clock)]
        targets += [("dock_model_distribution", ["tests/distribution/dock_model.c"], False, distribution_flags)]
        targets += [("effects_distribution", ["tests/distribution/effects.c"], False, distribution_flags)]
        targets += [("distribution_single", ["tests/distribution/single.c"], False, distribution_flags),
                    ("distribution_multi", ["tests/distribution/implementation.c", "tests/distribution/consumer.c"], False, distribution_flags)]
        targets += [("theme_registry_distribution", ["tests/distribution/theme_registry.c"], False, distribution_flags)]
        if args.suite:
            unknown = set(args.suite) - {name for name, *_ in targets}
            if unknown:
                parser.error("unknown suites: " + ", ".join(sorted(unknown)))
            targets = [target for target in targets if target[0] in args.suite]
        failures = []
        for name, sources, link_library, extra in targets:
            started = time.monotonic()
            module = build / f"{name}.wasm"
            try:
                command(flags + extra + [str(root / source) for source in sources] +
                        ([str(archive)] if link_library else []) +
                        ["-lm", "-Wl,-z,stack-size=8388608", "-o", str(module)], root)
                run_module(module, engine, args.fuel)
                print(f"PASS {name} ({time.monotonic() - started:.2f}s)", flush=True)
            except (RuntimeError, subprocess.SubprocessError, OSError, wasmtime.WasmtimeError) as exc:
                failures.append(name)
                print(f"FAIL {name}: {exc}", flush=True)
        if source_fingerprint(root) != fingerprint:
            raise RuntimeError("Portable source files changed during verification; rerun on a stable checkout")
        print(f"WASI runtime suites: {len(targets) - len(failures)}/{len(targets)} passed", flush=True)
        if failures:
            print("Failed suites: " + ", ".join(failures), flush=True)
        return int(bool(failures))


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.SubprocessError, OSError) as error:
        print(f"WASI runner failed: {error}", file=sys.stderr)
        sys.exit(1)
