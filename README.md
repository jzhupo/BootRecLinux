Install Windows under TinyCoreLinux

Step 1 - Apply WIM
wimlib-image apply /mnt/xxx/sources/install.wim 1 /dev/sda1

Step 2 - Fix boot files and BCD
sudo BootRec sda1 zh-CN


Note - This program need

ntfs-3g:
https://github.com/tuxera/ntfs-3g
Mount Windows partition in RW mode.

winregfs:
https://github.com/jbruchon/winregfs
Edit BCD hive file via FUSE.


Status
BIOS boot Windows 7 (tested)
