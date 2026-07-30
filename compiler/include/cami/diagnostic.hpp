#pragma once

#include "cami/source.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cami {

enum class DiagnosticSeverity : std::uint8_t {
    error,
    warning,
    note,
    help,
};

enum class SuggestionApplicability : std::uint8_t {
    machineApplicable,
    maybeIncorrect,
    informational,
};

struct DiagnosticLabel {
    SourceSpan span;
    std::string message;
    bool primary;
};

struct DiagnosticEdit {
    SourceSpan span;
    std::string replacement;
};

struct DiagnosticSuggestion {
    SuggestionApplicability applicability;
    std::string message;
    std::vector<DiagnosticEdit> edits;
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string code;
    std::string message;
    std::vector<DiagnosticLabel> labels;
    std::vector<std::string> notes;
    std::vector<DiagnosticSuggestion> suggestions;
};

[[nodiscard]] std::string renderDiagnostic(const Diagnostic& diagnostic,
                                           std::span<const SourceFile> sources);

} // namespace cami
