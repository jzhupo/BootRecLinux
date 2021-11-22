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
	char ch;
	int ret = -1;
	FILE *fr = fopen(source, "r");
	FILE *fw = fopen(target, "w");
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
	char *p = NULL;
	while (entry = readdir(dp))
	{
		if (ret != 0)
		{
			break;
		}
		if (entry->d_type = DT_REG)
		{
			char *srcFile = malloc(strlen(source) + strlen(entry->d_name) + 2);
			char *tgtFile = malloc(strlen(target) + strlen(entry->d_name) + 2);
			if ((srcFile != NULL) && (tgtFile != NULL))
			{
				sprintf(srcFile, "%s/%s", source, entry->d_name);
				sprintf(tgtFile, "%s/%s", target, entry->d_name);
				ret = CopyFile(srcFile, tgtFile);
				printf("Copy %s to %s (%s)\n", srcFile, tgtFile, (ret == 0) ? "successfully" : "failed");
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

