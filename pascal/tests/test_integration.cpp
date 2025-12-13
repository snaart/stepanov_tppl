#include "AppConfig.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include <cmath>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

std::string read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Could not open file: " + path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Помощник для запуска конвейера и возврата памяти JSON
std::string run_pipeline(const std::string &filename) {
  std::string paths[] = {"examples/" + filename, "../examples/" + filename,
                         "../../examples/" + filename};
  std::string text;
  bool found = false;
  for (const auto &p : paths) {
    std::ifstream f(p);
    if (f.good()) {
      text = read_file(p);
      found = true;
      break;
    }
  }
  if (!found)
    throw std::runtime_error("Could not find example file: " + filename);

  Lexer lexer(text);
  Parser parser(lexer);
  auto ast = parser.parse();

  SemanticAnalyzer analyzer;
  analyzer.analyze(ast.get());

  Interpreter interpreter;
  auto memory = interpreter.interpret(ast.get());
  return AppUtils::memory_to_json(memory);
}

// Функция для извлечения значения из строки JSON
double get_val(const std::string &json, const std::string &key) {
  std::string s = "\"" + key + "\": ";
  auto pos = json.find(s);
  if (pos == std::string::npos)
    return 0.0;
  auto start = pos + s.length();
  auto end = json.find_first_of(",}", start);
  return std::stod(json.substr(start, end - start));
}

class IntegrationTest : public ::testing::Test {};

TEST_F(IntegrationTest, Arithmetic) {
  auto memory_json = run_pipeline("arithmetic.pas");
  EXPECT_DOUBLE_EQ(get_val(memory_json, "X"), 17.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "Y"), 11.0);
}

TEST_F(IntegrationTest, Nested) {
  auto memory_json = run_pipeline("nested.pas");
  // y=2. a=3. b=10+3+10*2/4 = 13 + 20/4 = 18. c=3-18=-15. x=11.
  EXPECT_DOUBLE_EQ(get_val(memory_json, "Y"), 2.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "A"), 3.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "B"), 18.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "C"), -15.0);
}

TEST_F(IntegrationTest, ComplexMath) {
  auto memory_json = run_pipeline("complex_math.pas");
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LONG_EXPR"), 51.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "PARENTHESES"), 6.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "PRECEDENCE"), 30.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "NEGATION"), 10.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "COMPLEX"), 53.0);
}

TEST_F(IntegrationTest, DeepScope) {
  auto memory_json = run_pipeline("deep_scope.pas");
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LEVEL0"), 0.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LEVEL1"), 1.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LEVEL2"), 2.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LEVEL3"), 3.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "SUM_LEVELS"), 6.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "LEVEL1_MODIFIED"), 11.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "FINAL_VAL"), 100.0);
}

TEST_F(IntegrationTest, FeatureShowcase) {
  auto memory_json = run_pipeline("feature_showcase.pas");
  // Проверьте числовые значения
  EXPECT_DOUBLE_EQ(get_val(memory_json, "I"), 11.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "J"), 25.0);
  EXPECT_DOUBLE_EQ(get_val(memory_json, "X"), 3.14);
  // Y = 3.14 * 2 = 6.28
  EXPECT_DOUBLE_EQ(get_val(memory_json, "Y"), 6.28);
}

TEST_F(IntegrationTest, StringOps) {
  auto memory_json = run_pipeline("string_ops.pas");
  EXPECT_NE(memory_json.find("AlphaBetaGamma"), std::string::npos);
  EXPECT_NE(memory_json.find("One Two Three"), std::string::npos);
}

TEST_F(IntegrationTest, BooleanLogic) {
  auto memory_json = run_pipeline("boolean_logic.pas");
}
