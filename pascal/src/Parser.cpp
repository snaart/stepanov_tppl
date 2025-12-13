#include "Parser.h"
#include <format>
#include <stdexcept>

Parser::Parser(Lexer &lexer) : lexer_(lexer) {
  current_token_ = lexer_.get_next_token();
}

std::unique_ptr<AST> Parser::parse() {
  auto node = program();
  if (current_token_.type != TokenType::EOF_TOKEN) {
    throw std::runtime_error("Unexpected token after end of program");
  }
  return node;
}

void Parser::eat(TokenType type) {
  if (current_token_.type == type) {
    current_token_ = lexer_.get_next_token();
  } else {
    throw std::runtime_error(std::format(
        "Syntax error: expected token type {}, got {} at line {}, column {}",
        (int)type, (int)current_token_.type, current_token_.line,
        current_token_.column));
  }
}

std::unique_ptr<AST> Parser::factor() {
  Token token = current_token_;
  if (token.type == TokenType::PLUS) {
    eat(TokenType::PLUS);
    return std::make_unique<UnaryOp>(token, factor());
  }
  if (token.type == TokenType::MINUS) {
    eat(TokenType::MINUS);
    return std::make_unique<UnaryOp>(token, factor());
  }
  if (token.type == TokenType::INTEGER) {
    eat(TokenType::INTEGER);
    return std::make_unique<Num>(token);
  }
  if (token.type == TokenType::STRING_LITERAL) {
    eat(TokenType::STRING_LITERAL);
    return std::make_unique<StringLiteral>(token);
  }
  if (token.type == TokenType::BOOLEAN_CONST) {
    eat(TokenType::BOOLEAN_CONST);
    return std::make_unique<BooleanLiteral>(token);
  }
  if (token.type == TokenType::LPAREN) {
    eat(TokenType::LPAREN);
    auto node = expr();
    eat(TokenType::RPAREN);
    return node;
  }
  return variable();
}

std::unique_ptr<AST> Parser::term() {
  auto node = factor();
  while (current_token_.type == TokenType::MUL ||
         current_token_.type == TokenType::DIV) {
    Token token = current_token_;
    if (token.type == TokenType::MUL)
      eat(TokenType::MUL);
    else
      eat(TokenType::DIV);
    node = std::make_unique<BinOp>(std::move(node), token, factor());
  }
  return node;
}

std::unique_ptr<AST> Parser::expr() {
  auto node = term();
  while (current_token_.type == TokenType::PLUS ||
         current_token_.type == TokenType::MINUS) {
    Token token = current_token_;
    if (token.type == TokenType::PLUS)
      eat(TokenType::PLUS);
    else
      eat(TokenType::MINUS);
    node = std::make_unique<BinOp>(std::move(node), token, term());
  }
  return node;
}

std::unique_ptr<Var> Parser::variable() {
  auto node = std::make_unique<Var>(current_token_);
  eat(TokenType::ID);
  return node;
}

std::unique_ptr<AST> Parser::assignment() {
  auto left = variable();
  Token token = current_token_;
  eat(TokenType::ASSIGN);
  auto right = expr();
  return std::make_unique<Assign>(std::move(left), token, std::move(right));
}

std::unique_ptr<AST> Parser::statement() {
  if (current_token_.type == TokenType::BEGIN) {
    return compound_statement();
  }
  if (current_token_.type == TokenType::ID) {
    return assignment();
  }
  return std::make_unique<NoOp>();
}

std::vector<std::unique_ptr<AST>> Parser::statement_list() {
  std::vector<std::unique_ptr<AST>> results;
  results.push_back(statement());

  while (current_token_.type == TokenType::SEMI) {
    eat(TokenType::SEMI);
    results.push_back(statement());
  }

  if (current_token_.type == TokenType::ID ||
      current_token_.type == TokenType::BEGIN) {
    throw std::runtime_error(
        "Error in statement list logic (missing semi-colon?)");
  }

  return results;
}

std::unique_ptr<AST> Parser::compound_statement() {
  eat(TokenType::BEGIN);
  auto nodes = statement_list();
  eat(TokenType::END);

  auto root = std::make_unique<Compound>();
  root->children = std::move(nodes);
  return root;
}

std::unique_ptr<AST> Parser::type_spec() {
  Token token = current_token_;
  if (current_token_.type == TokenType::INTEGER_TYPE) {
    eat(TokenType::INTEGER_TYPE);
  } else if (current_token_.type == TokenType::REAL_TYPE) {
    eat(TokenType::REAL_TYPE);
  } else if (current_token_.type == TokenType::STRING_TYPE) {
    eat(TokenType::STRING_TYPE);
  } else if (current_token_.type == TokenType::BOOLEAN_TYPE) {
    eat(TokenType::BOOLEAN_TYPE);
  } else {
    throw std::runtime_error("Unknown type specification");
  }
  return std::make_unique<Type>(token);
}

std::vector<std::unique_ptr<AST>> Parser::variable_declaration() {
  std::vector<std::unique_ptr<Var>> var_nodes;
  std::vector<std::unique_ptr<AST>> var_decls;

  var_nodes.push_back(std::make_unique<Var>(current_token_));
  eat(TokenType::ID);

  while (current_token_.type == TokenType::COMMA) {
    eat(TokenType::COMMA);
    var_nodes.push_back(std::make_unique<Var>(current_token_));
    eat(TokenType::ID);
  }

  eat(TokenType::COLON);

  auto type_node = type_spec();

  // Создаем VarDesl для каждой переменной с узлом общего типа
  Token type_token = static_cast<Type *>(type_node.get())->token;

  for (auto &var : var_nodes) {
    var_decls.push_back(std::make_unique<VarDecl>(
        std::move(var), std::make_unique<Type>(type_token)));
  }

  eat(TokenType::SEMI);
  return var_decls;
}

std::vector<std::unique_ptr<AST>> Parser::declarations() {
  std::vector<std::unique_ptr<AST>> decls;

  if (current_token_.type == TokenType::VAR) {
    eat(TokenType::VAR);
    while (current_token_.type == TokenType::ID) {
      auto var_decl_list = variable_declaration();
      for (auto &d : var_decl_list) {
        decls.push_back(std::move(d));
      }
    }
  }
  return decls;
}

std::unique_ptr<AST> Parser::block() {
  auto decls = declarations();
  auto compound_stmt = compound_statement();
  return std::make_unique<Block>(std::move(decls), std::move(compound_stmt));
}

std::unique_ptr<AST> Parser::program() {
  eat(TokenType::PROGRAM);         // PROGRAM keyword
  Token var_name = current_token_; // Program name
  eat(TokenType::ID);
  eat(TokenType::SEMI);

  auto block_node = block();

  eat(TokenType::DOT);
  return std::make_unique<Program>(var_name.value, std::move(block_node));
}
