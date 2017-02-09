Epiphany
========

The Epiphany E16 (Parallella board) backend, originally based on the AArch64 backend.

Current version is based on [https://jonathan2251.github.io/lbd]

Install
-------
* Download LLVM 3.9.X source
* Clone repo into `./lib/Target` dir
* Apply LLVM_Epiphany.patch to your source dir
* Create dir `llvm-build` somewhere outside of the current dir, cd into it and run `cmake /path/to/llvm-source`
* Adjust cmake config, e.g. by using ccmake
* Build

Usage
-----
* Compile C code into LLVM IR using Clang (-emit-llvm option)
* Run `llc -march epiphany -mcpu E16 -O2 -filetype obj FILE.ll -o FILE.o` to get the relocatable object file
* Link it with e-gcc

What works
----------
* Asm and binary generation for most of the simple integer operations
* Branch optimization
* Register allocation optimization (-O2)

What doesn't work or was not tested
-----------------------------------
* Strings
* 64-bit types
* Floating point arithmetics (partially works)
* External library calls