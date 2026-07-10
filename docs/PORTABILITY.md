# External-module portability

KFI is one source tree that produces separate `.ko` artifacts for distinct
kernel/KMI families. A binary built for `android14-6.1` is not expected to load
on `android15-6.6`, even when both devices are ARM64.

## Required target inputs

A reproducible build records:

- kernel release and Android common-kernel branch;
- KMI generation and symbol list;
- generated kernel headers, `.config`, and `Module.symvers`;
- matching LLVM/binutils toolchain;
- page size, module-signing policy, MODVERSIONS, CFI/KCFI, and LTO settings;
- device/vendor module directories and SELinux deployment policy.

Run `scripts/probe_device.sh` on the target before selecting a build profile.
The script is read-only and reports unavailable fields explicitly.

## Profiles and capabilities

Profiles name the build family. They do not promise that a feature works on
every vendor kernel in that family. `KFI_IOC_GET_RUNTIME_INFO` reports the
compiled profile, release, architecture, page size, and relevant build flags.
`KFI_IOC_GET_CAPS` remains authoritative for callable features.

The initial profile names are documented in `kernel/profiles/README.md`. Builds
without an explicit profile report `generic`.

## Compatibility boundary

Kernel API differences are isolated in `kfi_compat.c`. Core and feature code
should call compatibility wrappers instead of adding scattered version checks.
Vendor backports that alter API signatures require a target profile or feature
probe; a version-number guess is not sufficient.

KFI does not require CFI/KCFI to be disabled. Indirect calls must use the exact
function prototype expected by the target kernel. A backend that cannot satisfy
that requirement stays disabled and does not advertise its capability.

## Artifact verification

After every build, run:

```sh
python3 scripts/verify_module.py dist/android15-6.6/kfi.ko
```

The verifier checks ELF machine/class/type, vermagic, KFI ABI metadata,
relocations, denied direct symbol dependencies, and an optional KMI allowlist:

```sh
python3 scripts/verify_module.py kfi.ko \
  --allow-symbol-list path/to/abi_symbollist
```

Verification proves artifact consistency, not loadability. Final acceptance is
an `insmod`/ioctl/`rmmod` cycle on the matching target kernel.
