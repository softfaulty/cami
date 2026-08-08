#include "cami/lexer.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

class TempDirectory final {
public:
    TempDirectory() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ =
            std::filesystem::temp_directory_path() / ("cami-lexer-test-" + std::to_string(nonce));
        std::filesystem::create_directories(path_);
    }

    ~TempDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

[[noreturn]] void fail(std::string_view message) {
    std::cerr << "lexer test failed: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

void writeSource(const std::filesystem::path& path, std::string_view bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    require(output.good(), "fixture should be writable");
}

cami::SourceFile loadSource(const TempDirectory& temp, std::string_view bytes) {
    const std::filesystem::path packageRoot = temp.path() / "package";
    const std::filesystem::path sourcePath = packageRoot / "src" / "main.cami";
    writeSource(sourcePath, bytes);
    return cami::SourceFile::load({9}, sourcePath, packageRoot);
}

std::string_view spelling(const cami::SourceFile& source, const cami::SourceSpan& span) {
    return source.bytes().substr(span.start.byteOffset,
                                 span.end.byteOffset - span.start.byteOffset);
}

void testEveryKeyword() {
    constexpr std::array expected{
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

    std::string text;
    for (std::string_view keyword : expected) {
        if (!text.empty()) {
            text.push_back(' ');
        }
        text.append(keyword);
    }

    TempDirectory temp;
    const cami::SourceFile source = loadSource(temp, text);
    const cami::LexResult result = cami::lex(source);

    require(result.diagnostics.empty(), "the complete keyword list should lex");
    require(result.tokens.size() == expected.size() + 1, "every keyword should produce one token");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        require(result.tokens[index].kind == cami::TokenKind::keyword,
                "reserved words should use the keyword kind");
        require(spelling(source, result.tokens[index].span) == expected[index],
                "keyword spelling should survive tokenization");
    }
}

void testIdentifiersAndTrivia() {
    TempDirectory temp;
    const cami::SourceFile source = loadSource(temp, "alpha _item2 core some ok err \t// normal\r\n"
                                                     "/// docs\n/* block */ /** docs */ if");
    const cami::LexResult result = cami::lex(source);

    require(result.diagnostics.empty(), "identifier and trivia fixture should lex");
    require(result.tokens.size() == 8, "seven written tokens plus EOF should remain");
    for (std::size_t index = 0; index < 7; ++index) {
        const cami::TokenKind expected =
            index == 6 ? cami::TokenKind::keyword : cami::TokenKind::identifier;
        require(result.tokens[index].kind == expected, "identifier classification should be exact");
    }

    const std::vector<cami::Trivia>& trivia = result.tokens[6].leadingTrivia;
    const std::array expectedKinds{
        cami::TriviaKind::whitespace, cami::TriviaKind::lineComment,
        cami::TriviaKind::whitespace, cami::TriviaKind::docLineComment,
        cami::TriviaKind::whitespace, cami::TriviaKind::blockComment,
        cami::TriviaKind::whitespace, cami::TriviaKind::docBlockComment,
        cami::TriviaKind::whitespace,
    };
    require(trivia.size() == expectedKinds.size(), "all formatter trivia should remain in order");
    for (std::size_t index = 0; index < expectedKinds.size(); ++index) {
        require(trivia[index].kind == expectedKinds[index],
                "trivia kinds should match the snapshot");
    }
    require(spelling(source, trivia[1].span) == "// normal", "line comment bytes should survive");
    require(spelling(source, trivia[3].span) == "/// docs", "doc line bytes should survive");
    require(spelling(source, trivia[5].span) == "/* block */", "block bytes should survive");
    require(spelling(source, trivia[7].span) == "/** docs */", "doc block bytes should survive");

    TempDirectory emptyDocTemp;
    const cami::SourceFile emptyDocSource = loadSource(emptyDocTemp, "/**/if");
    const cami::LexResult emptyDocResult = cami::lex(emptyDocSource);
    require(emptyDocResult.diagnostics.empty() &&
                emptyDocResult.tokens.front().leadingTrivia.front().kind ==
                    cami::TriviaKind::docBlockComment,
            "`/**/` should be an empty documentation block comment");
}

void testLongestPunctuation() {
    constexpr std::array expected{
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

    std::string text;
    for (std::string_view item : expected) {
        if (!text.empty()) {
            text.push_back(' ');
        }
        text.append(item);
    }

    TempDirectory temp;
    const cami::SourceFile source = loadSource(temp, text);
    const cami::LexResult result = cami::lex(source);

    require(result.diagnostics.empty(), "the complete punctuation set should lex");
    require(result.tokens.size() == expected.size() + 1,
            "longest match should keep each fixture spelling whole");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        require(result.tokens[index].kind == cami::TokenKind::punctuation,
                "operator spelling should use punctuation kind");
        require(spelling(source, result.tokens[index].span) == expected[index],
                "longest match should preserve the expected operator");
    }
}

void testLexicalErrors() {
    TempDirectory temp;
    const cami::SourceFile source = loadSource(temp, "caf\xc3\xa9\r/* never closed");
    const cami::LexResult result = cami::lex(source);

    require(result.diagnostics.size() == 3,
            "invalid scalar, CR and block comment should each report");
    require(result.diagnostics[0].code == "E0001", "non-ASCII identifier text should be invalid");
    require(result.diagnostics[1].code == "E0002", "lone CR should have a stable diagnostic");
    require(result.diagnostics[2].code == "E0003",
            "unterminated comment should have a stable code");
    require(result.tokens.back().leadingTrivia.size() == 1,
            "unterminated comment should remain formatter trivia");

    TempDirectory commentTemp;
    const cami::SourceFile commentSource = loadSource(commentTemp, "/*\r*/");
    const cami::LexResult commentResult = cami::lex(commentSource);
    require(commentResult.diagnostics.size() == 1 &&
                commentResult.diagnostics.front().code == "E0002",
            "a block comment should not hide a lone carriage return");
}

} // namespace

int main() {
    testEveryKeyword();
    testIdentifiersAndTrivia();
    testLongestPunctuation();
    testLexicalErrors();
    return EXIT_SUCCESS;
}
