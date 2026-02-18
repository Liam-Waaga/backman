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


#include "utils.hpp"


#include <string>
#include <filesystem>
#include <cstdlib>
#include <cctype>

std::filesystem::path resolve_path_with_environment(const std::string& path) {
  std::string result;
  result.reserve(path.size());

  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i] != '$') {
      result += path[i];
      continue;
    }

    // Handle ${VAR}
    if (i + 1 < path.size() && path[i + 1] == '{') {
      size_t end = path.find('}', i + 2);
      if (end == std::string::npos) {
        result += '$'; // malformed, keep literal
        continue;
      }

      std::string var = path.substr(i + 2, end - (i + 2));
      const char* val = std::getenv(var.c_str());

      if (val)
        result += val;
      else
        result += "${" + var + "}";

      i = end;
      continue;
    }

    // Handle $VAR
    size_t start = i + 1;
    size_t j = start;

    while (j < path.size() &&
           (std::isalnum(static_cast<unsigned char>(path[j])) || path[j] == '_')) {
      ++j;
    }

    if (j == start) {
      result += '$'; // lone $
      continue;
    }

    std::string var = path.substr(start, j - start);
    const char* val = std::getenv(var.c_str());

    if (val)
      result += val;
    else
      result += "$" + var;

    i = j - 1;
  }

  return std::filesystem::path(result);
}
