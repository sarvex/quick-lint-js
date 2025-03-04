// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cstddef>
#include <gmock/gmock.h>
#include <optional>
#include <quick-lint-js/cli/cli-location.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/diag-collector.h>
#include <quick-lint-js/diag-matcher.h>
#include <quick-lint-js/diag/diagnostic-types.h>
#include <quick-lint-js/port/unreachable.h>
#include <string_view>

using namespace std::literals::string_view_literals;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace quick_lint_js {
namespace {
template <class Value>
std::string get_matcher_message(::testing::Matcher<const Value &> matcher,
                                const Value &value) {
  ::testing::StringMatchResultListener listener;
  ExplainMatchResult(matcher, value, &listener);
  return listener.str();
}

TEST(test_diag_matcher, match_error_type) {
  padded_string code(u8"hello"_sv);

  ::testing::Matcher<const diag_collector::diag &> continue_matcher =
      DIAG_TYPE(diag_invalid_continue);
  EXPECT_TRUE(
      continue_matcher.Matches(diag_collector::diag(diag_invalid_continue{
          .continue_statement = source_code_span(&code[0], &code[5]),
      })));
  EXPECT_FALSE(continue_matcher.Matches(diag_collector::diag(diag_invalid_break{
      .break_statement = source_code_span(&code[0], &code[5]),
  })));

  ::testing::Matcher<const diag_collector::diag &> break_matcher =
      DIAG_TYPE(diag_invalid_break);
  EXPECT_FALSE(break_matcher.Matches(diag_collector::diag(diag_invalid_continue{
      .continue_statement = source_code_span(&code[0], &code[5]),
  })));
  EXPECT_TRUE(break_matcher.Matches(diag_collector::diag(diag_invalid_break{
      .break_statement = source_code_span(&code[0], &code[5]),
  })));
}

TEST(test_diag_matcher, match_error_type_message) {
  padded_string code(u8"hello"_sv);
  ::testing::Matcher<const diag_collector::diag &> matcher =
      DIAG_TYPE(diag_invalid_break);
  diag_collector::diag value(diag_invalid_continue{
      .continue_statement = source_code_span(&code[0], &code[5]),
  });
  EXPECT_EQ(get_matcher_message(matcher, value),
            "whose type (diag_invalid_continue) isn't diag_invalid_break");
}

TEST(test_diag_matcher, match_error_type_with_1_field) {
  padded_string code(u8"hello"_sv);

  ::testing::Matcher<const diag_collector::diag &> continue_matcher =
      DIAG_TYPE_OFFSETS(&code, diag_invalid_continue,  //
                        continue_statement, 0, u8"hello"_sv);
  EXPECT_TRUE(
      continue_matcher.Matches(diag_collector::diag(diag_invalid_continue{
          .continue_statement = source_code_span(&code[0], &code[5]),
      })));
  EXPECT_FALSE(continue_matcher.Matches(diag_collector::diag(diag_invalid_break{
      .break_statement = source_code_span(&code[0], &code[5]),
  })));

  ::testing::Matcher<const diag_collector::diag &> break_matcher =
      DIAG_TYPE_OFFSETS(&code, diag_invalid_break,  //
                        break_statement, 0, u8"hello"_sv);
  EXPECT_FALSE(break_matcher.Matches(diag_collector::diag(diag_invalid_continue{
      .continue_statement = source_code_span(&code[0], &code[5]),
  })));
  EXPECT_TRUE(break_matcher.Matches(diag_collector::diag(diag_invalid_break{
      .break_statement = source_code_span(&code[0], &code[5]),
  })));
}

TEST(test_diag_matcher, match_error_type_with_1_field_message) {
  padded_string code(u8"hello"_sv);
  ::testing::Matcher<const diag_collector::diag &> matcher =
      DIAG_TYPE_OFFSETS(&code, diag_invalid_continue,  //
                        continue_statement, 0, u8"hello"_sv);
  diag_collector::diag value(diag_invalid_break{
      .break_statement = source_code_span(&code[0], &code[5]),
  });
  EXPECT_EQ(get_matcher_message(matcher, value),
            "whose type (diag_invalid_break) isn't diag_invalid_continue");
}

TEST(test_diag_matcher, match_offsets_of_1_field_span) {
  padded_string code(u8"hello"_sv);

  ::testing::Matcher<const diag_collector::diag &> continue_matcher =
      DIAG_TYPE_OFFSETS(&code, diag_invalid_continue,  //
                        continue_statement, 1, u8"ello"_sv);
  EXPECT_TRUE(
      continue_matcher.Matches(diag_collector::diag(diag_invalid_continue{
          .continue_statement = source_code_span(&code[1], &code[5]),
      })));
  EXPECT_FALSE(
      continue_matcher.Matches(diag_collector::diag(diag_invalid_continue{
          .continue_statement = source_code_span(&code[0], &code[5]),
      })));
  EXPECT_FALSE(
      continue_matcher.Matches(diag_collector::diag(diag_invalid_continue{
          .continue_statement = source_code_span(&code[0], &code[4]),
      })));
}

TEST(test_diag_matcher, match_offsets_of_1_field_identifier) {
  padded_string code(u8"hello"_sv);

  ::testing::Matcher<const diag_collector::diag &> matcher =
      DIAG_TYPE_OFFSETS(&code, diag_assignment_to_undeclared_variable,  //
                        assignment, 1, u8"ello"_sv);
  EXPECT_TRUE(matcher.Matches(
      diag_collector::diag(diag_assignment_to_undeclared_variable{
          .assignment = identifier(source_code_span(&code[1], &code[5])),
      })));
  EXPECT_FALSE(matcher.Matches(
      diag_collector::diag(diag_assignment_to_undeclared_variable{
          .assignment = identifier(source_code_span(&code[0], &code[5])),
      })));
  EXPECT_FALSE(matcher.Matches(
      diag_collector::diag(diag_assignment_to_undeclared_variable{
          .assignment = identifier(source_code_span(&code[0], &code[4])),
      })));
}

TEST(test_diag_matcher, match_offsets_of_1_field_message) {
  padded_string code(u8"hello"_sv);
  {
    ::testing::Matcher<const diag_collector::diag &> matcher =
        DIAG_TYPE_OFFSETS(&code, diag_invalid_continue,  //
                          continue_statement, 0, u8"hello"_sv);
    diag_collector::diag value(diag_invalid_continue{
        .continue_statement = source_code_span(&code[1], &code[4]),
    });
    EXPECT_EQ(get_matcher_message(matcher, value),
              "whose .continue_statement (1-4) doesn't equal 0-5");
  }

  {
    ::testing::Matcher<const diag_collector::diag &> matcher =
        DIAG_TYPE_OFFSETS(&code, diag_invalid_break,  //
                          break_statement, 0, u8"hello"_sv);
    diag_collector::diag value(diag_invalid_break{
        .break_statement = source_code_span(&code[1], &code[4]),
    });
    EXPECT_EQ(get_matcher_message(matcher, value),
              "whose .break_statement (1-4) doesn't equal 0-5");
  }
}

TEST(test_diag_matcher, match_offsets_of_2_fields_span) {
  padded_string code(u8"...x,"_sv);

  ::testing::Matcher<const diag_collector::diag &> matcher =
      DIAG_TYPE_2_OFFSETS(&code,
                          diag_comma_not_allowed_after_spread_parameter,  //
                          comma, strlen(u8"...x"), u8","_sv, spread, 0,
                          u8"..."_sv);
  EXPECT_TRUE(matcher.Matches(
      diag_collector::diag(diag_comma_not_allowed_after_spread_parameter{
          .comma = source_code_span(&code[4], &code[5]),
          .spread = source_code_span(&code[0], &code[3]),
      })));
  EXPECT_FALSE(matcher.Matches(
      diag_collector::diag(diag_comma_not_allowed_after_spread_parameter{
          .comma = source_code_span(&code[3], &code[5]),
          .spread = source_code_span(&code[0], &code[3]),
      })))
      << "when first doesn't match";
  EXPECT_FALSE(matcher.Matches(
      diag_collector::diag(diag_comma_not_allowed_after_spread_parameter{
          .comma = source_code_span(&code[4], &code[5]),
          .spread = source_code_span(&code[1], &code[3]),
      })))
      << "when second doesn't match";
}

TEST(test_diag_matcher, match_offsets_of_2_fields_message) {
  padded_string code(u8"...x,"_sv);

  // Two wrong fields:
  {
    ::testing::Matcher<const diag_collector::diag &> matcher =
        DIAG_TYPE_2_OFFSETS(&code,
                            diag_comma_not_allowed_after_spread_parameter,  //
                            comma, strlen(u8"...x"), u8","_sv, spread, 0,
                            u8"..."_sv);
    diag_collector::diag value(diag_comma_not_allowed_after_spread_parameter{
        .comma = source_code_span(&code[3], &code[5]),
        .spread = source_code_span(&code[1], &code[3]),
    });
    EXPECT_EQ(get_matcher_message(matcher, value),
              "whose .comma (3-5) doesn't equal 4-5 and whose .spread (1-3) "
              "doesn't equal 0-3");
  }

  // Only first field is wrong:
  {
    ::testing::Matcher<const diag_collector::diag &> matcher =
        DIAG_TYPE_2_OFFSETS(&code,
                            diag_comma_not_allowed_after_spread_parameter,  //
                            comma, strlen(u8"...x"), u8","_sv, spread, 0,
                            u8"..."_sv);
    diag_collector::diag value(diag_comma_not_allowed_after_spread_parameter{
        .comma = source_code_span(&code[3], &code[5]),
        .spread = source_code_span(&code[0], &code[3]),
    });
    EXPECT_EQ(get_matcher_message(matcher, value),
              "whose .comma (3-5) doesn't equal 4-5 and whose .spread (0-3) "
              "equals 0-3");
  }

  // Only second field is wrong:
  {
    ::testing::Matcher<const diag_collector::diag &> matcher =
        DIAG_TYPE_2_OFFSETS(&code,
                            diag_comma_not_allowed_after_spread_parameter,  //
                            comma, strlen(u8"...x"), u8","_sv, spread, 0,
                            u8"..."_sv);
    diag_collector::diag value(diag_comma_not_allowed_after_spread_parameter{
        .comma = source_code_span(&code[4], &code[5]),
        .spread = source_code_span(&code[1], &code[3]),
    });
    EXPECT_EQ(get_matcher_message(matcher, value),
              "whose .comma (4-5) equals 4-5 and whose .spread (1-3) doesn't "
              "equal 0-3");
  }
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
