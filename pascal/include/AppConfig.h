#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "Types.h"
#include <map>
#include <string>
#include <vector>

struct Config {
  std::string input_file;
  bool variables_to_json = false;
  bool beauty_output = false;
  std::string json_output_file;
};

class AppUtils {
public:
  static Config parse_args(const std::vector<std::string> &args);
  static std::string memory_to_json(const std::map<std::string, Value> &memory);
  static void print_beauty_table(const std::map<std::string, Value> &memory);
};

#endif // APPCONFIG_H
