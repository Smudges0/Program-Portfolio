Fuse commands:

# Create a empty dir as a mount point
mkdir fusedir

# Start tfs and pass -s for single-threading, and the mount point.
# tfs will remain running in the background.  Can see it with ps -ef | grep tfs
./tfs -s ./fusedir

# Use -f flag to keep fuse running in the foreground (to see output in stdout)

# Check if mounted
mount | grep fuse
/home/simon/cs416-operating-systems/project4/tfs on /home/simon/cs416-operating-systems/project4/fusedir type fuse.tfs (rw,nosuid,nodev,relatime,user_id=1000,group_id=1000)

# Stop fuse and unmount.  tfs process will stop and mount will be gone.
fusermount -u ./fusedir

# Check if mounted
mount | grep fuse

# Even with no code, you can cd into nonexistent dirs and do ls, but nothing will show up.

~/cs416-operating-systems/project4$ ls -l
total 204
-rw-r--r-- 1 simon simon    202 Nov 28 17:53 Makefile
drwxr-xr-x 2 simon simon   4096 Nov 29 14:08 benchmark
-rw-r--r-- 1 simon simon   1607 Nov 29 20:59 block.c
-rw-r--r-- 1 simon simon    408 Nov 28 17:53 block.h
-rw-r--r-- 1 simon simon   8728 Dec  1 16:31 block.o
drwxr-xr-x 2 root  root       0 Dec  1 16:46 fusedir <------------
-rw-r--r-- 1 simon simon 102573 Nov 28 17:53 project4.pdf
-rwxr-xr-x 1 simon simon  27144 Dec  1 16:31 tfs
-rw-r--r-- 1 simon simon   8946 Nov 29 20:59 tfs.c
-rw-r--r-- 1 simon simon   1612 Nov 28 17:53 tfs.h
-rw-r--r-- 1 simon simon  26048 Dec  1 16:31 tfs.o

~/cs416-operating-systems/project4$ cd fusedir
~/cs416-operating-systems/project4/fusedir$ ls -l
total 0

~/cs416-operating-systems/project4/fusedir$ touch tempfile
~/cs416-operating-systems/project4/fusedir$ ls -l
total 0
~/cs416-operating-systems/project4/fusedir$ mkdir testdir
mkdir: cannot create directory ‘testdir’: File exists
~/cs416-operating-systems/project4/fusedir$ cd testdir
~/cs416-operating-systems/project4/fusedir/testdir$ ls -l
total 0
~/cs416-operating-systems/project4/fusedir/testdir$ cd ..
~/cs416-operating-systems/project4/fusedir$ ls -l
total 0
~/cs416-operating-systems/project4/fusedir$ 