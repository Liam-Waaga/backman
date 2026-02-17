#pragma once


#include "parser/parser.hpp"

#include <vector>
#include <filesystem>

struct Options {
  std::filesystem::path config_file = "";
  int                          jobs = 1;
  int                     verbosity = 0;
  std::filesystem::path     destdir = "";
  bool                   keep_going = false;
  std::vector<std::string>  targets;
};

extern Options options;

extern INI_Parser::INI_Data parsed_config;

std::filesystem::path resolve_path_with_environment(std::string path);
