#!/usr/bin/env python3
import argparse, struct, sys

SOF = b"\xAA\x55"

def crc16_ccitt(data: bytes, init=0xFFFF) -> int:
    crc = init
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc

def build(cmd: int, payload: bytes=b"") -> bytes:
    body = bytes([cmd]) + payload
    ln = len(body)
    crc = crc16_ccitt(body)
    return SOF + bytes([ln]) + body + struct.pack("<H", crc)

def q88(x: float) -> int:
    return int(round(x * 256.0))

def cmd_payload(args):
    name = args.command
    if name == "start": return 0x01, b""
    if name == "stop": return 0x02, b""
    if name == "set_speed": return 0x03, struct.pack("<h", int(args.rpm))
    if name == "get_status": return 0x04, b""
    if name == "clear_fault": return 0x05, b""
    if name == "set_pid_dual":
        return 0x06, struct.pack("<hhhhhh",
                                 q88(args.pos_kp), q88(args.pos_ki), q88(args.pos_kd),
                                 q88(args.spd_kp), q88(args.spd_ki), q88(args.spd_kd))
    if name == "set_pos": return 0x07, struct.pack("<i", int(args.mrev))
    if name == "set_motion": return 0x08, struct.pack("<hh", int(args.max_rpm), int(args.accel_rpm_s))
    if name == "camera": return 0x09, struct.pack("<HH", int(args.delay_us), int(args.width_us))
    if name == "set_current": return 0x0A, struct.pack("<H", int(args.ma))
    if name == "set_pid_current": return 0x0B, struct.pack("<hh", q88(args.cur_kp), q88(args.cur_ki))
    raise SystemExit("unknown command")

def hexdump(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)

def add_cmds(sp):
    c = sp.add_subparsers(dest="command", required=True)
    c.add_parser("start")
    c.add_parser("stop")
    c.add_parser("get_status")
    c.add_parser("clear_fault")
    s = c.add_parser("set_speed"); s.add_argument("--rpm", type=int, required=True)
    p = c.add_parser("set_pos"); p.add_argument("--mrev", type=int, required=True, help="milli-rev (1000=1rev)")
    m = c.add_parser("set_motion"); m.add_argument("--max_rpm", type=int, required=True); m.add_argument("--accel_rpm_s", type=int, required=True)
    d = c.add_parser("set_pid_dual")
    d.add_argument("--pos_kp", type=float, required=True); d.add_argument("--pos_ki", type=float, required=True); d.add_argument("--pos_kd", type=float, required=True)
    d.add_argument("--spd_kp", type=float, required=True); d.add_argument("--spd_ki", type=float, required=True); d.add_argument("--spd_kd", type=float, required=True)
    cam = c.add_parser("camera"); cam.add_argument("--delay_us", type=int, required=True); cam.add_argument("--width_us", type=int, required=True)
    cur = c.add_parser("set_current"); cur.add_argument("--ma", type=int, required=True)
    curpi = c.add_parser("set_pid_current"); curpi.add_argument("--cur_kp", type=float, required=True); curpi.add_argument("--cur_ki", type=float, required=True)

def main():
    ap = argparse.ArgumentParser(description="PC -> FW command sender")
    sub = ap.add_subparsers(dest="mode", required=True)
    f = sub.add_parser("frame", help="print frame hex + raw bytes")
    s = sub.add_parser("send", help="send over serial (pyserial needed)")
    s.add_argument("--port", required=True)
    s.add_argument("--baud", type=int, default=115200)
    add_cmds(f); add_cmds(s)

    args = ap.parse_args()
    cmd, payload = cmd_payload(args)
    fr = build(cmd, payload)

    if args.mode == "frame":
        print(hexdump(fr))
        sys.stdout.buffer.write(fr)
        return 0

    try:
        import serial  # type: ignore
    except Exception as e:
        raise SystemExit("pyserial not installed. pip install pyserial") from e

    with serial.Serial(args.port, args.baud, timeout=1) as ser:
        ser.write(fr); ser.flush()
    print("sent:", hexdump(fr))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
