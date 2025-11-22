#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <iostream>

#define UNIT_TEST
#include "cow.cpp"
#include <gmock/gmock-matchers.h>

// --- Класс-помощник для перехвата cin/cout ---
class IORedirect {
public:
    IORedirect(const std::string& input) : input_stream(input) {
        old_cin = std::cin.rdbuf(input_stream.rdbuf());
        old_cout = std::cout.rdbuf(output_stream.rdbuf());
        old_cerr = std::cerr.rdbuf(error_stream.rdbuf());
    }

    ~IORedirect() {
        std::cin.rdbuf(old_cin);
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
    }

    std::string getOutput() const { return output_stream.str(); }
    std::string getError() const { return error_stream.str(); }

private:
    std::stringstream input_stream;
    std::stringstream output_stream;
    std::stringstream error_stream;
    std::streambuf* old_cin;
    std::streambuf* old_cout;
    std::streambuf* old_cerr;
};

TEST(HelperTest, IsCowChar) {
    EXPECT_TRUE(is_cow_char('m'));
    EXPECT_TRUE(is_cow_char('M'));
    EXPECT_TRUE(is_cow_char('o'));
    EXPECT_TRUE(is_cow_char('O'));
    EXPECT_FALSE(is_cow_char('x'));
    EXPECT_FALSE(is_cow_char(' '));
    EXPECT_FALSE(is_cow_char('1'));
}

TEST(HelperTest, GetCommandCode) {
    // Проверка всех валидных команд
    EXPECT_EQ(get_command_code('m', 'o', 'o'), OP_LOOP_END);
    EXPECT_EQ(get_command_code('m', 'O', 'o'), OP_MOVE_LEFT);
    EXPECT_EQ(get_command_code('m', 'o', 'O'), OP_MOVE_RIGHT);
    EXPECT_EQ(get_command_code('m', 'O', 'O'), OP_EXEC_CELL);
    EXPECT_EQ(get_command_code('M', 'o', 'o'), OP_IO_CHAR);
    EXPECT_EQ(get_command_code('M', 'O', 'o'), OP_DEC);
    EXPECT_EQ(get_command_code('M', 'o', 'O'), OP_INC);
    EXPECT_EQ(get_command_code('M', 'O', 'O'), OP_LOOP_START);
    EXPECT_EQ(get_command_code('O', 'O', 'O'), OP_ZERO);
    EXPECT_EQ(get_command_code('M', 'M', 'M'), OP_REGISTER);
    EXPECT_EQ(get_command_code('O', 'O', 'M'), OP_PRINT_INT);
    EXPECT_EQ(get_command_code('o', 'o', 'm'), OP_READ_INT);

    // Проверка невалидных комбинаций
    EXPECT_EQ(get_command_code('m', 'm', 'm'), OP_INVALID);
    EXPECT_EQ(get_command_code('z', 'z', 'z'), OP_INVALID);
}

TEST(ParserTest, ParseLogic) {
    // 1. Пустая строка
    EXPECT_TRUE(parse("").empty());

    // 2. Игнорирование комментариев и пробелов
    std::vector<int> res = parse("MoO  Ignore This  MOo");
    // Должно распарсить: MoO (INC) и MOo (DEC)
    ASSERT_EQ(res.size(), 2);
    EXPECT_EQ(res[0], OP_INC);
    EXPECT_EQ(res[1], OP_DEC);

    // 3. Обрыв команды (2 символа в конце)
    res = parse("MoO Mo");
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0], OP_INC);

    // 4. Невалидная команда внутри COW-символов (mmm)
    // 'mmm' -> get_command_code вернет INVALID, парсер должен пропустить
    res = parse("mmm MoO");
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0], OP_INC);
}

class VMSingleOpTest : public ::testing::Test {
protected:
    VMState state;
};

TEST_F(VMSingleOpTest, OpMoveRightAndResize) {
    state.mem_ptr = state.memory.size() - 1; // Указатель на последний элемент
    exec_single_op(OP_MOVE_RIGHT, state);

    EXPECT_EQ(state.mem_ptr, 10000); // Старый размер
    EXPECT_GT(state.memory.size(), 10000); // Память должна увеличиться
}

TEST_F(VMSingleOpTest, OpMoveLeftBoundary) {
    state.mem_ptr = 0;
    exec_single_op(OP_MOVE_LEFT, state);
    EXPECT_EQ(state.mem_ptr, 0); // Не должен уйти в минус

    state.mem_ptr = 5;
    exec_single_op(OP_MOVE_LEFT, state);
    EXPECT_EQ(state.mem_ptr, 4);
}

TEST_F(VMSingleOpTest, OpIncDecZero) {
    exec_single_op(OP_INC, state);
    EXPECT_EQ(state.memory[0], 1);

    exec_single_op(OP_DEC, state);
    EXPECT_EQ(state.memory[0], 0);

    state.memory[0] = 123;
    exec_single_op(OP_ZERO, state);
    EXPECT_EQ(state.memory[0], 0);
}

TEST_F(VMSingleOpTest, OpRegister) {
    state.memory[0] = 42;
    // 1. Copy mem -> reg
    exec_single_op(OP_REGISTER, state);
    EXPECT_TRUE(state.reg_val.has_value());
    EXPECT_EQ(state.reg_val.value(), 42);

    // 2. Изменим память
    state.memory[0] = 0;

    // 3. Paste reg -> mem
    exec_single_op(OP_REGISTER, state);
    EXPECT_EQ(state.memory[0], 42);
    EXPECT_FALSE(state.reg_val.has_value());
}

TEST_F(VMSingleOpTest, OpIOChar) {
    // Вывод (память != 0)
    {
        IORedirect io("");
        state.memory[0] = 65; // 'A'
        exec_single_op(OP_IO_CHAR, state);
        EXPECT_EQ(io.getOutput(), "A");
    }

    // Ввод (память == 0) - Успех
    {
        IORedirect io("B");
        state.memory[0] = 0;
        exec_single_op(OP_IO_CHAR, state);
        EXPECT_EQ(state.memory[0], 66); // 'B'
    }

    // Ввод (память == 0) - EOF
    {
        IORedirect io(""); // Пустой ввод
        state.memory[0] = 0;
        exec_single_op(OP_IO_CHAR, state);
        EXPECT_EQ(state.memory[0], 0); // EOF -> 0
    }
}

TEST_F(VMSingleOpTest, OpPrintInt) {
    IORedirect io("");
    state.memory[0] = 12345;
    exec_single_op(OP_PRINT_INT, state);
    EXPECT_EQ(io.getOutput(), "12345");
}

TEST_F(VMSingleOpTest, OpReadInt) {
    // Успешное чтение
    {
        IORedirect io("987\n");
        exec_single_op(OP_READ_INT, state);
        EXPECT_EQ(state.memory[0], 987);
    }

    // Неуспешное чтение (буквы вместо цифр)
    {
        IORedirect io("abc");
        state.memory[0] = 55;
        exec_single_op(OP_READ_INT, state);
        EXPECT_EQ(state.memory[0], 0); // Должен сбросить в 0 при ошибке
    }
}

TEST_F(VMSingleOpTest, OpExecCellNormal) {
    // Запишем код OP_INC в ячейку 0
    state.memory[0] = OP_INC;
    state.mem_ptr = 1; // Указатель на ячейку 1
    state.memory[1] = 10;

    state.mem_ptr = 0;
    state.memory[0] = OP_INC;

    exec_single_op(OP_EXEC_CELL, state);
    EXPECT_EQ(state.memory[0], OP_INC + 1);
}

TEST_F(VMSingleOpTest, OpExecCellRecursionLimit) {
    IORedirect io("");
    // Создаем бесконечную рекурсию: ячейка содержит код "mOO" (OP_EXEC_CELL = 3)
    state.memory[0] = OP_EXEC_CELL;

    exec_single_op(OP_EXEC_CELL, state);

    EXPECT_NE(io.getError().find("Maximum recursion depth exceeded"), std::string::npos);
}

TEST_F(VMSingleOpTest, OpExecCellForbiddenLoops) {
    state.memory[0] = OP_LOOP_START;
    int original = state.memory[0];

    // Должен просто выйти (return), ничего не делая
    exec_single_op(OP_EXEC_CELL, state);
    EXPECT_EQ(state.memory[0], original);
}

TEST(ExecuteTest, BasicLoop) {
    // Программа: +++ [ - ] (Установить 3, цикл пока не 0 вычитать)
    // MOo (INC) * 3, MOO (LOOP START), MOo (DEC), moo (LOOP END)
    std::vector<int> prog = {
        OP_INC, OP_INC, OP_INC,
        OP_LOOP_START,
        OP_DEC,
        OP_LOOP_END,
        OP_PRINT_INT
    };

    IORedirect io("");
    execute(prog);
    EXPECT_EQ(io.getOutput(), "0");
}

TEST(ExecuteTest, LoopSkip) {
    // Программа: [ +++ ] (Если 0, пропустить цикл)
    std::vector<int> prog = {
        OP_LOOP_START,
        OP_INC, OP_INC, OP_INC,
        OP_LOOP_END,
        OP_PRINT_INT
    };

    IORedirect io("");
    execute(prog);
    EXPECT_EQ(io.getOutput(), "0"); // Цикл не выполнился
}

TEST(ExecuteTest, UnmatchedLoops) {
    // 1. Только начало цикла MOO (без moo) - должно игнорироваться как обычная команда (или прыгать в никуда, по коду jump_table = MAX)
    // В коде: если jump_table == NO_JUMP, просто идем дальше.
    std::vector<int> prog1 = { OP_INC, OP_LOOP_START, OP_INC };
    // Память: 1 -> Start(1!=0) -> но пары нет -> продолжаем -> Inc -> Итог 2
    {
        VMState state_check;
        IORedirect io("");
        execute(prog1);
        // Краша нет - тест пройден
    }

    // 2. Только конец цикла moo (без MOO)
    std::vector<int> prog2 = { OP_INC, OP_LOOP_END, OP_INC };
    // Память: 1 -> End(1!=0) -> но пары нет -> continue (пропуск инкремента указателя instr_ptr?? Нет, в коде continue внутри if(jump). else -> break switch -> instr_ptr++.)
    // Если jump_table == NO_JUMP, условие if (jump != NO_JUMP) false.
    // Выходит из if. Идет instr_ptr++.
    // Итог: выполнится INC, LOOP_END(nop), INC -> значение 2.
    {
         execute(prog2);
    }
}

TEST(ExecuteTest, NestedLoops) {
    // Тест на парсинг вложенности:
    std::vector<int> prog = {
        OP_LOOP_START,
            OP_LOOP_START,
            OP_LOOP_END,
        OP_LOOP_END
    };
    execute(prog);
}

TEST(MainTest, InvalidArgs) {
    IORedirect io("");
    char* argv[] = { (char*)"./cow" }; // Нет аргументов
    int res = cow_main(1, argv); // argc=1
    EXPECT_EQ(res, 1);
    EXPECT_THAT(io.getError(), ::testing::HasSubstr("Usage:"));
}

TEST(MainTest, FileNotFound) {
    IORedirect io("");
    char* argv[] = { (char*)"./cow", (char*)"non_existent_file.cow" };
    int res = cow_main(2, argv);
    EXPECT_EQ(res, 1);
    EXPECT_THAT(io.getError(), ::testing::HasSubstr("Could not open file"));
}

TEST(MainTest, EmptyFile) {
    // Создаем пустой файл
    std::ofstream f("empty.cow");
    f.close();

    IORedirect io("");
    char* argv[] = { (char*)"./cow", (char*)"empty.cow" };
    int res = cow_main(2, argv);
    EXPECT_EQ(res, 0); // Успешный выход, ничего не сделано

    remove("empty.cow");
}

TEST(MainTest, RunFile) {
    // Создаем файл с программой: выводит 'A'
    std::ofstream f("test.cow");
    // Напишем просто OOM (print int) на нуле (0)
    f << "OOM";
    f.close();

    IORedirect io("");
    char* argv[] = { (char*)"./cow", (char*)"test.cow" };
    int res = cow_main(2, argv);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(io.getOutput(), "0");

    remove("test.cow");
}


TEST(CoverageBooster, ReadIntNoNewline) {
    // В коде: if (std::cin.peek() == '\n') -> false
    IORedirect io("123 "); // Пробел вместо энтера
    VMState state;
    state.mem_ptr = 0;

    exec_single_op(OP_READ_INT, state);

    EXPECT_EQ(state.memory[0], 123);
}

TEST(CoverageBooster, MoveRightAggressiveResize) {
    // Пытаемся заставить сработать std::max(..., state.mem_ptr + 1)
    // Для этого нужно, чтобы memory.size() * 2 было МЕНЬШЕ чем mem_ptr + 1.
    // Это возможно только если вектор очень маленький.
    VMState state;
    state.memory.resize(1); // Размер 1
    state.mem_ptr = 0;      // Указывает на 0 элемент

    exec_single_op(OP_MOVE_RIGHT, state);

    // mem_ptr станет 1.
    // Логика: max(1*2, 1+1) = max(2, 2). Ветки равны.
    // Попробуем size = 0 (хотя конструктор создает 10000, но мы хакаем для теста)
    state.memory.clear();
    state.mem_ptr = 0;

    exec_single_op(OP_MOVE_RIGHT, state);

    // Проверяем, что не упало и расширилось
    EXPECT_GE(state.memory.size(), 1);
    EXPECT_EQ(state.mem_ptr, 1);
}

TEST(CoverageBooster, UnmatchedLoopStartSkip) {
    // MOO (Loop Start), память = 0. Должен прыгнуть к концу.
    // Но конца (moo) нет -> jump_table == NO_JUMP.
    // Код должен просто пойти дальше (instr_ptr++ в цикле while).

    // Программа: MOO (start) -> MOo (dec)
    // Память: 0.
    // Ожидание: MOO пропускается (как будто не сработал), выполняется MOo -> память станет -1

    std::vector<int> prog = { OP_LOOP_START, OP_DEC };

    VMState state;

    std::vector<int> instructions = {
        OP_LOOP_START,
        OP_DEC,
        OP_PRINT_INT
    };

    IORedirect io("");

    // Если jump_table[i] == NO_JUMP, код делает:
    // if (jump != NO_JUMP) instr_ptr = jump;
    // else { /* ничего, просто выход из if */ }
    // Далее instr_ptr++.

    execute(instructions);
    EXPECT_EQ(io.getOutput(), "-1");
}

TEST(CoverageBooster, UnmatchedLoopEndRepeat) {
    // moo (Loop End), память != 0. Должен прыгнуть в начало.
    // Но начала (MOO) нет -> jump_table == NO_JUMP.

    // Программа: MOo (inc 0->1), moo (end), OOM (print)
    // Если бы прыжок сработал, был бы бесконечный цикл.
    // Но пары нет -> прыжка нет -> идем дальше -> print.

    std::vector<int> instructions = {
        OP_INC,
        OP_LOOP_END,
        OP_PRINT_INT
    };

    IORedirect io("");
    execute(instructions);
    EXPECT_EQ(io.getOutput(), "1");
}

TEST(CoverageBooster, ParseEdgeCase) {
    // Парсинг строки, где символы есть, но их меньше 3 в конце
    // "M" -> игнор
    std::vector<int> res = parse("M");
    EXPECT_TRUE(res.empty());

    // "MO" -> игнор
    res = parse("MO");
    EXPECT_TRUE(res.empty());
}

TEST_F(VMSingleOpTest, SwitchDefaultCase) {
    // Устанавливаем начальное состояние
    state.memory[0] = 55;
    state.mem_ptr = 0;

    // Вызываем функцию с несуществующим кодом команды (например, 999)
    // Логика: цикл обработки mOO пропускается, switch попадает в default
    exec_single_op(999, state);

    // Проверяем, что ничего не сломалось и состояние VM не изменилось
    EXPECT_EQ(state.mem_ptr, 0);  // Указатель не сдвинулся
    EXPECT_EQ(state.memory[0], 55); // Память не изменилась
    EXPECT_FALSE(state.reg_val.has_value()); // Регистр не затронут
}

TEST_F(VMSingleOpTest, IndirectExecutionOfGarbage) {
    // 1. Записываем в память любое число
    state.memory[0] = 12345;
    state.mem_ptr = 0;

    // 2. Пытаемся выполнить это число через команду mOO (OP_EXEC_CELL)
    // (В тестах мы моделируем это, вызывая exec_single_op с кодом mOO,
    //  находясь на ячейке с мусором).

    // Важный момент: функция exec_single_op берет команду из аргумента,
    // а не из текущей ячейки. Но если аргумент = OP_EXEC_CELL (3),
    // она лезет в state.memory[state.mem_ptr] за "настоящей" командой.

    // Но тут есть нюанс реализации exec_single_op:
    // Она читает stored_val = state.memory[state.mem_ptr].
    // Если это 12345, цикл завершается (так как 12345 != OP_EXEC_CELL).
    // Дальше идет switch(12345).

    // Чтобы все сработало, мы должны передать OP_EXEC_CELL как аргумент.

    exec_single_op(OP_EXEC_CELL, state);

    // Ожидаем, что ничего не произошло (default: break)
    EXPECT_EQ(state.memory[0], 12345);
    EXPECT_EQ(state.mem_ptr, 0);
}

TEST_F(VMSingleOpTest, LoopOpsAreNoOpsInSingleExecution) {
    // Настраиваем состояние, чтобы убедиться, что оно НЕ изменится
    state.memory[0] = 777;
    state.mem_ptr = 0;

    // 1. Тестируем OP_LOOP_START (MOO)
    // При прямом вызове exec_single_op он должен просто сделать break
    exec_single_op(OP_LOOP_START, state);

    EXPECT_EQ(state.mem_ptr, 0);      // Указатель на месте
    EXPECT_EQ(state.memory[0], 777);  // Память не изменилась

    // 2. Тестируем OP_LOOP_END (moo)
    // То же самое - break
    exec_single_op(OP_LOOP_END, state);

    EXPECT_EQ(state.mem_ptr, 0);
    EXPECT_EQ(state.memory[0], 777);
}