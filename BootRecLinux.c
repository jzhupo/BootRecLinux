#include <stdio.h>

#define _In_
#define _Out_
#define _Out_opt_
#define C_ASSERT(expr)
#define DEVICE_TYPE ULONG

#include "reactos/sdk/include/host/typedefs.h"
#include "reactos/sdk/include/psdk/guiddef.h"

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

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
	ULONGLONG Signature;			// 0
	ULONG Revision;					// 8
	ULONG HeaderSize;				// 12
	ULONG HeaderCRC32;				// 16
	ULONG Reserved;					// 20
	ULONGLONG MyLBA;				// 24
	ULONGLONG AlternateLBA;			// 32
	ULONGLONG FirstUsableLBA;		// 40
	ULONGLONG LastUsableLBA;		// 48
	GUID DiskGUID;					// 56
	ULONGLONG PartitionEntryLBA;	// 72
	ULONG NumberOfEntries;			// 80
	ULONG SizeOfPartitionEntry;		// 84
	ULONG PartitionEntryCRC32;		// 88
} EFI_PARTITION_HEADER, *PEFI_PARTITION_HEADER;

typedef struct _EFI_PARTITION_ENTRY
{
	GUID PartitionType;		// 0
	GUID UniquePartition;	// 16
	ULONGLONG StartingLBA;	// 32
	ULONGLONG EndingLBA;	// 40
	ULONGLONG Attributes;	// 48
	WCHAR Name[0x24];		// 56, 0x48 bytes, 36 UTF-16LE code units
} EFI_PARTITION_ENTRY, *PEFI_PARTITION_ENTRY;

typedef struct _PARTITION_TABLE_ENTRY
{
	UCHAR BootIndicator;				// 0
	UCHAR StartHead;
	UCHAR StartSector;
	UCHAR StartCylinder;
	UCHAR SystemIndicator;				// 4
	UCHAR EndHead;
	UCHAR EndSector;
	UCHAR EndCylinder;
	ULONG SectorCountBeforePartition;	// 8
	ULONG PartitionSectorCount;			// 12
} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

typedef struct _MASTER_BOOT_RECORD
{
	UCHAR MasterBootRecordCodeAndData[0x1B8];	// 0
	ULONG Signature;							// 440
	USHORT Reserved;							// 444
	PARTITION_TABLE_ENTRY PartitionTable[4];	// 446
	USHORT MasterBootRecordMagic;				// 510
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

/* Tag for Fstub allocations */
#define TAG_FSTUB 'BtsF'
/* Partition entry size (bytes) - FIXME: It's hardcoded as Microsoft does, but according to specs, it shouldn't be */
#define PARTITION_ENTRY_SIZE 128
/* Defines "EFI PART" */
#define EFI_HEADER_SIGNATURE 0x5452415020494645ULL
/* Defines version 1.0 */
#define EFI_HEADER_REVISION_1 0x00010000
/* Defines system type for MBR showing that a GPT is following */
#define EFI_PMBR_OSTYPE_EFI 0xEE
/* Defines size to store a complete GUID + null char */
#define EFI_GUID_STRING_SIZE 0x27

#include "reactos/boot/environ/include/bcd.h"

typedef enum _BL_BOOT_TYPE
{
	BIOS_BOOT = 1,
	UEFI_BOOT = 2,
} BOOT_TYPE;

DEFINE_GUID(GUID_WINDOWS_BOOTMGR,
			0x9DEA862C,
			0x5CDD,
			0x4E70,
			0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);

int DiskSectorRead(char *data, char *device_name, ULONGLONG lba)
{
	if (data == NULL)
	{
		return 0;
	}
	if (device_name == NULL)
	{
		return 0;
	}
#if 0
	FILE *fp = fopen("../MBR.bin", "rb");
#else
	char filename[PATH_MAX];
	sprintf(filename, "/dev/%s", device_name);
	FILE *fp = fopen(filename, "rb");
#endif
	if (fp == NULL)
	{
		return 0;
	}
	fseek(fp, lba * 512, SEEK_SET);
	int dsize = fread(data, 1, 512, fp);
	fclose(fp);
	return dsize;
}

PBCD_DEVICE_OPTION BcdElementDeviceRead(char *filename)
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

PBCD_DEVICE_OPTION BcdElementDeviceUpdate(PBCD_DEVICE_OPTION device, char *device_name, int boot_type)
{
	if (device == NULL)
	{
		return NULL;
	}
	if (device_name == NULL)
	{
		return NULL;
	}
	if ((device_name[0] == 's') &&
		(device_name[1] == 'd') &&
		(device_name[2] >= 'a') && (device_name[2] <= 'z') &&
		(device_name[3] >= '1') && (device_name[3] <= '4') &&
		(device_name[4] == '\0'))
	{
	}
	else
	{
		return NULL;
	}
	int dsize = 0;
	PMASTER_BOOT_RECORD mbr = NULL;
	PEFI_PARTITION_HEADER gpt = NULL;
	char disk_name[4];
	disk_name[0] = device_name[0];
	disk_name[1] = device_name[1];
	disk_name[2] = device_name[2];
	disk_name[3] = '\0';
	if (dsize == 512)
	{
		// MBR
		mbr = malloc(512);
		dsize = DiskSectorRead(mbr, disk_name, 0);
		device->DeviceDescriptor.DeviceType = PartitionDevice;
		device->DeviceDescriptor.Size = sizeof(ULONG) * 4 + sizeof(device->DeviceDescriptor.Mbr);
		device->DeviceDescriptor.Mbr.DiskNumber = 1 + device_name[2] - 'a';
		device->DeviceDescriptor.Mbr.Offset = 512ULL * mbr->PartitionTable[device_name[3] - '1'].SectorCountBeforePartition;
		device->DeviceDescriptor.Mbr.Signature = mbr->Signature;
	}
	if (1 == 2)
	{
		// GPT
		gpt = malloc(512);
		dsize = DiskSectorRead(gpt, disk_name, 1);
		device->DeviceDescriptor.Size = sizeof(ULONG) * 4 + sizeof(device->DeviceDescriptor.Gpt);
		device->DeviceDescriptor.Gpt.DiskSignature = gpt->DiskGUID;
		ULONGLONG lba = gpt->PartitionEntryLBA + (device_name[3] - '1') / 4;
		dsize = DiskSectorRead(gpt, disk_name, lba);
		PEFI_PARTITION_ENTRY gpt_entry = gpt;
		gpt_entry = gpt_entry + (device_name[3] - '1') % 4;
		device->DeviceDescriptor.Gpt.PartitionSignature = gpt_entry->UniquePartition;
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

char *BcdElementFileName(char *name, char *path, GUID object, ULONG element, char *suffix)
{
	if (name == NULL)
	{
		return NULL;
	}
	sprintf(name, "%s/{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}/Elements/%08x/Element.%s",
			path,
			object.Data1,
			object.Data2,
			object.Data3,
			object.Data4[0], object.Data4[1],
			object.Data4[2], object.Data4[3], object.Data4[4], object.Data4[5], object.Data4[6], object.Data4[7],
			element,
			suffix);
	return name;
}

void *BcdElementFileRead(char *path, GUID object, ULONG element, char *suffix)
{
	char filename[PATH_MAX];
	BcdElementFileName(filename, path, object, element, suffix);
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		return 0;
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

int BcdElementFileWrite(char *path, GUID object, ULONG element, char *suffix, void *data, int size)
{
	char filename[PATH_MAX];
	BcdElementFileName(filename, path, object, element, suffix);
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		return 0;
	}
	int dsize = fwrite(data, size, 1, fp);
	fclose(fp);
	return dsize;
}

int BcdElementStringWrite(char *path, GUID object, ULONG element, char *data)
{
	return BcdElementFileWrite(path, object, element, "sz", data, strlen(data));
}

int BcdElementBinaryWrite(char *path, GUID object, ULONG element, void *data, int size)
{
	return BcdElementFileWrite(path, object, element, "bin", data, size);
}

int main()
{
	PBCD_DEVICE_OPTION application_device;
#if 0
	application_device = BcdElementDeviceRead("../BCD_Part_0_0_Element.bin");
#else
	application_device = malloc(sizeof(BCD_DEVICE_OPTION));
#endif
	BcdElementDeviceUpdate(application_device, "sda1", BIOS_BOOT);
	BcdElementDevicePrint(application_device);
	char path[] = "mnt";
	GUID my_guid;
	BcdElementBinaryWrite(path, my_guid, BcdLibraryDevice_ApplicationDevice, application_device, sizeof(application_device));
	BcdElementStringWrite(path, my_guid, BcdLibraryString_ApplicationPath, "\\Windows\\system32\\winload.exe");
	BcdElementStringWrite(path, my_guid, BcdLibraryString_Description, "Windows 7");
	BcdElementStringWrite(path, my_guid, BcdLibraryString_PreferredLocale, "zh-CN");
	BcdElementBinaryWrite(path, my_guid, BcdOSLoaderDevice_OSDevice, NULL, 0);
	BcdElementStringWrite(path, my_guid, BcdOSLoaderString_SystemRoot, "\\Windows");
	BcdElementBinaryWrite(path, GUID_WINDOWS_BOOTMGR, BcdLibraryDevice_ApplicationDevice, NULL, 0);
	BcdElementStringWrite(path, GUID_WINDOWS_BOOTMGR, BcdLibraryString_Description, "Windows Boot Manager");
	BcdElementStringWrite(path, GUID_WINDOWS_BOOTMGR, BcdBootMgrObject_DefaultObject, my_guid);
	BcdElementStringWrite(path, GUID_WINDOWS_BOOTMGR, BcdBootMgrObjectList_DisplayOrder, my_guid);
	BcdElementBinaryWrite(path, GUID_WINDOWS_BOOTMGR, BcdBootMgrInteger_Timeout, NULL, 0);
	if (application_device)
	{
		free(application_device);
	}
	return 0;
}
