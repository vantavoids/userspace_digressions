#include <unistd.h>

#include "fs/kernel_fs.hpp"
#include "util/log.hpp"

int main()
{
	util::log::info("gressions :D");
	fs::mount_kernel_filesystems();
loop:
	pause(); //be nice to mr. cpu
	goto loop;
}
