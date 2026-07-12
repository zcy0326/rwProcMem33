# Event ring source release

This source package adds the ABI 1.3 per-client event ring milestone on top of
the maps/threads source-complete tree.

Validation performed without building `kfi.ko`:

- CMake userspace build: passed
- CTest: 5/5 passed
- Python tool tests: 10/10 passed
- C11 UAPI layout compile: passed
- Python and shell syntax checks: passed

This snapshot is ready for publication to the configured `codex/kfi-v1`
remote branch after local verification.
