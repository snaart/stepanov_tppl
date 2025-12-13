#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <string>

class Lexer {
public:
  explicit Lexer(std::string text);

  Token get_next_token();

private:
  std::string text_;
  size_t pos_;
  int line_;
  int column_;

  void advance();
  char peek() const;
  Token number();
  Token string_literal();
  Token id_or_keyword();
  void skip_whitespace();
};

#endif // LEXER_H
