

#include <filesystem>
#include <sys/types.h>
#include <vector>
class Subprocess {

public:
  Subprocess();

  Subprocess(Subprocess &) = delete;
  Subprocess(Subprocess &&);

  ~Subprocess();

  /* returns true if the executable */
  /* 1. exists */
  /* 2. is executable by the current user */
  /* TODO: implement */
  bool set_executable(std::filesystem::path executable);

  /* TODO: implement */
  void add_argument(std::string arg);

  /* returns whether or not execution failed */
  /* TODO: implement */
  bool run();

  /* returns the pid of the process */
  /* TODO: implement */
  pid_t detach();

  /* returns the return code of the process */
  /* TODO: implement */
  int join();

private:
  std::vector<char *> env;
  std::vector<char *> command;
  std::filesystem::path executable;

  bool has_executed;
  bool has_detached;
  bool has_joined;

  int return_code;
  int kill_signal_code;

  pid_t pid;
};
