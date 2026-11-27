/* SPDX-License-Identifier: MIT */
static struct {
 uint32_t (*versions)(void);
 cui_bridge_status (*set)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_handle_v1);
 cui_bridge_status (*get)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_handle_v1 *);
} spinner_icon_api;
static int spinner_icon_supported;
static void go_spinner_icon_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&spinner_icon_api.versions, sizeof spinner_icon_api.versions, "cui_bridge_spinner_icon_resource_versions");
  go_load_optional_symbol(&spinner_icon_api.set, sizeof spinner_icon_api.set, "cui_bridge_spinner_set_icon_resource_v1");
  go_load_optional_symbol(&spinner_icon_api.get, sizeof spinner_icon_api.get, "cui_bridge_spinner_get_icon_resource_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  spinner_icon_api.versions = cui_bridge_spinner_icon_resource_versions;
  spinner_icon_api.set = cui_bridge_spinner_set_icon_resource_v1;
  spinner_icon_api.get = cui_bridge_spinner_get_icon_resource_v1;
 }
#endif
 spinner_icon_supported = spinner_icon_api.versions && spinner_icon_api.set && spinner_icon_api.get && (spinner_icon_api.versions() & 1);
}
static int go_spinner_icon_supported(void) { return spinner_icon_supported; }
static cui_bridge_status go_spinner_icon_set(cui_bridge_app *a, cui_bridge_node n, cui_bridge_icon_handle_v1 h) {
 return spinner_icon_supported ? spinner_icon_api.set(a,n,h) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_spinner_icon_get(cui_bridge_app *a, cui_bridge_node n, cui_bridge_icon_handle_v1 *h) {
 return spinner_icon_supported ? spinner_icon_api.get(a,n,h) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
