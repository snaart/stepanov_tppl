#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "ScopedSymbolTable.h"
#include <memory>

class SemanticAnalyzer : public NodeVisitor {
public:
  SemanticAnalyzer();
  void visit(Block &node) override;
  void visit(Program &node) override;
  void visit(Compound &node) override;
  void visit(NoOp &node) override;
  void visit(VarDecl &node) override;
  void visit(Type &node) override;
  void visit(StringLiteral &node) override;
  void visit(BooleanLiteral &node) override;
  void visit(Assign &node) override;
  void visit(Var &node) override;
  void visit(BinOp &node) override;
  void visit(UnaryOp &node) override;
  void visit(Num &node) override;

  void analyze(AST *tree);

private:
  std::shared_ptr<ScopedSymbolTable> current_scope;
};

#endif // SEMANTIC_ANALYZER_H
