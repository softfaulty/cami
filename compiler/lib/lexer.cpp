#include "cami/lexer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace cami {
namespace {

constexpr std::array keywords{
    std::string_view{"as"},       std::string_view{"bool"},    std::string_view{"break"},
    std::string_view{"byte"},     std::string_view{"char"},    std::string_view{"class"},
    std::string_view{"comptime"}, std::string_view{"const"},   std::string_view{"continue"},
    std::string_view{"defer"},    std::string_view{"do"},      std::string_view{"else"},
    std::string_view{"enum"},     std::string_view{"extern"},  std::string_view{"f32"},
    std::string_view{"f64"},      std::string_view{"false"},   std::string_view{"fn"},
    std::string_view{"for"},      std::string_view{"i8"},      std::string_view{"i16"},
    std::string_view{"i32"},      std::string_view{"i64"},     std::string_view{"i128"},
    std::string_view{"if"},       std::string_view{"impl"},    std::string_view{"import"},
    std::string_view{"in"},       std::string_view{"int"},     std::string_view{"isize"},
    std::string_view{"let"},      std::string_view{"match"},   std::string_view{"module"},
    std::string_view{"none"},     std::string_view{"null"},    std::string_view{"override"},
    std::string_view{"package"},  std::string_view{"private"}, std::string_view{"protected"},
    std::string_view{"public"},   std::string_view{"return"},  std::string_view{"step"},
    std::string_view{"struct"},   std::string_view{"switch"},  std::string_view{"trait"},
    std::string_view{"true"},     std::string_view{"u8"},      std::string_view{"u16"},
    std::string_view{"u32"},      std::string_view{"u64"},     std::string_view{"u128"},
    std::string_view{"uint"},     std::string_view{"union"},   std::string_view{"unsafe"},
    std::string_view{"usize"},    std::string_view{"virtual"}, std::string_view{"void"},
    std::string_view{"where"},    std::string_view{"while"},   std::string_view{"with"},
};

// longest first is the lexical rule in a form the loop cannot misunderstand
constexpr std::array punctuation{
    std::string_view{"..."}, std::string_view{"..="}, std::string_view{"<<="},
    std::string_view{">>="}, std::string_view{".."},  std::string_view{"->"},
    std::string_view{"::"},  std::string_view{"=>"},  std::string_view{"++"},
    std::string_view{"--"},  std::string_view{"&&"},  std::string_view{"||"},
    std::string_view{"+="},  std::string_view{"-="},  std::string_view{"*="},
    std::string_view{"/="},  std::string_view{"%="},  std::string_view{"&="},
    std::string_view{"|="},  std::string_view{"^="},  std::string_view{"=="},
    std::string_view{"!="},  std::string_view{"<="},  std::string_view{">="},
    std::string_view{"<<"},  std::string_view{">>"},  std::string_view{"("},
    std::string_view{")"},   std::string_view{"{"},   std::string_view{"}"},
    std::string_view{"["},   std::string_view{"]"},   std::string_view{","},
    std::string_view{";"},   std::string_view{":"},   std::string_view{"."},
    std::string_view{"?"},   std::string_view{"@"},   std::string_view{"+"},
    std::string_view{"-"},   std::string_view{"*"},   std::string_view{"/"},
    std::string_view{"%"},   std::string_view{"&"},   std::string_view{"|"},
    std::string_view{"^"},   std::string_view{"~"},   std::string_view{"!"},
    std::string_view{"="},   std::string_view{"<"},   std::string_view{">"},
};

bool isIdentifierStart(char character) {
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
           character == '_';
}

bool isIdentifierContinue(char character) {
    return isIdentifierStart(character) || (character >= '0' && character <= '9');
}

bool isKeyword(std::string_view spelling) {
    return std::find(keywords.begin(), keywords.end(), spelling) != keywords.end();
}

std::size_t scalarByteWidth(std::string_view text, std::size_t offset) {
    std::size_t width = 1;
    while (offset + width < text.size() &&
           (static_cast<unsigned char>(text[offset + width]) & 0xc0U) == 0x80U) {
        ++width;
    }
    return width;
}

SourceSpan makeSpan(const SourceFile& source, std::size_t start, std::size_t end) {
    const std::optional<SourceSpan> span = source.span(start, end);
    if (!span) {
        throw std::logic_error("lexer produced a span outside validated source");
    }
    return *span;
}

Diagnostic error(std::string code, std::string message, SourceSpan span, std::string labelMessage) {
    return Diagnostic{
        .severity = DiagnosticSeverity::error,
        .code = std::move(code),
        .message = std::move(message),
        .labels = {{.span = span, .message = std::move(labelMessage), .primary = true}},
        .notes = {},
        .suggestions = {},
    };
}

bool startsWith(std::string_view bytes, std::size_t offset, std::string_view spelling) {
    return bytes.substr(offset).starts_with(spelling);
}

} // namespace

LexResult lex(const SourceFile& source) {
    const std::string_view bytes = source.bytes();
    LexResult result;
    std::vector<Trivia> leadingTrivia;
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const std::size_t start = offset;

        if (bytes[offset] == ' ' || bytes[offset] == '\t' || bytes[offset] == '\n' ||
            (bytes[offset] == '\r' && offset + 1 < bytes.size() && bytes[offset + 1] == '\n')) {
            while (offset < bytes.size()) {
                if (bytes[offset] == ' ' || bytes[offset] == '\t' || bytes[offset] == '\n') {
                    ++offset;
                    continue;
                }
                if (bytes[offset] == '\r' && offset + 1 < bytes.size() &&
                    bytes[offset + 1] == '\n') {
                    offset += 2;
                    continue;
                }
                break;
            }
            leadingTrivia.push_back({TriviaKind::whitespace, makeSpan(source, start, offset)});
            continue;
        }

        if (bytes[offset] == '\r') {
            ++offset;
            const SourceSpan span = makeSpan(source, start, offset);
            Diagnostic diagnostic = error("E0002", "lone carriage return in source", span,
                                          "use LF or a complete CRLF line ending");
            diagnostic.suggestions.push_back({
                .applicability = SuggestionApplicability::machineApplicable,
                .message = "replace this carriage return with a line feed",
                .edits = {{.span = span, .replacement = "\n"}},
            });
            result.diagnostics.push_back(std::move(diagnostic));
            continue;
        }

        if (startsWith(bytes, offset, "//")) {
            const TriviaKind kind = startsWith(bytes, offset, "///") ? TriviaKind::docLineComment
                                                                     : TriviaKind::lineComment;
            offset += kind == TriviaKind::docLineComment ? 3 : 2;
            while (offset < bytes.size() && bytes[offset] != '\r' && bytes[offset] != '\n') {
                offset += scalarByteWidth(bytes, offset);
            }
            leadingTrivia.push_back({kind, makeSpan(source, start, offset)});
            continue;
        }

        if (startsWith(bytes, offset, "/*")) {
            const TriviaKind kind = startsWith(bytes, offset, "/**") ? TriviaKind::docBlockComment
                                                                     : TriviaKind::blockComment;
            const std::size_t terminator = bytes.find("*/", offset + 2);
            const std::size_t commentEnd =
                terminator == std::string_view::npos ? bytes.size() : terminator + 2;

            for (std::size_t byte = offset + 2; byte < commentEnd; ++byte) {
                if (bytes[byte] == '\r' && (byte + 1 == bytes.size() || bytes[byte + 1] != '\n')) {
                    const SourceSpan span = makeSpan(source, byte, byte + 1);
                    result.diagnostics.push_back(error("E0002", "lone carriage return in source",
                                                       span,
                                                       "use LF or a complete CRLF line ending"));
                }
            }

            if (terminator == std::string_view::npos) {
                offset = bytes.size();
                const SourceSpan span = makeSpan(source, start, offset);
                leadingTrivia.push_back({kind, span});
                result.diagnostics.push_back(error("E0003", "unterminated block comment", span,
                                                   "this comment never reaches `*/`"));
                continue;
            }

            offset = terminator + 2;
            leadingTrivia.push_back({kind, makeSpan(source, start, offset)});
            continue;
        }

        if (isIdentifierStart(bytes[offset])) {
            ++offset;
            while (offset < bytes.size() && isIdentifierContinue(bytes[offset])) {
                ++offset;
            }

            const std::string_view spelling = bytes.substr(start, offset - start);
            result.tokens.push_back({
                .kind = isKeyword(spelling) ? TokenKind::keyword : TokenKind::identifier,
                .span = makeSpan(source, start, offset),
                .leadingTrivia = std::move(leadingTrivia),
            });
            leadingTrivia.clear();
            continue;
        }

        const auto matched =
            std::find_if(punctuation.begin(), punctuation.end(), [&](std::string_view spelling) {
                return startsWith(bytes, offset, spelling);
            });
        if (matched != punctuation.end()) {
            offset += matched->size();
            result.tokens.push_back({
                .kind = TokenKind::punctuation,
                .span = makeSpan(source, start, offset),
                .leadingTrivia = std::move(leadingTrivia),
            });
            leadingTrivia.clear();
            continue;
        }

        offset += scalarByteWidth(bytes, offset);
        const SourceSpan span = makeSpan(source, start, offset);
        result.diagnostics.push_back(
            error("E0001", "invalid source character", span,
                  "this character is not valid outside a literal or comment"));
    }

    result.tokens.push_back({
        .kind = TokenKind::endOfFile,
        .span = makeSpan(source, bytes.size(), bytes.size()),
        .leadingTrivia = std::move(leadingTrivia),
    });
    return result;
}

std::string_view tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::identifier:
        return "identifier";
    case TokenKind::keyword:
        return "keyword";
    case TokenKind::punctuation:
        return "punctuation";
    case TokenKind::endOfFile:
        return "eof";
    }

    return "unknown";
}

std::string_view triviaKindName(TriviaKind kind) {
    switch (kind) {
    case TriviaKind::whitespace:
        return "whitespace";
    case TriviaKind::lineComment:
        return "line-comment";
    case TriviaKind::blockComment:
        return "block-comment";
    case TriviaKind::docLineComment:
        return "doc-line-comment";
    case TriviaKind::docBlockComment:
        return "doc-block-comment";
    }

    return "unknown";
}

} // namespace cami
