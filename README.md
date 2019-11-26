## Filesystem playground

This repository contains some simple tasks from MIPT filesystem course.

The code is somwhere ugly mix of C and C++, but i tried to keep the structure as simple as possible 
to draw attention to usage of system functions call.
#### Contents
 1.  `arg-hider` - example on how program arguments are stored in /proc/ filesystem.
 This program hides the first argument from itself.
 2.  `ps-lsof-implementation` - shows, how can we obtain information we get from 
 `ps` and `lsof` commands from /proc/ filesystem. 