#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  ifndef __WATCOMC__
// Visual C++ 2005 incorrectly displays a warning about the use of POSIX APIs
// on Windows, which is supposed to be POSIX compliant...
#    define chdir _chdir
#    define putenv _putenv
#  endif // !__WATCOMC__
#elif defined __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h> // for chdir()
#include <stdio.h>
#include <stdlib.h> // for system()
#include <string.h>
#else
#  include <unistd.h>
#endif
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/x.H>

#include <blkid/blkid.h>
#include "public.h"


/* The form description */
Fl_Double_Window *window;
Fl_File_Chooser *fileChooser;
Fl_Input *inputWim;
Fl_Choice *choiceBoot;
Fl_Choice *choiceInstall;
Fl_Choice *choiceEdition;
Fl_Choice *choiceLanguage;
const char *deviceNames[10];
int deviceNumber = 0;
const char *languageNames[100] = {
	"zh-CN",
	"zh-HK",
	"zh-TW",
	"en-US"
};


int iterateChoiceDevices()
{
	blkid_dev dev;
	blkid_cache cache;
	blkid_dev_iterate iter;
	const char *devname = NULL;
	if (blkid_get_cache(&cache, NULL))
	{
		return 1;
	}
	blkid_probe_all(cache);
	iter = blkid_dev_iterate_begin(cache);
	while (!blkid_dev_next(iter, &dev))
	{
		devname = blkid_dev_devname(dev);
		if (strncmp(devname, "/dev/sd", 7) == 0)
		{
			deviceNames[deviceNumber] = devname + 5;
			deviceNumber++;
		}
	}
	blkid_dev_iterate_end(iter);
	for (int i = 0; i < deviceNumber; i++)
	{
		if (choiceBoot)
		{
			choiceBoot->add(deviceNames[i]);
		}
		if (choiceInstall)
		{
			choiceInstall->add(deviceNames[i]);
		}
	}
	return 0;
}

void updateDeviceBox(Fl_Widget *w, void *v)
{
	Fl_Choice *choice = (Fl_Choice *)w;
	if (v != NULL)
	{
		Fl_Box *box = (Fl_Box *)v;
		blkid_cache cache;
		if (blkid_get_cache(&cache, NULL))
		{
			return;
		}
		char deviceName[PATH_MAX];
		sprintf(deviceName, "/dev/%s", deviceNames[choice->value()]);
		char *type = blkid_get_tag_value(cache, "TYPE", deviceName);
		char *label = blkid_get_tag_value(cache, "LABEL", deviceName);
		char *uuid = blkid_get_tag_value(cache, "UUID", deviceName);
		char info[1024];
		sprintf(info, "Type: %s, Label: %s, UUID: %s",
			type ? type : "N/A",
			label ? label : "N/A",
			uuid ? uuid : "N/A");
		box->copy_label(info);
	}
}

void showFileChooser(void)
{
	fileChooser->show();
	while (fileChooser->visible()) {
		Fl::wait();
	}
	int count = fileChooser->count();
	if (count > 0)
	{
		inputWim->value("");
		choiceEdition->clear();
		for (int i = 1; i <= count; i++)
		{
			if (fileChooser->value(i) != NULL)
			{
				inputWim->value(fileChooser->value(i));
				choiceEdition->add("Windows 7");
			}
		}
	}
}

void doSetup(Fl_Widget *, void *)
{
	return;

	const char ntfs3g[] = "/usr/local/bin/ntfs-3g";
	char mountCommand[PATH_MAX];
	char deviceBoot[PATH_MAX];
	sprintf(deviceBoot, "/dev/%s", deviceNames[choiceBoot->value()]);
	char mountBoot[PATH_MAX];
	sprintf(mountBoot, "/mnt/%s", deviceNames[choiceBoot->value()]);
	char deviceInstall[PATH_MAX];
	sprintf(deviceInstall, "/dev/%s", deviceNames[choiceInstall->value()]);
	char mountInstall[PATH_MAX];
	sprintf(mountInstall, "/mnt/%s", deviceNames[choiceInstall->value()]);
	int ret = 0;
	if (strcmp(mountBoot, mountInstall) != 0)
	{
		MakeDirs(mountBoot);
		sprintf(mountCommand, "%s %s %s", ntfs3g, deviceBoot, mountBoot);
		printf("Command: %s\n", mountCommand);
		ret = system(mountCommand);
		printf("Return: %d\n", ret);
	}
	if (ret == 0)
	{
		MakeDirs(mountInstall);
		sprintf(mountCommand, "%s %s %s", ntfs3g, deviceInstall, mountInstall);
		printf("Command: %s\n", mountCommand);
		ret = system(mountCommand);
		printf("Return: %d\n", ret);
	}
	if (ret == 0)
	{
		printf("Function: BootFileFix");
		ret = BootFileFix(mountBoot, mountInstall);
		printf("Return: %d\n", ret);
	}
	if (ret == 0)
	{
		printf("Function: BcdFileFix");
		const char *locale = languageNames[choiceLanguage->value()];
		ret = BcdFileFix(mountBoot, deviceInstall, locale);
		printf("Return: %d\n", ret);
	}
	if (strcmp(mountBoot, mountInstall) != 0)
	{
		sprintf(mountCommand, "umount %s", mountBoot);
		printf("Command: %s\n", mountCommand);
		system(mountCommand);
	}
	if (1)
	{
		sprintf(mountCommand, "umount %s", mountInstall);
		printf("Command: %s\n", mountCommand);
		system(mountCommand);
	}
	fl_alert("Done");
}

void doBCDWindow(Fl_Widget *, void *)
{
}

void doExit(Fl_Widget *, void *)
{
	exit(0);
}

int main(int argc, char **argv)
{
	int i, j;
	char deviceName[5];
	Fl_Widget *obj = NULL;
	const size_t MAX_LABEL = 256;
	wchar_t* wstr = NULL;

	window = new Fl_Double_Window(600, 450);

	obj = new Fl_Box(FL_FRAME_BOX, 10, 15, 580, 100, "Select location of Windows installation files");
	obj->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);
	fileChooser = new Fl_File_Chooser(".", "*", Fl_File_Chooser::SINGLE, "Select install.wim");
	fileChooser->preview(0);
	inputWim = new Fl_Input(20, 80, 490, 25, 0);
	Fl_Button *buttonFile = new Fl_Button(515, 80, 65, 25, "Search...");
	buttonFile->callback((Fl_Callback *)showFileChooser);

	obj = new Fl_Box(FL_FRAME_BOX, 10, 125, 580, 100, "Select location of the Boot drive");
	obj->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);
	choiceBoot = new Fl_Choice(70, 190, 80, 25, "Device:");
	Fl_Box *boxBoot = new Fl_Box(FL_NO_BOX, 70, 160, 500, 25, NULL);
	boxBoot->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	choiceBoot->callback((Fl_Callback *)updateDeviceBox, boxBoot);
	Fl_Choice *choiceCode = new Fl_Choice(300, 190, 80, 25, "Boot code:");
	choiceCode->add("BIOS");
	choiceCode->add("UEFI");
	choiceCode->add("No");
	choiceCode->value(0);
	choiceLanguage = new Fl_Choice(500, 190, 80, 25, "Language:");
	choiceLanguage->add("zh-CN");
	choiceLanguage->add("zh-HK");
	choiceLanguage->add("zh-TW");
	choiceLanguage->add("en-US");
	choiceLanguage->value(0);

	obj = new Fl_Box(FL_FRAME_BOX, 10, 235, 580, 100, "Select location of the Installation drive");
	obj->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);
	choiceInstall = new Fl_Choice(70, 300, 80, 25, "Device:");
	Fl_Box *boxInstall = new Fl_Box(FL_NO_BOX, 70, 270, 500, 25, NULL);
	boxInstall->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	choiceInstall->callback((Fl_Callback *)updateDeviceBox, boxInstall);

	obj = new Fl_Box(FL_FRAME_BOX, 10, 345, 580, 95, "Options");
	obj->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);
	choiceEdition = new Fl_Choice(70, 365, 200, 25, "Edition:");
	obj = new Fl_Button(450, 405, 60, 25, "Setup");
	obj->callback(doSetup);
	obj = new Fl_Button(510, 365, 70, 25, "BCD Edit");
	obj->callback(doBCDWindow);
	obj = new Fl_Button(520, 405, 60, 25, "Exit");
	obj->callback(doExit);

	iterateChoiceDevices();

	window->end();
	window->show(argc,argv);
	Fl::run();
	return 0;
}
