### Simple FUSE Hello world driver

Mounts a FUSE FS with a single file `hello`, containing `Hello world`. 

#### Usage:

1. Compile `gcc -Wall hello.c pkg-config fuse3 --cflags --libs -o hello`
2. Run `./hello <mountpoint>`
