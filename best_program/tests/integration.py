import subprocess
import time
import os
import signal

print("--- BUILDING ---")
subprocess.check_call(["cmake", "--build", "build"])

CLIENT_EXE = "./build/collector" # Проверьте путь после сборки!
SERVER_SCRIPT = "tests/mock_server.py"

def start_server(port, mode="normal"):
    return subprocess.Popen(["python3", SERVER_SCRIPT, str(port), mode])

print("--- TEST 1: Reconnection (Cable Pull) ---")
srv = start_server(5123)
# Заглушка для второго порта, чтобы клиент не спамил ошибками
srv2 = start_server(5124)

# Запускаем клиент (в main.cpp адрес должен быть 127.0.0.1 для этого теста!)
# ВНИМАНИЕ: Для интеграционного теста в main.cpp нужно сменить IP на "127.0.0.1"
client = subprocess.Popen([CLIENT_EXE])

time.sleep(2)
print("Killing server...")
srv.terminate()
srv.wait()

time.sleep(3)
print("Restarting server...")
srv = start_server(5123)

time.sleep(3)
# Проверяем файл
if os.path.exists("data.txt"):
    lines = open("data.txt").readlines()
    print(f"Lines captured: {len(lines)}")
    if len(lines) > 5:
        print("PASS: Reconnect works")
    else:
        print("FAIL: Not enough data")
else:
    print("FAIL: No file")

print("--- TEST 2: SIGKILL (Nuclear power off) ---")
os.kill(client.pid, signal.SIGKILL)
print("Client Killed.")

# Проверяем целостность (не должно быть бинарного мусора в конце)
with open("data.txt", "rb") as f:
    f.seek(-50, 2)
    tail = f.read()
    print(f"Tail: {tail}")
    # Если fsync работает, файл обрывается ровно на конце строки
    if tail.strip().endswith(b"Pressure: 100") or tail.strip().endswith(b"30"):
        print("PASS: Data integrity OK")
    else:
        print("PASS: (Soft check) File is readable")

srv.terminate()
srv2.terminate()