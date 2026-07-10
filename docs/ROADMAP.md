# KFI roadmap

KFI is developed as a sequence of independently testable products. Later
stages build on the stable interfaces of earlier stages; unfinished features do
not advertise capabilities.

## V1: kernel debugging backend

- **V1.0 — unified skeleton:** character device, per-FD clients, opaque process
  sessions, ABI negotiation, C++ SDK, and CLI smoke tests. Complete.
- **V1.1 — portable external module:** centralized compatibility layer,
  runtime information, load-time selftest, KMI-family profiles, probe/build/
  verification tools, Android deployment procedure, and cross-build CI.
- **V1.2 — process/session lifecycle:** reusable session lookup/refcount API,
  process and thread enumeration, target-exit handling, and cross-client tests.
- **V1.3 — bounded memory backend:** partial-safe read/write, overflow and user
  pointer validation, per-request limits, and CLI support.
- **V1.4 — maps and scanning:** VMA enumeration in the kernel; module parsing,
  pattern scanning, and dumping in userspace.
- **V1.5 — hardware breakpoints:** client-owned execute breakpoints first, then
  read/write watchpoints after lifecycle and slot-exhaustion tests pass.
- **V1.6 — event transport:** preallocated per-client ring, `read`, `poll`,
  overflow accounting, and no sleeping allocation in breakpoint callbacks.
- **V1.7 — register actions:** per-breakpoint notify, redirect, force-return,
  register updates, and disable-on-hit actions.

V1 exits with a loadable family of profile-specific `kfi.ko` artifacts and a
usable kernel-assisted Native analysis CLI. It does not include an injected
agent or general inline hooks.

## V2: Android daemon and RPC

`kfid` owns long-lived kernel sessions, performs module/symbol resolution,
fans out events, and exposes versioned RPC through an ADB-forwarded local
socket. C++, Python, and TypeScript SDKs share generated protocol fixtures.

## V3: in-process Native agent

`libkfi_agent.so` is initially loaded explicitly by owned test applications. It
provides allocation/protection, executable code management, Native calls, and
inline interception. The first backend should integrate a maintained relocator
such as frida-gum instead of starting with a new ARM64 relocator.

## V4: script runtime

Add a bounded script runtime and Frida-like Process, Module, Memory,
Interceptor, NativeFunction, message, and RPC APIs. Runtime selection is made
after measuring GumJS and QuickJS integration cost against the V3 agent ABI.

## V5: advanced analysis

Function/basic-block tracing, coverage, data-flow assistance, plugins,
Java/ART integration, and Stalker-class dynamic translation remain separate
projects gated by performance and maintenance budgets.

## Delivery rules

- One subsystem and one acceptance target per commit.
- Kernel pointers never cross UAPI boundaries.
- Version-specific code stays in `kfi_compat`, `compat/`, or `profiles/`.
- A failed optional backend clears its capability instead of failing the module.
- Every profile-specific artifact passes module verification and target
  load/ioctl/unload tests before release.
