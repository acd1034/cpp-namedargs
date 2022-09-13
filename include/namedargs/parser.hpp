/// @file fundamental.hpp
#pragma once
#include <algorithm>  // std::find_if
#include <functional> // std::invoke
#include <span>
#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <namedargs/fundamental.hpp>

namespace namedargs {
  enum class TokenKind {
    num,   // Numeric literals
    str,   // String literals
    ident, // Identifiers
    punct, // Punctuators
    eof,   // End-of-file markers
  };

  struct Token {
    TokenKind kind;
    std::string_view sv;
    std::int64_t num; // Used if TokenKind::num
  };

  struct parse_error : std::runtime_error {
    explicit parse_error(const std::string& msg) : std::runtime_error(msg) {}
    explicit parse_error(const char* msg) : std::runtime_error(msg) {}
    parse_error(const parse_error&) noexcept = default;
    ~parse_error() noexcept override = default;
  };

  constexpr bool isspace(char c) {
    return ('\t' <= c and c <= '\r') or c == ' ';
  }

  constexpr bool isdigit(char c) { return '0' <= c and c <= '9'; }

  constexpr bool isident1(char c) {
    return ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or c == '_';
  }

  constexpr bool isident2(char c) { return isident1(c) or isdigit(c); }

  constexpr bool ispunct(char c) {
    return ('!' <= c and c <= '/') or (':' <= c and c <= '@')
           or ('[' <= c and c <= '`') or ('{' <= c and c <= '~');
  }

  template <class Pred>
  constexpr std::size_t find_if_not(std::string_view sv, Pred pred,
                                    std::size_t pos = 0) {
    for (; pos < sv.size(); ++pos)
      if (not std::invoke(pred, sv[pos]))
        return pos;
    return std::string_view::npos;
  }

  constexpr std::pair<std::int64_t, const char*>
  from_chars(const char* first, const char* last, std::int64_t num = 0) {
    return first != last and namedargs::isdigit(*first)
             ? namedargs::from_chars(first + 1, last, 10 * num + (*first - '0'))
             : std::make_pair(num, first);
  }

  constexpr std::pair<std::span<Token>, bool> //
  consume(TokenKind tk, std::span<Token> toks) {
    if (toks.front().kind == tk)
      return {toks.subspan(1), true};
    else
      return {toks, false};
  }

  constexpr std::pair<std::span<Token>, bool> //
  consume_punct(std::string_view punct, std::span<Token> toks) {
    if (toks.front().kind == TokenKind::punct and toks.front().sv == punct)
      return {toks.subspan(1), true};
    else
      return {toks, false};
  }

  constexpr std::span<Token> //
  expect(TokenKind tk, std::span<Token> toks) {
    if (toks.front().kind == tk)
      return toks.subspan(1);
    else
      throw parse_error("unexpected token in namedargs::expect");
  }

  constexpr std::span<Token> //
  expect_punct(std::string_view punct, std::span<Token> toks) {
    if (toks.front().kind == TokenKind::punct and toks.front().sv == punct)
      return toks.subspan(1);
    else
      throw parse_error("unexpected token in namedargs::expect_punct");
  }

  template <class T, class U>
  constexpr auto find(const std::vector<std::pair<T, U>>& v, const T& key) {
    return std::find_if(v.begin(), v.end(),
                        [&key](const auto& x) { return x.first == key; });
  }

  struct ArgParser {
  private:
    using ArgType = std::variant<std::int64_t, std::string_view>;
    std::string_view input_{};
    std::vector<Token> tokens_{};
    std::vector<std::pair<std::string_view, ArgType>> args_{};

  public:
    constexpr explicit ArgParser(std::string_view input)
      : input_(std::move(input)) {}

    // tokenize

    constexpr std::string_view skip_whitespaces(std::string_view sv) {
      const std::size_t pos = find_if_not(sv, isspace, 1);
      return sv.substr(pos);
    }

    constexpr std::string_view tokenize_number(std::string_view sv) {
      const char* start = sv.data();
      const auto [num, ptr] = namedargs::from_chars(start, start + sv.size());
      // TODO: throw parse_error
      const std::size_t size = icast<std::size_t>(ptr - start);
      tokens_.push_back({TokenKind::num, sv.substr(0, size), num});
      return sv.substr(size);
    }

    constexpr std::string_view tokenize_string_literal(std::string_view sv) {
      const std::size_t pos = sv.find_first_of('\'', 1);
      if (pos == std::string_view::npos)
        throw parse_error("unclosed string literal");
      tokens_.push_back({TokenKind::str, sv.substr(1, pos - 1), {}});
      return sv.substr(pos + 1);
    }

    constexpr std::string_view tokenize_identifier(std::string_view sv) {
      const std::size_t pos = find_if_not(sv, isident2, 1);
      tokens_.push_back({TokenKind::ident, sv.substr(0, pos), {}});
      return sv.substr(pos);
    }

    constexpr std::string_view tokenize_punct(std::string_view sv) {
      tokens_.push_back({TokenKind::punct, sv.substr(0, 1), {}});
      return sv.substr(1);
    }

    constexpr std::string_view tokenize() {
      std::string_view sv = input_;
      while (not sv.empty()) {
        // Skip whitespace characters.
        if (namedargs::isspace(sv.front())) {
          sv = skip_whitespaces(sv);
          continue;
        }

        // Numeric literal
        if (namedargs::isdigit(sv.front())) {
          sv = tokenize_number(sv);
          continue;
        }

        // String literal
        if (sv.starts_with('\'')) {
          sv = tokenize_string_literal(sv);
          continue;
        }

        // Identifier
        if (isident1(sv.front())) {
          sv = tokenize_identifier(sv);
          continue;
        }

        // Punctuators
        if (ispunct(sv.front())) {
          sv = tokenize_punct(sv);
          continue;
        }

        throw parse_error("unexpected character");
      }
      tokens_.push_back({TokenKind::eof, sv, {}});
      return sv;
    }

    // parse

    // args = stmt?
    constexpr std::span<Token> parse_args(std::span<Token> toks) {
      if (auto [toks2, consumed] = consume(TokenKind::eof, toks); consumed)
        return toks2;
      toks = parse_stmt(toks);
      toks = expect(TokenKind::eof, toks);
      return toks;
    }

    // stmt = assign ("," assign)*
    constexpr std::span<Token> parse_stmt(std::span<Token> toks) {
      toks = parse_assign(toks);
      for (;;) {
        if (auto [toks2, consumed] = consume_punct(",", toks); consumed)
          toks = parse_assign(toks2);
        else
          return toks;
      }
    }

    // assign = ident "=" primary
    constexpr std::span<Token> parse_assign(std::span<Token> toks) {
      auto [toks2, ident] = parse_ident(toks);
      toks2 = expect_punct("=", toks2);
      auto [toks3, arg] = parse_primary(toks2);
      args_.push_back({std::move(ident), std::move(arg)});
      return toks3;
    }

    constexpr std::pair<std::span<Token>, std::string_view>
    parse_ident(std::span<Token> toks) {
      if (const auto& tok = toks.front(); tok.kind == TokenKind::ident) {
        if (auto it = find(args_, tok.sv); it == args_.end())
          return {toks.subspan(1), tok.sv};
        else
          throw parse_error("argument already exists");
      } else
        throw parse_error("unexpected token; expecting TokenKind::ident");
    }

    // primary = str | num
    constexpr std::pair<std::span<Token>, ArgType>
    parse_primary(std::span<Token> toks) {
      switch (toks.front().kind) {
      case TokenKind::str:
        return {toks.subspan(1), toks.front().sv};
      case TokenKind::num:
        return {toks.subspan(1), toks.front().num};
      default:
        throw parse_error(
          "unexpected token; expecting TokenKind::str or TokenKind::num");
      }
    }

    constexpr std::span<Token> parse() {
      std::span<Token> toks(tokens_);
      toks = parse_args(toks);
      return toks;
    }
  };
} // namespace namedargs
