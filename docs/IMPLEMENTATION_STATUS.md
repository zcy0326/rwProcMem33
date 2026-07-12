# Implementation status

| Area | Status | Verification | Pending |
|---|---|---|---|
| ABI 1.2 and runtime info | Implemented | C/C++ layout assertions; userspace build | Target kernel build and device test |
| Per-open client and opaque sessions | Implemented | Static review; userspace RAII lifecycle tests | Concurrent kernel/device tests |
| Shared transport dispatcher | Implemented | Static review | Target kernel build |
| Character-device transport | Implemented | Userspace endpoint tests | Android device-node test |
| Private procfs transport | Implemented, optional enum filter, default off | Generator tests; path validation tests; static review | Target kernel build and procfs ioctl test |
| C++ endpoint abstraction | Implemented | CTest endpoint coverage | Device integration |
| Process memory backend | Kernel and C++ initial implementation | UAPI wiring; automatic chunking and partial-progress tests | Target kernel build and device read/write test |
| Maps and threads | Source complete for initial paged enumeration | ABI/layout assertions; pagination tests; SDK and CLI wiring; static review | Target kernel build and live-process pagination tests |
| Module/proc visibility controls | Implemented | UAPI assertions; static review | Target kernel/device test; proc hook portability |
| Hardware breakpoints and events | Not started | None | Next V1 milestone |

Kernel sources in this branch have not been built against a matching
Android/GKI tree and have not been validated with `insmod` on a device.
