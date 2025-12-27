import socket
import sys
import time
import struct

PORT = int(sys.argv[1])
MODE = sys.argv[2] if len(sys.argv) > 2 else "normal"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('0.0.0.0', PORT))
s.listen(1)
print(f"Mock server on {PORT} mode {MODE}")

while True:
    try:
        c, a = s.accept()
        print(f"Conn: {a}")

        if MODE == "hang_connect":
            time.sleep(10)
            c.close()
            continue

        data = c.recv(1024)
        if b"isu_pt" in data:
            c.send(b"granted")

        while True:
            req = c.recv(1024)
            if not req: break

            if MODE == "hang_data":
                time.sleep(10)
                break

            ts = struct.pack(">q", int(time.time()*1000000))
            if PORT == 5123:
                payload = ts + struct.pack(">fh", 25.5, 100)
            else:
                payload = ts + struct.pack(">iii", 10, 20, 30)

            chk = sum(payload) % 256
            if MODE == "bad_sum": chk = (chk + 1) % 256

            c.send(payload + bytes([chk]))
            time.sleep(0.1)
    except:
        pass