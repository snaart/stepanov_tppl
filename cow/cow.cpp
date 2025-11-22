#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stack>
#include <cstdint>
#include <optional>
#include <limits>

constexpr int MAX_RECURSION_DEPTH = 100;

enum OpCode {
    OP_LOOP_END   = 0,  // moo
    OP_MOVE_LEFT  = 1,  // mOo
    OP_MOVE_RIGHT = 2,  // moO
    OP_EXEC_CELL  = 3,  // mOO
    OP_IO_CHAR    = 4,  // Moo
    OP_DEC        = 5,  // MOo
    OP_INC        = 6,  // MoO
    OP_LOOP_START = 7,  // MOO
    OP_ZERO       = 8,  // OOO
    OP_REGISTER   = 9,  // MMM
    OP_PRINT_INT  = 10, // OOM
    OP_READ_INT   = 11, // oom
    OP_INVALID    = -1
};

struct VMState {
    std::vector<int> memory;
    std::size_t mem_ptr;
    std::optional<int> reg_val;

    VMState() : memory(10000, 0), mem_ptr(0), reg_val(std::nullopt) {}
};

inline bool is_cow_char(char c) {
    return c == 'm' || c == 'M' || c == 'o' || c == 'O';
}

int get_command_code(char c1, char c2, char c3) {
    uint32_t packed =
        (static_cast<uint32_t>(static_cast<uint8_t>(c1)) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(c2)) << 8) |
        static_cast<uint32_t>(static_cast<uint8_t>(c3));

    switch (packed) {
        case 0x6D6F6F: return OP_LOOP_END;
        case 0x6D4F6F: return OP_MOVE_LEFT;
        case 0x6D6F4F: return OP_MOVE_RIGHT;
        case 0x6D4F4F: return OP_EXEC_CELL;
        case 0x4D6F6F: return OP_IO_CHAR;
        case 0x4D4F6F: return OP_DEC;
        case 0x4D6F4F: return OP_INC;
        case 0x4D4F4F: return OP_LOOP_START;
        case 0x4F4F4F: return OP_ZERO;
        case 0x4D4D4D: return OP_REGISTER;
        case 0x4F4F4D: return OP_PRINT_INT;
        case 0x6F6F6D: return OP_READ_INT;
        default: return OP_INVALID;
    }
}

std::vector<int> parse(const std::string& source) {
    std::vector<int> instructions;
    instructions.reserve(source.length() / 3);

    for (size_t i = 0; i < source.length(); ++i) {
        if (!is_cow_char(source[i])) {
            continue;
        }
        if (i + 2 < source.length()) {
            int cmd = get_command_code(source[i], source[i+1], source[i+2]);
            if (cmd != OP_INVALID) {
                instructions.push_back(cmd);
                i += 2;
            }
        }
    }
    return instructions;
}

void exec_single_op(int command, VMState& state) {
    int effective_cmd = command;

    for (int depth = 0; effective_cmd == OP_EXEC_CELL; ++depth) {

        if (depth > MAX_RECURSION_DEPTH) {
            std::cerr << "Error: Maximum recursion depth exceeded via mOO." << std::endl;
            return;
        }

        int stored_val = state.memory[state.mem_ptr];

        if (stored_val == OP_LOOP_START || stored_val == OP_LOOP_END) {
             return;
        }

        // Подменяем команду и идем на следующую итерацию цикла
        effective_cmd = stored_val;
    }

    switch (effective_cmd) {
        case OP_LOOP_END:
        case OP_LOOP_START:
            break;
        case OP_MOVE_LEFT:
            if (state.mem_ptr > 0) state.mem_ptr--;
            break;
        case OP_MOVE_RIGHT:
            state.mem_ptr++;
            if (state.mem_ptr >= state.memory.size()) {
                // Увеличиваем память, если вышли за границы
                state.memory.resize(std::max(state.memory.size() * 2, state.mem_ptr + 1), 0);
            }
            break;
        case OP_IO_CHAR:
            if (state.memory[state.mem_ptr] == 0) {
                // Чтение символа
                char input_char;
                if (std::cin.get(input_char)) {
                    state.memory[state.mem_ptr] = static_cast<int>(static_cast<unsigned char>(input_char));
                } else {
                    state.memory[state.mem_ptr] = 0;
                }
            } else {
                // Вывод символа
                std::cout << static_cast<char>(state.memory[state.mem_ptr]);
            }
            break;

        case OP_DEC:
            state.memory[state.mem_ptr]--;
            break;

        case OP_INC:
            state.memory[state.mem_ptr]++;
            break;

        case OP_ZERO:
            state.memory[state.mem_ptr] = 0;
            break;

        case OP_REGISTER:
            if (!state.reg_val.has_value()) {
                state.reg_val = state.memory[state.mem_ptr];
            } else {
                state.memory[state.mem_ptr] = state.reg_val.value();
                state.reg_val = std::nullopt;
            }
            break;

        case OP_PRINT_INT:
            std::cout << state.memory[state.mem_ptr];
            break;

        case OP_READ_INT:
            {
                int val;
                if (std::cin >> val) {
                    state.memory[state.mem_ptr] = val;
                    // Очистка буфера от перевода строки, чтобы он не попал в следующий char
                    if (std::cin.peek() == '\n') std::cin.ignore();
                } else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    state.memory[state.mem_ptr] = 0;
                }
            }
            break;

        default:
            break;
    }
}

void execute(const std::vector<int>& instructions) {
    VMState state;
    std::size_t instr_ptr = 0;
    std::size_t n_instr = instructions.size();

    // ИСПОЛЬЗУЕМ MAX() КАК МАРКЕР "НЕТ ПЕРЕХОДА"
    constexpr std::size_t NO_JUMP = std::numeric_limits<std::size_t>::max();

    // Инициализируем таблицу значением NO_JUMP вместо 0
    std::vector<std::size_t> jump_table(n_instr, NO_JUMP);
    std::stack<std::size_t> loop_stack;

    // Предварительный проход
    for (std::size_t i = 0; i < n_instr; ++i) {
        if (instructions[i] == OP_LOOP_START) {
            loop_stack.push(i);
        } else if (instructions[i] == OP_LOOP_END) {
            if (!loop_stack.empty()) {
                std::size_t start = loop_stack.top();
                loop_stack.pop();
                jump_table[start] = i;
                jump_table[i] = start;
            }
        }
    }

    while (instr_ptr < n_instr) {
        int command = instructions[instr_ptr];

        if (command == OP_LOOP_START) { // MOO
            if (state.memory[state.mem_ptr] == 0) {
                // Проверяем на NO_JUMP
                if (jump_table[instr_ptr] != NO_JUMP) {
                     instr_ptr = jump_table[instr_ptr];
                } else {
                    // Если парной скобки нет - просто идем дальше
                }
            }
        } else if (command == OP_LOOP_END) { // moo
            if (state.memory[state.mem_ptr] != 0) {
                // Проверяем на NO_JUMP
                if (jump_table[instr_ptr] != NO_JUMP) {
                    instr_ptr = jump_table[instr_ptr];
                    continue; // Прыжок назад, пропускаем инкремент в конце цикла
                }
            }
        } else {
            exec_single_op(command, state);
        }
        instr_ptr++;
    }
}
int cow_main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (source.empty()) return 0;

    execute(parse(source));
    return 0;
}

#ifndef UNIT_TEST
int main(int argc, char* argv[]) {
    return cow_main(argc, argv);
}
#endif