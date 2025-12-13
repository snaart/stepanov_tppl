#include "SemanticAnalyzer.h"
#include <iostream>
#include <stdexcept>

SemanticAnalyzer::SemanticAnalyzer() {
  current_scope = std::make_shared<ScopedSymbolTable>("GLOBAL", 1);
}

void SemanticAnalyzer::analyze(AST *tree) { tree->accept(*this); }

void SemanticAnalyzer::visit(Program &node) { node.block->accept(*this); }

void SemanticAnalyzer::visit(Block &node) {
  for (const auto &decl : node.declarations) {
    decl->accept(*this);
  }
  node.compound_statement->accept(*this);
}

void SemanticAnalyzer::visit(VarDecl &node) {
  auto type_node = dynamic_cast<Type *>(node.type_node.get());
  std::string type_name = type_node ? type_node->token.value : "REAL";

  if (current_scope->lookup(node.var_node->name, true)) {
    throw std::runtime_error("Duplicate declaration of identifier: " +
                             node.var_node->name);
  }

  // Мы определяем значение по умолчанию правильного типа, чтобы отслеживать его существование
  if (type_name == "INTEGER")
    current_scope->define(node.var_node->name, 0);
  else if (type_name == "REAL")
    current_scope->define(node.var_node->name, 0.0);
  else if (type_name == "STRING")
    current_scope->define(node.var_node->name, std::string(""));
  else if (type_name == "BOOLEAN")
    current_scope->define(node.var_node->name, false);
  else
    current_scope->define(node.var_node->name, 0.0); // Default
}

void SemanticAnalyzer::visit(Type &node) {
  // No-op
}

void SemanticAnalyzer::visit(StringLiteral &node) {
  // No-op
}

void SemanticAnalyzer::visit(BooleanLiteral &node) {
  // No-op
}

void SemanticAnalyzer::visit(Compound &node) {
  for (const auto &child : node.children) {
    child->accept(*this);
  }
}

void SemanticAnalyzer::visit(NoOp &node) {
  // No-op
}

void SemanticAnalyzer::visit(Assign &node) {
  node.right->accept(*this);
  node.left->accept(*this); // Посетите Var, чтобы проверить определение
}

void SemanticAnalyzer::visit(Var &node) {
  auto val = current_scope->lookup(node.name);
  if (!val) {
    throw std::runtime_error("Semantic Error: Undefined variable '" +
                             node.name + "'");
  }
}

void SemanticAnalyzer::visit(BinOp &node) {
  node.left->accept(*this);
  node.right->accept(*this);
}

void SemanticAnalyzer::visit(UnaryOp &node) { node.expr->accept(*this); }

void SemanticAnalyzer::visit(Num &node) {
  // No-op
}
