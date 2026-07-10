# Build

## Kernel module

Use the target Android/Linux kernel build tree and its matching toolchain:

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" modules
```

An Android profile can be embedded in runtime information with:

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" ARCH=arm64 LLVM=1 \
  KFI_PROFILE=android15-6.6 modules
```

The character-device transport is enabled by default. To add the private
procfs transport, first generate its build configuration, then pass the
transport switches to Kbuild:

```sh
python3 scripts/gen_private_config.py \
  --output kernel/generated/kfi_private_config.h \
  --user-config build/generated/kfi_endpoint.json

make -C "$KERNEL_OUT" M="$PWD/kernel" ARCH=arm64 LLVM=1 \
  KFI_TRANSPORT_CHAR=y KFI_TRANSPORT_PROC_PRIVATE=y modules
```

Use `KFI_TRANSPORT_CHAR=n` only when the proc transport is enabled. The build
rejects configurations with no transport and rejects proc builds that lack the
generated header. `scripts/build_kfi.py` exposes the same selection through
`--proc-private` and `--no-char`.

For Android GKI, build against the exact device KMI and symbol list. The new
module uses exported kernel APIs and does not require disabling CFI/KCFI.
Verify every output with `python3 scripts/verify_module.py kernel/kfi.ko`.

## Userspace

On Linux or with the Android NDK toolchain:

```sh
cmake -S user -B build/user -DCMAKE_BUILD_TYPE=Release
cmake --build build/user
```

The resulting binary is `build/user/kfi`.

Run userspace tests with `ctest --test-dir build/user --output-on-failure`.

See `PORTABILITY.md` for artifact boundaries and `ANDROID_DEPLOY.md` for the
target probe, load, smoke-test, and unload sequence.
