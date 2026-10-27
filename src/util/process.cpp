#include "util/process.hpp"

#include <cerrno>
#include <expected>
#include <spawn.h>
#include <cstdlib>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>

std::expected<int, std::error_code> util::process::run(const std::vector<std::string> &argv)
{
	std::vector<char *> c_argv;
	c_argv.reserve(argv.size() + 1);
	for (const auto &arg : argv)
		c_argv.push_back(const_cast<char *>(arg.c_str()));
	c_argv.push_back(nullptr);

	pid_t pid{};
	if (int err = posix_spawnp(&pid, c_argv[0], nullptr, nullptr, c_argv.data(), environ); err != 0)
		return std::unexpected(std::error_code(err, std::system_category()));

	int status{};
	while (waitpid(pid, &status, 0) < 0)
		if (errno != EINTR)
			return std::unexpected(std::error_code(errno, std::system_category()));

	if (WIFSIGNALED(status))
		return 128 + WTERMSIG(status);
	return WEXITSTATUS(status);
}
