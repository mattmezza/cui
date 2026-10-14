use std::{env, path::PathBuf, process::Command};
fn run(command: &mut Command) {
    let status = command
        .status()
        .expect("CUI package build requires CMake and a C11 compiler");
    assert!(
        status.success(),
        "CUI package native build failed: {command:?}"
    );
}
fn main() {
    println!("cargo:rustc-check-cfg=cfg(cui_bridge_static)");
    println!("cargo:rerun-if-env-changed=CUI_BRIDGE_LIB_DIR");
    println!("cargo:rerun-if-env-changed=CUI_PACKAGE_BUILD_TYPE");
    if let Ok(dir) = env::var("CUI_BRIDGE_LIB_DIR") {
        println!("cargo:rustc-link-search=native={dir}");
        println!("cargo:rustc-link-lib=dylib=cui_bridge");
        #[cfg(target_os = "linux")]
        println!("cargo:rustc-link-arg=-Wl,-rpath,{dir}");
        return;
    }
    assert_eq!(
        env::var("CARGO_CFG_TARGET_OS").unwrap(),
        "linux",
        "Alpha source packaging currently validates Linux only"
    );
    assert_eq!(
        env::var("HOST").unwrap(),
        env::var("TARGET").unwrap(),
        "Cross compilation is not validated"
    );
    let root = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let staged = root.join("sdk/source");
    let checkout = root.join("../..");
    let source = if staged.join("CMakeLists.txt").is_file() {
        staged
    } else if checkout.join(".git").exists() {
        checkout
    } else {
        panic!("Packaged CUI sources are missing; rebuild the alpha crate artifact")
    };
    let build = PathBuf::from(env::var_os("OUT_DIR").unwrap()).join("cui-native");
    let native = env::var_os("CARGO_FEATURE_NATIVE").is_some();
    run(Command::new("cmake")
        .arg("-S")
        .arg(root.join("packaging"))
        .arg("-B")
        .arg(&build)
        .arg(format!("-DCUI_PACKAGE_ROOT={}", source.display()))
        .arg(format!(
            "-DCUI_BUILD_NATIVE={}",
            if native { "ON" } else { "OFF" }
        ))
        .arg(format!(
            "-DCMAKE_BUILD_TYPE={}",
            env::var("CUI_PACKAGE_BUILD_TYPE").unwrap_or_else(|_| "Release".into())
        )));
    run(Command::new("cmake").arg("--build").arg(&build).args([
        "--target",
        "cui_rust_bridge",
        "-j2",
    ]));
    println!("cargo:rerun-if-changed={}", source.display());
    println!("cargo:rustc-link-search=native={}", build.display());
    println!(
        "cargo:rustc-link-search=native={}",
        build.join("cui_source").display()
    );
    println!("cargo:rustc-cfg=cui_bridge_static");
    println!("cargo:rustc-link-lib=static=cui_rust_bridge");
    if native {
        println!("cargo:rustc-link-lib=static=cui_sdl3");
        println!("cargo:rustc-link-lib=static=cui_text");
        println!("cargo:rustc-link-lib=static=cui_bidi");
    }
    println!("cargo:rustc-link-lib=static=cui");
    println!("cargo:rustc-link-lib=m");
    if native {
        let output = Command::new("pkg-config")
            .args(["--libs", "sdl3", "freetype2", "harfbuzz"])
            .output()
            .expect("sdl feature requires pkg-config");
        assert!(
            output.status.success(),
            "Missing system SDL3/FreeType/HarfBuzz libraries"
        );
        for flag in String::from_utf8(output.stdout).unwrap().split_whitespace() {
            if let Some(lib) = flag.strip_prefix("-l") {
                println!("cargo:rustc-link-lib={lib}");
            } else if let Some(path) = flag.strip_prefix("-L") {
                println!("cargo:rustc-link-search=native={path}");
            } else {
                println!("cargo:rustc-link-arg={flag}");
            }
        }
    }
}
