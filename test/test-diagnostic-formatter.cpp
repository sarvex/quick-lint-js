// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cstddef>
#include <cstring>
#include <gtest/gtest.h>
#include <quick-lint-js/diag/diagnostic-formatter.h>
#include <quick-lint-js/diag/diagnostic.h>
#include <quick-lint-js/fe/language.h>
#include <quick-lint-js/fe/source-code-span.h>
#include <quick-lint-js/port/char8.h>
#include <string_view>

using namespace std::literals::string_view_literals;

namespace quick_lint_js {
namespace {
source_code_span empty_span(nullptr, nullptr);

class string_diagnostic_formatter
    : public diagnostic_formatter<string_diagnostic_formatter> {
 public:
  explicit string_diagnostic_formatter()
      : diagnostic_formatter<string_diagnostic_formatter>(translator()) {}

  void write_before_message(std::string_view, diagnostic_severity,
                            const source_code_span&) {}

  void write_message_part(std::string_view, diagnostic_severity,
                          string8_view message_part) {
    this->message += message_part;
  }

  void write_after_message(std::string_view, diagnostic_severity,
                           const source_code_span&) {
    this->message += u8'\n';
  }

  string8 message;
};

TEST(test_diagnostic_formatter, origin_span) {
  static constexpr const char8* code = u8"hello world";
  static const source_code_span span(&code[0], &code[5]);

  struct test_diagnostic_formatter
      : public diagnostic_formatter<test_diagnostic_formatter> {
    using diagnostic_formatter<test_diagnostic_formatter>::diagnostic_formatter;

    void write_before_message(std::string_view, diagnostic_severity,
                              const source_code_span& origin_span) {
      EXPECT_TRUE(same_pointers(origin_span, span));
      this->write_before_message_call_count += 1;
    }

    void write_message_part(std::string_view, diagnostic_severity,
                            string8_view) {}

    void write_after_message(std::string_view, diagnostic_severity,
                             const source_code_span& origin_span) {
      EXPECT_TRUE(same_pointers(origin_span, span));
      this->write_after_message_call_count += 1;
    }

    int write_before_message_call_count = 0;
    int write_after_message_call_count = 0;
  };

  translator t;
  t.use_messages_from_source_code();

  test_diagnostic_formatter formatter(t);
  formatter.format_message("E9999"sv, diagnostic_severity::error,
                           QLJS_TRANSLATABLE("something happened"),
                           diagnostic_message_args{{
                               {0, diagnostic_arg_type::source_code_span},
                           }},
                           &span);

  EXPECT_EQ(formatter.write_before_message_call_count, 1);
  EXPECT_EQ(formatter.write_after_message_call_count, 1);
}

TEST(test_diagnostic_formatter, single_span_simple_message) {
  string_diagnostic_formatter formatter;
  formatter.format_message("E9999"sv, diagnostic_severity::error,
                           QLJS_TRANSLATABLE("something happened"),
                           diagnostic_message_args{{
                               {0, diagnostic_arg_type::source_code_span},
                           }},
                           &empty_span);
  EXPECT_EQ(formatter.message, u8"something happened\n");
}

TEST(test_diagnostic_formatter, diagnostic_with_single_message) {
  constexpr diagnostic_info info = {
      .code = 9999,
      .severity = diagnostic_severity::error,
      .message_formats = {QLJS_TRANSLATABLE("something happened")},
      .message_args =
          {
              diagnostic_message_args{{
                  {0, diagnostic_arg_type::source_code_span},
              }},
          },
  };

  string_diagnostic_formatter formatter;
  formatter.format(info, &empty_span);
  EXPECT_EQ(formatter.message, u8"something happened\n");
}

TEST(test_diagnostic_formatter, diagnostic_with_two_messages) {
  constexpr diagnostic_info info = {
      .code = 9999,
      .severity = diagnostic_severity::error,
      .message_formats =
          {
              QLJS_TRANSLATABLE("something happened"),
              QLJS_TRANSLATABLE("see here"),
          },
      .message_args =
          {
              diagnostic_message_args{{
                  {0, diagnostic_arg_type::source_code_span},
              }},
              diagnostic_message_args{
                  {
                      {0, diagnostic_arg_type::source_code_span},
                  },
              },
          },
  };

  string_diagnostic_formatter formatter;
  formatter.format(info, &empty_span);
  EXPECT_EQ(formatter.message,
            u8"something happened\n"
            u8"see here\n");
}

TEST(test_diagnostic_formatter, message_with_zero_placeholder) {
  const char8* code = u8"hello world";
  source_code_span hello_span(&code[0], &code[5]);

  string_diagnostic_formatter formatter;
  formatter.format_message("E9999"sv, diagnostic_severity::error,
                           QLJS_TRANSLATABLE("this {0} looks fishy"),
                           diagnostic_message_args{{
                               {0, diagnostic_arg_type::source_code_span},
                           }},
                           &hello_span);
  EXPECT_EQ(formatter.message, u8"this hello looks fishy\n");
}

TEST(test_diagnostic_formatter, message_with_extra_identifier_placeholder) {
  const char8* code = u8"hello world";
  struct test_diag {
    source_code_span hello;
    identifier world;
  };
  test_diag diag = {
      .hello = source_code_span(&code[0], &code[5]),
      .world = identifier(source_code_span(&code[6], &code[11])),
  };

  string_diagnostic_formatter formatter;
  formatter.format_message(
      "E9999"sv, diagnostic_severity::error,
      QLJS_TRANSLATABLE("this {1} looks fishy"),
      diagnostic_message_args{{
          {offsetof(test_diag, hello), diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, world), diagnostic_arg_type::identifier},
      }},
      &diag);
  EXPECT_EQ(formatter.message, u8"this world looks fishy\n");
}

TEST(test_diagnostic_formatter, message_with_multiple_span_placeholders) {
  const char8* code = u8"let me = be(free);";
  struct test_diag {
    source_code_span let_span;
    source_code_span me_span;
    source_code_span be_span;
  };
  test_diag diag = {
      .let_span = source_code_span(&code[0], &code[3]),
      .me_span = source_code_span(&code[4], &code[6]),
      .be_span = source_code_span(&code[9], &code[11]),
  };
  ASSERT_EQ(diag.let_span.string_view(), u8"let"_sv);
  ASSERT_EQ(diag.me_span.string_view(), u8"me"_sv);
  ASSERT_EQ(diag.be_span.string_view(), u8"be"_sv);

  string_diagnostic_formatter formatter;
  formatter.format_message(
      "E9999"sv, diagnostic_severity::error,
      QLJS_TRANSLATABLE("free {1} and {0} {1} {2}"),
      diagnostic_message_args{{
          {offsetof(test_diag, let_span),
           diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, me_span), diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, be_span), diagnostic_arg_type::source_code_span},
      }},
      &diag);
  EXPECT_EQ(formatter.message, u8"free me and let me be\n");
}

TEST(test_diagnostic_formatter, message_with_char_placeholder) {
  struct test_diag {
    source_code_span span;
    char8 c;
  };
  test_diag diag = {
      .span = empty_span,
      .c = u8'Q',
  };
  string_diagnostic_formatter formatter;
  formatter.format_message(
      "E9999"sv, diagnostic_severity::error,
      QLJS_TRANSLATABLE("what is this '{1}' nonsense?"),
      diagnostic_message_args{{
          {offsetof(test_diag, span), diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, c), diagnostic_arg_type::char8},
      }},
      &diag);
  EXPECT_EQ(formatter.message, u8"what is this 'Q' nonsense?\n");
}

TEST(test_diagnostic_formatter, message_with_escaped_curlies) {
  const char8* code = u8"hello world";
  source_code_span code_span(&code[0], &code[3]);

  string_diagnostic_formatter formatter;
  formatter.format_message("E9999"sv, diagnostic_severity::error,
                           QLJS_TRANSLATABLE("a {{0} b }} c"),
                           diagnostic_message_args{{
                               {0, diagnostic_arg_type::source_code_span},
                           }},
                           &code_span);
  EXPECT_EQ(formatter.message, u8"a {0} b }} c\n");
}

TEST(test_diagnostic_formatter, enum_kind_placeholder) {
  struct test_diag {
    source_code_span empty_span;
    enum_kind kind;
  };
  constexpr diagnostic_message_args message_args = {
      diagnostic_message_args{{
          {offsetof(test_diag, empty_span),
           diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, kind), diagnostic_arg_type::enum_kind},
      }},
  };

  {
    test_diag diag = {
        .empty_span = empty_span,
        .kind = enum_kind::normal,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected enum\n");
  }
}

TEST(test_diagnostic_formatter, statement_kind_placeholder) {
  struct test_diag {
    source_code_span empty_span;
    statement_kind statement;
  };
  constexpr diagnostic_message_args message_args = {
      diagnostic_message_args{{
          {offsetof(test_diag, empty_span),
           diagnostic_arg_type::source_code_span},
          {offsetof(test_diag, statement), diagnostic_arg_type::statement_kind},
      }},
  };

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::do_while_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected 'do-while' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::do_while_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected a 'do-while' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::for_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected 'for' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::for_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected a 'for' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::if_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected 'if' statement\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::if_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected an 'if' statement\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::while_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected 'while' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::while_loop,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected a 'while' loop\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::with_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected 'with' statement\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::with_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected a 'with' statement\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::labelled_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:headlinese}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected labelled statement\n");
  }

  {
    test_diag diag = {
        .empty_span = empty_span,
        .statement = statement_kind::labelled_statement,
    };
    string_diagnostic_formatter formatter;
    formatter.format_message("E9999"sv, diagnostic_severity::error,
                             QLJS_TRANSLATABLE("expected {1:singular}"),
                             message_args, &diag);
    EXPECT_EQ(formatter.message, u8"expected a labelled statement\n");
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
