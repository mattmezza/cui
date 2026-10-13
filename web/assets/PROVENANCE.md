# Original CUI gallery media

`alpine-lake-original.png` was generated on 2026-10-12 using the built-in imagegen tool for this project. It is an illustrative alpine scene, not a photograph of a verified real location. No upstream GPUI branding or media was used.

Prompt: “A polished editorial landscape image of a serene alpine lake in Switzerland, mountain ridges in the background and a small wooden lakeside cabin, clear early morning light, natural greens and turquoise water with warm sunlit stone, realistic fine detail. Wide horizontal 16:9 landscape. No people, text, UI, brands, logos or watermark.”

`alpine-lake.jpg` is the web derivative. `../gallery_image.inc` embeds a 256×144 RGBA derivative for the real C/WASM Image and Attachment loader. The original generated PNG is retained here; ImageMagick performed format conversion and resizing. The gallery's ready image is static; animation remains tested separately by the image model tests.

## CUI website product media

`cui-workspace-light.png` and `cui-workspace-dark.png` are original screenshots of this repository's actual C → WASM Resizable composition, captured at device scale 2. `cui-social.png` is an original 1200×630 layout composed from the light screenshot, text and a generated gradient; it contains no upstream screenshot or branding. Regenerate with `uv run scripts/generate_site_media.py --directory build/web --chromium /path/to/chrome`. The script pins Playwright 1.58.0 and Pillow 11.3.0. Text is rasterized with Noto Sans, supplied by the developer's installed font; no font file is distributed. These images illustrate the current working composition and do not certify complete native-platform or GPUI parity.
