# Notes

## 2026-01-27 — Device receives its own `device_data` WS push

**Symptom**
- Device logs show it receives `device_data` Socket.IO messages that it originally reported.

**Root cause**
- Server broadcasts `device_data` with `socketio.emit(...)` without scoping to a room or excluding the device's own `sid`, so all connected clients (including the device itself) receive it.

**Options**
- Minimal: `socketio.emit(..., skip_sid=online_devices.get(device_id))` to skip the device connection.
- Cleaner: split rooms (e.g., `apps` room for apps, `device_{id}` for devices) and emit data only to app room.

