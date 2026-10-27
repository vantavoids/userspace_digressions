#include <cstdlib>
#include <unistd.h>

#include "fs/fstab.hpp"
#include "fs/kernel_fs.hpp"
#include "util/log.hpp"

int main()
{
	setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
	util::log::info("gressions :D");
	fs::mount_kernel_filesystems();
	fs::mount_fstab();
loop:
	pause(); //be nice to mr. cpu
	goto loop;
}
