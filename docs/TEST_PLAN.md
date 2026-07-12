# KFI V1 test plan

## Tests available without a kernel module

Run from the repository root:

```sh
cmake -S user -B build/user -DCMAKE_BUILD_TYPE=Release
cmake --build build/user --parallel
ctest --test-dir build/user --output-on-failure
PYTHONPATH=. python3 -m unittest discover -s tests/tools -p 'test_*.py'
```

Current userspace coverage includes endpoint normalization, environment
selection, RAII session lifetime, automatic memory chunking, partial-progress
errors, UAPI layouts, enumeration page accumulation, invalid counts, unknown
flags, non-progressing cursors, event stats, poll readiness, event batches,
and malformed event streams.

## Target-kernel tests

Run on an isolated matching Android/GKI kernel, with KASAN and lockdep when
available:

1. A caller without `CAP_SYS_PTRACE` cannot open an endpoint.
2. Version/capability/runtime queries are internally consistent.
3. Session IDs are isolated by client and stale IDs return `ENOENT`.
4. Concurrent memory operations and session close do not trigger UAF or leaks.
5. Memory operations cover cross-page, inaccessible, partially mapped, and
   exited targets while reporting accurate progress.
6. Thread enumeration includes the leader exactly once and paginates processes
   with more than `KFI_ENUM_MAX_ENTRIES` threads.
7. Thread creation and exit during enumeration do not crash the kernel; results
   are documented as best effort.
8. Map enumeration covers anonymous, file-backed, shared/private, executable,
   deleted, and long-path VMAs.
9. Map pagination makes progress from address zero through `END`, including a
   process whose map layout changes between requests.
10. Session open and explicit close produce ordered lifecycle events.
11. Blocking and nonblocking event reads return only complete 128-byte records.
12. `poll`/`epoll` report readable and shutdown states correctly.
13. Filling the ring increments `lost`, creates observable sequence gaps, and
    does not allocate in the producer path.
14. Concurrent readers do not duplicate or reorder events.
15. Character and proc transports return identical ioctl and event behavior.
16. Failure of optional proc filtering leaves the private transport usable.
17. Repeated load/open/attach/read/maps/threads/events/close/unload cycles pass
    kmemleak, KASAN, and lockdep checks.
18. 32-bit compat clients observe the same fixed-width UAPI layouts.
19. Probe/load/ioctl/read/poll/unload succeeds on every supported KMI profile.
