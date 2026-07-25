#include "cami/cli.hpp"

#include "cami/version.hpp"

#include <ostream>

namespace cami {
namespace {

constexpr std::string_view helpText = R"(Usage: cami <command>

Commands:
  version     Print compiler and format versions
  help        Print this help

Options:
  -h, --help     Print this help
  -V, --version  Print compiler and format versions
)";

// keep enum to int conv in one place; repeating casts everywhere would look ugly as fuck lmao
int exitCode(ExitCode code) { return static_cast<int>(code); }

void printVersion(std::ostream& output) {
    output << "cami compiler " CAMI_COMPILER_VER "\n"
           << "cami language " CAMI_LANG_VER "\n"
           << "package manifest " CAMI_MANIFEST_VER "\n"
           << "LLVM " CAMI_LLVM_VER "\n";
}

} // namespace

int runCli(std::span<const std::string_view> arguments, std::ostream& output, std::ostream& error) {
    if (arguments.empty()) {
        output << helpText;
        return exitCode(ExitCode::success);
    }

    const std::string_view command = arguments.front();
    const bool requestsHelp = command == "help" || command == "-h" || command == "--help";
    const bool requestsVersion = command == "version" || command == "-V" || command == "--version";

    if (arguments.size() > 1 && (requestsHelp || requestsVersion)) {
        error << "error: unexpected argument '" << arguments[1] << "'\n";
        return exitCode(ExitCode::user_error);
    }

    if (requestsHelp) {
        output << helpText;
        return exitCode(ExitCode::success);
    }

    if (requestsVersion) {
        printVersion(output);
        return exitCode(ExitCode::success);
    }

    error << "error: unknown command '" << command << "'\n"
          << "run 'cami -h' for usage\n";
    return exitCode(ExitCode::user_error);
}
} // namespace cami
