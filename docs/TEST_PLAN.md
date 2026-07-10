# KFI V1 test plan

Run these tests on an isolated target kernel built with KASAN and lockdep when
available.

1. A caller without `CAP_SYS_PTRACE` cannot open `/dev/kfi`.
2. `kfi version` returns ABI 1.0 and a nonzero client ID.
3. Two open file descriptors receive different client IDs.
4. `kfi caps` advertises only client isolation and opaque sessions.
5. `kfi attach <live-pid>` returns a nonzero session ID.
6. Attaching a missing PID returns `ESRCH`.
7. Closing a session twice returns `ENOENT`.
8. A session ID created by client A cannot be closed by client B.
9. Exiting without explicit close releases every session without leaks.
10. Repeated open/attach/close cycles pass kmemleak, KASAN, and lockdep checks.
