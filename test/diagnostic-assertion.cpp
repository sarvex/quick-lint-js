// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cstdio>
#include <cstdlib>
#include <quick-lint-js/container/concat.h>
#include <quick-lint-js/container/hash-map.h>
#include <quick-lint-js/container/string-view.h>
#include <quick-lint-js/diagnostic-assertion.h>
#include <quick-lint-js/fe/diagnostic-types.h>
#include <quick-lint-js/port/char8.h>
#include <quick-lint-js/util/narrow-cast.h>
#include <type_traits>

using namespace std::literals::string_view_literals;

namespace quick_lint_js {
namespace {
#define U8SV(string) u8##string##_sv

#define QLJS_DIAG_TYPE(name, code, severity, struct_body, format_call) \
  {U8SV(#name), ::quick_lint_js::diag_type::name},
hash_map<string8_view, diag_type> diag_type_name_to_diag_type =
    hash_map<string8_view, diag_type>{QLJS_X_DIAG_TYPES};
}
#undef QLJS_DIAG_TYPE

bool is_diag_type_char(char8 c) {
  return (u8'a' <= c && c <= u8'z') ||  //
         (u8'A' <= c && c <= u8'Z') ||  //
         (u8'0' <= c && c <= u8'9') ||  //
         c == u8'_';
}

diagnostic_assertion diagnostic_assertion::parse(
    const char8* specification, unsigned long specification_length) {
  const char8* p = specification;
  std::vector<std::string> errors;

  padded_string_size leading_space_count = 0;
  for (; *p == u8' '; ++p) {
    leading_space_count += 1;
  }

  padded_string_size caret_count = 0;
  if (*p == u8'`') {
    ++p;
  } else {
    for (; *p == u8'^'; ++p) {
      caret_count += 1;
    }
  }

  for (; *p == u8' '; ++p) {
  }

  const char8* diag_type_begin = p;
  for (; is_diag_type_char(*p); ++p) {
  }
  const char8* diag_type_end = p;
  string8_view diag_type_span =
      make_string_view(diag_type_begin, diag_type_end);

  if (p != specification + specification_length) {
    if (*p == ' ') {
      errors.push_back("trailing whitespace is not allowed in _diag");
    } else {
      errors.push_back(concat("unexpected '"sv,
                              to_string_view(string8_view(p, 1)),
                              "' in _diag"sv));
    }
  }

  diag_type type;
  auto diag_type_it = diag_type_name_to_diag_type.find(diag_type_span);
  if (diag_type_it == diag_type_name_to_diag_type.end()) {
    if (errors.empty()) {
      errors.push_back(concat("invalid diagnostic type: '"sv,
                              to_string_view(diag_type_span), "'"sv));
    }
    type = diag_type();
  } else {
    type = diag_type_it->second;
  }

  std::vector<std::string>* parse_error_messages = nullptr;
  if (!errors.empty()) {
    // We intentionally leak the pointer so diagnostic_assertion can remain
    // trivial.
    static_assert(std::is_trivial_v<diagnostic_assertion>);
    parse_error_messages = new std::vector<std::string>(std::move(errors));
  }

  return diagnostic_assertion{
      .type = type,
      .span_begin_offset = leading_space_count,
      .span_end_offset = leading_space_count + caret_count,
      .parse_error_messages = parse_error_messages,
  };
}

diagnostic_assertion operator""_diag(const char8* specification,
                                     unsigned long specification_length) {
  return diagnostic_assertion::parse(specification, specification_length);
}
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
