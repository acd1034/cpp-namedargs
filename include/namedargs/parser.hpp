/// @file fundamental.hpp
#pragma once
#include <algorithm> // std::min, std::find_if, std::sort, std::ranges::lower_bound
#include <functional> // std::invoke
#include <optional>
#include <span>
#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <namedargs/ctype.hpp>
#include <namedargs/from_chars.hpp>
#include <namedargs/fundamental.hpp>

namespace namedargs {
  template <class T>
  struct ArgParserTraits;

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

  template <class Pred>
  constexpr std::size_t //
  find_if_not(std::string_view sv, Pred pred, std::size_t pos = 0) {
    for (; pos < sv.size(); ++pos)
      if (not std::invoke(pred, sv[pos]))
        return pos;
    return std::string_view::npos;
  }

  constexpr std::optional<std::span<Token>> //
  consume(TokenKind kind, std::span<Token> toks) {
    if (toks.front().kind == kind)
      return toks.subspan(1);
    else
      return std::nullopt;
  }

  constexpr std::optional<std::span<Token>> //
  consume_punct(std::string_view punct, std::span<Token> toks) {
    if (toks.front().kind == TokenKind::punct and toks.front().sv == punct)
      return toks.subspan(1);
    else
      return std::nullopt;
  }

  constexpr std::span<Token> //
  expect(TokenKind kind, std::span<Token> toks) {
    if (toks.front().kind != kind)
      throw parse_error("unexpected token");
    return toks.subspan(1);
  }

  constexpr std::span<Token> //
  expect_punct(std::string_view punct, std::span<Token> toks) {
    if (toks.front().kind != TokenKind::punct)
      throw parse_error("unexpected token; expecting TokenKind::punct");
    if (toks.front().sv != punct)
      throw parse_error("unexpected punctuator");
    return toks.subspan(1);
  }

  template <class T, class U>
  constexpr auto find(const std::vector<std::pair<T, U>>& v, const T& key) {
    return std::find_if(v.begin(), v.end(),
                        [&key](const auto& x) { return x.first == key; });
  }

  // Checks if T is assignable from any of variant types
  template <class T, class U>
  inline constexpr bool variant_assignable_from_any_v = false;

  template <class T, class... Types>
  inline constexpr bool
    variant_assignable_from_any_v<T, std::variant<Types...>> =
      (std::assignable_from<T, Types> or ...);

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
      const std::size_t pos =
        std::min(find_if_not(sv, namedargs::isspace, 1), sv.size());
      return sv.substr(pos);
    }

    constexpr std::string_view tokenize_number(std::string_view sv) {
      const char* first = sv.data();
      std::int64_t num{};
      if (auto [ptr, ec] = from_chars(first, first + sv.size(), num);
          ec == std::errc{}) {
        const std::size_t size = icast<std::size_t>(ptr - first);
        tokens_.push_back({TokenKind::num, sv.substr(0, size), num});
        return sv.substr(size);
      } else
        throw parse_error("conversion from chars to integer failed");
    }

    constexpr std::string_view tokenize_string_literal(std::string_view sv) {
      sv = sv.substr(1);
      const std::size_t pos = sv.find_first_of('\'');
      if (pos == std::string_view::npos)
        throw parse_error("unclosed string literal");
      tokens_.push_back({TokenKind::str, sv.substr(0, pos), {}});
      return sv.substr(pos + 1);
    }

    constexpr std::string_view tokenize_identifier(std::string_view sv) {
      const std::size_t pos = std::min(find_if_not(sv, isident2, 1), sv.size());
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
        if (namedargs::ispunct(sv.front())) {
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
      if (auto toks2 = consume(TokenKind::eof, toks))
        return *toks2;
      toks = parse_stmt(toks);
      toks = expect(TokenKind::eof, toks);
      return toks;
    }

    // stmt = assign ("," assign)*
    constexpr std::span<Token> parse_stmt(std::span<Token> toks) {
      toks = parse_assign(toks);
      for (;;) {
        if (auto toks2 = consume_punct(",", toks))
          toks = parse_assign(*toks2);
        else
          return toks;
      }
    }

    // assign = ident "=" primary
    constexpr std::span<Token> parse_assign(std::span<Token> toks) {
      auto [ident, toks2] = parse_ident(toks);
      toks2 = expect_punct("=", toks2);
      auto [arg, toks3] = parse_primary(toks2);
      args_.push_back({std::move(ident), std::move(arg)});
      return toks3;
    }

    constexpr std::pair<std::string_view, std::span<Token>>
    parse_ident(std::span<Token> toks) {
      if (toks.front().kind != TokenKind::ident)
        throw parse_error("unexpected token; expecting TokenKind::ident");
      if (auto it = namedargs::find(args_, toks.front().sv); it != args_.end())
        throw parse_error("argument already exists");
      return {toks.front().sv, toks.subspan(1)};
    }

    // primary = str | num
    constexpr std::pair<ArgType, std::span<Token>>
    parse_primary(std::span<Token> toks) {
      switch (toks.front().kind) {
      case TokenKind::str:
        return {toks.front().sv, toks.subspan(1)};
      case TokenKind::num:
        return {toks.front().num, toks.subspan(1)};
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

    constexpr void execute() {
      tokenize();
      parse();
      std::sort(args_.begin(), args_.end(), [](const auto& x, const auto& y) {
        return x.first.compare(y.first) < 0;
      });
    }

    constexpr std::pair<decltype(args_.cbegin()), bool> //
    find(std::string_view key) const {
      auto it = std::ranges::lower_bound(args_.begin(), args_.end(), key, {},
                                         [](const auto& x) { return x.first; });
      if (it == args_.end() or key < it->first)
        return {args_.end(), false};
      else
        return {it, true};
    }

    template <class T, class U>
    constexpr T& assign_or(T& out, std::string_view key, U&& value) const {
      static_assert(variant_assignable_from_any_v<T&, ArgType>);
      if (auto [it, found] = find(key); found)
        return std::visit(
          [&out](const auto& x) -> T& {
            if constexpr (std::assignable_from<T&, decltype(x)>)
              return out = x;
            else
              throw parse_error("value is not assignable");
          },
          it->second);
      else
        return out = std::forward<U>(value);
    }
  };

  template <class T>
  constexpr auto parse_args(std::string_view sv)
    -> decltype(ArgParserTraits<T>::convert(std::declval<ArgParser>())) {
    ArgParser parser(sv);
    parser.execute();
    return ArgParserTraits<T>::convert(parser);
  }
} // namespace namedargs
