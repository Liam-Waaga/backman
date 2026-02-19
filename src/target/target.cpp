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

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <thread>
#include <unistd.h>
#include <string>
#include <sys/wait.h>
#include <sys/types.h>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std::chrono_literals;

inline std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
  return str;
}

Target::Target(INI_Parser::INI_Section target_config) {
  std::vector<std::string>               paths = target_config["path"];
  std::vector<std::string>           elavateds = target_config["elavated"];
  std::vector<std::string>               names = target_config["name"];
  std::vector<std::string>               dests = target_config["dest"];
  std::vector<std::string>   compress_programs = target_config["compress_program"];
  std::vector<std::string>            encrypts = target_config["encrypt"];
  std::vector<std::string>    one_file_systems = target_config["one_file_system"];
  std::vector<std::string>    before_hooks_arr = target_config["before_hook"];
  std::vector<std::string>       end_hooks_arr = target_config["end_hook"];
  std::vector<std::string>        excludes_arr = target_config["exclude"];
  std::vector<std::string> elavate_program_arr = target_config["elavate_program"];
  std::vector<std::string>           tar_flags = target_config["add_tar_flag"];

  if (paths.size() != 1) {
    Logger::logf(Logger::ERROR, "path may only be defined once but defined %d times", paths.size());
    std::exit(1);
  } else {
    this->path = resolve_path_with_environment(paths[0]);
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

  if (names.size() != 1) {
    Logger::logf(Logger::ERROR, "name may only be defined once but defined %d times", names.size());
    std::exit(1);
  } else {
    this->name = names[0];
  }

  if (options.destdir != "") {
    this->destdir = options.destdir;
  } else {
    if (dests.size() > 1) {
      Logger::logf(Logger::ERROR, "dest may only be defined once but defined %d times", dests.size());
      std::exit(1);
    } else if (dests.size() == 1) {
      this->destdir = resolve_path_with_environment(dests[0]);
    } else {
      if (parsed_config[0]["default_dest"].size() == 1)
        this->destdir = resolve_path_with_environment(parsed_config[0]["default_dest"][0]);
      else if (parsed_config[0]["default_dest"].size() > 1) {
        Logger::logf(Logger::ERROR, "default_dest may only be defined once but defined %d times", parsed_config[0]["default_dest"].size());
        std::exit(1);
      } else {
        this->destdir = resolve_path_with_environment("$HOME/Backups");
      }
    }
  }

  if (compress_programs.size() > 1) {
    Logger::logf(Logger::ERROR, "compress_program may only be defined once but defined %d times", compress_programs.size());
    std::exit(1);
  } else if (compress_programs.size() == 1) {
    this->compress_program = compress_programs[0];
  } else {
    this->compress_program = "xz -9e --threads=0";
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
    this->encrypt = this->elavated;
  }

  if (one_file_systems.size() > 1) {
    Logger::logf(Logger::ERROR, "one_file_system may only be defined once but defined %d times", one_file_systems.size());
    std::exit(1);
  } else if (one_file_systems.size() == 1) {
    if (toLower(one_file_systems[0]) == "true") this->one_file_system = true;
    else if (toLower(one_file_systems[0]) == "false") this->one_file_system = false;
    else {
      Logger::logf(Logger::ERROR, "invalid value \"%s\" for one_file_system, must be bool", one_file_systems[0].c_str());
      std::exit(1);
    }

  } else {
    this->one_file_system = true;
  }

  if (elavate_program_arr.size() > 1) {
    Logger::logf(Logger::ERROR, "elavate_program may only be defined once but defined %d times", elavate_program_arr.size());
    std::exit(1);
  } else if (elavate_program_arr.size() == 1) {
    this->elavate_program = elavate_program_arr[0];
  } else {
    this->elavate_program = "su";
  }

  for (size_t i = 0; i < excludes_arr.size(); i++) {
    this->excludes.emplace_back(excludes_arr[i]);
  }

  for (size_t i = 0; i < tar_flags.size(); i++) {
    this->tar_flags.push_back(tar_flags[i]);
  }


  this->destfile = this->destdir / this->get_file_name();



  for (size_t i = 0; i < before_hooks_arr.size(); i++) {
    this->before_hooks.emplace_back(
      "BACKMAN_TARGET_DESTFILE=\""
      + this->destfile.generic_string()
      + "\" BACKMAN_TARGET_NAME=\""
      + this->name
      + "\" BACKMAN_TARGET_DESTDIR=\""
      + this->destdir.generic_string()
      + "\""
      + before_hooks_arr[i]
    );
  }

  for (size_t i = 0; i < end_hooks_arr.size(); i++) {
    this->end_hooks.emplace_back(
      "BACKMAN_TARGET_DESTFILE=\""
      + this->destfile.generic_string()
      + "\" BACKMAN_TARGET_NAME=\""
      + this->name
      + "\" BACKMAN_TARGET_DESTDIR=\""
      + this->destdir.generic_string()
      + "\""
      + end_hooks_arr[i]
    );
  }

#ifndef NDEBUG
  std::string before_hooks_arr_str = "[";
  for (size_t i = 0; i < before_hooks_arr.size(); i++) {
    before_hooks_arr_str += "\"" + before_hooks_arr[i] + "\", ";
  }
  before_hooks_arr_str += "]";

  std::string end_hooks_arr_str = "[";
  for (size_t i = 0; i < end_hooks_arr.size(); i++) {
    end_hooks_arr_str += "\"" + end_hooks_arr[i] + "\", ";
  }
  end_hooks_arr_str += "]";

  std::string excludes_arr_str = "[";
  for (size_t i = 0; i < excludes_arr.size(); i++) {
    excludes_arr_str += "\"" + excludes_arr[i] + "\", ";
  }
  excludes_arr_str += "]";

  std::printf(
    "path:             %s\n"
    "elavated:         %d\n"
    "name:             %s\n"
    "dest:             %s\n"
    "compress_level:   %s\n"
    "compress_program: %s\n"
    "encrypt:          %d\n"
    "one_file_system:  %d\n"
    "before_hooks:     %s\n"
    "end_hooks:        %s\n"
    "excludes:         %s\n",
    this->path.c_str(),
    (int) this->elavated,
    this->name.c_str(),
    this->dest.c_str(),
    this->compress_level.c_str(),
    this->compress_program.c_str(),
    (int) this->encrypt,
    (int) this->one_file_system,
    before_hooks_arr_str.c_str(),
    end_hooks_arr_str.c_str(),
    excludes_arr_str.c_str()
  );
#endif
}

std::string Target::get_file_name() {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char buff[128];
  strftime(buff, sizeof(buff), "%Y-%m-%d", &tm);
  std::string ext = ".tar.";
  ext += this->compress_program;
  if (this->encrypt) {
    ext += ".gpg";
  }
  std::string name = this->name + "_" + buff + ext;
  // char *file_name = Logger::safe_format("%s_%s.tar.%s", this->name.c_str(), buff, ext.c_str());
  // std::string file_name_str(file_name);
  // free(file_name);
  return name;
}


void Target::run_main() {

  if (this->encrypt && this->passphrase == "") {
    Logger::log(Logger::ERROR, "encrypt set to true but no passphrase was provided (bug)");
    std::exit(1);
  }


  if (geteuid() == 0 || getegid() == 0) {
    this->elavated = false; /* technically true but we don't need to elavate so we set it to false so we don't later */
  }

  std::vector<const char *> tar_command;
  std::vector<const char *> gpg_command;

  if (this->elavated) {
    tar_command.push_back(this->elavate_program.c_str());
    tar_command.push_back("--");
  }
  tar_command.push_back("tar");
  if (this->one_file_system) {
    tar_command.push_back("--one-file-system");
  }
  tar_command.push_back("-cp");
  tar_command.push_back("--xattrs");
  tar_command.push_back("--acls");
  tar_command.push_back("-I");


  tar_command.push_back(this->compress_program.c_str());

  for (size_t i = 0; i < this->excludes.size(); i++) {
    std::string exclude = "--exclude=\"" + excludes[i].generic_string() + "\"";
    tar_command.push_back(exclude.c_str());
  }

  for (std::string arg : this->tar_flags) {
    tar_command.push_back(arg.c_str());
  }

  tar_command.push_back(this->path.c_str());
  if (!encrypt) {
    tar_command.push_back("-f");
    tar_command.push_back(this->destfile.c_str());
  }

  tar_command.push_back(NULL);
  /* tar command constructed */



  gpg_command.push_back("gpg");
  gpg_command.push_back("--batch");
  gpg_command.push_back("--yes");
  gpg_command.push_back("--pinentry-mode");
  gpg_command.push_back("loopback");
  gpg_command.push_back("--passphrase-fd");
  size_t gpg_command_pw_fd_index = gpg_command.size();
  gpg_command.push_back("0");
  gpg_command.push_back("--symmetric");
  gpg_command.push_back("--cipher-algo");
  gpg_command.push_back("AES256");
  gpg_command.push_back("-o");
  gpg_command.push_back(this->destfile.c_str());
  gpg_command.push_back(NULL);


  try {
    fs::create_directories(this->destdir);
  } catch (const std::exception &e) {
    Logger::logf(Logger::ERROR, "error creating destination directory (no permission?)\"%s\"", e.what());
    std::exit(1);
  }


  /* actually run the programs */
  if (this->encrypt) {
    int tar_and_gpg_pipefds[2]; /* [1] is write and [0] is read */
    pipe(tar_and_gpg_pipefds);
    pid_t tar_pid = fork();
    if (tar_pid == 0) {
      close(tar_and_gpg_pipefds[0]); /* close read end */
      dup2(tar_and_gpg_pipefds[1], 1); /* redirect stdout to the pipe */
      close(tar_and_gpg_pipefds[1]);
      execvp(tar_command[0], (char *const *) tar_command.data());
      Logger::log(Logger::ERROR, "execvp() failed");
      std::exit(1);
    }
    if (tar_pid > 0) {
      this->children.push_back(tar_pid);
    }
    if (tar_pid == -1) {
      Logger::log(Logger::ERROR, "fork() call failed");
      std::exit(1);
    }

    int passphrase_pipefds[2];
    pipe(passphrase_pipefds);

    pid_t gpg_pid = fork();

    if (gpg_pid == 0) {

      close(passphrase_pipefds[1]); /* close write end */
      std::string pw_fd = std::to_string(passphrase_pipefds[0]);
      gpg_command[gpg_command_pw_fd_index] = pw_fd.c_str();

      close(tar_and_gpg_pipefds[1]); /* close write end */
      dup2(tar_and_gpg_pipefds[0], 0); /* redirect pipe to stdin */
      close(tar_and_gpg_pipefds[0]);
      execvp(gpg_command[0], (char * const *) gpg_command.data());
      Logger::log(Logger::ERROR, "execvp() failed");
      kill(tar_pid, SIGTERM);
      std::exit(1);
    }
    if (gpg_pid > 0) {
      this->children.push_back(gpg_pid);

      /* gpg expects to recieve a newline as well, as that is what is supplied when given normally, as well as when given with [fd]<<< */
      this->passphrase += '\n';

      std::printf("%s", passphrase.c_str());

      /* actually ignore the null terminator this time because it isn't expecting a c string */
      write(passphrase_pipefds[1], this->passphrase.c_str(), this->passphrase.length());
      close(passphrase_pipefds[1]);
    }
    if (gpg_pid == -1) {
      Logger::log(Logger::ERROR, "fork() failed");
      std::exit(1);
    }
    close(tar_and_gpg_pipefds[0]);
    close(tar_and_gpg_pipefds[1]);


  } else {
    /* no encryption */
    pid_t tar_pid = fork();
    if (tar_pid == 0) {
      execvp(tar_command[0], (char *const *) tar_command.data());
      Logger::log(Logger::ERROR, "execvp() failed");
      std::exit(1);
    }
    if (tar_pid > 0) {
      this->children.push_back(tar_pid);
    }
    if (tar_pid == -1) {
      Logger::log(Logger::ERROR, "fork() call failed");
      std::exit(1);
    }
  }
}

void Target::set_passphrase() {
  if (this->is_encrypted()) {
    get_pass:

    std::string pass;
    std::string verify_pass;
    std::printf("Passphrase for target \"%s\": ", this->name.c_str());
    getline_noecho(std::cin, pass);
    std::printf("\n");

    std::printf("Confirm passphrase for target \"%s\": ", this->name.c_str());
    getline_noecho(std::cin, verify_pass);
    std::printf("\n");

    if (pass != verify_pass) {
      std::printf("Passphrases don't match\n");
      goto get_pass;
    }


    this->passphrase = pass;
  }

}

bool Target::is_encrypted() {
  return this->encrypt;
}

bool Target::has_exited() {
  for (pid_t cpid : this->children) {
    int status = 0;
    waitpid(cpid, &status, WNOHANG);
    bool has_exited = WIFEXITED(status) || WIFSIGNALED(status);
    if (has_exited)
      return true;
  }
  return false;
}


void Target::wait_main() {
  for (pid_t cpid : this->children) {
    int stat = 0;
    waitpid(cpid, &stat, 0);
  }
}

std::string Target::get_name() {
  return this->name;
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

bool Target::run_end_hooks() {
  return Target::run_hooks(this->end_hooks);
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
