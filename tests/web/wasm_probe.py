"""Load compiled test reactors with an intercepted browser fetch."""

from pathlib import Path
from playwright.sync_api import Page, Route


def route_wasm_probe(page: Page, wasm: Path) -> str:
    """Serve the exact local artifact, including in an offline browser context."""
    url = "https://cui-wasm-probe.test/module.wasm"

    def serve(route: Route) -> None:
        route.fulfill(path=wasm, content_type="application/wasm",
                      headers={"access-control-allow-origin": "*"})

    page.route(url, serve)
    return url
