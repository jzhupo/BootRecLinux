extern int MakeDirs(const char *path);
extern int CopyFile(const char *source, const char *target);

extern int BcdFileFix(const char *mountPoint, const char *deviceName, const char *locale);
extern int BootFileFix(const char *mountBoot, const char *mountWindows);