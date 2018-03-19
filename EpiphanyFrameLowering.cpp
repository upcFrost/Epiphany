
//===- EpiphanyFrameLowering.cpp - Epiphany Frame Information ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//
// 
// e-gcc creates the following stack
//
// |                                   | Higher address
// |-----------------------------------|
// |                                   |
// | arguments passed on the stack     |
// |                                   |
// |-----------------------------------| <- prev_fp + 2
// | prev_lr                           |
// | prev_fp                           |
// |-----------------------------------| <- prev_sp, fp
// |                                   |
// | callee-saved registers            |
// |                                   |
// |-----------------------------------|
// |                                   |
// | local variables                   |
// |                                   |
// |-----------------------------------|
// |          2 empty bytes            |
// |            for lr/fp              |
// |-----------------------------------| <- sp
// |                                   | Lower address
//


#include "EpiphanyFrameLowering.h"

#include "EpiphanyMachineFunction.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanySubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "frame-info"

// Prologue should save the original FP and LR, and adjust fp into position
// LR and FP are neighbors, so we can use 64-bit store/load
//   strd lr, [sp], -offset
//   add  fp, offset
//@emitPrologue {
void EpiphanyFrameLowering::emitPrologue(MachineFunction &MF,
    MachineBasicBlock &MBB) const {
  assert(&MF.front() == &MBB && "Shrink-wrapping not yet supported");
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineModuleInfo &MMI = MF.getMMI();

  const EpiphanyInstrInfo &TII = *STI.getInstrInfo();

  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();
  unsigned SP = Epiphany::SP;
  unsigned LR = Epiphany::LR;
  unsigned FP = Epiphany::FP;
  unsigned STRi64_pmd = Epiphany::STRi64_pmd;
  unsigned ADDri_r32 = Epiphany::ADDri_r32;
  unsigned CFIIndex;

  // First, compute final stack size.
  uint64_t StackSize = MFI.getStackSize();

  // If we have calls, we should allocate 16 bytes to store LR and FP by the callee func
  if (MF.getFrameInfo().hasCalls()) {
    StackSize += 16;
  }

  // No need to allocate space on the stack.
  if (StackSize == 0 && !MFI.adjustsStack()) return;

  const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();

  // Create label for prologue
  MCSymbol *FrameLabel = MF.getContext().createTempSymbol();

  // if framepointer enabled, set it to point to the stack pointer.
  if (hasFP(MF)) {
    // Save old LR and FP to stack
    BuildMI(MBB, MBBI, DL, TII.get(STRi64_pmd), SP).addReg(LR).addReg(SP).addImm(-StackSize).setMIFlag(MachineInstr::FrameSetup);

    // Adjust FP
    BuildMI(MBB, MBBI, DL, TII.get(ADDri_r32), FP).addReg(SP).addImm(StackSize).setMIFlag(MachineInstr::FrameSetup);

    // emit ".cfi_def_cfa_register $fp"
    CFIIndex = MF.addFrameInst(MCCFIInstruction::createDefCfaRegister(FrameLabel, MRI->getDwarfRegNum(FP, true)));
    BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex).setMIFlags(MachineInstr::FrameSetup);
  } else {
    // Just adjust SP if no frame present
    TII.adjustStackPtr(SP, -StackSize, MBB, MBBI);
  }

  // emit ".cfi_def_cfa_offset StackSize"
  CFIIndex = MF.addFrameInst(MCCFIInstruction::createDefCfaOffset(FrameLabel, -StackSize));
  BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex).setMIFlags(MachineInstr::FrameSetup);

  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();

  // Spill all callee-saves
  if (!CSI.empty()) {
    // Find the instruction past the last instruction that saves a callee-saved
    // register to the stack.
    for (unsigned i = 0; i < CSI.size(); ++i)
      ++MBBI;

    // Iterate over list of callee-saved registers and emit .cfi_offset
    // directives.
    DEBUG(dbgs() << "\nCallee-saved regs spilled in prologue\n");
    for (auto I : CSI) {
      int64_t Offset = MFI.getObjectOffset(I.getFrameIdx()) - getOffsetOfLocalArea();
      unsigned Reg = I.getReg();
      // Reg is in CPURegs.
      DEBUG(dbgs() << Reg << "\n");
      CFIIndex = MF.addFrameInst(MCCFIInstruction::createOffset(FrameLabel, MRI->getDwarfRegNum(Reg, true), Offset));
      BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex).setMIFlags(MachineInstr::FrameSetup);
    }
  }

}
//}

//@emitEpilogue {
void EpiphanyFrameLowering::emitEpilogue(MachineFunction &MF,
    MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  const EpiphanyInstrInfo &TII = *STI.getInstrInfo();

  DebugLoc dl = MBBI->getDebugLoc();
  unsigned SP = Epiphany::SP;
  unsigned LR = Epiphany::LR;
  unsigned LDRi64 = Epiphany::LDRi64;

  // Get the number of bytes from FrameInfo
  uint64_t StackSize = MFI.getStackSize();

  if (MF.getFrameInfo().hasCalls()) {
    StackSize += 16;
  }

  if (!StackSize)
    return;

  // if framepointer enabled, set it to point to the stack pointer.
  if (hasFP(MF)) {
    // Restore old LR and FP from SP + offset
    BuildMI(MBB, MBBI, dl, TII.get(LDRi64), LR).addReg(SP).addImm(StackSize).setMIFlag(MachineInstr::FrameDestroy);
  }

  // Adjust stack.
  TII.adjustStackPtr(SP, StackSize, MBB, MBBI);
}
//}

/// getFrameIndexReference - Provide a base+offset reference to an FI slot for
/// debug info.  It's the same as what we use for resolving the code-gen
/// references for now.  FIXME: This can go wrong when references are
/// SP-relative and simple call frames aren't used.
int EpiphanyFrameLowering::getFrameIndexReference(const MachineFunction &MF,
    int FI, unsigned &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (hasFP(MF)) {
    const auto *RegInfo = static_cast<const EpiphanyRegisterInfo *>(
        MF.getSubtarget().getRegisterInfo());
    FrameReg = RegInfo->getFrameRegister(MF);
    //return MFI.getObjectOffset(FI) + 16;
    return MFI.getObjectOffset(FI);
  } else {
    FrameReg = Epiphany::SP;
    return MFI.getObjectOffset(FI) + MFI.getStackSize();
  }
}

static void setAliasRegs(MachineFunction &MF, BitVector &SavedRegs, unsigned Reg) {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
    SavedRegs.set(*AI);
}

// This method is called immediately before PrologEpilogInserter scans the 
//  physical registers used to determine what callee saved registers should be 
//  spilled. This method is optional. 
void EpiphanyFrameLowering::determineCalleeSaves(MachineFunction &MF,
    BitVector &SavedRegs,
    RegScavenger *RS) const {
  //@determineCalleeSaves-body
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  const auto *RegInfo = static_cast<const EpiphanyRegisterInfo *>(MF.getSubtarget().getRegisterInfo());

  DEBUG(dbgs() << "*** determineCalleeSaves\nUsed CSRs:";
      for (int Reg = SavedRegs.find_first(); Reg != -1;
        Reg = SavedRegs.find_next(Reg))
      dbgs() << ' ' << PrintReg(Reg, RegInfo);
      dbgs() << "\n";);

}

bool EpiphanyFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  // Reserve call frame if the size of the maximum call frame fits into 16-bit
  // immediate field and there are no variable sized objects on the stack.
  // Make sure the second register scavenger spill slot can be accessed with one
  // instruction.
  return isInt<16>(MFI.getMaxCallFrameSize() + getStackAlignment()) &&
    !MFI.hasVarSizedObjects();
}

/// Assign callee-saved regs to frame indexes
///
/// \param MF Machine function
/// \param TRI Register info
/// \param CSI Callee-save regs info
/// \return True if success
bool EpiphanyFrameLowering::assignCalleeSavedSpillSlots(MachineFunction &MF,
    const TargetRegisterInfo *TRI, std::vector<CalleeSavedInfo> &CSI) const {
  // TODO: Probably this method can be moved completely into reimplemented determineCalleeSaves()
  // as it not only assigns but also redefines paired regs
  if (CSI.empty())
    return true; // Early exit if no callee saved registers are modified!

  MachineFrameInfo &MFI = MF.getFrameInfo();

  unsigned NumFixedSpillSlots;
  const TargetFrameLowering::SpillSlot *FixedSpillSlots = getCalleeSavedSpillSlots(NumFixedSpillSlots);

  // Now that we know which registers need to be saved and restored, allocate
  // stack slots for them.
  for (auto CS = CSI.begin(); CS != CSI.end(); ++CS) {
    unsigned Reg = CS->getReg();
    if (Reg == Epiphany::LR) {
      DEBUG(dbgs() << "Erasing LR from CSI, it will be handled by prologue/epilogue inserters\n");
      CSI.erase(CS--);
      continue;
    }

    int FrameIdx;
    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    if (TRI->hasReservedSpillSlot(MF, Reg, FrameIdx)) {
      CS->setFrameIdx(FrameIdx);
      continue;
    }

    // Check to see if this physreg must be spilled to a particular stack slot on this target.
    const TargetFrameLowering::SpillSlot *FixedSlot = FixedSpillSlots;
    const TargetFrameLowering::SpillSlot *LastFixedSlot = FixedSlot + NumFixedSpillSlots;
    while (FixedSlot != LastFixedSlot && FixedSlot->Reg != Reg) {
      ++FixedSlot;
    }

    if (FixedSlot != LastFixedSlot) {
      // Spill it to the stack where we must and bail out
      FrameIdx = MFI.CreateFixedSpillStackObject(RC->getSize(), FixedSlot->Offset);
      CS->setFrameIdx(FrameIdx);
      continue;
    } else {
      // Nope, just spill it anywhere convenient.
      unsigned Align = RC->getAlignment();
      unsigned StackAlign = getStackAlignment();

      // Check if this index can be paired
      unsigned sra = 0, srb = 0;
      auto Next = CS;
      Next++;
      if (Next != CSI.end()) {
        unsigned CurrentReg = CS->getReg();
        unsigned NextReg = Next->getReg();
        // Getting target class
        const TargetRegisterClass *TRC = TRI->getMinimalPhysRegClass(CurrentReg) == &Epiphany::GPR32RegClass ||
                                         TRI->getMinimalPhysRegClass(CurrentReg) == &Epiphany::GPR16RegClass
                                         ? &Epiphany::GPR64RegClass
                                         : &Epiphany::FPR64RegClass;
        // Check if we can find superreg for paired regs
        sra = TRI->getMatchingSuperReg(CurrentReg, Epiphany::isub_lo, TRC);
        srb = TRI->getMatchingSuperReg(NextReg, Epiphany::isub_hi, TRC);
        if ((!sra || !srb) || sra != srb) {
          srb = TRI->getMatchingSuperReg(CurrentReg, Epiphany::isub_hi, TRC);
          sra = TRI->getMatchingSuperReg(NextReg, Epiphany::isub_lo, TRC);
        }

        // Check if pair was formed
        if ((sra && srb) && sra == srb) {
          // Remove subregs and set superreg as Callee-saved
          CSI.erase(CS--, ++Next);
          CSI.emplace_back(sra);
          continue;
        }
      }

      // If unable to pair for some reason - just assign to the next frame index
      Align = std::min(Align, StackAlign);
      FrameIdx = MFI.CreateStackObject(RC->getSize(), Align, true);
      CS->setFrameIdx(FrameIdx);
    }
  }

  return true;
}

// hasFP - Returns true if the specified function should have a dedicated frame
// pointer register.
bool EpiphanyFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  DEBUG(
      if (MF.getTarget().Options.DisableFramePointerElim(MF)) {
      dbgs() << "\nHas FP: DisableFramePointerElim set\n";
      }
      if (TRI->needsStackRealignment(MF)) {
      dbgs() << "\nHas FP: Stack realign needed\n";
      }
      if (MFI.hasVarSizedObjects()) {
      dbgs() << "\nHas FP: Has var sized objects\n";
      }
      if (MFI.isFrameAddressTaken()) {
      dbgs() << "\nHas FP: Frame address taken\n";
      };);

  return (MF.getTarget().Options.DisableFramePointerElim(MF) ||
      TRI->needsStackRealignment(MF) ||
      MFI.hasVarSizedObjects() ||
      MFI.isFrameAddressTaken());
}

/// Set local frame max alignment to 8, used by EpiphanyLoadStoreOptimizer
void EpiphanyFrameLowering::processFunctionBeforeFrameFinalized(MachineFunction &MF,
    RegScavenger *RS) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setLocalFrameMaxAlign(8);
}

/// Eliminate pseudo ADJCALLSTACKUP/ADJCALLSTACKDOWN instructions
/// See EpiphanyInstrInfo.td and EpiphanyInstrInfo.cpp
MachineBasicBlock::iterator EpiphanyFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const {
  unsigned SP = Epiphany::SP;

  if (!hasReservedCallFrame(MF)) {
    // Keep positive if adjusting up, negate if down
    int64_t Amount = I->getOperand(0).getImm();
    if (I->getOpcode() == Epiphany::ADJCALLSTACKDOWN) {
      Amount = -Amount;
    }
    // Issue adjustment commands
    STI.getInstrInfo()->adjustStackPtr(SP, Amount, MBB, I);
  }

  return MBB.erase(I);
}

