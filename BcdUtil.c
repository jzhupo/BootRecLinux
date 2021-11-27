#include <stdio.h>
#include "public.h"

#define C_ASSERT(expr)
#define DEVICE_TYPE ULONG

#include "reactos/sdk/include/host/typedefs.h"
#define DECLSPEC_SELECTANY
#include "reactos/sdk/include/psdk/initguid.h"

typedef enum _BL_DEVICE_TYPE
{
	DiskDevice = 0,
	LegacyPartitionDevice = 2,
	SerialDevice = 3,
	UdpDevice = 4,
	BootDevice = 5,
	PartitionDevice = 6,
	LocateDevice = 8,
} BL_DEVICE_TYPE;

typedef struct _BL_DEVICE_DESCRIPTOR
{
	ULONG DeviceType;
	ULONG Flags;
	ULONG Size;
	ULONG Unknown;
	union
	{
		struct
		{
			ULONG Type;
		} Local;
		struct
		{
			ULONGLONG Offset;
			ULONG Reserve1;
			ULONG Reserve2;
			ULONG Reserve3;
			ULONG DiskNumber;
			ULONG Signature;
			ULONG Reserve4;
			ULONG Reserve5;
			ULONG Reserve6;
			ULONG Reserve7;
			ULONG Reserve8;
			ULONG Reserve9;
			ULONG Reserve103;
		} Mbr;
		struct
		{
			GUID PartitionSignature;
			ULONG Reserve1;
			ULONG Reserve2;
			GUID DiskSignature;
		} Gpt;
	};
} BL_DEVICE_DESCRIPTOR, *PBL_DEVICE_DESCRIPTOR;

typedef struct _EFI_PARTITION_HEADER
{
	ULONGLONG Signature;		 // 0
	ULONG Revision;				 // 8
	ULONG HeaderSize;			 // 12
	ULONG HeaderCRC32;			 // 16
	ULONG Reserved;				 // 20
	ULONGLONG MyLBA;			 // 24
	ULONGLONG AlternateLBA;		 // 32
	ULONGLONG FirstUsableLBA;	 // 40
	ULONGLONG LastUsableLBA;	 // 48
	GUID DiskGUID;				 // 56
	ULONGLONG PartitionEntryLBA; // 72
	ULONG NumberOfEntries;		 // 80
	ULONG SizeOfPartitionEntry;	 // 84
	ULONG PartitionEntryCRC32;	 // 88
} EFI_PARTITION_HEADER, *PEFI_PARTITION_HEADER;

typedef struct _EFI_PARTITION_ENTRY
{
	GUID PartitionType;	   // 0
	GUID UniquePartition;  // 16
	ULONGLONG StartingLBA; // 32
	ULONGLONG EndingLBA;   // 40
	ULONGLONG Attributes;  // 48
	WCHAR Name[0x24];	   // 56, 0x48 bytes, 36 UTF-16LE code units
} EFI_PARTITION_ENTRY, *PEFI_PARTITION_ENTRY;

typedef struct _PARTITION_TABLE_ENTRY
{
	UCHAR BootIndicator; // 0
	UCHAR StartHead;
	UCHAR StartSector;
	UCHAR StartCylinder;
	UCHAR SystemIndicator; // 4
	UCHAR EndHead;
	UCHAR EndSector;
	UCHAR EndCylinder;
	ULONG SectorCountBeforePartition; // 8
	ULONG PartitionSectorCount;		  // 12
} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

typedef struct _MASTER_BOOT_RECORD
{
	UCHAR MasterBootRecordCodeAndData[0x1B8]; // 0
	ULONG Signature;						  // 440
	USHORT Reserved;						  // 444
	PARTITION_TABLE_ENTRY PartitionTable[4];  // 446
	USHORT MasterBootRecordMagic;			  // 510
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

/* Partition entry size (bytes) - FIXME: It's hardcoded as Microsoft does, but according to specs, it shouldn't be */
#define PARTITION_ENTRY_SIZE 128
/* Defines "EFI PART" */
#define EFI_HEADER_SIGNATURE 0x5452415020494645ULL
/* Defines system type for MBR showing that a GPT is following */
#define EFI_PMBR_OSTYPE_EFI 0xEE
/* Defines size to store a complete GUID + null char */
#define EFI_GUID_STRING_SIZE 0x27

typedef enum _BL_BOOT_TYPE
{
	BIOS_BOOT = 1,
	UEFI_BOOT = 2,
} BOOT_TYPE;

#include "reactos/boot/environ/include/bcd.h"

DEFINE_GUID(GUID_WINDOWS_BOOTMGR,
			0x9DEA862C,
			0x5CDD,
			0x4E70,
			0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);

DEFINE_GUID(GUID_MY_CUSTOM,
			0x544E8052,
			0xA9E9,
			0x11E2,
			0x8B, 0x71, 0xF2, 0xC4, 0x2D, 0x24, 0x79, 0x4C);

int DiskSectorRead(void *data, const char *deviceName, ULONGLONG lba)
{
	if (data == NULL)
	{
		return -1;
	}
	if (deviceName == NULL)
	{
		return -1;
	}
	char filename[PATH_MAX];
	sprintf(filename, "/dev/%s", deviceName);
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		return -1;
	}
	fseek(fp, lba * 512, SEEK_SET);
	int dsize = fread(data, 1, 512, fp);
	fclose(fp);
	return dsize;
}

PBCD_DEVICE_OPTION BcdElementDeviceRead(const char *filename)
{
	if (filename == NULL)
	{
		return NULL;
	}
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	int dsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	PBCD_DEVICE_OPTION device = malloc(dsize);
	if (device != NULL)
	{
		fread(device, 1, dsize, fp);
	}
	fclose(fp);
	return device;
}

void BcdElementDevicePrint(PBCD_DEVICE_OPTION device)
{
	if (device == NULL)
	{
		return;
	}
	switch (device->DeviceDescriptor.DeviceType)
	{
	case PartitionDevice:
		printf("DeviceType = %s (0x%x)\n", "PartitionDevice", device->DeviceDescriptor.DeviceType);
		printf("Size = 0x%x\n", device->DeviceDescriptor.Size);
		printf("Offset = 0x%llx\n", device->DeviceDescriptor.Mbr.Offset);
		printf("DiskNumber = 0x%x\n", device->DeviceDescriptor.Mbr.DiskNumber);
		printf("Signature = 0x%x\n", device->DeviceDescriptor.Mbr.Signature);
		break;
	case LocateDevice:
		printf("DeviceType = %s (0x%x)\n", "LocateDevice", device->DeviceDescriptor.DeviceType);
		printf("Size = 0x%x\n", device->DeviceDescriptor.Size);
		switch (device->DeviceDescriptor.Local.Type)
		{
		default:
			printf("Type = %s (0x%x)\n", "Unknown", device->DeviceDescriptor.Local.Type);
			break;
		}
		break;
	case BootDevice:
		printf("DeviceType = %s (0x%x)\n", "BootDevice", device->DeviceDescriptor.DeviceType);
		printf("Size = 0x%x\n", device->DeviceDescriptor.Size);
		break;
	default:
		printf("DeviceType = %s (0x%x)\n", "Unknown", device->DeviceDescriptor.DeviceType);
		printf("%u, 0x%x, %u\n", device->DeviceDescriptor.Flags, device->DeviceDescriptor.Size, device->DeviceDescriptor.Unknown);
		break;
	}
	return;
}

PBCD_DEVICE_OPTION BcdElementDeviceUpdate(PBCD_DEVICE_OPTION device, const char *deviceName, int *bootType)
{
	if (device == NULL)
	{
		return NULL;
	}
	if (deviceName == NULL)
	{
		return NULL;
	}
	if ((deviceName[0] == 's') &&
		(deviceName[1] == 'd') &&
		(deviceName[2] >= 'a') && (deviceName[2] <= 'z') &&
		(deviceName[3] >= '1') && (deviceName[3] <= '4') &&
		(deviceName[4] == '\0'))
	{
	}
	else
	{
		return NULL;
	}
	int dsize = 0;
	char diskName[4];
	diskName[0] = deviceName[0];
	diskName[1] = deviceName[1];
	diskName[2] = deviceName[2];
	diskName[3] = '\0';
	PMASTER_BOOT_RECORD mbr = malloc(512);
	PEFI_PARTITION_HEADER gpt = malloc(512);
	dsize = DiskSectorRead(mbr, diskName, 0);
	dsize = DiskSectorRead(gpt, diskName, 1);
	if (gpt->Signature == EFI_HEADER_SIGNATURE)
	{
		*bootType = UEFI_BOOT;
	}
	else
	{
		*bootType = BIOS_BOOT;
	}
	if (*bootType == UEFI_BOOT)
	{
		// GPT
		device->DeviceDescriptor.DeviceType = PartitionDevice;
		device->DeviceDescriptor.Size = sizeof(ULONG) * 4 + sizeof(device->DeviceDescriptor.Gpt);
		device->DeviceDescriptor.Gpt.DiskSignature = gpt->DiskGUID;
		ULONGLONG lba = gpt->PartitionEntryLBA + (deviceName[3] - '1') / 4;
#if 0
		dsize = DiskSectorRead(gpt, diskName, lba);
		PEFI_PARTITION_ENTRY gpt_entry = malloc(512);
		gpt_entry = gpt_entry + (deviceName[3] - '1') % 4;
		device->DeviceDescriptor.Gpt.PartitionSignature = gpt_entry->UniquePartition;
		if (gpt_entry != NULL)
		{
			free(gpt_entry);
		}
#endif
	}
	else
	{
		// MBR
		device->DeviceDescriptor.DeviceType = PartitionDevice;
		device->DeviceDescriptor.Size = sizeof(ULONG) * 4 + sizeof(device->DeviceDescriptor.Mbr);
		device->DeviceDescriptor.Mbr.DiskNumber = 1 + deviceName[2] - 'a';
		device->DeviceDescriptor.Mbr.Offset = 512ULL * mbr->PartitionTable[deviceName[3] - '1'].SectorCountBeforePartition;
		device->DeviceDescriptor.Mbr.Signature = mbr->Signature;
	}
	if (mbr != NULL)
	{
		free(mbr);
	}
	if (gpt != NULL)
	{
		free(gpt);
	}
	return device;
}

char *BcdObjectGUIDName(char *name, GUID object)
{
	if (name == NULL)
	{
		return NULL;
	}
	sprintf(name, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
			object.Data1,
			object.Data2,
			object.Data3,
			object.Data4[0], object.Data4[1],
			object.Data4[2], object.Data4[3], object.Data4[4], object.Data4[5], object.Data4[6], object.Data4[7]);
	return name;
}

char *BcdDescriptionDirName(char *name, const char *path, GUID object)
{
	if (name == NULL)
	{
		return NULL;
	}
	sprintf(name, "%s/Objects/{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}/Description",
			path,
			object.Data1,
			object.Data2,
			object.Data3,
			object.Data4[0], object.Data4[1],
			object.Data4[2], object.Data4[3], object.Data4[4], object.Data4[5], object.Data4[6], object.Data4[7]);
	return name;
}

char *BcdElementDirName(char *name, const char *path, GUID object, ULONG element)
{
	if (name == NULL)
	{
		return NULL;
	}
	sprintf(name, "%s/Objects/{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}/Elements/%08x",
			path,
			object.Data1,
			object.Data2,
			object.Data3,
			object.Data4[0], object.Data4[1],
			object.Data4[2], object.Data4[3], object.Data4[4], object.Data4[5], object.Data4[6], object.Data4[7],
			element);
	return name;
}

void *BcdElementFileRead(const char *path, GUID object, ULONG element, const char *suffix)
{
	char filename[PATH_MAX];
	BcdElementDirName(filename, path, object, element);
	strcat(filename, "/Element.");
	strcat(filename, suffix);
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	int dsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	void *data = malloc(dsize);
	if (data != NULL)
	{
		fread(data, 1, dsize, fp);
	}
	fclose(fp);
	return data;
}

int BcdDescriptionWrite(const char *path, GUID object, ULONG element)
{
	char filename[PATH_MAX];
	BcdDescriptionDirName(filename, path, object);
	MakeDirs(filename);
	strcat(filename, "/Type.dw");
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf("Write %s failed\n", filename);
		return -1;
	}
	printf("Write %s\n", filename);
	int dsize = fprintf(fp, "%08x\n", element);
	fclose(fp);
	return dsize;
}

int BcdElementStringWrite(const char *path, GUID object, ULONG element, const char *data)
{
	char filename[PATH_MAX];
	BcdElementDirName(filename, path, object, element);
	MakeDirs(filename);
	strcat(filename, "/Element.sz");
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf("Write %s failed\n", filename);
		return -1;
	}
	printf("Write %s\n", filename);
	int dsize = fprintf(fp, "%s", data);
	fclose(fp);
	return dsize;
}

int BcdElementMultiStringWrite(const char *path, GUID object, ULONG element, const char *data)
{
	char filename[PATH_MAX];
	BcdElementDirName(filename, path, object, element);
	MakeDirs(filename);
	strcat(filename, "/Element.msz");
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf("Write %s failed\n", filename);
		return -1;
	}
	printf("Write %s\n", filename);
	int dsize = fprintf(fp, "%s", data);
	fclose(fp);
	return dsize;
}

int BcdElementBinaryWrite(const char *path, GUID object, ULONG element, const void *data, int size)
{
	char filename[PATH_MAX];
	BcdElementDirName(filename, path, object, element);
	MakeDirs(filename);
	strcat(filename, "/Element.bin");
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		printf("Write %s failed\n", filename);
		return -1;
	}
	printf("Write %s\n", filename);
	int dsize = fwrite(data, size, 1, fp);
	fclose(fp);
	return dsize;
}

int BcdFileFix(const char *mountPoint, const char *deviceName, const char *locale)
{
	ULONG timeout = 30;
	const char winRegFs[] = "/usr/bin/mount.winregfs";
	const char winRegFsMountPoint[] = "/mnt/winregfs";
	const char bcdTemplate[] = "/usr/share/BootRec/BCD_Template";
	char bcdStore[PATH_MAX];
	sprintf(bcdStore, "%s/Boot/BCD", mountPoint);
	char mountCommand[PATH_MAX];
	int ret = 0;
	ret = CopyFile(bcdTemplate, bcdStore);
	if (ret != 0)
	{
		return ret;
	}
	MakeDirs(winRegFsMountPoint);
	sprintf(mountCommand, "%s %s %s", winRegFs, bcdStore, winRegFsMountPoint);
	ret = system(mountCommand);
	if (ret != 0)
	{
		return ret;
	}
	BCD_DEVICE_OPTION bcdDevice;
	int bootType = BIOS_BOOT;
	BcdElementDeviceUpdate(&bcdDevice, deviceName, &bootType);
	BcdElementDevicePrint(&bcdDevice);
	char guidToString[PATH_MAX];
	BcdObjectGUIDName(guidToString, GUID_MY_CUSTOM);
	BcdDescriptionWrite(winRegFsMountPoint, GUID_MY_CUSTOM, 0x10200003UL);
	BcdElementBinaryWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdLibraryDevice_ApplicationDevice, &bcdDevice, sizeof(bcdDevice));
	if (bootType == UEFI_BOOT)
	{
		BcdElementStringWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdLibraryString_ApplicationPath, "\\Windows\\system32\\winload.efi");
	}
	else
	{
		BcdElementStringWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdLibraryString_ApplicationPath, "\\Windows\\system32\\winload.exe");
	}
	BcdElementStringWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdLibraryString_Description, "Windows 7");
	BcdElementStringWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdLibraryString_PreferredLocale, locale);
	BcdElementBinaryWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdOSLoaderDevice_OSDevice, &bcdDevice, sizeof(bcdDevice));
	BcdElementStringWrite(winRegFsMountPoint, GUID_MY_CUSTOM, BcdOSLoaderString_SystemRoot, "\\Windows");
	BcdDescriptionWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, 0x10100002UL);
	BcdElementBinaryWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdLibraryDevice_ApplicationDevice, &bcdDevice, sizeof(bcdDevice));
	if (bootType == UEFI_BOOT)
	{
		BcdElementStringWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdLibraryString_ApplicationPath, "\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
	}
	BcdElementStringWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdLibraryString_Description, "Windows Boot Manager");
	BcdElementStringWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdBootMgrObject_DefaultObject, guidToString);
	strcat(guidToString, "\n");
	BcdElementMultiStringWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdBootMgrObjectList_DisplayOrder, guidToString);
	BcdElementBinaryWrite(winRegFsMountPoint, GUID_WINDOWS_BOOTMGR, BcdBootMgrInteger_Timeout, &timeout, sizeof(timeout));
	sprintf(mountCommand, "fusermount -u %s", winRegFsMountPoint);
	system(mountCommand);
	return ret;
}
