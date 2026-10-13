# SDL GPU renderer

The Vulkan/SPIR-V adapter submits mesh commands through a dedicated vertex buffer
and premultiplied triangle pipeline. The fragment shader evaluates quadratic
coverage from interpolated ST derivatives; it never rasterizes a mesh bitmap.
The frame limit is 196608 mesh vertices and 32768 draw records. Invalid meshes
or capacity overflow reject the frame before the render pass changes the target.

The RGBA8 UNORM target retains the existing byte-domain primitive behavior. Linear
premultiplied paint is unpremultiplied, sRGB encoded, and premultiplied by alpha
and analytic coverage before ONE/ONE_MINUS_SRC_ALPHA blending. This conversion
preserves colors alongside byte primitives; it is not linear-light framebuffer
compositing. The native pixel tests require a real Vulkan device and window,
and exercise overlap, transformed placement, DPI clipping and curve coverage.

Coverage follows the frozen lockfile's `gpui-pre-wgpu 0.3.8` shader function
`src/shaders.wgsl::fs_path_rasterization`, including the norm of both S
derivatives in its0.001 solid fast path. Intel Vulkan readback checks numeric
coverage for ordinary and vertical-S triangles, mixed rectangle/mesh ordering,
borrowed vertices, mesh capacity failure and unknown-command rejection.
