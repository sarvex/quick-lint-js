// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <quick-lint-js/diagnostic-assertion.h>
#include <quick-lint-js/fe/diagnostic-types.h>

namespace quick_lint_js {
namespace {
diagnostic_assertion parse(const char8* specification) {
  diagnostic_assertion da =
      diagnostic_assertion::parse(specification, strlen(specification));

  if (da.parse_error_messages) {
    EXPECT_FALSE(da.parse_error_messages->empty())
        << "if parse_error_messages was provided, it should not be empty";
    for (const std::string& s : *da.parse_error_messages) {
      ADD_FAILURE() << "diagnostic_assertion::parse failed: " << s;
    }
  }

  return da;
}

TEST(test_diagnostic_assertion, parse_one_character_span) {
  diagnostic_assertion da = parse(u8"^ diag_assignment_to_const_variable");
  EXPECT_EQ(da.type, diag_type::diag_assignment_to_const_variable);
  EXPECT_EQ(da.span_begin_offset, 0);
  EXPECT_EQ(da.span_end_offset, 1);
}

TEST(test_diagnostic_assertion, parse_one_character_span_at_nonzero) {
  diagnostic_assertion da = parse(u8"     ^ diag_assignment_to_const_variable");
  EXPECT_EQ(da.type, diag_type::diag_assignment_to_const_variable);
  EXPECT_EQ(da.span_begin_offset, 5);
  EXPECT_EQ(da.span_end_offset, 6);
}

TEST(test_diagnostic_assertion, parse_multiple_character_span) {
  diagnostic_assertion da = parse(u8"^^^^ diag_assignment_to_const_variable");
  EXPECT_EQ(da.type, diag_type::diag_assignment_to_const_variable);
  EXPECT_EQ(da.span_begin_offset, 0);
  EXPECT_EQ(da.span_end_offset, 4);
}

TEST(test_diagnostic_assertion, parse_unit_character_span) {
  diagnostic_assertion da = parse(u8"` diag_assignment_to_const_variable");
  EXPECT_EQ(da.type, diag_type::diag_assignment_to_const_variable);
  EXPECT_EQ(da.span_begin_offset, 0);
  EXPECT_EQ(da.span_end_offset, 0);
}

TEST(test_diagnostic_assertion, parse_unit_character_span_at_nonzero) {
  diagnostic_assertion da = parse(u8"    ` diag_assignment_to_const_variable");
  EXPECT_EQ(da.type, diag_type::diag_assignment_to_const_variable);
  EXPECT_EQ(da.span_begin_offset, 4);
  EXPECT_EQ(da.span_end_offset, 4);
}

TEST(test_diagnostic_assertion, parse_spaces_between_caret_and_diag_type) {
  diagnostic_assertion da =
      parse(u8"^     diag_pointless_strict_comp_against_empty_array_literal");
  EXPECT_EQ(da.type,
            diag_type::diag_pointless_strict_comp_against_empty_array_literal);
  EXPECT_EQ(da.span_begin_offset, 0);
  EXPECT_EQ(da.span_end_offset, 1);
}

TEST(test_diagnostic_assertion, invalid_diag_type_fails) {
  diagnostic_assertion da = u8"^ diag_does_not_exist"_diag;
  ASSERT_NE(da.parse_error_messages, nullptr);
  EXPECT_THAT(*da.parse_error_messages,
              ::testing::ElementsAreArray({
                  "invalid diagnostic type: 'diag_does_not_exist'",
              }));
}

TEST(test_diagnostic_assertion, trailing_whitespace_fails) {
  diagnostic_assertion da = u8"^ diag_assignment_to_const_variable   "_diag;
  ASSERT_NE(da.parse_error_messages, nullptr);
  EXPECT_THAT(*da.parse_error_messages,
              ::testing::ElementsAreArray({
                  "trailing whitespace is not allowed in _diag",
              }));
}

TEST(test_diagnostic_assertion, stray_invalid_character_fails) {
  diagnostic_assertion da = u8"^~ diag_assignment_to_const_variable"_diag;
  ASSERT_NE(da.parse_error_messages, nullptr);
  EXPECT_THAT(*da.parse_error_messages, ::testing::ElementsAreArray({
                                            "unexpected '~' in _diag",
                                        }));
}
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
