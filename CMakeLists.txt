set(LLVM_TARGET_DEFINITIONS Epiphany.td)

tablegen(LLVM EpiphanyGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM EpiphanyGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM EpiphanyGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM EpiphanyGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM EpiphanyGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM EpiphanyGenCallingConv.inc -gen-callingconv)

add_public_tablegen_target(EpiphanyCommonTableGen)

add_llvm_target(EpiphanyCodeGen
  EpiphanyTargetMachine.cpp
  EpiphanyFrameLowering.cpp
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
