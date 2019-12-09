## Example of ext2 filesystem driver

Simple driver which shows printing dir entries or file contents by its name or inode in 
ext2 filesystem.

Example of usage: `ext2_driver testfs2.img -contents -name Makefile` - this will print contents
of 'Makefile'.

or

`ext2_driver testfs2.img -entries -inode 2` - this will print entries of root inode.
