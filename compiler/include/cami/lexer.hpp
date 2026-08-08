#pragma once

#include "cami/diagnostic.hpp"
#include "cami/source.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace cami {

enum class TokenKind : std::uint8_t {
    identifier,
    keyword,
    punctuation,
    endOfFile,
};

enum class TriviaKind : std::uint8_t {
    whitespace,
    lineComment,
    blockComment,
    docLineComment,
    docBlockComment
};

struct Trivia {
    TriviaKind kind;
    SourceSpan span;
};

struct Token {
    TokenKind kind;
    SourceSpan span;
    std::vector<Trivia> leadingTrivia;
};

struct LexResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;
};

[[nodiscard]] LexResult lex(const SourceFile& source);
[[nodiscard]] std::string_view tokenKindName(TokenKind kind);
[[nodiscard]] std::string_view triviaKindName(TriviaKind kind);

} // namespace cami
