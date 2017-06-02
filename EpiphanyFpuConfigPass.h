//===---------------------EpiphanyFpuConfigPass.h--------------------------===//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYFPUCONFIGPASS_H
#define _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYFPUCONFIGPASS_H

#include "Epiphany.h"
#include "EpiphanyConfig.h"
#include "EpiphanyMachineFunction.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

namespace llvm {
  void initializeEpiphanyFpuConfigPassPass(PassRegistry&);

  class EpiphanyFpuConfigPass : public MachineFunctionPass {

    private:
      const EpiphanyInstrInfo *TII;

      unsigned originalFrameIdx;
      unsigned fpuFrameIdx;
      unsigned ialuFrameIdx;

    public:
      static char ID;
      EpiphanyFpuConfigPass() : MachineFunctionPass(ID) {
        initializeEpiphanyFpuConfigPassPass(*PassRegistry::getPassRegistry());
      }

      StringRef getPassName() const {
        return "Epiphany FPU/IALU2 config flag optimization pass";
      }

      void insertConfigInst(MachineBasicBlock *MBB, MachineBasicBlock::iterator insertPos, 
          MachineRegisterInfo &MRI, const EpiphanySubtarget &ST, unsigned frameIdx);
      bool runOnMachineFunction(MachineFunction &MF);
  };

} // namespace llvm

#endif
