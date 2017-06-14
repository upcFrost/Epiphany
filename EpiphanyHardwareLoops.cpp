#include "EpiphanyHardwareLoops.h"

using namespace llvm;

char EpiphanyHardwareLoops::ID = 0;

INITIALIZE_PASS_BEGIN(EpiphanyHardwareLoops, "epiphany_hw_loops", "Epiphany Hardware Loops", false, false);
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(EpiphanyHardwareLoops, "epiphany_hw_loops", "Epiphany Hardware Loops", false, false);

bool optimizeBlock(MachineBasicBlock *MBB) {
  return false;
}

bool EpiphanyHardwareLoops::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const EpiphanySubtarget &>(MF.getSubtarget());
  TII = Subtarget->getInstrInfo();
  TRI = Subtarget->getRegisterInfo();
  MFI = &MF.getFrameInfo();
  MRI = &MF.getRegInfo();
  MLI = &getAnalysis<MachineLoopInfo>();

  bool Changed = false;
  for (auto &L : *MLI)
    if (!L->getParentLoop()) {
      bool L0Used = false;
      bool L1Used = false;
      Changed |= convertToHardwareLoop(L, L0Used, L1Used);
    }

  return Changed;
}

/// createEpiphanyLoadStoreOptimizationPass - returns an instance of the
/// load / store optimization pass.
FunctionPass *llvm::createEpiphanyLoadStoreOptimizationPass() {
  return new EpiphanyHardwareLoops();
 
}
