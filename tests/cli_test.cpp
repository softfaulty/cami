#include "cami/cli.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

struct CliResult {
    int exitCode;
    std::string output;
    std::string error;
};

template <std::size_t size> CliResult run(const std::array<std::string_view, size>& arguments) {
    std::ostringstream output;
    std::ostringstream error;
    const int exitCode = cami::runCli(arguments, output, error);
    return {exitCode, output.str(), error.str()};
}

void require(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    std::cerr << "cli test failed: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void testHelp() {
    const CliResult result = run(std::array<std::string_view, 1>{"--help"});

    require(result.exitCode == 0, "help should succeed");
    require(result.output.starts_with("Usage: cami <command>"), "help should include usage");
    require(result.error.empty(), "help should not write to stderr");
}

void testVersion() {
    const CliResult result = run(std::array<std::string_view, 1>{"version"});

    require(result.exitCode == 0, "version should succeed");
    require(result.output.find("cami compiler ") != std::string::npos,
            "version should include compiler version");
    require(result.output.find("cami language 0.1") != std::string::npos,
            "version should include language version");
    require(result.output.find("package manifest 1") != std::string::npos,
            "version should include manifest version");
    require(result.output.find("LLVM ") != std::string::npos,
            "version should include LLVM version");
    require(result.error.empty(), "version should not write to stderr");
}

void testUnknownCommand() {
    const CliResult result = run(std::array<std::string_view, 1>{"summon"});

    require(result.exitCode == 1, "unknown commands should use the user-error exit code");
    require(result.output.empty(), "unknown commands should not write to stdout");
    require(result.error.find("unknown command 'summon'") != std::string::npos,
            "error should name the command");
}

void testUnexpectedArgument() {
    const CliResult result = run(std::array<std::string_view, 2>{"version", "extra"});

    require(result.exitCode == 1, "unexpected arguments should use the user-error exit code");
    require(result.output.empty(), "unexpected arguments should not write to stdout");
    require(result.error.find("unexpected argument 'extra'") != std::string::npos,
            "error should name the argument");
}

} // namespace

int main() {
    testHelp();
    testVersion();
    testUnknownCommand();
    testUnexpectedArgument();
    return EXIT_SUCCESS;
}
