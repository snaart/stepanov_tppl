#include "AppConfig.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <cmath>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <variant>
#include <vector>

double get_double_test(const Value &v) {
  if (std::holds_alternative<double>(v))
    return std::get<double>(v);
  if (std::holds_alternative<int>(v))
    return static_cast<double>(std::get<int>(v));
  throw std::runtime_error("Not a number");
}

// --- Lexer Tests ---
TEST(LexerTest, BasicTokens) {
  Lexer l1("123 + -");
  Token t1 = l1.get_next_token();
  EXPECT_EQ(t1.type, TokenType::INTEGER);
  EXPECT_EQ(t1.line, 1);
}

// --- Interpreter Tests ---
TEST(InterpreterTest, ComplexCalculation) {
  std::string code1 = "PROGRAM Test; VAR x, y, z : REAL; BEGIN x := 2 + 3 * 4; "
                      "y := (2 + 3) * 4; z := 10 / 2 - 1 END.";
  Lexer l1(code1);
  Parser p1(l1);
  auto ast1 = p1.parse();
  Interpreter i1;
  auto res1 = i1.interpret(ast1.get());

  EXPECT_DOUBLE_EQ(get_double_test(res1["X"]), 14.0);
  EXPECT_DOUBLE_EQ(get_double_test(res1["Y"]), 20.0);
  EXPECT_DOUBLE_EQ(get_double_test(res1["Z"]), 4.0);
}

TEST(InterpreterTest, VariableUsage) {
  std::string code2 =
      "PROGRAM Test; VAR a, b : REAL; BEGIN a := 5; b := a + 10; END.";
  Lexer l2(code2);
  Parser p2(l2);
  auto ast2 = p2.parse();
  Interpreter i2;
  auto res2 = i2.interpret(ast2.get());
  EXPECT_DOUBLE_EQ(get_double_test(res2["B"]), 15.0);
}

TEST(InterpreterTest, UnaryOperators) {
  std::string code3 =
      "PROGRAM Test; VAR x, y, z : REAL; BEGIN x := -5; y := +3; z := -x END.";
  Lexer l3(code3);
  Parser p3(l3);
  auto ast3 = p3.parse();
  Interpreter i3;
  auto res3 = i3.interpret(ast3.get());
  EXPECT_DOUBLE_EQ(get_double_test(res3["X"]), -5.0);
  EXPECT_DOUBLE_EQ(get_double_test(res3["Y"]), 3.0);
  EXPECT_DOUBLE_EQ(get_double_test(res3["Z"]), 5.0);
}

TEST(InterpreterTest, DivisionByZero) {
  std::string code4 = "PROGRAM Test; VAR x : REAL; BEGIN x := 10 / 0 END.";
  Lexer l4(code4);
  Parser p4(l4);
  auto ast4 = p4.parse();
  Interpreter i4;
  EXPECT_THROW(i4.interpret(ast4.get()), std::runtime_error);
}

TEST(InterpreterTest, UndefinedVariable) {
  // x is defined, y is NOT. Должно сломаться
  std::string code5 = "PROGRAM Test; VAR x : REAL; BEGIN x := y + 1 END.";
  Lexer l5(code5);
  Parser p5(l5);
  auto ast5 = p5.parse();
  Interpreter i5;
  EXPECT_THROW(i5.interpret(ast5.get()), std::runtime_error);
}

// --- AppUtils Tests ---
TEST(AppUtilsTest, JsonOutput) {
  std::map<std::string, Value> mem = {{"a", 1.0}, {"b", 2.2}};
  std::string json = AppUtils::memory_to_json(mem);
  EXPECT_NE(json.find("\"a\": 1.000000"), std::string::npos);
  EXPECT_NE(json.find("\"b\": 2.200000"), std::string::npos);
}
