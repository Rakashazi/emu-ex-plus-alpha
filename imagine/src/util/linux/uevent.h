#pragma once

#include <logger/interface.h>
#include <util/fd-utils.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

static int openUeventFd()
{
	struct sockaddr_nl addr;
	mem_zero(addr);
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = 0xffffffff;

	/*
	*	netlink(7) on nl_pid:
 	*	If the application sets it to 0, the kernel takes care of assigning it.
	*	The  kernel assigns the process ID to the first netlink socket the process
	*	opens and assigns a unique nl_pid to every netlink socket that the
	*	process subsequently creates.
	*/
	addr.nl_pid = getpid();

	auto s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if(s < 0)
	{
		logErr("uevent socket failed: %s", strerror(errno));
		return -1;
	}

	if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		logErr("uevent bind failed: %s", strerror(errno));
		close(s);
		return -1;
	}

	fd_setNonblock(s, 1);

	return s;
}
