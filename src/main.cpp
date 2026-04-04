/*
 * Backup manager to make backups using tar with gpg encryption and xz compression
 * Copyright (C) 2026 N Liam Waaga
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "parser/parser.hpp"

#include "target/target.hpp"
#include "log/log.h"
#include "utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stddef.h>
#include <format>
#include <string>
#include <vector>
#include <iostream>


namespace fs = std::filesystem;

Options options;
INI_Parser::INI_Data parsed_config;

static constexpr const char * const version_string = VERSION " built from " GIT_HASH;

static constexpr char const * const help_format =
"Backup manager: Copyright N Liam Waaga 2026\n"
"Version: %s\n"
"Usage: backman <options> <targets>\n"
"Options:\n"
"  -h,  --help            Display this help text\n"
"       --version         Display this help text (includes version)\n"
"  -v,  --verbose         Increase verbocity (unimplemented)\n"
"  -j,  --jobs    <jobs>  Number of jobs to use (for hooks)\n"
"       --destdir <dir>   Destination directory to put the archives (overrides dest option for targets)\n"
"  -c,  --config  <file>  Config file, default $XDG_CONFIG_HOME/backman/backman.ini\n"
"       --keep-going      Keep going after an errored target (unimplemented)\n"
"       --print-targets   Print all available targets\n"
"       --generate-config\n"
"                         Generate an example config (for reference)\n"
"                         If a config file path is specified, that path is used instead\n"
;

inline std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
  return str;
}


/* i hate argument parsing */
void parse_args(int argc, char **argv) {
  if (argc == 1) {
    std::printf(help_format, version_string);
    std::exit(1);
  }
  for (int i = 1; i < argc; i++) {
    std::string opt = argv[i];
    if (opt == "--help") {
      std::printf(help_format, version_string);
      std::exit(0);
    } else if (opt == "--verbose") {
      options.verbosity++;
    } else if (opt == "--config") {
      if (++i < argc) {
        options.config_file = fs::path(argv[i]);
        continue;
      }
      Logger::log(Logger::ERROR, "option --config requires argument");
    } else if (opt == "--version") {
      std::printf(help_format, version_string);
      std::exit(0);
    } else if (opt == "--jobs") {
      if (++i < argc) {
        try {
          options.jobs = std::stoi(argv[i]);
        } catch (...) {
          Logger::logf(Logger::ERROR, "invalid argument to --jobs \"%s\"", argv[i]);
          std::exit(1);
        }
        continue;
      }
      Logger::log(Logger::ERROR, "option --jobs requires argument");
      std::exit(1);
    } else if (opt == "--destdir") {
      if (++i < argc) {
        options.destdir = argv[i];
        continue;
      }
      Logger::log(Logger::ERROR, "option --destdir requires argument");
      std::exit(1);
    } else if (opt == "--keep-going") {
      options.keep_going = true;
    } else if (opt == "--print-targets") {
      options.print_targets = true;
    } else if (opt == "--generate-config") {
      options.generate_example = true;
    } else {
      bool is_opt = false;
      bool accepts_arg = false;
      for (size_t j = 0; j < opt.length(); j++) {
        if (j == 0) {
          if (opt[j] == '-') {
            is_opt = true;
          }
          continue;
        }
        if (j == 2 && is_opt && opt[j] == '-') {
          Logger::logf(Logger::ERROR, "unknown option \"%s\"", opt.c_str());
          std::exit(1);
        }
        if (is_opt) {
          if (opt[j] == 'h') {
            std::printf(help_format, version_string);
            std::exit(0);
          } else if (opt[j] == 'v') {
            options.verbosity++;
          } else if (opt[j] == 'j') {
            if (!accepts_arg) {
              accepts_arg = true;

              if (++i < argc) {
                try {
                  options.jobs = std::stoi(argv[i]);
                  continue;
                } catch (...) {
                  Logger::logf(Logger::ERROR, "invalid argument \"%s\" for -j", argv[i]);
                  std::exit(1);
                }
              }
              Logger::log(Logger::ERROR, "option -j requires argument");
              std::exit(1);
            }
            Logger::logf(Logger::ERROR, "cannot have multiple options which accept arguments in \"%s\"", opt.c_str());
            std::exit(1);
          } else if (opt[j] == 'c') {
            if (!accepts_arg) {
              accepts_arg = true;
              if (++i < argc) {
                options.config_file = argv[i];
                continue;
              }
              Logger::log(Logger::ERROR, "option -c requires argument");
              std::exit(1);
            }
            Logger::logf(Logger::ERROR, "cannot have multiple options which accept arguments in \"%s\"", opt.c_str());
            std::exit(1);
          } else {
            Logger::logf(Logger::ERROR, "unkown option '%c'", opt[j]);
            std::exit(1);
          }
        }
      }
      if (!is_opt) {
        options.targets.push_back(opt);
        if (opt == "all") {
          options.all_targets = true;
        }
      }
    }
  }
}

void generate_example_config() {
  while (std::filesystem::exists(options.config_file)) {
    options.config_file.append(".example");
  }
  constexpr const char * config_file_format =
    "#default backup file destination (for convenience sake)\n"
    "default_dest = {}\n"
    "same_passowrd = true\n"
    "\n"
    "[target]\n"
    "path = {}\n"
    "name = home\n"
    "encrypt = true\n"
    "exclude = {}\n"
    "\n"
    "[target]\n"
    "path = /\n"
    "name = root\n"
    "encrypt = true\n"
    "exclude = /home\n"
  ;
  fs::path home_dir = getenv("HOME");
  fs::path backup_dir = home_dir / "Backups";
  std::string config_file_contents = std::format(config_file_format, backup_dir.string(), home_dir.string(), backup_dir.string());

  std::fstream config_file{options.config_file, std::fstream::out};
  config_file << config_file_contents;

#ifndef NDEBUG
  std::printf("%s", config_file_contents.c_str());
#endif

  config_file.close();

  std::exit(options.targets.size() > 0);
}

int main(int argc, char **argv) {
  options.config_file = resolve_path_with_environment("$XDG_CONFIG_HOME/backman/backman.ini");
  if (std::getenv("XDG_CONFIG_HOME") == NULL)
    options.config_file = resolve_path_with_environment("$HOME/.config/backman/backman.ini");

  parse_args(argc, argv);

#ifndef NDEBUG
  printf(
    "config_file: %s\n"
    "jobs:        %d\n"
    "verbosity    %d\n"
    "destdir      %s\n"
    "keep_going   %d\n",
    options.config_file.c_str(),
    options.jobs,
    options.verbosity,
    options.destdir.c_str(),
    options.keep_going
  );
#endif

  if (options.generate_example) {
    generate_example_config();
  }


  if (fs::exists(options.config_file)) {
    parsed_config = INI_Parser::ini_parse(options.config_file);
  } else {
    Logger::logf(Logger::ERROR, "config file \"%s\" does not exist", options.config_file.c_str());
    std::exit(1);
  }

  std::vector<Target> targets;
  for (INI_Parser::INI_Section section : parsed_config) {
    if (section.get_section_name() == "") {

      if (parsed_config[0]["default_dest"].size() == 1)
        options.destdir = resolve_path_with_environment(parsed_config[0]["default_dest"][0]);
      else if (parsed_config[0]["default_dest"].size() > 1) {
        Logger::logf(Logger::ERROR, "default_dest may only be defined once but defined %d times", parsed_config[0]["default_dest"].size());
        std::exit(1);
      } else {
        options.destdir = resolve_path_with_environment("$HOME/Backups");
      }

      if (parsed_config[0]["same_password"].size() == 1) {
        std::string val = toLower(parsed_config[0]["same_password"][0]);
        if (val == "true" || val == "false") {
          options.same_password = val == "true";
        } else {
          Logger::logf(Logger::ERROR, "same_password expects a bool (true or false), not \"%s\"", parsed_config[0]["same_password"][0].c_str());
          std::exit(1);
        }
      } else {
        Logger::log(Logger::ERROR, "same_password defined multiple times");
        std::exit(1);
      }


    } else if (section.get_section_name() == "target") {
      targets.emplace_back(section);
    } else {
      Logger::logf(Logger::WARN, "Invalid section \"%s\", ignoring", section.get_section_name().c_str());
      std::printf("Press enter to continue or ^C to stop: ");
      std::cin.get();
    }
  }
  if (options.print_targets) {
    std::printf(
      "Config file: %s\n",
      options.config_file.c_str()
    );
    for (auto target : targets) {
      std::printf(
        "Name: %s\n"
        "Path: %s\n"
        "\n",
        target.get_name().c_str(),
        target.get_path().c_str()
      );
    }
    std::exit(0);
  }

  if (options.all_targets && options.targets.size() > 1) {
    Logger::log(Logger::ERROR, "target all called simultaneously to other targets");
    std::exit(1);
  }

  for (size_t i = 0; i < options.targets.size(); i++) {
    for (size_t j = i + 1; j < options.targets.size(); j++) {
      if (options.targets[i] == options.targets[j]) {
        Logger::logf(Logger::ERROR, "target \"%s\" requested multiple times", options.targets[i].c_str());
        std::exit(1);
      }
    }
  }


  for (size_t i = 0; i < targets.size(); i++) {
    for (size_t j = 0; j < options.targets.size(); j++) {
      if (options.all_targets || targets[i].get_name() == options.targets[j]) {
        targets[i].set_passphrase();
        goto next1;
      }
    }
    targets.erase(targets.begin() + i--);
    next1: ;
  }

  if (!options.all_targets) {
    for (size_t j = 0; j < options.targets.size(); j++) {
      for (size_t i = 0; i < targets.size(); i++) {
        if (targets[i].get_name() == options.targets[j]) {
          goto next2;
        }
      }
      Logger::logf(Logger::ERROR, "target \"%s\" not found", options.targets[j].c_str());
      std::exit(1);
      next2: ;
    }
  }


  for (auto target : targets) {
    std::printf("Running %s before hooks\n", target.get_name().c_str());
    target.run_before_hooks();
    std::printf("Running %s\n", target.get_name().c_str());
    target.run_main();
    target.wait_main();
    std::printf("Running %s end hooks\n", target.get_name().c_str());
    target.run_end_hooks();
  }
}
