#include "../include/Interpreter.h"
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/SemanticAnalyzer.h"
#include <gtest/gtest.h>

// Функция для получения строки из значения
std::string get_string_val(const Value &v) {
  if (std::holds_alternative<std::string>(v))
    return std::get<std::string>(v);
  throw std::runtime_error("Expected string");
}

// Функция для получения bool из Value
bool get_bool_val(const Value &v) {
  if (std::holds_alternative<bool>(v))
    return std::get<bool>(v);
  throw std::runtime_error("Expected bool");
}

TEST(TypeTest, StringLexing) {
  Lexer lexer("'hello world'");
  Token t = lexer.get_next_token();
  EXPECT_EQ(t.type, TokenType::STRING_LITERAL);
  EXPECT_EQ(t.value, "hello world");
}

TEST(TypeTest, BooleanLexing) {
  Lexer lexer("TRUE FALSE BOOLEAN");
  EXPECT_EQ(lexer.get_next_token().type, TokenType::BOOLEAN_CONST);
  EXPECT_EQ(lexer.get_next_token().type, TokenType::BOOLEAN_CONST);
  EXPECT_EQ(lexer.get_next_token().type, TokenType::BOOLEAN_TYPE);
}

TEST(TypeTest, StringParsing) {
  Lexer lexer("PROGRAM Test; VAR s: STRING; BEGIN s := 'test'; END.");
  Parser parser(lexer);
  auto tree = parser.parse();
  SemanticAnalyzer analyzer;
  analyzer.analyze(tree.get());
  Interpreter interpreter;
  auto result = interpreter.interpret(tree.get());

  EXPECT_EQ(get_string_val(result["S"]), "test");
}

TEST(TypeTest, StringConcatenation) {
  Lexer lexer(
      "PROGRAM Test; VAR s: STRING; BEGIN s := 'hello' + ' ' + 'world'; END.");
  Parser parser(lexer);
  auto tree = parser.parse();
  SemanticAnalyzer analyzer;
  analyzer.analyze(tree.get());
  Interpreter interpreter;
  auto result = interpreter.interpret(tree.get());

  EXPECT_EQ(get_string_val(result["S"]), "hello world");
}

TEST(TypeTest, BooleanLogic) {
  Lexer lexer("PROGRAM Test; VAR b: BOOLEAN; BEGIN b := TRUE; END.");
  Parser parser(lexer);
  auto tree = parser.parse();
  SemanticAnalyzer analyzer;
  analyzer.analyze(tree.get());
  Interpreter interpreter;
  auto result = interpreter.interpret(tree.get());

  EXPECT_EQ(get_bool_val(result["B"]), true);
}

TEST(TypeTest, TypeMismatchString) {
  // Пробуем присвоить int строке
  Lexer lexer("PROGRAM Test; VAR s: STRING; BEGIN s := 123; END.");
  Parser parser(lexer);
  auto tree = parser.parse();
  SemanticAnalyzer analyzer;
  analyzer.analyze(tree.get());
  Interpreter interpreter;
  try {
    interpreter.interpret(tree.get());
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error &e) {
    EXPECT_EQ(std::string(e.what()), "Runtime error: Type mismatch in "
                                     "assignment. Expected String, got Real");
  }
}

TEST(TypeTest, BooleanAdditionError) {
  Lexer lexer("PROGRAM Test; VAR b: BOOLEAN; r: REAL; BEGIN b := TRUE; r := b "
              "+ 10; END.");
  Parser parser(lexer);
  auto tree = parser.parse();
  SemanticAnalyzer analyzer;
  analyzer.analyze(tree.get());
  Interpreter interpreter;

  try {
    interpreter.interpret(tree.get());
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error &e) {
    EXPECT_EQ(std::string(e.what()),
              "Runtime error: Expected number, got Boolean");
  }
}
