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

#pragma once

#include "parser/parser.hpp"


#include <filesystem>
#include <sys/types.h>
#include <vector>

namespace fs = std::filesystem;

class Target {
  public:

  Target(INI_Parser::INI_Section target_config);

  /* begins execution of the target */
  void        run_main();
  void        wait_main();
  bool        has_exited();
  bool        run_before_hooks();
  bool        run_end_hooks();
  void        set_passphrase();
  bool        is_encrypted();
  std::string get_name();

  class SystemCommand {
    public:
    SystemCommand(const std::string &command);

    void run();
    /* returns -1 internal error, otherwise returns the exit code of the command */
    int  wait();
    bool has_exited();

    private:
    bool         failed = false; /* error other than command (fork failed) */
    bool            ran = false;
    bool         exited = false;
    int       exit_code = 0;
    pid_t          cpid = -1;
    std::string command = "";
  };

  private:
  fs::path                           path;
  bool                               elavated;
  std::string                        name;
  fs::path                           dest;
  std::string                        compress_level;
  std::string                        compress_program;
  bool                               encrypt;
  bool                               one_file_system;
  std::string                        passphrase;
  std::vector<SystemCommand>         before_hooks;
  std::vector<SystemCommand>         end_hooks;
  std::vector<std::filesystem::path> excludes;
  std::vector<pid_t>                 children;
  struct ProcExitStatusSimple {
    int    code = -1;
    bool exited = false;
    bool failed = false;
  };
  std::vector<ProcExitStatusSimple>  children_status;
  std::filesystem::path              old_path;
  std::string                        elavate_program;

  // std::vector<std::string>           tar_flags;

  static bool run_hooks(std::vector<SystemCommand> hooks);
  std::string get_file_name();

};
