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
#include "llvm/IR/Constants.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
  class EpiphanySubtarget;

  class EpiphanyFrameLowering : public TargetFrameLowering {
  protected:
    const EpiphanySubtarget &STI;

  public:
    explicit EpiphanyFrameLowering(const EpiphanySubtarget &sti)
        : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 8, 0, 8), STI(sti) {}

    /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
    /// the function.
    void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

    void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

    void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const override;

    int getFrameIndexReference(const MachineFunction &MF, int FI, unsigned &FrameReg) const override;

    void processFunctionBeforeFrameFinalized(MachineFunction &MF, RegScavenger *RS) const override;

    // Callee-saved regs spill-restore
    bool assignCalleeSavedSpillSlots(MachineFunction &MF,
                                     const TargetRegisterInfo *TRI, std::vector<CalleeSavedInfo> &CSI) const override;

    /// Returns true if the specified function should have a dedicated frame
    /// pointer register.  This is true if the function has variable sized allocas,
    /// if it needs dynamic stack realignment, if frame pointer elimination is
    /// disabled, or if the frame address is taken.
    bool hasFP(const MachineFunction &MF) const override;

    bool hasReservedCallFrame(const MachineFunction &MF) const override;

    MachineBasicBlock::iterator eliminateCallFramePseudoInstr(MachineFunction &MF,
                                                              MachineBasicBlock &MBB,
                                                              MachineBasicBlock::iterator I) const override;

  };

} // End llvm namespace

#endif
