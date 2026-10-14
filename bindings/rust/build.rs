use std::env;
fn main() {
    println!("cargo:rerun-if-env-changed=CUI_BRIDGE_LIB_DIR");
    let dir = env::var("CUI_BRIDGE_LIB_DIR").expect("set CUI_BRIDGE_LIB_DIR to the absolute bridge build directory");
    println!("cargo:rustc-link-search=native={dir}");
    println!("cargo:rustc-link-lib=dylib=cui_bridge");
    #[cfg(target_os = "linux")]
    println!("cargo:rustc-link-arg=-Wl,-rpath,{dir}");
}
