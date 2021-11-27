#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "public.h"

int Usage()
{
	fprintf(stderr, "Usage: bootrec sda1 zh-CN\n");
	return 1;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		return Usage();
	}
	const char *deviceName = argv[1];
	if ((strlen(deviceName) == 4) &&
		(deviceName[0] == 's') &&
		(deviceName[1] == 'd') &&
		(deviceName[2] >= 'a') && (deviceName[2] <= 'z') &&
		(deviceName[3] >= '1') && (deviceName[3] <= '4'))
	{
	}
	else
	{
		return Usage();
	}
	const char *locale = argv[2];
	const char ntfs3g[] = "/usr/local/bin/ntfs-3g";
	const char ntfs3gMountPoint[] = "/mnt/ntfs";
	char mountCommand[PATH_MAX];
	int ret = 0;
	if (ret == 0)
	{
		MakeDirs(ntfs3gMountPoint);
		sprintf(mountCommand, "%s /dev/%s %s", ntfs3g, deviceName, ntfs3gMountPoint);
		printf("Command: %s\n", mountCommand);
		ret = system(mountCommand);
		printf("Return: %d\n", ret);
	}
	if (ret == 0)
	{
		printf("Function: BootFileFix");
		ret = BootFileFix(ntfs3gMountPoint);
		printf("Return: %d\n", ret);
	}
	if (ret == 0)
	{
		printf("Function: BcdFileFix");
		ret = BcdFileFix(ntfs3gMountPoint, deviceName, locale);
		printf("Return: %d\n", ret);
	}
	sprintf(mountCommand, "umount %s", ntfs3gMountPoint);
	printf("Command: %s\n", mountCommand);
	system(mountCommand);
	return ret;
}

