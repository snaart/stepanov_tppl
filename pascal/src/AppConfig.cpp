#include "AppConfig.h"
#include <format>
#include <iostream>
#include <print>
#include <stdexcept>

Config AppUtils::parse_args(const std::vector<std::string> &args) {
  Config config;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--variables-to-json") {
      config.variables_to_json = true;
    } else if (args[i] == "--beauty-variables-output") {
      config.beauty_output = true;
    } else if (args[i] == "--json-output-file") {
      if (i + 1 < args.size()) {
        config.json_output_file = args[++i];
      } else {
        throw std::runtime_error(
            "Error: --json-output-file requires a filename argument");
      }
    } else {
      if (config.input_file.empty()) {
        config.input_file = args[i];
      } else {
        throw std::runtime_error(
            "Error: Multiple input files specified or unknown flag: " +
            args[i]);
      }
    }
  }
  return config;
}

std::string
AppUtils::memory_to_json(const std::map<std::string, Value> &memory) {
  std::string json = "{";
  bool first = true;
  for (const auto &[key, value] : memory) {
    if (!first)
      json += ", ";
    first = false;

    std::string val_str;
    if (std::holds_alternative<int>(value))
      val_str = std::to_string(std::get<int>(value));
    else if (std::holds_alternative<double>(value))
      val_str = std::to_string(std::get<double>(value));
    else if (std::holds_alternative<bool>(value))
      val_str = std::get<bool>(value) ? "true" : "false";
    else if (std::holds_alternative<std::string>(value))
      val_str = "\"" + std::get<std::string>(value) + "\"";
    else
      val_str = "null";

    json += std::format("\"{}\": {}", key, val_str);
  }
  json += "}";
  return json;
}

void AppUtils::print_beauty_table(const std::map<std::string, Value> &memory) {
  std::print("+{:-^20}+{:-^20}+\n", "", "");
  std::print("|{:^20}|{:^20}|\n", "Variable", "Value");
  std::print("+{:-^20}+{:-^20}+\n", "", "");
  for (const auto &[key, value] : memory) {
    std::string val_str;
    if (std::holds_alternative<int>(value))
      val_str = std::to_string(std::get<int>(value));
    else if (std::holds_alternative<double>(value))
      val_str = std::format("{:.4f}", std::get<double>(value));
    else if (std::holds_alternative<bool>(value))
      val_str = std::get<bool>(value) ? "TRUE" : "FALSE";
    else if (std::holds_alternative<std::string>(value))
      val_str = std::get<std::string>(value);
    else
      val_str = "None";

    std::print("| {:<19}| {:>19}|\n", key, val_str);
  }
  std::print("+{:-^20}+{:-^20}+\n", "", "");
}
