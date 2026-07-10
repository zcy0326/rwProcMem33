# KFI V1 test plan

Run these tests on an isolated target kernel built with KASAN and lockdep when
available.

1. A caller without `CAP_SYS_PTRACE` cannot open `/dev/kfi`.
2. `kfi version` returns ABI 1.1 and a nonzero client ID.
3. Two open file descriptors receive different client IDs.
4. `kfi caps` advertises client isolation, opaque sessions, and runtime info.
5. `kfi attach <live-pid>` returns a nonzero session ID.
6. Attaching a missing PID returns `ESRCH`.
7. Closing a session twice returns `ENOENT`.
8. A session ID created by client A cannot be closed by client B.
9. Exiting without explicit close releases every session without leaks.
10. Repeated open/attach/close cycles pass kmemleak, KASAN, and lockdep checks.
11. Nonzero request reserved fields and unknown flags return `EINVAL`.
12. `kfi runtime` matches the target release, machine, page size, and profile.
13. A 32-bit compat client receives the same fixed-width UAPI layouts.
14. `verify_module.py` rejects wrong-machine, wrong-ABI, missing-vermagic, and
    denied-symbol artifacts.
15. Probe/load/ioctl/unload succeeds on every supported KMI build target.
