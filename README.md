Epiphany
========

The Epiphany E16 (Parallella board) backend, originally based on the AArch64 backend.

Current version is based on [https://jonathan2251.github.io/lbd]

Prerequisites
-------------
* You will need some compiler. It might be a good idea to install stock llvm/clang from your distro's repo, as you will still need it later.
* You will need clang to generate IR code. You can use any clang version available (e.g. from your distro's repo), but it should be able to generate 32-bit code.
* You will need cmake
* It'd be good idea to have ccmake also

Install
-------
* Download LLVM 4.0.0 source (version matters heavily) and unpack it somewhere: `wget http://releases.llvm.org/4.0.0/llvm-4.0.0.src.tar.xz`
* Clone repo into `./lib/Target` dir of the LLVM source dir: `git clone https://github.com/upcFrost/Epiphany`
* Choose branch you're interested in. `master` is completely outdated, current 'stable' version can be found in `from-scratch` branch which should be downloaded by default. Remember that 'stable' doesn't mean 'latest', it means 'at least you can build and run it'. You can check the latest unstable branch online by the latest commit date.
* Apply LLVM_Epiphany.patch to your source dir: `cd` to LLVM src dir and run `patch -p1 < PATH_TO_LLVM_Epiphany.patch`
* Create dir `llvm-build` somewhere outside of the current dir, cd into it and run `ccmake /path/to/llvm-source` (if you have ccmake)
* Adjust cmake config, e.g. by using ccmake, you will need to specify/add the `Epiphany` target in `LLVM_TARGETS_TO_BUILD`. Or, you can just leave `all`, but then the build might take quite a long time.
* Build by running `cmake --build .` from `llvm-build` dir

Usage 
-----
* Compile C code into LLVM IR using Clang, use 32-bit target. 
  Example: `clang ${EINCS} -I ${ESDK}/tools/e-gnu.x86_64/epiphany-elf/include -S -c FILE.c -emit-llvm -m32 -o FILE.ll `
* Run `llc -march epiphany -mcpu E16 -O2 -filetype obj FILE.ll -o FILE.o` to get the relocatable object file
* Link it with e-gcc, `e-gcc -g -le-lib -T ${ELDF} FILE.o -o FILE.elf`
* Use the ELF file as an Epiphany kernel in your code
* If build fails, pls add `-debug -print-after-all -print-before-all &> debug.log` to the `llc` command and check the debug output file

What works
----------
* Asm and binary generation for most of the simple integer operations
* Branch optimization
* Register allocation optimization (-O2)
* Strings (don't forget to use `-m32` switch as strings frequently rely on `size_t`)
* External library calls
* 64-bit types (partially)
* Floating point arithmetics (partially, in simple cases)
* Load/store optimization (partially)

What doesn't work or was not tested
-----------------------------------
* 64-bit types (partially)
* Floating point arithmetics (partially)
* Load/store optimization (partially)
* Hardware loops
* Not all functions are implemented yet

