//===-- EpiphanyHardwareLoops.cpp - Identify and generate hardware loops --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass identifies loops where we can generate the Epiphany hardware
// loop instruction.  The hardware loop can perform loop branches with a
// zero-cycle overhead.
//
// Criteria for hardware loops:
//  - Countable loops (w/ ind. var for a trip count)
//  - Try inner-most loops first
// 
// HW loop constraints:
//  - All interrupts disabled
//  - 64-bit loop start alignment
//  - 64-bit next-to-last instruction alignment
//  - All instructions should be 32-bit wide
//  - No function calls in loops
//  - Minimum loop length of 8 instructions
//
//===----------------------------------------------------------------------===//
//
#ifndef _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYHWLOOPSPASS_H
#define _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYHWLOOPSPASS_H

#include "Epiphany.h"
#include "EpiphanyConfig.h"
#include "EpiphanyMachineFunction.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
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
 
  class EpiphanyHardwareLoops : public MachineFunctionPass {
    private:
      const EpiphanyInstrInfo  *TII;
      const TargetRegisterInfo *TRI;
      const EpiphanySubtarget  *Subtarget;
      MachineRegisterInfo *MRI;
      MachineFrameInfo    *MFI;
      MachineLoopInfo     *MLI;

    public:
      static char ID;
      EpiphanyHardwareLoops() : MachineFunctionPass(ID) {
        initializeEpiphanyHardwareLoopsPass(*PassRegistry::getPassRegistry());
      }

      StringRef getPassName() const {
        return "Epiphany Hardware Loops Pass";
      }
      bool runOnMachineFunction(MachineFunction &MF);

  }

}

#endif
