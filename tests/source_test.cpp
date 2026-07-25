#include "cami/source.hpp"

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
        path_ =
            std::filesystem::temp_directory_path() / ("cami-source-test-" + std::to_string(nonce));
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
    std::cerr << "source test failed: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

void writeBytes(const std::filesystem::path& path, std::string_view bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    require(output.good(), "fixture should be writable");
}

void requireLocation(const cami::SourceFile& source, std::size_t byteOffset,
                     cami::SourceLocation expected) {
    const std::optional<cami::SourceLocation> actual = source.locationAt(byteOffset);
    if (!actual) {
        fail("location should exist");
    }
    require(*actual == expected, "location should have exact byte, line and scalar column");
}

void testLocationsAndNormalization() {
    TempDirectory temp;
    const std::filesystem::path packageRoot = temp.path() / "package";
    const std::filesystem::path sourcePath = packageRoot / "src" / "main.cami";
    const std::string bytes = "first\r\n\tcaf\xc3\xa9 \xf0\x9f\x92\x9c\nlast";
    writeBytes(sourcePath, bytes);

    const cami::SourceFile source = cami::SourceFile::load(
        cami::SourceId{42}, packageRoot / "src" / ".." / "src" / "main.cami", packageRoot);

    require(source.id() == cami::SourceId{42}, "source ID should survive loading");
    require(source.path() == "src/main.cami", "path should be normalized and package-relative");
    require(source.bytes() == bytes, "source bytes should remain untouched");
    require(source.lineCount() == 3, "fixture should contain three lines");

    requireLocation(source, 0, {0, 1, 1});
    requireLocation(source, 5, {5, 1, 6});
    requireLocation(source, 6, {6, 1, 6});
    requireLocation(source, 7, {7, 2, 1});
    requireLocation(source, 8, {8, 2, 2});
    requireLocation(source, 11, {11, 2, 5});
    requireLocation(source, 13, {13, 2, 6});
    requireLocation(source, 14, {14, 2, 7});
    requireLocation(source, 18, {18, 2, 8});
    requireLocation(source, 19, {19, 3, 1});
    requireLocation(source, 23, {23, 3, 5});

    require(!source.locationAt(12), "a byte inside a scalar is not a source location");
    require(!source.locationAt(15), "a byte inside a four-byte scalar is not a source location");
    require(!source.locationAt(24), "a byte past EOF is not a source location");

    const std::optional<cami::SourceSpan> span = source.span(11, 18);
    if (!span) {
        fail("valid scalar boundaries should produce a span");
    }
    require(*span == cami::SourceSpan{{42}, {11, 2, 5}, {18, 2, 8}},
            "span should retain source identity and exact half-open positions");
    require(!source.span(18, 11), "a backwards span should fail");
    require(!source.span(12, 18), "a span cannot start inside a scalar");
}

void testInvalidUtf8() {
    TempDirectory temp;
    const std::filesystem::path packageRoot = temp.path() / "package";
    const std::filesystem::path sourcePath = packageRoot / "invalid.cami";
    const std::string bytes{"ok\n\xf0\x28\x8c\xbc", 7};
    writeBytes(sourcePath, bytes);

    try {
        static_cast<void>(cami::SourceFile::load({1}, sourcePath, packageRoot));
        require(false, "invalid UTF-8 should fail");
    } catch (const cami::SourceLoadError& error) {
        require(error.kind() == cami::SourceLoadErrorKind::invalidUtf8,
                "invalid UTF-8 should have its own error kind");
        require(error.byteOffset() == 3, "invalid UTF-8 should report the sequence start");
    }
}

void testPathErrors() {
    TempDirectory temp;
    const std::filesystem::path packageRoot = temp.path() / "package";
    const std::filesystem::path outsidePath = temp.path() / "outside.cami";
    std::filesystem::create_directories(packageRoot);
    writeBytes(outsidePath, "void main() {}\n");

    try {
        static_cast<void>(cami::SourceFile::load({1}, outsidePath, packageRoot));
        require(false, "outside source should fail");
    } catch (const cami::SourceLoadError& error) {
        require(error.kind() == cami::SourceLoadErrorKind::outsidePackage,
                "outside source should keep its error kind");
    }

    try {
        static_cast<void>(cami::SourceFile::load({2}, packageRoot / "missing.cami", packageRoot));
        require(false, "missing source should fail");
    } catch (const cami::SourceLoadError& error) {
        require(error.kind() == cami::SourceLoadErrorKind::io,
                "missing source should be an I/O error");
    }
}

} // namespace

int main() {
    testLocationsAndNormalization();
    testInvalidUtf8();
    testPathErrors();
    return EXIT_SUCCESS;
}
