BUILD ?= build
.PHONY: all check lint format format-check gallery bench dist dist-check
all:
	cmake -S . -B $(BUILD) -G Ninja
	cmake --build $(BUILD)
check: all
	python scripts/test_local.py --build-dir $(BUILD)
lint:
	python scripts/check_manifest.py
	python scripts/check_licenses.py
	python scripts/theme_presets.py --check
format:
	python scripts/format.py
format-check:
	python scripts/format.py --check
gallery: all
	$(BUILD)/cui_gallery
bench: all
	$(BUILD)/cui_bench
dist:
	python scripts/amalgamate.py
dist-check:
	python scripts/amalgamate.py --check
