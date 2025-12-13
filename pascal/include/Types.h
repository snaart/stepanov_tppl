#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <string>
#include <variant>

using Value = std::variant<std::monostate, int, double, bool, std::string>;

// Helper для печати Value
struct ValuePrinter {
  void operator()(std::monostate) const { std::cout << "None"; }
  void operator()(int i) const { std::cout << i; }
  void operator()(double d) const { std::cout << d; }
  void operator()(bool b) const { std::cout << (b ? "TRUE" : "FALSE"); }
  void operator()(const std::string &s) const { std::cout << "'" << s << "'"; }
};

inline std::ostream &operator<<(std::ostream &os, const Value &val) {
  if (std::holds_alternative<int>(val))
    os << std::get<int>(val);
  else if (std::holds_alternative<double>(val))
    os << std::get<double>(val);
  else if (std::holds_alternative<bool>(val))
    os << (std::get<bool>(val) ? "TRUE" : "FALSE");
  else if (std::holds_alternative<std::string>(val))
    os << "'" << std::get<std::string>(val) << "'";
  else
    os << "None";
  return os;
}

#endif // TYPES_H
