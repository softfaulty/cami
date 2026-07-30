#include "cami/diagnostic.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace {

class TempDirectory final {
public:
    TempDirectory() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("cami-diagnostic-test-" + std::to_string(nonce));
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

[[noreturn]] void fail(std::string_view message, std::string_view actual = {}) {
    std::cerr << "diagnostic test failed: " << message << '\n';
    if (!actual.empty()) {
        std::cerr << actual;
    }
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

void testRenderedDiagnostic() {
    TempDirectory temp;
    const std::filesystem::path packageRoot = temp.path() / "package";
    const std::filesystem::path sourcePath = packageRoot / "src" / "main.cami";
    writeSource(sourcePath, "\tconsume(record);\n\trecord.write();\n");

    const cami::SourceFile source = cami::SourceFile::load({7}, sourcePath, packageRoot);
    const std::optional<cami::SourceSpan> moved = source.span(9, 15);
    const std::optional<cami::SourceSpan> used = source.span(19, 25);
    if (!moved || !used) {
        fail("fixture spans should be valid");
    }

    const cami::Diagnostic diagnostic{
        .severity = cami::DiagnosticSeverity::error,
        .code = "E0204",
        .message = "use of moved value `record`",
        .labels =
            {
                {.span = *used, .message = "used here after move", .primary = true},
                {.span = *moved, .message = "value moved here", .primary = false},
            },
        .notes = {"class handles move when passed by value"},
        .suggestions =
            {
                {
                    .applicability = cami::SuggestionApplicability::maybeIncorrect,
                    .message = "pass `&record` if `consume` only needs to borrow it",
                    .edits = {{.span = *moved, .replacement = "&record"}},
                },
            },
    };

    const std::array<cami::SourceFile, 1> sources{source};
    const std::string rendered = cami::renderDiagnostic(diagnostic, sources);
    const std::string expected = "error[E0204]: use of moved value `record`\n"
                                 "  --> src/main.cami:2:2\n"
                                 "    |\n"
                                 "1 |     consume(record);\n"
                                 "    |             ------ value moved here\n"
                                 "2 |     record.write();\n"
                                 "    |     ^^^^^^ used here after move\n"
                                 "    |\n"
                                 "note: class handles move when passed by value\n"
                                 "help: pass `&record` if `consume` only needs to borrow it\n";

    if (rendered != expected) {
        fail("rendered diagnostic should match the snapshot", rendered);
    }

    require(diagnostic.suggestions.front().edits.front().replacement == "&record",
            "structured replacement should remain available after rendering");
}

} // namespace

int main() {
    testRenderedDiagnostic();
    return EXIT_SUCCESS;
}
