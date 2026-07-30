#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cami {

struct SourceId {
    std::uint32_t value;

    bool operator==(const SourceId&) const = default;
};

struct SourceLocation {
    std::size_t byteOffset;
    std::size_t line;
    std::size_t column;

    bool operator==(const SourceLocation&) const = default;
};

struct SourceSpan {
    SourceId source;
    SourceLocation start;
    SourceLocation end;

    bool operator==(const SourceSpan&) const = default;
};

enum class SourceLoadErrorKind : std::uint8_t {
    io,
    outsidePackage,
    invalidUtf8,
};

class SourceLoadError final : public std::runtime_error {
public:
    SourceLoadError(SourceLoadErrorKind kind, std::filesystem::path path,
                    std::optional<std::size_t> byteOffset, const std::string& message);

    [[nodiscard]] SourceLoadErrorKind kind() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] std::optional<std::size_t> byteOffset() const noexcept;

private:
    SourceLoadErrorKind kind_;
    std::filesystem::path path_;
    std::optional<std::size_t> byteOffset_;
};

class SourceFile final {
public:
    static SourceFile load(SourceId id, const std::filesystem::path& path,
                           const std::filesystem::path& packageRoot);
    [[nodiscard]] SourceId id() const noexcept;
    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] std::string_view bytes() const noexcept;
    [[nodiscard]] std::size_t lineCount() const noexcept;
    [[nodiscard]] std::optional<std::size_t> lineStartOffset(std::size_t line) const noexcept;
    [[nodiscard]] std::optional<std::string_view> lineText(std::size_t line) const noexcept;
    [[nodiscard]] std::optional<SourceLocation> locationAt(std::size_t byteOffset) const;
    [[nodiscard]] std::optional<SourceSpan> span(std::size_t start, std::size_t end) const;

private:
    SourceFile(SourceId id, std::string normalizedPath, std::string bytes,
               std::vector<std::size_t> lineStarts);

    SourceId id_;
    std::string path_;
    std::string bytes_;
    std::vector<std::size_t> lineStarts_;
};
} // namespace cami
