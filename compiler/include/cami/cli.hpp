#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace cami {

// user mistakes are 1 and compiler features reserve 2
enum class ExitCode : int { success = 0, user_error = 1, compiler_error = 2 };

int runCli(std::span<const std::string_view> arguments, std::ostream& output, std::ostream& error);

} // namespace cami
