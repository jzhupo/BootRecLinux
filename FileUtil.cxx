#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int MakeDirs(const char *path)
{
	const size_t len = strlen(path);
	char _path[PATH_MAX];
	char *p = NULL;
	int ret = 0;
	errno = 0;
	if (len > sizeof(_path) - 1)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(_path, path);
	if (_path[len - 1] == '/')
	{
		_path[len - 1] = '\0';
	}
	for (p = _path + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = '\0';
			ret = mkdir(_path, S_IRWXU);
			if (ret != 0)
			{
				if (errno != EEXIST)
				{
					return ret;
				}
			}
			*p = '/';
		}
	}
	ret = mkdir(_path, S_IRWXU);
	if (errno != EEXIST)
	{
		return ret;
	}
	return 0;
}

int CopyFile(const char *source, const char *target)
{
	int ch;
	int ret = -1;
	FILE *fr = fopen(source, "rb");
	FILE *fw = fopen(target, "wb");
	if ((fr != NULL) && (fw != NULL))
	{
		while ((ch = fgetc(fr)) != EOF)
		{
			fputc(ch, fw);
		}
		ret = 0;
	}
	if (fr != NULL)
	{
		fclose(fr);
	}
	if (fw != NULL)
	{
		fclose(fw);
	}
	printf("Copy %s to %s (%s)\n", source, target, (ret == 0) ? "successfully" : "failed");
	return ret;
}

int CopyDirs(const char *source, const char *target)
{
	if ((source == NULL) || (target == NULL))
	{
		return -1;
	}
	DIR *dp = opendir(source);
	if (dp == NULL)
	{
		return -1;
	}
	int ret = MakeDirs(target);
	struct dirent *entry;
	while (entry = readdir(dp))
	{
		if (ret != 0)
		{
			break;
		}
		if ((entry->d_type == DT_REG) && (entry->d_name[0] != '.'))
		{
			char *srcFile = (char *)malloc(strlen(source) + strlen(entry->d_name) + 2);
			char *tgtFile = (char *)malloc(strlen(target) + strlen(entry->d_name) + 2);
			if ((srcFile != NULL) && (tgtFile != NULL))
			{
				sprintf(srcFile, "%s/%s", source, entry->d_name);
				sprintf(tgtFile, "%s/%s", target, entry->d_name);
				ret = CopyFile(srcFile, tgtFile);
				if (srcFile != NULL)
				{
					free(srcFile);
				}
				if (tgtFile != NULL)
				{
					free(tgtFile);
				}
				if (ret != 0)
				{
					break;
				}
			}
		}
	}
	if (dp != NULL)
	{
		closedir(dp);
	}
	return ret;
}

int BootFileFix(const char *mountBoot, const char *mountWindows)
{
	int ret = 0;
	DIR *dp = NULL;
	char source[PATH_MAX];
	char target[PATH_MAX];
	sprintf(source, "%s/Windows/Boot/PCAT", mountWindows);
	dp = opendir(source);
	if (dp == NULL)
	{
		return -1;
	}
	struct dirent *entry;
	while (entry = readdir(dp))
	{
		if (ret != 0)
		{
			break;
		}
		if ((entry->d_type == DT_DIR) && (entry->d_name[0] != '.'))
		{
			char *srcFile = (char *)malloc(strlen(source) + strlen(entry->d_name) + 2);
			char *tgtFile = (char *)malloc(strlen(mountBoot) + strlen(entry->d_name) + 7);
			if ((srcFile != NULL) && (tgtFile != NULL))
			{
				sprintf(srcFile, "%s/%s", source, entry->d_name);
				sprintf(tgtFile, "%s/Boot/%s", mountBoot, entry->d_name);
				ret = CopyDirs(srcFile, tgtFile);
				if (srcFile != NULL)
				{
					free(srcFile);
				}
				if (tgtFile != NULL)
				{
					free(tgtFile);
				}
				if (ret != 0)
				{
					break;
				}
			}
		}
	}
	if (dp != NULL)
	{
		closedir(dp);
	}
	if (ret == 0)
	{
		sprintf(source, "%s/Windows/Boot/Fonts", mountWindows);
		sprintf(target, "%s/Boot/Fonts", mountBoot);
		ret = CopyDirs(source, target);
	}
	if (ret == 0)
	{
		sprintf(source, "%s/Windows/Boot/Resources", mountWindows);
		sprintf(target, "%s/Boot/Resources", mountBoot);
		// optional
		CopyDirs(source, target);
	}
	if (ret == 0)
	{
		sprintf(source, "%s/Windows/Boot/PCAT/memtest.exe", mountWindows);
		sprintf(target, "%s/Boot/memtest.exe", mountBoot);
		ret = CopyFile(source, target);
	}
	if (ret == 0)
	{
		sprintf(source, "%s/Windows/Boot/PCAT/bootmgr", mountWindows);
		sprintf(target, "%s/bootmgr", mountBoot);
		ret = CopyFile(source, target);
	}
	return ret;
}
