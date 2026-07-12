# Source fixes applied

This archive was rebuilt from the uploaded source tree and synchronized with
the newer ABI 1.2/session/memory implementation reviewed on the
`codex/kfi-v1` branch.

Applied fixes:

- linked `kfi_uapi`, `kfi_session`, and `kfi_memory` into the module object;
- fixed the runtime self-test request-header field;
- synchronized module, UAPI, and CMake versions to 1.2.0;
- added reference-counted sessions and the memory ioctl implementation;
- replaced the 64 KiB high-order `kmalloc` buffer with `kvzalloc`;
- preserved partial user-copy progress;
- corrected bool `filldir_t` skip semantics;
- made proc enumeration filtering optional instead of transport-fatal;
- added normalized endpoint path validation;
- added RAII sessions, automatic memory request chunking, explicit close error
  handling, and partial-progress exceptions;
- added an injectable syscall layer and userspace unit tests.

Validation performed in this environment:

- userspace CMake build: passed;
- CTest: 2/2 passed;
- Python tool tests: 10/10 passed;
- UAPI C11 size assertions: passed;
- Python and shell syntax checks: passed.

The kernel module was intentionally not built or loaded because no matching
Android/GKI kernel build tree was supplied.
