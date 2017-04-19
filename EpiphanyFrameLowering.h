//==- EpiphanyFrameLowering.h - Define frame lowering for Epiphany -*- C++ -*--=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class implements the Epiphany-specific parts of the TargetFrameLowering
// class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANY_FRAMEINFO_H
#define LLVM_EPIPHANY_FRAMEINFO_H

#include "EpiphanyConfig.h"

#include "Epiphany.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
  class EpiphanySubtarget;

  class EpiphanyFrameLowering : public TargetFrameLowering {
    protected:
      const EpiphanySubtarget &STI;

    public:
      explicit EpiphanyFrameLowering(const EpiphanySubtarget &sti)
        : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 16, 0, 16), STI(sti) {}

      /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
      /// the function.
      void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
      void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

      void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const override;
      int getFrameIndexReference(const MachineFunction &MF, int FI, unsigned &FrameReg) const override;

      bool spillCalleeSavedRegisters(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
          const std::vector<CalleeSavedInfo> &CSI, const TargetRegisterInfo *TRI) const override;

      bool hasFP(const MachineFunction &MF) const override;

      bool hasReservedCallFrame(const MachineFunction &MF) const override;

      MachineBasicBlock::iterator eliminateCallFramePseudoInstr(MachineFunction &MF, 
          MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const override;

  };

} // End llvm namespace

#endif
