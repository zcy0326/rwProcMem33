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

See `PORTABILITY.md` for artifact boundaries and `ANDROID_DEPLOY.md` for the
target probe, load, smoke-test, and unload sequence.
