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
  std::vector<std::string>  one_file_systems = target_config["one_file_system"];
  std::vector<std::string>  before_hooks_arr = target_config["before_hook"];
  std::vector<std::string>     end_hooks_arr = target_config["end_hook"];
  std::vector<std::string>      excludes_arr = target_config["exclude"];
  // std::vector<std::string>         tar_flags = target_config["add_tar_flag"];

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

  if (names.size() != 1) {
    Logger::logf(Logger::ERROR, "name may only be defined once but defined %d times", names.size());
    std::exit(1);
  } else {
    this->name = names[0];
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


  for (size_t i = 0; i < before_hooks_arr.size(); i++) {
    this->before_hooks.emplace_back(before_hooks_arr[i]);
  }

  for (size_t i = 0; i < end_hooks_arr.size(); i++) {
    this->end_hooks.emplace_back(end_hooks_arr[i]);
  }

  for (size_t i = 0; i < excludes_arr.size(); i++) {
    this->excludes.emplace_back(excludes_arr[i]);
  }

  // for (size_t i = 0; i < tar_flags.size(); i++) {
  //   this->tar_flags.push_back(tar_flags[i]);
  // }

#ifndef NDEBUG
  std::string before_hooks_arr_str = "[";
  for (size_t i = 0; i < before_hooks_arr.size(); i++) {
    before_hooks_arr_str += before_hooks_arr[i] + ", ";
  }
  before_hooks_arr_str += "]";

  std::string end_hooks_arr_str = "[";
  for (size_t i = 0; i < end_hooks_arr.size(); i++) {
    end_hooks_arr_str += end_hooks_arr[i] + ", ";
  }
  end_hooks_arr_str += "]";

  std::string excludes_arr_str = "[";
  for (size_t i = 0; i < excludes_arr.size(); i++) {
    excludes_arr_str += excludes_arr[i] + ", ";
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
  std::string ext = "";
  ext += this->compress_program;
  if (this->encrypt) {
    ext += ".gpg";
  }
  char *file_name = Logger::safe_format("%s_%s.tar.%s", this->name.c_str(), buff, ext.c_str());
  std::string file_name_str(file_name);
  free(file_name);
  return file_name_str;
}


void Target::run_main() {
  size_t tar_command_arguments_count = 0;

  tar_command_arguments_count++; /* tar */
  if (this->one_file_system) {
    tar_command_arguments_count++; /* --one-file-system */
  }
  tar_command_arguments_count++; /* -cp */
  tar_command_arguments_count++; /* --xattrs */
  tar_command_arguments_count++; /* --acls */
  /* we let tar take care of the compression program so that we don't have to deal with it */
  tar_command_arguments_count++; /* -I */
  tar_command_arguments_count++; /* compression program */
  tar_command_arguments_count += this->excludes.size(); /* --exclude= */
  if (!encrypt) {
    tar_command_arguments_count++; /* -f */
    tar_command_arguments_count++; /* path */
  }
  tar_command_arguments_count++; /* dest */
  tar_command_arguments_count++; /* NULL */


  char const **tar_command = (char const**) malloc(sizeof(*tar_command) * tar_command_arguments_count);
  char *destination_file_path = Logger::safe_format("%s/%s", this->dest.c_str(), this->get_file_name().c_str());

  size_t tar_command_used = 0;
  tar_command[tar_command_used++] = "tar";
  if (this->one_file_system) {
    tar_command[tar_command_used++] = "--one-file-system";
  }
  tar_command[tar_command_used++] = "-cp";
  tar_command[tar_command_used++] = "--xattrs";
  tar_command[tar_command_used++] = "--acls";
  tar_command[tar_command_used++] = "-I";

  char *compress_command = Logger::safe_format("%s %s --threads=0", this->compress_program.c_str(), this->compress_level.c_str());

  tar_command[tar_command_used++] = compress_command;

  char **exclude_args = (char **) malloc(sizeof(*exclude_args) * this->excludes.size());

  for (size_t i = 0; i < this->excludes.size(); i++) {
    exclude_args[i] = Logger::safe_format("--exclude=%s", excludes[i].c_str());
    tar_command[tar_command_used++] = exclude_args[i];
  }

  tar_command[tar_command_used++] = this->path.c_str();
  if (!encrypt) {
    tar_command[tar_command_used++] = "-f";
    tar_command[tar_command_used++] = destination_file_path;
  }

  tar_command[tar_command_used++] = NULL;
  /* tar command constructed */

  size_t gpg_command_arguments_count = 0;
  if (this->encrypt) {
    gpg_command_arguments_count++; /* gpg */
    gpg_command_arguments_count++; /* --pinentry-mode */
    gpg_command_arguments_count++; /* loopback */
    gpg_command_arguments_count++; /* --symmetric */
    gpg_command_arguments_count++; /* --cipher-algo */
    gpg_command_arguments_count++; /* AES256 */
    gpg_command_arguments_count++; /* --passphase-fd */
    gpg_command_arguments_count++; /* `fd` */
    gpg_command_arguments_count++; /* -o */
    gpg_command_arguments_count++; /* `destination_file_path` */
    gpg_command_arguments_count++; /* NULL */
  }

  char const **gpg_command = (char const **) malloc(sizeof(*gpg_command) * gpg_command_arguments_count);

  /* TODO: construct gpg_command */
  size_t gpg_command_used = 0;
  gpg_command[gpg_command_used++] = "gpg";
  gpg_command[gpg_command_used++] = "--pinentry-mode";
  gpg_command[gpg_command_used++] = "loopback";
  gpg_command[gpg_command_used++] = "--symmetric";
  gpg_command[gpg_command_used++] = "--cipher-algo";
  gpg_command[gpg_command_used++] = "AES256";
  gpg_command[gpg_command_used++] = "--passphrase-fd";
  size_t gpg_command_pwfd_index = gpg_command_used++;
  gpg_command[gpg_command_pwfd_index] = "0"; /* Placeholder */
  gpg_command[gpg_command_used++] = "-o";
  gpg_command[gpg_command_used++] = destination_file_path;
  gpg_command[gpg_command_used++] = NULL;

  if (this->encrypt) {
    int tar_and_gpg_pipefds[2]; /* [0] is write and [1] is read */
    pipe(tar_and_gpg_pipefds);
    pid_t tar_pid = fork();
    if (tar_pid == 0) {
      close(tar_and_gpg_pipefds[1]); /* close read end */
      dup2(tar_and_gpg_pipefds[0], 1); /* redirect stdout to the pipe */
      execvp(tar_command[0], /* yolo cast */ (char *const *) tar_command);
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

      close(passphrase_pipefds[0]); /* close write end */
      gpg_command[gpg_command_pwfd_index] = Logger::safe_format("%d", passphrase_pipefds[1]);
      /* no need to free because we immediately exec or exit in which case the resources are taken care of */

      close(tar_and_gpg_pipefds[0]); /* close write end */
      dup2(tar_and_gpg_pipefds[1], 0); /* redirect pipe to stdin */
      execvp(gpg_command[0], (char * const *) gpg_command);
      Logger::log(Logger::ERROR, "execvp() failed");
      kill(tar_pid, SIGKILL);
      std::exit(1);
    }
    if (gpg_pid > 0) {
      this->children.push_back(gpg_pid);

      /* gpg expects to recieve a newline as well, as that is what is supplied when given normally, as well as when given with [fd]<<< */
      passphrase += '\n';

      /* actually ignore the null terminator this time because we it is given EOF on close() */
      write(passphrase_pipefds[0], this->passphrase.c_str(), this->passphrase.length());
      close(passphrase_pipefds[0]);
    }
    if (gpg_pid == -1) {
      Logger::log(Logger::ERROR, "fork() failed");
      std::exit(1);
    }

  } else {
    /* no encryption */
    pid_t tar_pid = fork();
    if (tar_pid == 0) {
      execvp(tar_command[0], /* yolo cast */ (char *const *) tar_command);
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


  free(compress_command);
  free(tar_command);
  for (size_t i = 0; i < this->excludes.size(); i++) {
    free(exclude_args[i]);
  }
  free(exclude_args);
  free(destination_file_path);
  free(gpg_command);
}

int Target::wait_main() {
  /* TODO: implement wait_main() */
  return -1;
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
