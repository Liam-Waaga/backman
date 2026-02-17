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
#include "log/log.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>




bool is_valid_char_for_field(const char c) {
  if (c >= 48 && c <= 57) return true;
  if (c >= 65 && c <= 90) return true;
  if (c >= 97 && c <= 122) return true;
  if (c == 95) return true;
  return false;
}

/* Begin borrowed code */

// Source - https://stackoverflow.com/a/25385766
// Posted by Galik, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-29, License - CC BY-SA 4.0

const char* ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

/* End borrowed code */


INI_Parser::INI_Field::INI_Field(std::string line) {
  trim(line);

  std::string field = "";
  std::size_t i = 0;
  for (; i < line.length(); i++) {
    char c = line[i];
    if (c == ' ') {
      break;
    } else if (c == '\t') {
      break;
    } else if (c == '=') {
      break;
    } else if (!is_valid_char_for_field(c)) {
      throw std::runtime_error("Invalid character field name");
    }
    field += c;
  }
  if (field == "")
    throw std::runtime_error("Field must not be empty");

  trim(line = line.substr(line.find('=') + 1));

  char quotes = 0;

  std::string value = "";

  for (i = 0; i < line.length(); i++) {
    char c = line[i];
    if (c == '\'' || c == '"') {
      if (quotes == c) quotes = 0;
      else if (quotes == 0) quotes = c;
      else value += c;
      continue;
    }
    if (!quotes) {
      if (c == ' ' || c == '\t') {
        break;
      }
      if (c == '#' || c == ';') {
        break;
      }
    }
    value += c;
  }

  this->_field = field;
  this->_value = value;
}


std::string INI_Parser::INI_Field::get_field() {
  return _field;
}

std::string &INI_Parser::INI_Field::get_value() {
  return _value;
}

bool is_comment_or_empty(std::string line) {
  trim(line);
  return line.size() == 0 || line[0] == ';' || line[0] == '#';
}



INI_Parser::INI_Section::INI_Section(std::vector<std::string> &lines, std::size_t &current_line, bool section_global) {
  if (!section_global) {

    std::string header = lines[current_line];

    // Trim leading whitespace
    std::size_t first = header.find_first_not_of(" \t\r");
    if (first == std::string::npos || header[first] != '[')
      throw std::runtime_error("Bad section header");

    // Find closing bracket
    std::size_t end = header.find(']', first + 1);
    if (end == std::string::npos)
      throw std::runtime_error("Missing closing bracket in section header");

    // Extract section name (between brackets)
    std::string name = header.substr(first + 1, end - (first + 1));

    // Trim whitespace around name
    std::size_t name_start = name.find_first_not_of(" \t\r");
    std::size_t name_end   = name.find_last_not_of(" \t\r");

    if (name_start == std::string::npos)
      throw std::runtime_error("Bad section name");

    name = name.substr(name_start, name_end - name_start + 1);

    // Check for trailing garbage after ']'
    std::string after = header.substr(end + 1);

    if (!is_comment_or_empty(after))
      Logger::logf(Logger::WARN, "Unknown text after section label at line %d", current_line + 1);

    _section_name = name;

    current_line++; // move past header
  }
  else {
    _section_name = "";
  }

  // Parse fields
  for (; current_line < lines.size(); current_line++) {

    std::string line = lines[current_line];

    if (is_comment_or_empty(line))
      continue;

    // Trim leading whitespace to detect section start
    std::size_t first = line.find_first_not_of(" \t\r");
    if (first != std::string::npos && line[first] == '[')
      break;

    try {
      _fields.emplace_back(line);
    }
    catch (std::runtime_error &e) {
      Logger::logf(Logger::WARN, "Error \"%s\" in INI parsing at line %d. Ignoring line (\"%s\")", e.what(), current_line + 1, line.c_str());
    }
  }
}


std::vector<std::string> INI_Parser::INI_Section::operator[](std::string field_name) const {
  std::vector<std::string> ret;
  for (auto field : _fields) {
    if (field.get_field() == field_name) {
      ret.push_back(field.get_value());
    }
  }
  return ret;
}

std::string INI_Parser::INI_Section::get_section_name() {
  return _section_name;
}

std::vector<INI_Parser::INI_Section> INI_Parser::ini_parse(std::filesystem::path ini_path) {
  if (std::filesystem::exists(ini_path)) {
    std::string file_string = "";
    FILE *file = fopen(ini_path.c_str(), "rb");
    if (file == NULL) {
      throw std::runtime_error("config file not found");
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    char *buff = (char *) malloc(size);

    fseek(file, 0, SEEK_SET);
    fread(buff, 1, size, file);

    file_string = buff;

    free(buff);

    return INI_Parser::ini_parse(file_string);
  } else {
    throw std::runtime_error("config file not found");
  }
}

std::vector<std::string> split(std::string str, const std::string delim) {
  std::vector<std::string> ret;
  size_t pos = str.find(delim);
  while (pos != std::string::npos) {
    ret.push_back(str.substr(0, pos));
    str.erase(0, pos + delim.length());
    pos = str.find(delim);
  }
  return ret;
}

std::vector<INI_Parser::INI_Section> INI_Parser::ini_parse(std::string ini_source) {
  std::vector<std::string> lines = split(ini_source, "\n");

  return INI_Parser::ini_parse(lines);
}

std::vector<INI_Parser::INI_Section> INI_Parser::ini_parse(std::vector<std::string> ini_source) {
  std::size_t i = 0;
  std::vector<INI_Parser::INI_Section> ini_data;
  while (i != ini_source.size()) {
    ini_data.emplace_back(ini_source, i, i == 0 ? true : false);
  }
  return ini_data;
}
