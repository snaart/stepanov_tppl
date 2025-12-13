#ifndef AST_H
#define AST_H

#include "Token.h"
#include <memory>
#include <string>
#include <vector>

struct NodeVisitor;

struct AST {
  virtual ~AST() = default;
  virtual void accept(NodeVisitor &visitor) = 0;
};

struct BinOp : AST {
  std::unique_ptr<AST> left;
  Token op;
  std::unique_ptr<AST> right;
  BinOp(std::unique_ptr<AST> l, Token o, std::unique_ptr<AST> r)
      : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
  void accept(NodeVisitor &visitor) override;
};

struct UnaryOp : AST {
  Token op;
  std::unique_ptr<AST> expr;
  UnaryOp(Token o, std::unique_ptr<AST> e)
      : op(std::move(o)), expr(std::move(e)) {}
  void accept(NodeVisitor &visitor) override;
};

struct Num : AST {
  Token token;
  double value;
  explicit Num(Token t) : token(std::move(t)) {
    value = std::stod(token.value);
  }
  void accept(NodeVisitor &visitor) override;
};

struct Var : AST {
  Token token;
  std::string name;
  explicit Var(Token t) : token(std::move(t)), name(token.value) {}
  void accept(NodeVisitor &visitor) override;
};

struct Assign : AST {
  std::unique_ptr<Var> left;
  Token op;
  std::unique_ptr<AST> right;
  Assign(std::unique_ptr<Var> l, Token o, std::unique_ptr<AST> r)
      : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
  void accept(NodeVisitor &visitor) override;
};

struct Compound : AST {
  std::vector<std::unique_ptr<AST>> children;
  void accept(NodeVisitor &visitor) override;
};

struct NoOp : AST {
  void accept(NodeVisitor &visitor) override;
};

struct Type : AST {
  Token token;
  explicit Type(Token t) : token(std::move(t)) {}
  void accept(NodeVisitor &visitor) override;
};

struct VarDecl : AST {
  std::unique_ptr<Var> var_node;
  std::unique_ptr<AST> type_node;
  VarDecl(std::unique_ptr<Var> var, std::unique_ptr<AST> type)
      : var_node(std::move(var)), type_node(std::move(type)) {}
  void accept(NodeVisitor &visitor) override;
};

struct Block : AST {
  std::vector<std::unique_ptr<AST>> declarations;
  std::unique_ptr<AST> compound_statement;
  Block(std::vector<std::unique_ptr<AST>> decls, std::unique_ptr<AST> compound)
      : declarations(std::move(decls)),
        compound_statement(std::move(compound)) {}
  void accept(NodeVisitor &visitor) override;
};

struct Program : AST {
  std::string name;
  std::unique_ptr<AST> block;
  Program(std::string n, std::unique_ptr<AST> b)
      : name(std::move(n)), block(std::move(b)) {}
  void accept(NodeVisitor &visitor) override;
};

struct StringLiteral : AST {
  Token token;
  std::string value;
  explicit StringLiteral(Token t) : token(std::move(t)), value(token.value) {}
  void accept(NodeVisitor &visitor) override;
};

struct BooleanLiteral : AST {
  Token token;
  bool value;
  explicit BooleanLiteral(Token t) : token(std::move(t)) {
    value = (token.value == "TRUE");
  }
  void accept(NodeVisitor &visitor) override;
};

struct NodeVisitor {
  virtual void visit(BinOp &node) = 0;
  virtual void visit(UnaryOp &node) = 0;
  virtual void visit(Num &node) = 0;
  virtual void visit(Var &node) = 0;
  virtual void visit(Assign &node) = 0;
  virtual void visit(Compound &node) = 0;
  virtual void visit(NoOp &node) = 0;
  virtual void visit(Program &node) = 0;
  virtual void visit(Block &node) = 0;
  virtual void visit(VarDecl &node) = 0;
  virtual void visit(Type &node) = 0;
  virtual void visit(StringLiteral &node) = 0;
  virtual void visit(BooleanLiteral &node) = 0;
  virtual ~NodeVisitor() = default;
};

inline void BinOp::accept(NodeVisitor &v) { v.visit(*this); }
inline void UnaryOp::accept(NodeVisitor &v) { v.visit(*this); }
inline void Num::accept(NodeVisitor &v) { v.visit(*this); }
inline void Var::accept(NodeVisitor &v) { v.visit(*this); }
inline void Assign::accept(NodeVisitor &v) { v.visit(*this); }
inline void Compound::accept(NodeVisitor &v) { v.visit(*this); }
inline void NoOp::accept(NodeVisitor &v) { v.visit(*this); }
inline void Program::accept(NodeVisitor &v) { v.visit(*this); }
inline void Block::accept(NodeVisitor &v) { v.visit(*this); }
inline void VarDecl::accept(NodeVisitor &v) { v.visit(*this); }
inline void Type::accept(NodeVisitor &v) { v.visit(*this); }
inline void StringLiteral::accept(NodeVisitor &v) { v.visit(*this); }
inline void BooleanLiteral::accept(NodeVisitor &v) { v.visit(*this); }

#endif // AST_H
