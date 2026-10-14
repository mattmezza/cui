# NanoSVG transform validation patch

The pinned parser previously left `nsvg__parseMatrix` output unchanged when fewer
than six arguments were present. Its caller then multiplied an uninitialized
local matrix, or reused a prior transform. Translate, scale and skew read missing
arguments directly; rotate read an absent angle or a missing second center
coordinate when given two arguments. These are genuine undefined reads, beyond
GCC's optimized maybe-uninitialized warning.

The local patch preserves the pinned upstream revision. Rejected matrix, translate,
scale, rotate and skew argument counts now initialize output to identity. Missing
parentheses and excess arguments take the same safe branch. A failed operation
therefore leaves the accumulated transform unchanged, while subsequent valid
operations remain effective. Numeric overflow to infinity is rejected in argument
parsing. Identity fallback is consistent with the parser's tolerant handling of
unsupported or malformed transform text; it does not create a new public status API.

The patch does not claim strict SVG grammar validation. The existing scanner can
skip arbitrary nonnumeric text, and finite matrices can still overflow when
composed. General SVG validation remains the icon adapter's responsibility.
Valid matrix values, one/two-argument translate/scale, one/three-argument rotate,
one-argument skew and transform-list composition retain their geometry.

A separate explicit size_t cast for the existing ftell error sentinel fixes a
preexisting signed-comparison diagnostic found by whole-header strict compilation.
There are no warning suppressions or dependency bumps. `third_party/nanosvg/PIN.md`
records original/patched hashes and patch provenance. The integrator updates the
manifest hash and regenerates the generated icon header outside this worker's scope.

Private `tests/unit/test_svg_transform_validation.c` directly includes the pinned
implementation. It checks every malformed helper, missing delimiters, excess or
missing arguments, malformed center rotation, overflow arguments, valid transforms,
valid-invalid-valid composition (no stale transform reuse), and parsed SVG path
bounds. Assertions remain active under compilers that define NDEBUG for -O3.

Validation commands (all tests headless):

```sh
cc -std=c11 -O3 -Wall -Wextra -Wpedantic -Werror tests/unit/test_svg_transform_validation.c -lm -o /tmp/test-svg-transform-gcc
/tmp/test-svg-transform-gcc
zig cc -std=c11 -O3 -Wall -Wextra -Wpedantic -Werror tests/unit/test_svg_transform_validation.c -lm -o /tmp/test-svg-transform-zig
/tmp/test-svg-transform-zig
cc -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tests/unit/test_svg_transform_validation.c -lm -o /tmp/test-svg-transform-sanitize
/tmp/test-svg-transform-sanitize
cmake -S . -B build -G Ninja
cmake --build build
python scripts/test_local.py --build-dir build
```

The Clang-family optimized check uses pinned Zig 0.16.0 `zig cc`, not a system
Clang installation. Integrator must register the private test in CMake.
