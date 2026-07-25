#include "cami/cli.hpp"

#include <exception>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argumentCount, char** argumentValues) try {
    std::vector<std::string_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argumentCount > 0 ? argumentCount - 1 : 0));

    for (int i = 1; i < argumentCount; ++i) {
        arguments.emplace_back(argumentValues[i]);
    }

    return cami::runCli(std::span<const std::string_view>(arguments), std::cout, std::cerr);
} catch (const std::exception& exception) {
    // leave a stable compiler error instead of platform-specific crash junk
    std::cerr << "internal compiler error: " << exception.what() << '\n';
    return static_cast<int>(cami::ExitCode::compiler_error);
}
