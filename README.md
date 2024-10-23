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

## Troubleshooting Notes

This build guide assumes a Unix-like development environment. Windows users should use WSL or a similar Unix-like environment.

### 1. Executables Run

Make sure the executable file is in the directories listed in the `PATH` environment variable. If not, add the directory containing the executable file (example for `program1`):

```sh
export PATH=$PATH:/home/username/Downloads/quash_project/
```
Second, check execute permissions (example for program1):
```sh
ls -l program1
chmod +x program1
```
Now run Quash and it should work as intended:
```sh
[QUASH-@directory]$ program1 &
Background job started: [1] 2118414 program1 &
[QUASH-@directory]$ Hello, World!
```
### 2. Jobs
To start a background job:
```sh
[QUASH-@directory]$ find -type f | grep '*.c' > out.txt &
```
This background job will finish if you start a second background job:
```sh
[QUASH-@directory]$ sleep 15 &
```
Test the jobs command by executing different jobs:
```sh
[QUASH-@directory]$ sleep 15 &
Background job started: [1] 2120155 sleep 15 &
[QUASH-@directory]$ sleep 15 &
Background job started: [2] 2120156 sleep 15 &
[QUASH-@directory]$ jobs
[1] 2120155 sleep 15 &
[2] 2120156 sleep 15 &
```