﻿#include <lambda/parse_ast.h>

#include <ublib/macros.h>
#include <ublib/utility.h>

#include <cassert>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>

using namespace ublib::prelude;

namespace lambda {

std::ostream& operator<<(std::ostream& os, Parse_ast const& ast) noexcept {
  return match(ast)(
      RLAM(Parse_ast::Variable const& v) { return os << v.name(); },
      RLAM(Parse_ast::Call const& v) {
        return os << v.callee() << ' ' << v.argument();
      },
      RLAM(Parse_ast::Lambda const& v) {
        return os << '(' << '/' << v.parameter() << '.' << v.expression()
                  << ')';
      });
}

Parse_ast parse_from(std::istream& inp) {
  // modified from my tapl-re reason project
  constexpr static auto eof = std::char_traits<char>::eof();

  struct helper {
    std::istream& inp;

    void eat_whitespace() {
      while (std::isspace(inp.peek())) {
        inp.get();
      }
    }

    void unexpected_thing() {
      if (inp.peek() == eof) {
        throw Parse_error("unexpected end of file");
      } else {
        throw Parse_error("unexpected character");
      }
    }

    std::string get_var() {
      eat_whitespace();
      auto ch = inp.get();
      if (not(std::isalpha(ch) or ch == '_')) {
        throw Parse_error("expected a variable");
      }

      std::string buffer;
      buffer.push_back(static_cast<char>(ch));
      for (;;) {
        ch = inp.peek();
        if (std::isalnum(ch) or ch == '_') {
          inp.get();
          buffer.push_back(static_cast<char>(ch));
        } else {
          break;
        }
      }
      return std::move(buffer);
    }

    void get_dot() {
      eat_whitespace();
      if (inp.peek() == '.') {
        inp.get();
      } else {
        unexpected_thing();
      }
    }

    void get_close_paren() {
      eat_whitespace();
      if (inp.peek() == ')') {
        inp.get();
      } else {
        unexpected_thing();
      }
    }

    Parse_ast parse_app_list(Parse_ast fst) {
      eat_whitespace();
      auto ch = inp.peek();

      switch (ch) {
      case ')':
      case eof:
        return fst;
      case '(': {
        return parse_app_list(Parse_ast::Call(std::move(fst), parse_term()));
      }
      case '/':
      case '\\':
        throw Parse_error("attempted to define a lambda in a callee");
      default:
        if (std::isalpha(ch) or ch == '_') {
          return parse_app_list(
              Parse_ast::Call(std::move(fst), Parse_ast::Variable(get_var())));
        } else {
          unexpected_thing();
        }
      }
    }

    std::optional<Parse_ast> maybe_parse_term() {
      eat_whitespace();
      auto ch = inp.peek();
      switch (ch) {
      case eof:
      case ')':
        return std::nullopt;
      case '/':
      case '\\': {
        inp.get();
        auto name = get_var();
        get_dot();
        return Parse_ast::Lambda(std::move(name), parse_term());
      }
      case '(': {
        inp.get();
        auto fst = parse_term();
        get_close_paren();
        return parse_app_list(std::move(fst));
      }
      default:
        if (std::isalpha(ch) or ch == '_') {
          return parse_app_list(Parse_ast::Variable(get_var()));
        } else {
          unexpected_thing();
        }
      }
    }

    Parse_ast parse_term() {
      if (auto tm = maybe_parse_term()) {
        return std::move(*tm);
      } else {
        unexpected_thing();
      }
    }
  };
  return helper{inp}.parse_term();
}

std::ostream& operator<<(std::ostream& os, Parse_error const& e) {
  return os << "Parse error: " << e.what();
}

} // namespace lambda
