#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "AST.h"
#include "ScopedSymbolTable.h"
#include "Types.h"
#include <map>
#include <memory>
#include <string>

class Interpreter : public NodeVisitor {
private:
  Value current_result;
  std::shared_ptr<ScopedSymbolTable> current_scope;

public:
  std::shared_ptr<ScopedSymbolTable> global_scope;

  std::map<std::string, Value> interpret(AST *tree);

  void visit(Program &node) override;
  void visit(Block &node) override;
  void visit(VarDecl &node) override;
  void visit(Type &node) override;
  void visit(StringLiteral &node) override;
  void visit(BooleanLiteral &node) override;
  void visit(Compound &node) override;
  void visit(NoOp &node) override;
  void visit(Assign &node) override;
  void visit(Var &node) override;
  void visit(Num &node) override;
  void visit(UnaryOp &node) override;
  void visit(BinOp &node) override;
};

#endif // INTERPRETER_H
