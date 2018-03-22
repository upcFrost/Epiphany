set(LLVM_TARGET_DEFINITIONS Epiphany.td)

tablegen(LLVM EpiphanyGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM EpiphanyGenCodeEmitter.inc -gen-emitter)
tablegen(LLVM EpiphanyGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM EpiphanyGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM EpiphanyGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM EpiphanyGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM EpiphanyGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM EpiphanyGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM EpiphanyGenCallingConv.inc -gen-callingconv)
tablegen(LLVM EpiphanyGenAsmWriter.inc -gen-asm-writer)

add_public_tablegen_target(EpiphanyCommonTableGen)

add_llvm_target(EpiphanyCodeGen
        EpiphanyAsmPrinter.cpp
        EpiphanyFpuConfigPass.cpp
        EpiphanyFrameLowering.cpp
        EpiphanyISelLowering.cpp
        EpiphanyISelDAGToDAG.cpp
        EpiphanyInstrInfo.cpp
        EpiphanyLoadStoreOptimizer.cpp
        EpiphanyVregLoadStoreOptimizer.cpp
        EpiphanyHardwareLoops.cpp
        EpiphanyMachineFunction.cpp
        EpiphanyMCInstLower.cpp
        EpiphanyRegisterInfo.cpp
        EpiphanySubtarget.cpp
        EpiphanyTargetMachine.cpp
        EpiphanyTargetObjectFile.cpp
        EpiphanyTargetTransformInfo.cpp
        )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
add_subdirectory(InstPrinter)
add_subdirectory(AsmParser)

