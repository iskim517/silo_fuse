SiLo implementation on fuse for college class project

1. install fuse library
sudo apt-get install libfuse-dev

2. compile
make

3. run
[program] -s [-d] monutdir

-s는 single threaded, -d는 디버그, mountdir는 fuse를 돌릴 디렉토리

4. unmount
fusermount -u mountdir
