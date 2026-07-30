#include "cami/diagnostic.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string_view>
#include <vector>

namespace cami {
namespace {

constexpr std::size_t tabWidth = 4;

std::string_view severityName(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::error:
        return "error";
    case DiagnosticSeverity::warning:
        return "warning";
    case DiagnosticSeverity::note:
        return "note";
    case DiagnosticSeverity::help:
        return "help";
    }

    return "error";
}

const SourceFile* findSource(std::span<const SourceFile> sources, SourceId id) {
    const auto source =
        std::find_if(sources.begin(), sources.end(),
                     [id](const SourceFile& candidate) { return candidate.id() == id; });
    return source == sources.end() ? nullptr : &*source;
}

std::size_t scalarByteWidth(std::string_view text, std::size_t offset) {
    std::size_t width = 1;
    while (offset + width < text.size() &&
           (static_cast<unsigned char>(text[offset + width]) & 0xc0U) == 0x80U) {
        ++width;
    }
    return width;
}

std::size_t displayWidth(std::string_view text, std::size_t byteEnd) {
    std::size_t byteOffset = 0;
    std::size_t width = 0;

    while (byteOffset < byteEnd) {
        if (text[byteOffset] == '\t') {
            width += tabWidth - (width % tabWidth);
            ++byteOffset;
            continue;
        }

        byteOffset += scalarByteWidth(text, byteOffset);
        ++width;
    }

    return width;
}

std::string expandTabs(std::string_view text) {
    std::string expanded;
    std::size_t width = 0;

    for (std::size_t byteOffset = 0; byteOffset < text.size();) {
        if (text[byteOffset] == '\t') {
            const std::size_t spaces = tabWidth - (width % tabWidth);
            expanded.append(spaces, ' ');
            width += spaces;
            ++byteOffset;
            continue;
        }

        const std::size_t scalarBytes = scalarByteWidth(text, byteOffset);
        expanded.append(text.substr(byteOffset, scalarBytes));
        byteOffset += scalarBytes;
        ++width;
    }

    return expanded;
}

std::size_t decimalWidth(std::size_t value) {
    std::size_t width = 1;
    while (value >= 10) {
        value /= 10;
        ++width;
    }
    return width;
}

} // namespace

std::string renderDiagnostic(const Diagnostic& diagnostic, std::span<const SourceFile> sources) {
    std::ostringstream output;
    output << severityName(diagnostic.severity) << '[' << diagnostic.code
           << "]: " << diagnostic.message << '\n';

    const DiagnosticLabel* primary = nullptr;
    for (const DiagnosticLabel& label : diagnostic.labels) {
        if (label.primary) {
            primary = &label;
            break;
        }
    }

    if (primary != nullptr) {
        if (const SourceFile* source = findSource(sources, primary->span.source)) {
            output << "  --> " << source->path() << ':' << primary->span.start.line << ':'
                   << primary->span.start.column << '\n';
        }
    }

    std::vector<DiagnosticLabel> labels = diagnostic.labels;
    std::stable_sort(labels.begin(), labels.end(), [](const auto& left, const auto& right) {
        if (left.span.source != right.span.source) {
            return left.span.source.value < right.span.source.value;
        }
        return left.span.start.byteOffset < right.span.start.byteOffset;
    });

    std::size_t gutterWidth = 1;
    for (const DiagnosticLabel& label : labels) {
        gutterWidth = std::max(gutterWidth, decimalWidth(label.span.start.line));
    }

    if (!labels.empty()) {
        output << std::string(gutterWidth + 3, ' ') << "|\n";
    }

    std::size_t previousLine = 0;
    SourceId previousSource{0};
    bool hasPrevious = false;

    for (const DiagnosticLabel& label : labels) {
        const SourceFile* source = findSource(sources, label.span.source);
        if (source == nullptr) {
            continue;
        }

        const std::optional<std::string_view> line = source->lineText(label.span.start.line);
        const std::optional<std::size_t> lineStart = source->lineStartOffset(label.span.start.line);
        if (!line || !lineStart) {
            continue;
        }

        if (hasPrevious &&
            (previousSource != label.span.source || label.span.start.line > previousLine + 1)) {
            output << "...\n";
        }

        output << std::string(gutterWidth - decimalWidth(label.span.start.line), ' ')
               << label.span.start.line << " | " << expandTabs(*line) << '\n';

        const std::size_t relativeStart = label.span.start.byteOffset - *lineStart;
        const std::size_t relativeEnd =
            label.span.end.line == label.span.start.line
                ? std::min(label.span.end.byteOffset - *lineStart, line->size())
                : line->size();
        const std::size_t markerStart = displayWidth(*line, relativeStart);
        const std::size_t markerEnd = displayWidth(*line, relativeEnd);
        const std::size_t markerWidth = std::max<std::size_t>(1, markerEnd - markerStart);
        const char marker = label.primary ? '^' : '-';

        output << std::string(gutterWidth + 3, ' ') << "| " << std::string(markerStart, ' ')
               << std::string(markerWidth, marker);
        if (!label.message.empty()) {
            output << ' ' << label.message;
        }
        output << '\n';

        previousLine = label.span.start.line;
        previousSource = label.span.source;
        hasPrevious = true;
    }

    if (!labels.empty()) {
        output << std::string(gutterWidth + 3, ' ') << "|\n";
    }

    for (const std::string& note : diagnostic.notes) {
        output << "note: " << note << '\n';
    }
    for (const DiagnosticSuggestion& suggestion : diagnostic.suggestions) {
        output << "help: " << suggestion.message << '\n';
    }

    return output.str();
}

} // namespace cami
