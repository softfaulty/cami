#include "cami/source.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace cami {
namespace {

bool isContinuation(unsigned char byte) { return (byte & 0xc0U) == 0x80U; }

std::size_t scalarWidth(std::string_view bytes, std::size_t offset) {
    const auto byte = static_cast<unsigned char>(bytes[offset]);
    const std::size_t remaining = bytes.size() - offset;

    if (byte <= 0x7fU) {
        return 1;
    }

    if (byte >= 0xc2U && byte <= 0xdfU && remaining >= 2) {
        return isContinuation(static_cast<unsigned char>(bytes[offset + 1])) ? 2 : 0;
    }

    if (byte >= 0xe0U && byte <= 0xefU && remaining >= 3) {
        const auto second = static_cast<unsigned char>(bytes[offset + 1]);
        const auto third = static_cast<unsigned char>(bytes[offset + 2]);
        const bool validSecond =
            (byte == 0xe0U && second >= 0xa0U && second <= 0xbfU) ||
            (byte == 0xedU && second >= 0x80U && second <= 0x9fU) ||
            (((byte >= 0xe1U && byte <= 0xecU) || (byte >= 0xeeU && byte <= 0xefU)) &&
             isContinuation(second));

        return validSecond && isContinuation(third) ? 3 : 0;
    }

    if (byte >= 0xf0U && byte <= 0xf4U && remaining >= 4) {
        const auto second = static_cast<unsigned char>(bytes[offset + 1]);
        const auto third = static_cast<unsigned char>(bytes[offset + 2]);
        const auto fourth = static_cast<unsigned char>(bytes[offset + 3]);
        const bool validSecond = (byte == 0xf0U && second >= 0x90U && second <= 0xbfU) ||
                                 (byte == 0xf4U && second >= 0x80U && second <= 0x8fU) ||
                                 (byte >= 0xf1U && byte <= 0xf3U && isContinuation(second));

        return validSecond && isContinuation(third) && isContinuation(fourth) ? 4 : 0;
    }

    return 0;
}

std::optional<std::size_t> invalidUtf8Offset(std::string_view bytes) {
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const std::size_t width = scalarWidth(bytes, offset);

        if (width == 0) {
            // report the sequence start, pointing into its wreckage would be fake precision
            return offset;
        }

        offset += width;
    }

    return std::nullopt;
}

std::vector<std::size_t> findLineStarts(std::string_view bytes) {
    std::vector<std::size_t> starts{0};

    for (std::size_t offset = 0; offset < bytes.size(); ++offset) {
        if (bytes[offset] == '\n') {
            starts.push_back(offset + 1);
        }
    }

    return starts;
}

std::string readBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);

    if (!input) {
        throw SourceLoadError(SourceLoadErrorKind::io, path, std::nullopt,
                              "could not open source files: " + path.string());
    }

    std::string bytes{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };

    if (input.bad()) {
        throw SourceLoadError(SourceLoadErrorKind::io, path, std::nullopt,
                              "could not read source file: " + path.string());
    }

    return bytes;
}

std::string utf8Path(const std::filesystem::path& path) {
    const std::u8string encoded = path.generic_u8string();

    return {
        reinterpret_cast<const char*>(encoded.data()),
        encoded.size(),
    };
}

bool leavesPackage(const std::filesystem::path& relative) {
    return relative.empty() || relative.is_absolute() ||
           (!relative.empty() && *relative.begin() == "..");
}

} // namespace

SourceLoadError::SourceLoadError(SourceLoadErrorKind kind, std::filesystem::path path,
                                 std::optional<std::size_t> byteOffset, const std::string& message)
    : std::runtime_error(message), kind_(kind), path_(std::move(path)), byteOffset_(byteOffset) {}

SourceLoadErrorKind SourceLoadError::kind() const noexcept { return kind_; }

const std::filesystem::path& SourceLoadError::path() const noexcept { return path_; }

std::optional<std::size_t> SourceLoadError::byteOffset() const noexcept { return byteOffset_; }

SourceFile SourceFile::load(SourceId id, const std::filesystem::path& path,
                            const std::filesystem::path& packageRoot) {
    std::error_code error;

    const std::filesystem::path normalizedRoot =
        std::filesystem::weakly_canonical(packageRoot, error);

    if (error) {
        throw SourceLoadError(SourceLoadErrorKind::io, packageRoot, std::nullopt,
                              "could not resolve package root: " + packageRoot.string());
    }

    const std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path, error);

    if (error) {
        throw SourceLoadError(SourceLoadErrorKind::io, path, std::nullopt,
                              "could not resolve source path: " + path.string());
    }

    // canonicalize first so a symlink cant cosplay as package local source
    const std::filesystem::path relativePath = normalizedPath.lexically_relative(normalizedRoot);

    if (leavesPackage(relativePath)) {
        throw SourceLoadError(SourceLoadErrorKind::outsidePackage, path, std::nullopt,
                              "source file is outside package root: " + path.string());
    }

    std::string bytes = readBytes(normalizedPath);
    const std::optional<std::size_t> invalidOffset = invalidUtf8Offset(bytes);

    if (invalidOffset) {
        throw SourceLoadError(SourceLoadErrorKind::invalidUtf8, path, invalidOffset,
                              "invalid UTF-8 at byte " + std::to_string(*invalidOffset) + " in " +
                                  path.string());
    }

    std::vector<std::size_t> lineStarts = findLineStarts(bytes);
    return SourceFile(id, utf8Path(relativePath), std::move(bytes), std::move(lineStarts));
}

SourceFile::SourceFile(SourceId id, std::string normalizedPath, std::string bytes,
                       std::vector<std::size_t> lineStarts)
    : id_(id), path_(std::move(normalizedPath)), bytes_(std::move(bytes)),
      lineStarts_(std::move(lineStarts)) {}

SourceId SourceFile::id() const noexcept { return id_; }

std::string_view SourceFile::path() const noexcept { return path_; }

std::string_view SourceFile::bytes() const noexcept { return bytes_; }

std::size_t SourceFile::lineCount() const noexcept { return lineStarts_.size(); }

std::optional<SourceLocation> SourceFile::locationAt(std::size_t byteOffset) const {
    if (byteOffset > bytes_.size()) {
        return std::nullopt;
    }

    if (byteOffset < bytes_.size() &&
        isContinuation(static_cast<unsigned char>(bytes_[byteOffset]))) {
        return std::nullopt;
    }

    const auto nextLine = std::upper_bound(lineStarts_.begin(), lineStarts_.end(), byteOffset);
    const std::size_t lineIndex =
        static_cast<std::size_t>(std::distance(lineStarts_.begin(), nextLine) - 1);
    const std::size_t lineStart = lineStarts_[lineIndex];

    std::size_t column = 1;
    std::size_t offset = lineStart;
    while (offset < byteOffset) {
        if (bytes_[offset] == '\r' && offset + 1 < bytes_.size() && bytes_[offset + 1] == '\n') {
            ++offset;
            continue;
        }

        offset += scalarWidth(bytes_, offset);
        ++column;
    }

    return SourceLocation{byteOffset, lineIndex + 1, column};
}

std::optional<SourceSpan> SourceFile::span(std::size_t start, std::size_t end) const {
    if (start > end) {
        return std::nullopt;
    }

    const std::optional<SourceLocation> startLocation = locationAt(start);
    const std::optional<SourceLocation> endLocation = locationAt(end);
    if (!startLocation || !endLocation) {
        return std::nullopt;
    }

    return SourceSpan{id_, *startLocation, *endLocation};
}

} // namespace cami
