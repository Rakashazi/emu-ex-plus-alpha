#pragma once

#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ctype.hh>

namespace IG
{

static void baswap(BluetoothAddr *dst, const BluetoothAddr *src)
{
	unsigned char *d = (unsigned char *) dst;
	const unsigned char *s = (const unsigned char *) src;
	int i;

	for (i = 0; i < 6; i++)
		d[i] = s[5-i];
}

static int bachk(const char *str)
{
	if (!str)
		return -1;

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isXdigit(*str++))
			return -1;

		if (!isXdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}

static BluetoothAddrString ba2str(BluetoothAddr ba)
{
	return formatArray<sizeof(BluetoothAddrString)>("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
		ba.data()[5], ba.data()[4], ba.data()[3], ba.data()[2], ba.data()[1], ba.data()[0]);
}

static int str2ba(const char *str, BluetoothAddr *ba)
{
	BluetoothAddr b;
	int i;

	if (bachk(str) < 0) {
		*ba = {};
		return -1;
	}

	for (i = 0; i < 6; i++, str += 3)
		b.data()[i] = strtol(str, NULL, 16);

	baswap(ba, &b);

	return 0;
}

}
