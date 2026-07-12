# Event ring milestone

KFI ABI 1.3 adds a preallocated event stream to every opened client. Both the
character-device and private procfs transports expose the same fixed-size
records through `read()` and readiness through `poll()`/`epoll()`.

## Kernel design

- Each `kfi_client` owns a 256-entry ring of 128-byte `kfi_event` records.
- Producers use a spinlock and preallocated storage only. Event emission does
  not allocate or sleep, so later hardware-breakpoint callbacks can use it.
- The ring drops new records when full, increments `lost`, and still advances
  sequence numbers. Sequence gaps and `KFI_IOC_GET_EVENT_STATS` expose loss.
- Readers are serialized by a mutex. Records are copied to a temporary buffer,
  copied to userspace, and only then removed from the ring.
- Blocking reads wake on new events or client shutdown. `O_NONBLOCK` returns
  `EAGAIN` when no record is queued. Shutdown drains queued records and then
  returns EOF.

## Initial producers

Session creation and explicit session close emit `SESSION_OPENED` and
`SESSION_CLOSED`. Process/thread lifecycle and hardware breakpoint event types
are reserved for their corresponding backends.

## Userspace

`libkfi` provides:

```cpp
client.event_stats();
client.wait_for_events(timeout_ms);
client.read_events(max_events);
```

The CLI provides one-shot smoke commands:

```sh
kfi event-stats
kfi events PID
```

The event ring has not yet been built or exercised on a matching Android/GKI
kernel. Current verification is UAPI layout, userspace mock behavior, and
static review.
