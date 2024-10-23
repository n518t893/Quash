> # Quash Shell Project
Quite a Shell (quash) program using the UNIX system calls. 

## Features

- Background jobs
- I/O redirection
- Pipes
- Built-in commands (echo, export, cd, pwd, jobs, quit, exit)
## Installation
To build Quash use:
> `make`

To clean quash use:
> `make clean`

## Usage

To run Quash use:
> `./quash`

## Troubleshooting notes
This build guide assumes a Unix-like development environment. Windows users should use WSL or a similar Unix-like environment.
# 1. Executables run: 
Make sure the executable file is in the the directories in the environment variable PATH.
If not, add the directory executable file is in(example program1):
> `export PATH=$PATH:/home/username/Downloads/quash_project/`
Second, check execute Permissions(example for program1):
> [QUASH]$ sleep 15 &ls -l program1`
> `chmod +x program1`
Now run Quash and it should work as intended:
[QUASH-@directory]$ program1 &
Background job started: [1] 2118414     program1 &
[QUASH-@directory]$ Hello, World!
# 2. Jobs:
[QUASH-@directory]$ find -type f | grep '*.c' > out.txt &
This background job will finished if started the second background job:
[QUASH-@directory]$ sleep 15 &
Test command `jobs` by execute a different jobs:
[QUASH - @directory]$ sleep 15 &
Background job started: [1]      2120155        sleep 15 & 
[QUASH - @directory]$ sleep 15 &
Background job started: [2]      2120156        sleep 15 & 
[QUASH - @directory]$ jobs
[1]      2120155        sleep 15 & 
[2]      2120156        sleep 15 & 
