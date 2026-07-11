# Bundled Windows test applications

These files are used by the one-click "DXVK GPU Test" in the application list
(see issue #20). They are executed in-place and are **never copied into a Wine
bottle**; the DLLs are only picked up by the test process itself via a
per-process `WINEDLLOVERRIDES`, so the content and registry of user bottles
stay untouched.

| File | Origin | License |
|------|--------|---------|
| `d3d11-triangle.exe` | DXVK Direct3D 11 demo (https://github.com/doitsujin/dxvk, `d3d11-triangle` test build) | zlib/libpng |
| `d3d11.dll`, `dxgi.dll` | DXVK v2.7.1 release (x64), https://github.com/doitsujin/dxvk/releases/tag/v2.7.1 | zlib/libpng |
| `d3dcompiler_47.dll` | Microsoft DirectX shader compiler (x64), redistributable component of the Windows SDK | Microsoft SDK redistributable |

DXVK v2.x is bundled deliberately (instead of v3.x): the GPU test should run on
as many Vulkan drivers as possible, and DXVK v3 raised the minimum driver
requirements (e.g. `VK_KHR_load_store_op_none`). To upgrade, replace the two
DXVK DLLs with the x64 DLLs from a newer release tarball. This is the bare
minimum DLL set: DXVK's `d3d11` requires DXVK's `dxgi` (Wine's builtin dxgi
provides no DXVK adapter), and Wine's builtin d3dcompiler cannot compile the
demo's shaders.
