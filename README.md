# Ramdisk

OS Class Project

LvXin & JiangTianyuan
-----

## Usage

First run `make` to compile and install the module.
Then run the test program `ramdisk_test` to operate the Ramdisk.

```
ramdisk_test <OPTION> <INPUT> <OUTPUT>
OPTION:
   ./ramdisk_test -c: cmd line mode, user input commands manually in terminal.
   ./ramdisk_test -f: file mode. For this option, <INPUT> and <OUTPUT> has to be specified.
```

## Test Files
There are four test files (`simple_create.in`, `huge_create.in`, `simple_wr.in`, `huge_wr.in`) that are deliberately written in the purpose of testing the Ramdisk. Run the program `ramdisk_test` in file mode with them if you would like to.

