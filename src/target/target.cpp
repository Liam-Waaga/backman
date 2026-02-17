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

#include "target/target.hpp"
#include "parser/parser.hpp"
#include "log/log.h"
#include "utils.hpp"

#include <cstddef>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <string>
#include <sys/wait.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>

using namespace std::chrono_literals;

inline std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
  return str;
}

Target::Target(INI_Parser::INI_Section target_config) {
  std::vector<std::string>             paths = target_config["path"];
  std::vector<std::string>         elavateds = target_config["elavated"];
  std::vector<std::string>             names = target_config["name"];
  std::vector<std::string>             dests = target_config["dest"];
  std::vector<std::string>   compress_levels = target_config["compress_level"];
  std::vector<std::string> compress_programs = target_config["compress_program"];
  std::vector<std::string>          encrypts = target_config["encrypt"];
  std::vector<std::string>  before_hooks_str = target_config["before_hook"];
  std::vector<std::string>     end_hooks_str = target_config["end_hook"];
  std::vector<std::string>          excludes = target_config["exclude"];

  if (paths.size() != 1) {
    Logger::logf(Logger::ERROR, "path may only be defined once but defined %d times", paths.size());
    std::exit(1);
  } else {
    this->path = paths[0];
  }

  if (elavateds.size() > 1) {
    Logger::logf(Logger::ERROR, "elavated may only be defined once but defined %d times", elavateds.size());
    std::exit(1);
  } else if (elavateds.size() == 1) {
    if (toLower(elavateds[0]) == "true") this->elavated = true;
    else if (toLower(elavateds[0]) == "false") this->elavated = false;
    else {
      Logger::logf(Logger::ERROR, "invalid value \"%s\" for elavated, must be bool", elavateds[0].c_str());
      std::exit(1);
    }
  } else {
    this->elavated = false;
  }

  if (names.size() > 1) {
    Logger::logf(Logger::ERROR, "name may only be defined once but defined %d times", names.size());
    std::exit(1);
  } else if (names.size() == 1) {
    this->name = names[0];
  } else {
    this->name = "";
  }

  if (dests.size() > 1) {
    Logger::logf(Logger::ERROR, "dest may only be defined once but defined %d times", dests.size());
    std::exit(1);
  } else if (dests.size() == 1) {
    this->dest = dests[0];
  } else {
    if (parsed_config[0]["default_dest"].size() == 1)
      this->dest = parsed_config[0]["default_dest"][0];
    else if (parsed_config[0]["default_dest"].size() > 1) {
      Logger::logf(Logger::ERROR, "default_dest may only be defined once but defined %d times", parsed_config[0]["default_dest"].size());
      std::exit(1);
    } else {
      this->dest = resolve_path_with_environment("$HOME/Backups");
    }
  }

  if (compress_levels.size() > 1) {
    Logger::logf(Logger::ERROR, "compress_level may only be defined once but defined %d times", compress_levels.size());
    std::exit(1);
  } else if (compress_levels.size() == 1) {
    this->compress_level = compress_levels[0];
  } else {
    this->compress_level = "9e";
  }

  if (compress_programs.size() > 1) {
    Logger::logf(Logger::ERROR, "compress_program may only be defined once but defined %d times", compress_programs.size());
    std::exit(1);
  } else if (compress_programs.size() == 1) {
    this->compress_program = compress_programs[0];
    if (this->compress_program != "xz") {
      Logger::logf(Logger::ERROR, "only xz is supported as a compress program, not \"%s\"", this->compress_program.c_str());
      std::exit(1);
    }
  } else {
    this->compress_program = "xz";
  }

  if (encrypts.size() > 1) {
    Logger::logf(Logger::ERROR, "encrypt may only be defined once but defined %d times", encrypts.size());
    std::exit(1);
  } else if (encrypts.size() == 1) {
    if (toLower(encrypts[0]) == "true") this->encrypt = true;
    else if (toLower(encrypts[0]) == "false") this->encrypt = false;
    else {
      Logger::logf(Logger::ERROR, "invalid value \"%s\" for encrypt, must be bool", encrypts[0].c_str());
      std::exit(1);
    }

  } else {
    this->encrypt = false;
  }

  for (size_t i = 0; i < before_hooks_str.size(); i++) {
    this->before_hooks.emplace_back(before_hooks_str[i]);
  }

  for (size_t i = 0; i < end_hooks_str.size(); i++) {
    this->end_hooks.emplace_back(end_hooks_str[i]);
  }

  for (size_t i = 0; i < excludes.size(); i++) {
    this->excludes.emplace_back(excludes[i]);
  }

}

void Target::run_main() {
  /* TODO: implement run_main */
}


bool Target::run_hooks(std::vector<Target::SystemCommand> hooks) {
  int status = 0;

  int num_hooks = hooks.size();
  int running_children = 0;
  while (num_hooks > 0) {

    /* spawn children */
    if (running_children < options.jobs) {
      hooks[--num_hooks].run();
      running_children++;
    } else {
      /* stop spawning and start checking for exits */
      for (int i = hooks.size() - 1; i >= num_hooks; i--) {
        if (hooks[i].has_exited()) {
          status |= hooks[i].wait();
          running_children--;
        }
      }
      std::this_thread::sleep_for(50ms);
    }
  }
  for (size_t i = 0; i < hooks.size(); i++) {
    status |= hooks[i].wait();
  }

  return status > 0;
}


bool Target::run_before_hooks() {
  return Target::run_hooks(this->before_hooks);
}

bool Target::SystemCommand::has_exited() {
  int status = 0;
  waitpid(this->cpid, &status, WNOHANG);
  return WIFEXITED(status) || WIFSIGNALED(status);
}

void Target::SystemCommand::run() {
  this->ran = true;
  pid_t cpid = fork();
  if (cpid == 0) {
    std::exit(system(this->command.c_str()));
  } else if (cpid == -1) {
    Logger::logf(Logger::ERROR, "fork failed, can't run hook \"%s\"", command.c_str());
    this->failed = true;
  } else {
    this->cpid = cpid;
  }
}

int Target::SystemCommand::wait() {
  if (this->exited) {
    return this->exit_code;
  }
  if (this->failed) {
    this->exit_code = -1;
    return -1;
  }
  int wstatus = 0;
  waitpid(this->cpid, &wstatus, 0);
  if (WIFSIGNALED(wstatus)) {
    this->exit_code = WTERMSIG(wstatus);
    return this->exit_code;
  }
  if (WIFEXITED(wstatus)) {
    this->exit_code = WEXITSTATUS(wstatus);
    return this->exit_code;
  }
  /* something weird is happening */
  this->failed = true;
  this->exit_code = -1;
  return -1;
}

Target::SystemCommand::SystemCommand(const std::string &command) {
  this->command = command;
}
