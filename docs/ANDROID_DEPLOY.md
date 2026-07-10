# Android deployment

## 1. Probe the target

```sh
adb push scripts/probe_device.sh /data/local/tmp/
adb shell su -c 'chmod 700 /data/local/tmp/probe_device.sh'
adb shell su -c '/data/local/tmp/probe_device.sh' > device-profile.txt
```

Use the report to select the Android common-kernel/KMI branch, toolchain,
generated headers, `Module.symvers`, page size, and build profile.

## 2. Build and verify

```sh
make -C "$KERNEL_OUT" M="$PWD/kernel" ARCH=arm64 LLVM=1 modules
python3 scripts/verify_module.py kernel/kfi.ko
```

Do not deploy an artifact whose machine, vermagic, ABI, or symbol verification
fails.

## 3. Load

```sh
adb push kernel/kfi.ko /data/local/tmp/
adb shell su -c 'insmod /data/local/tmp/kfi.ko'
adb shell su -c 'dmesg | tail -n 50'
adb shell su -c 'cat /proc/devices | grep kfi'
adb shell su -c 'ls -l /dev/kfi /sys/class/kfi 2>/dev/null'
```

Android ueventd may not create the node for a dynamically loaded external
module. If `/proc/devices` lists `kfi` but `/dev/kfi` is absent, create a
temporary owner-only node for the test session:

```sh
adb shell su -c 'major=$(awk '\''$2 == "kfi" { print $1 }'\'' /proc/devices); test -n "$major" && mknod /dev/kfi c "$major" 0 && chmod 600 /dev/kfi'
```

Production images should use a stable ueventd rule and a dedicated SELinux
domain for the daemon. Do not make the node world-accessible.

## 4. Smoke test

```sh
adb push build/user/kfi /data/local/tmp/
adb shell su -c '/data/local/tmp/kfi version'
adb shell su -c '/data/local/tmp/kfi caps'
adb shell su -c '/data/local/tmp/kfi runtime'
```

The runtime output must match the intended release, machine, page size, and
profile.

## 5. Unload

Close all KFI clients before unloading:

```sh
adb shell su -c 'rmmod kfi'
adb shell su -c 'dmesg | tail -n 50'
```

An unload failure indicates an open reference or target-kernel module policy.
Do not force removal; identify and close the holder first.
