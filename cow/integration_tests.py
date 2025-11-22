import subprocess
import sys
import os

GREEN = '\033[92m'
RED = '\033[91m'
RESET = '\033[0m'

def run_test(
        exe_path,
        test_name,
        cow_code,
        expected_output,
        input_data=None
):
    filename = "temp_test.cow"

    with open(filename, "w") as f:
        f.write(cow_code)

    try:
        result = subprocess.run(
            [exe_path, filename],
            input=input_data,
            capture_output=True,
            text=True,
            timeout=2
        )

        actual_output = result.stdout

        if actual_output == expected_output:
            print(f"{GREEN}[PASS]{RESET} {test_name}")
            return True
        else:
            print(f"{RED}[FAIL]{RESET} {test_name}")
            print(f"  Expected: '{expected_output}'")
            print(f"  Got:      '{actual_output}'")
            print(f"  Code:     {cow_code}")
            return False

    except subprocess.TimeoutExpired:
        print(f"{RED}[FAIL]{RESET} {test_name} (Timed out - infinite loop?)")
        return False
    except Exception as e:
        print(f"{RED}[ERROR]{RESET} {test_name}: {e}")
        return False
    finally:
        if os.path.exists(filename):
            os.remove(filename)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 integration_tests.py <path_to_cow_executable>")
        sys.exit(1)

    exe_path = sys.argv[1]
    all_passed = True

    print("--- Starting Integration Tests ---")

    # Тест 1: Просто вывод числа 0 (OOM = Print Int)
    # Код: OOM
    if not run_test(exe_path, "Print Zero", "OOM", "0"):
        all_passed = False

    # Тест 2: Инкремент и вывод (MoO = INC, OOM = Print)
    # Код: MoO MoO OOM (должно быть 2)
    if not run_test(exe_path, "Increment Twice", "MoO MoO OOM", "2"):
        all_passed = False

    # Тест 3: Вывод символа 'A' (ASCII 65)
    # Тут код длинный, сгенерируем его питоном: 65 раз MoO (INC), потом Moo (Print Char)
    code_char_a = ("MoO " * 65) + "Moo"
    if not run_test(exe_path, "Print Char 'A'", code_char_a, "A"):
        all_passed = False

    # Тест 4: Проверка ввода (oom = Read Int, OOM = Print Int)
    # Код: oom OOM
    # Ввод: "123" -> Ожидаем "123"
    if not run_test(exe_path, "Read and Print Int", "oom OOM", "123", input_data="123"):
        all_passed = False

    # Тест 5: Проверка программы Hello, World!
    # Код: в файле ../cow_examples/hello.cow
    # Ожидаем "Hello, World!"
    file = None
    with open("../cow_examples/hello.cow", 'r') as f:
        file = f.read()
    if not run_test(exe_path, "Hello, World!", file, "Hello, World!"):
        all_passed = False

    # Тест 6: Проверка программы на числа Фибонначи!
    # Код: в файле ../cow_examples/fib.cow
    # Ожидаем "1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, ..."
    file = None
    with open("../cow_examples/fib.cow", 'r') as f:
        file = f.read()
    if not run_test(exe_path, "Fibo", file, "1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, ..."):
        all_passed = False

    # Тест 7: Проверка программы сложения двух чисел
    # Код: в файле ../cow_examples/add.cow
    file = None
    with open("../cow_examples/add.cow", 'r') as f:
        file = f.read()
    for i in range(10):
        if not run_test(exe_path, f"Add {i} + {i}", file, f"{i+i}", input_data=f"{i} {i}"):
            all_passed = False

    # Тест 8: 99 бутылок пива
    # Код: в файле ../cow_examples/99.cow
    # Вывод: в файле ../cow_examples/99.txt
    file = None
    with open("../cow_examples/99.cow", 'r') as f:
        file = f.read()

    output_file = None
    with open("../cow_examples/99.txt", 'r') as f:
        output_file = f.read()

    if not run_test(exe_path, f"99 bottles of beer", file, output_file):
        all_passed = False

    if all_passed:
        print(f"\n{GREEN}All integration tests passed!{RESET}")
        sys.exit(0)
    else:
        print(f"\n{RED}Some tests failed.{RESET}")
        sys.exit(1)