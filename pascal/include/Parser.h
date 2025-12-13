#ifndef PARSER_H
#define PARSER_H

#include "AST.h"
#include "Lexer.h"
#include <memory>
#include <vector>

class Parser {
public:
  explicit Parser(Lexer &lexer);

  std::unique_ptr<AST> parse();

private:
  Lexer &lexer_;
  Token current_token_;

  void eat(TokenType type);
  std::unique_ptr<AST> program();
  std::unique_ptr<AST> block();
  std::vector<std::unique_ptr<AST>> declarations();
  std::vector<std::unique_ptr<AST>> variable_declaration();
  std::unique_ptr<AST> type_spec();
  std::unique_ptr<AST> compound_statement();
  std::vector<std::unique_ptr<AST>> statement_list();
  std::unique_ptr<AST> statement();
  std::unique_ptr<AST> assignment();
  std::unique_ptr<Var> variable();
  std::unique_ptr<AST> expr();
  std::unique_ptr<AST> term();
  std::unique_ptr<AST> factor();
};

#endif // PARSER_H
