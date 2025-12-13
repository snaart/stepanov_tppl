#include "AppConfig.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::string read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main(int argc, char *argv[]) {
  try {
    std::vector<std::string> args(argv + 1, argv + argc);
    Config config = AppUtils::parse_args(args);

    if (config.input_file.empty()) {
      std::cerr << "Usage: pascal [options] <input_file>" << std::endl;
      return 1;
    }

    std::string text = read_file(config.input_file);

    Lexer lexer(text);
    Parser parser(lexer);
    auto ast = parser.parse();

    // Семантический анализ
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast.get());

    // Интерпритация
    Interpreter interpreter;
    auto memory = interpreter.interpret(ast.get());

    if (config.variables_to_json) {
      std::cout << AppUtils::memory_to_json(memory) << std::endl;
    } else {
      AppUtils::print_beauty_table(memory);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
