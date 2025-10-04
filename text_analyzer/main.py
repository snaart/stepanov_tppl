import sys
from collections import Counter


def get_line_count(lines):
    print(f"Количество строк: {len(lines)}")


def get_char_count(lines):
    content = "".join(lines)
    print(f"Количество символов: {len(content)}")


def get_empty_line_count(lines):
    empty_lines = 0
    for line in lines:
        if not line.strip():
            empty_lines += 1
    print(f"Количество пустых строк: {empty_lines}")


def get_char_frequency(lines):
    content = "".join(lines)
    char_frequency = Counter(content)
    print("Частотный словарь символов:")
    # Сортируем для более предсказуемого вывода
    for char, freq in sorted(char_frequency.items()):
        # repr() помогает отображать невидимые символы, такие как '\n'
        char_display = repr(char)[1:-1]
        print(f"  '{char_display}': {freq}")


def run_analyzer(filename):
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Ошибка: Файл '{filename}' не найден.")
        return
    except Exception as e:
        print(f"Произошла ошибка при чтении файла: {e}")
        return

    operations = {
        '1': get_line_count,
        '2': get_char_count,
        '3': get_empty_line_count,
        '4': get_char_frequency,
    }

    while True:
        print("\nВыберите операцию для анализа файла:")
        print("1. Количество строк")
        print("2. Количество символов")
        print("3. Количество пустых строк")
        print("4. Частотный словарь символов")
        print("5. Выполнить все операции")
        print("0. Выход")

        choice = input("Введите номера опций через запятую (например, 1,3): ")

        if choice.strip() == '0':
            print("Завершение работы.")
            break

        choices = {c.strip() for c in choice.split(',')}

        print("-" * 30)

        if '5' in choices:
            for key in sorted(operations.keys()):
                operations[key](lines)
        else:
            for user_choice in choices:
                if user_choice in operations:
                    operations[user_choice](lines)
                else:
                    if user_choice:
                        print(f"Неизвестная опция: '{user_choice}'")

        print("-" * 30)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Использование: python main.py <имя_файла>")
    else:
        file_to_analyze = sys.argv[1]
        run_analyzer(file_to_analyze)