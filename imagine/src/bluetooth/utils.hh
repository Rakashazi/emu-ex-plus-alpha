#pragma once

#include <bluetooth/BluetoothAdapter.hh>

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
		if (!isxdigit(*str++))
			return -1;

		if (!isxdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}

static int ba2str(const BluetoothAddr *ba, char *str)
{
	return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		ba->b[5], ba->b[4], ba->b[3], ba->b[2], ba->b[1], ba->b[0]);
}

static int str2ba(const char *str, BluetoothAddr *ba)
{
	BluetoothAddr b;
	int i;

	if (bachk(str) < 0) {
		memset(ba, 0, sizeof(*ba));
		return -1;
	}

	for (i = 0; i < 6; i++, str += 3)
		b.b[i] = strtol(str, NULL, 16);

	baswap(ba, &b);

	return 0;
}
