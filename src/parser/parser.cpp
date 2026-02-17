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
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>




bool is_valid_char_for_field(const char c) {
  if (c >= 48 && c <= 57) return true;
  if (c >= 65 && c <= 90) return true;
  if (c >= 97 && c <= 122) return true;
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

  trim(line = line.substr(i + 1));

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
    if (lines[current_line].size() > 0) {
      if (lines[current_line][0] != '[') {
        throw std::runtime_error("Bad section header");
      }
    }
    std::size_t end = lines[current_line].find_first_of(']');
    if (end == 0) {
      throw std::runtime_error("Bad section name");
    }
    std::string tmp = lines[current_line].substr(end + 1);
    if (!is_comment_or_empty(tmp))
      Logger::logf(Logger::WARN, "Unkown text after section label at line %d, ignoring extra", current_line);
    std::string name = lines[current_line].substr(1, end);
    if (name.size() == 0) throw std::runtime_error("Bad section name");

    this->_section_name = name;
  } else {
    this->_section_name = "";
  }

  for (; current_line < lines.size(); current_line++) {
    std::string line = lines[current_line];
    if (!is_comment_or_empty(line)) {
      if (line[0] == '[')
        break;
      try {
        _fields.push_back(INI_Parser::INI_Field(line));
      } catch (std::runtime_error &e) {
        Logger::logf(Logger::WARN, "Error \"%s\" in INI parsing at line %d. Ignoring line", e.what(), current_line + 1);
      }
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
  std::ifstream ini_file;
  ini_file.open(ini_path, std::ios::ate);
  char * file_mem = new char [ini_file.tellg()];
  ini_file.read(file_mem, ini_file.tellg());
  std::string file_string = std::string(file_mem);
  return INI_Parser::ini_parse(file_string);
}

std::vector<INI_Parser::INI_Section> INI_Parser::ini_parse(std::string ini_source) {
  ini_source += '\n';
  std::vector<std::string> lines;
  std::size_t i = 0;
  std::size_t j;

  while ((j = ini_source.find('\n', i))) {
    lines.push_back(ini_source.substr(i, j - i));
    i = ++j;
  }

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
