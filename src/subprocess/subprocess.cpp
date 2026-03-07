

#include "subprocess/subprocess.hpp"
#include "log/log.h"

Subprocess::Subprocess() {}

Subprocess::Subprocess(Subprocess &&other) {
  this->env = std::move(other.env);
  this->command = std::move(other.command);
  this->executable = std::move(other.executable);
  this->has_executed = std::move(other.has_executed);
  this->has_detached = std::move(other.has_detached);
  this->has_joined = std::move(other.has_joined);
}

Subprocess::~Subprocess() {
  if (has_executed && !has_detached && !has_joined) {
    Logger::log(Logger::ERROR,
                "Subprocess object destructed without joining or detaching");
    std::terminate();
  }
}
