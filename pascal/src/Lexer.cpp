#include "Lexer.h"
#include <algorithm>
#include <cctype>
#include <format>
#include <stdexcept>

Lexer::Lexer(std::string text)
    : text_(std::move(text)), pos_(0), line_(1), column_(1) {}

void Lexer::advance() {
  if (pos_ < text_.length()) {
    if (text_[pos_] == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
    pos_++;
  }
}

char Lexer::peek() const {
  if (pos_ + 1 >= text_.length())
    return '\0';
  return text_[pos_ + 1];
}

void Lexer::skip_whitespace() {
  while (pos_ < text_.length() && std::isspace(text_[pos_])) {
    advance();
  }
}

Token Lexer::number() {
  int start_col = column_;
  size_t start = pos_;
  while (pos_ < text_.length() && std::isdigit(text_[pos_])) {
    advance();
  }

  if (pos_ < text_.length() && text_[pos_] == '.') {
    // Смотрим вперед, чтобы увидеть, действительно ли это число (цифра следует за точкой)
    // Только проверяем, является ли следующий символ цифрой, чтобы разрешить «КОНЕЦ». или диапазоны «1..»
    if (pos_ + 1 < text_.length() && std::isdigit(text_[pos_ + 1])) {
      advance();
      while (pos_ < text_.length() && std::isdigit(text_[pos_])) {
        advance();
      }
      return {TokenType::INTEGER, text_.substr(start, pos_ - start), line_,
              start_col};
    }
  }

  return {TokenType::INTEGER, text_.substr(start, pos_ - start), line_,
          start_col};
}

Token Lexer::string_literal() {
  int start_col = column_;
  advance(); // Пропустить начальную кавычку
  size_t start = pos_;
  while (pos_ < text_.length() && text_[pos_] != '\'') {
    advance();
  }

  if (pos_ >= text_.length()) {
    throw std::runtime_error("Unterminated string literal");
  }

  std::string value = text_.substr(start, pos_ - start);
  advance(); // Пропускаем закрывающую кавычку
  return {TokenType::STRING_LITERAL, value, line_, start_col};
}

Token Lexer::id_or_keyword() {
  int start_col = column_;
  size_t start = pos_;
  while (pos_ < text_.length() &&
         (std::isalnum(text_[pos_]) || text_[pos_] == '_')) {
    advance();
  }
  std::string result = text_.substr(start, pos_ - start);

  // Нормализация к верхнему регистру для внутренней обработки
  // (Я загуглил, что это стандарт Pascal)
  // Это фактически делает его нечувствительным к регистру
  std::string upper_result = result;
  std::transform(upper_result.begin(), upper_result.end(), upper_result.begin(),
                 ::toupper);

  if (upper_result == "BEGIN")
    return {TokenType::BEGIN, upper_result, line_, start_col};
  if (upper_result == "END")
    return {TokenType::END, upper_result, line_, start_col};
  if (upper_result == "PROGRAM")
    return {TokenType::PROGRAM, upper_result, line_, start_col};
  if (upper_result == "VAR")
    return {TokenType::VAR, upper_result, line_, start_col};
  if (upper_result == "INTEGER")
    return {TokenType::INTEGER_TYPE, upper_result, line_, start_col};
  if (upper_result == "REAL")
    return {TokenType::REAL_TYPE, upper_result, line_, start_col};
  if (upper_result == "DIV")
    return {TokenType::DIV, upper_result, line_, start_col};
  if (upper_result == "STRING")
    return {TokenType::STRING_TYPE, upper_result, line_, start_col};
  if (upper_result == "BOOLEAN")
    return {TokenType::BOOLEAN_TYPE, upper_result, line_, start_col};
  if (upper_result == "TRUE" || upper_result == "FALSE")
    return {TokenType::BOOLEAN_CONST, upper_result, line_, start_col};

  return {TokenType::ID, upper_result, line_, start_col};
}

Token Lexer::get_next_token() {
  skip_whitespace();

  if (pos_ >= text_.length()) {
    return {TokenType::EOF_TOKEN, "", line_, column_};
  }

  char current = text_[pos_];
  int start_col = column_;

  if (std::isalpha(current)) {
    return id_or_keyword();
  }

  if (std::isdigit(current)) {
    return number();
  }

  if (current == '\'') {
    return string_literal();
  }

  if (current == ':' && peek() == '=') {
    advance();
    advance();
    return {TokenType::ASSIGN, ":=", line_, start_col};
  }

  if (current == ':') {
    advance();
    return {TokenType::COLON, ":", line_, start_col};
  }

  if (current == ',') {
    advance();
    return {TokenType::COMMA, ",", line_, start_col};
  }

  switch (current) {
  case '+':
    advance();
    return {TokenType::PLUS, "+", line_, start_col};
  case '-':
    advance();
    return {TokenType::MINUS, "-", line_, start_col};
  case '*':
    advance();
    return {TokenType::MUL, "*", line_, start_col};
  case '/':
    advance();
    return {TokenType::DIV, "/", line_, start_col};
  case '(':
    advance();
    return {TokenType::LPAREN, "(", line_, start_col};
  case ')':
    advance();
    return {TokenType::RPAREN, ")", line_, start_col};
  case '.':
    advance();
    return {TokenType::DOT, ".", line_, start_col};
  case ';':
    advance();
    return {TokenType::SEMI, ";", line_, start_col};
  default:
    throw std::runtime_error(
        std::format("Unknown character '{}' at line {}, column {}", current,
                    line_, column_));
  }
}
