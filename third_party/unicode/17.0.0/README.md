# Unicode 17.0.0 segmentation and casing data

These upstream Unicode files are vendored unchanged and licensed under the
included Unicode License V3. `manifest.json` pins their URLs and SHA-256 hashes.
Generation is offline and checks every input hash before producing output.

- `GraphemeBreakProperty.txt`: Grapheme_Cluster_Break.
- `emoji-data.txt`: Extended_Pictographic.
- `DerivedCoreProperties.txt`: Indic_Conjunct_Break (GB9c), Cased and Case_Ignorable.
- `GraphemeBreakTest.txt`: all 766 default extended-grapheme conformance cases.
- `WordBreakProperty.txt`: Word_Break.
- `WordBreakTest.txt`: all 1,944 default word conformance cases.
- `PropList.txt`: White_Space for word-segment detection and Command query trimming.
- `UnicodeData.txt`: simple case mappings for typed shortcut labels and Command search.
- `SpecialCasing.txt`: full case mappings, including expansions and final sigma.

Run `python3 scripts/generate_grapheme.py` from the repository to regenerate the
marked property table in `src/base/cui_edit.c` and the embedded test fixture in
`tests/unit/cui_edit_grapheme_tests.inc`; use `--check` to verify reproduction.
The implementation follows UAX #29 revision 47. Unicode 17 matches the frozen
GPUI reference's `unicode-segmentation` 1.13.3 dependency. Updating the Unicode
version requires a deliberate data, generator, API-version, and conformance
update; there is no runtime download or system Unicode dependency.

Run `python3 scripts/generate_word.py` (or `--check`) for the marked word-property
table in `src/base/cui_edit.c` and `tests/unit/cui_edit_word_tests.inc`. The raw
Unicode files remain byte-identical to upstream, including upstream whitespace.

Run `python3 scripts/generate_kbd_uppercase.py` (or `--check`) for the Kbd table
and its native/WASI test fixture. The formatter applies default uppercase to
one scalar, without locale or surrounding-text rules. [Unicode case mapping](https://www.unicode.org/reports/tr44/tr44-36.html#Case_And_Case_Mapping)

Run `python3 scripts/generate_command_unicode.py` (or `--check`) for default
Command search tables. Search lowercases full strings with final-sigma context.
