#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
  INTEGER,
  PLUS,
  MINUS,
  MUL,
  DIV,
  LPAREN,
  RPAREN,
  BEGIN,
  END,
  DOT,
  SEMI,
  ASSIGN,
  ID,
  EOF_TOKEN,
  VAR,
  INTEGER_TYPE,
  REAL_TYPE,
  COMMA,
  COLON,
  PROGRAM,
  STRING_LITERAL,
  BOOLEAN_CONST,
  STRING_TYPE,
  BOOLEAN_TYPE
};

struct Token {
  TokenType type;
  std::string value;
  int line;
  int column;
};

#endif // TOKEN_H
