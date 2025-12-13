#include "Interpreter.h"
#include <cmath>
#include <stdexcept>
#include <variant>

// Helper to get type name
std::string get_type_name(size_t index) {
  switch (index) {
  case 0:
    return "None";
  case 1:
    return "Integer";
  case 2:
    return "Real";
  case 3:
    return "Boolean";
  case 4:
    return "String";
  default:
    return "Unknown";
  }
}

// Функция для получения двойного значения из значения
double get_double(const Value &v) {
  if (std::holds_alternative<double>(v))
    return std::get<double>(v);
  if (std::holds_alternative<int>(v))
    return static_cast<double>(std::get<int>(v));
  throw std::runtime_error("Runtime error: Expected number, got " +
                           get_type_name(v.index()));
}

std::map<std::string, Value> Interpreter::interpret(AST *tree) {
  global_scope = std::make_shared<ScopedSymbolTable>("GLOBAL", 1);
  current_scope = global_scope;

  if (tree) {
    tree->accept(*this);
  }
  return global_scope->get_symbols();
}

void Interpreter::visit(Program &node) { node.block->accept(*this); }

void Interpreter::visit(Block &node) {
  for (const auto &decl : node.declarations) {
    decl->accept(*this);
  }
  node.compound_statement->accept(*this);
}

void Interpreter::visit(VarDecl &node) {
  // Определить значение по умолчанию на основе типа, если это возможно, или просто 0,0
  // Нам нужно привести type_node к Type*, чтобы проверить токен
  auto type_ptr = dynamic_cast<Type *>(node.type_node.get());
  if (type_ptr) {
    if (type_ptr->token.type == TokenType::INTEGER_TYPE) {
      current_scope->define(node.var_node->name, 0); // int 0
    } else if (type_ptr->token.type == TokenType::REAL_TYPE) {
      current_scope->define(node.var_node->name, 0.0); // double 0.0
    } else if (type_ptr->token.type == TokenType::STRING_TYPE) {
      current_scope->define(node.var_node->name, std::string(""));
    } else if (type_ptr->token.type == TokenType::BOOLEAN_TYPE) {
      current_scope->define(node.var_node->name, false);
    } else {
      current_scope->define(node.var_node->name, 0.0);
    }
  } else {
    current_scope->define(node.var_node->name, 0.0);
  }
}

void Interpreter::visit(Type &node) {
  // No-op
}

void Interpreter::visit(StringLiteral &node) { current_result = node.value; }

void Interpreter::visit(BooleanLiteral &node) { current_result = node.value; }

void Interpreter::visit(Compound &node) {
  for (const auto &child : node.children) {
    child->accept(*this);
  }
}

void Interpreter::visit(NoOp &node) {
  // Do nothing
}

void Interpreter::visit(Assign &node) {
  node.right->accept(*this);
  auto var_ptr = current_scope->lookup(node.left->name, true);
  if (var_ptr) {
    // Разрешить Int -> Real приведение
    if (std::holds_alternative<double>(*var_ptr) &&
        std::holds_alternative<int>(current_result)) {
      current_result = static_cast<double>(std::get<int>(current_result));
    }
    // Разрешить преобразование Real -> Int
    // (поскольку AST хранит все числа как двойные значения)
    else if (std::holds_alternative<int>(*var_ptr) &&
             std::holds_alternative<double>(current_result)) {
      current_result = static_cast<int>(std::get<double>(current_result));
    } else if (var_ptr->index() != current_result.index()) {
      throw std::runtime_error(
          "Runtime error: Type mismatch in assignment. Expected " +
          get_type_name(var_ptr->index()) + ", got " +
          get_type_name(current_result.index()));
    }
    current_scope->assign(node.left->name, current_result);
  } else {
    // Строгий режим: переменная должна быть объявлена
    throw std::runtime_error("Runtime Error: Undefined variable '" +
                             node.left->name + "'");
  }
}

void Interpreter::visit(Var &node) {
  auto val = current_scope->lookup(node.name);
  if (!val) {
    throw std::runtime_error("Undefined variable: " + node.name);
  }
  current_result = *val;
}

void Interpreter::visit(Num &node) { current_result = node.value; }

void Interpreter::visit(UnaryOp &node) {
  node.expr->accept(*this);
  double val = get_double(current_result);
  if (node.op.type == TokenType::PLUS) {
    current_result = +val;
  } else if (node.op.type == TokenType::MINUS) {
    current_result = -val;
  }
}

void Interpreter::visit(BinOp &node) {
  node.left->accept(*this);
  Value left_val = current_result;

  node.right->accept(*this);
  Value right_val = current_result;

  // Конкатенация строк
  if (std::holds_alternative<std::string>(left_val) &&
      std::holds_alternative<std::string>(right_val) &&
      node.op.type == TokenType::PLUS) {
    current_result =
        std::get<std::string>(left_val) + std::get<std::string>(right_val);
    return;
  }

  // Арифметика
  double l_dbl = get_double(left_val);
  double r_dbl = get_double(right_val);

  switch (node.op.type) {
  case TokenType::PLUS:
    current_result = l_dbl + r_dbl;
    break;
  case TokenType::MINUS:
    current_result = l_dbl - r_dbl;
    break;
  case TokenType::MUL:
    current_result = l_dbl * r_dbl;
    break;
  case TokenType::DIV:
    if (r_dbl == 0)
      throw std::runtime_error("Division by zero");
    current_result = l_dbl / r_dbl;
    break;
  default:
    break;
  }
}
