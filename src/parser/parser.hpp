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


#include <cstddef>
#include <vector>
#include <string>
#include <filesystem>

namespace INI_Parser {


    class INI_Field {

        public:
        INI_Field(std::string line);

        std::string get_field();
        std::string &get_value();


        private:
        std::string _field;
        std::string _value;
    };


    class INI_Section {

        public:
        /* Don't include newline */
        /* expects lines to be the entire ini document */
        /* nothing bad happens if it isn't except logs give the wrong line numbers */
        /* auto increments current_line to be the first line of the next section */
        /* if there is no other section, current_line will equal lines.size() */
        INI_Section(std::vector<std::string> &lines, std::size_t &current_line, bool section_global = false);

        /* returns the value of the field contained inside the field with the name `field` */
        /* returns an empty string if the field is not found (or field's value is an empty string) */
        /* returns the *first* found result */
        std::vector<std::string> operator[](std::string field) const;

        std::string get_section_name();

        private:

        void add_field(INI_Field field);

        std::vector<INI_Field> _fields;
        std::string _section_name;
    };

    typedef std::vector<INI_Section> INI_Data;

    /* the following parse functions return the first INI_Field to be the global options */
    /* ie options declared without section header */
    /* and the section name is "" */

    /* accepts the *contents* of the ini as a single string, not a path */
    INI_Data ini_parse(std::string ini_source);

    /* accepts the *contents* of the ini as a vector of strings, each value is a line, without newlines */
    INI_Data ini_parse(std::vector<std::string> ini_source);

    /* accepts the path to the ini */
    INI_Data ini_parse(std::filesystem::path ini_path);
}
