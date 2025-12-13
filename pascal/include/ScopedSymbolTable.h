#ifndef SCOPED_SYMBOL_TABLE_H
#define SCOPED_SYMBOL_TABLE_H

#include "Types.h"
#include <map>
#include <memory>
#include <optional>
#include <string>

class ScopedSymbolTable {
public:
  ScopedSymbolTable(
      std::string scope_name, int scope_level,
      std::shared_ptr<ScopedSymbolTable> enclosing_scope = nullptr)
      : scope_name_(std::move(scope_name)), scope_level_(scope_level),
        enclosing_scope_(std::move(enclosing_scope)) {}

  void define(const std::string &name, Value value) { symbols_[name] = value; }

  std::optional<Value> lookup(const std::string &name,
                              bool current_scope_only = false) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
      return it->second;
    }
    if (current_scope_only) {
      return std::nullopt;
    }
    if (enclosing_scope_) {
      return enclosing_scope_->lookup(name);
    }
    return std::nullopt;
  }

  void assign(const std::string &name, Value value) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
      symbols_[name] = value;
      return;
    }
    if (enclosing_scope_) {
      enclosing_scope_->assign(name, value);
      return;
    }
    throw std::runtime_error("Undefined variable: " + name);
  }

  const std::map<std::string, Value> &get_symbols() const { return symbols_; }

private:
  std::string scope_name_;
  int scope_level_;
  std::shared_ptr<ScopedSymbolTable> enclosing_scope_;
  std::map<std::string, Value> symbols_;
};

#endif // SCOPED_SYMBOL_TABLE_H
