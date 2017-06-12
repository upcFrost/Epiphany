
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
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

  const EpiphanyInstrInfo &TII =
    *static_cast<const EpiphanyInstrInfo*>(STI.getInstrInfo());
  const EpiphanyRegisterInfo &RegInfo =
    *static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();
  EpiphanyABIInfo ABI = STI.getABI();
  unsigned SP = Epiphany::SP;
  unsigned LR = Epiphany::LR;
  unsigned FP = Epiphany::FP;
  unsigned STRi64_pmd = Epiphany::STRi64_pmd;
  unsigned ADDri_r32 = Epiphany::ADDri_r32;
  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;
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
  MachineLocation DstML, SrcML;

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
  if (CSI.size()) {
    // Find the instruction past the last instruction that saves a callee-saved
    // register to the stack.
    for (unsigned i = 0; i < CSI.size(); ++i)
      ++MBBI;

    // Iterate over list of callee-saved registers and emit .cfi_offset
    // directives.
    DEBUG(dbgs() << "\nCallee-saved regs spilled in prologue\n");
    for (std::vector<CalleeSavedInfo>::const_iterator I = CSI.begin(), E = CSI.end(); I != E; ++I) {
      int64_t Offset = MFI.getObjectOffset(I->getFrameIdx()) - getOffsetOfLocalArea();
      unsigned Reg = I->getReg();
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
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

  const EpiphanyInstrInfo &TII =
    *static_cast<const EpiphanyInstrInfo *>(STI.getInstrInfo());
  const EpiphanyRegisterInfo &RegInfo =
    *static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

  DebugLoc dl = MBBI->getDebugLoc();
  EpiphanyABIInfo ABI = STI.getABI();
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
    BuildMI(MBB, MBBI, dl, TII.get(LDRi64), LR).addReg(SP).addImm(StackSize).setMIFlag(MachineInstr::FrameSetup);
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
    const EpiphanyRegisterInfo *RegInfo = static_cast<const EpiphanyRegisterInfo *>(
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
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();
  const EpiphanyRegisterInfo *RegInfo = static_cast<const EpiphanyRegisterInfo *>(MF.getSubtarget().getRegisterInfo());

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

// Spill callee-saved regs to stack
bool EpiphanyFrameLowering::spillCalleeSavedRegisters(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, const std::vector<CalleeSavedInfo> &CSI, const TargetRegisterInfo *TRI) const {
  MachineFunction *MF = MBB.getParent();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  MachineFrameInfo &MFI = MF->getFrameInfo();

  // Debug output
  DebugLoc DL;
  if (MI != MBB.end()) {
    DL = MI->getDebugLoc();
  }

  DEBUG(dbgs() << "\nCallee-saved regs in the current block:\n";
      for (auto I = CSI.begin(), E = CSI.end(); I != E; ++I) {
      TRI->dumpReg(I->getReg());
      });

  int i = 0;
  for (auto I = CSI.begin(), E = CSI.end(); I != E; ++I) {
    // Add the callee-saved register as live-in.
    // It's killed at the spill, unless the register is LR and return address
    // is taken.
    unsigned Reg = I->getReg();
    bool IsRAAndRetAddrIsTaken = (Reg == Epiphany::LR) && MF->getFrameInfo().isReturnAddressTaken();
    if (!IsRAAndRetAddrIsTaken) {
      MBB.addLiveIn(Reg);
    }
    bool IsKill = !IsRAAndRetAddrIsTaken;

    // Try to pair the spill
    bool Pair = false;

    // FIXME: Wrong frame indexing. Probably should use fixed stack objects or smth like this.
/*    bool stepForward = true;*/
    //unsigned suba, subb, frameidx, sra = 0, srb = 0;
    //if(i+1 < CSI.size()){
      //suba = I->getReg();
      //subb = (++I)->getReg();
      //// Getting target class and matching superreg
      //const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(suba) == &Epiphany::GPR32RegClass ? &Epiphany::GPR64RegClass : &Epiphany::FPR64RegClass;
      //sra = TRI->getMatchingSuperReg (suba, Epiphany::isub_lo, RC);
      //srb = TRI->getMatchingSuperReg (subb, Epiphany::isub_hi, RC);
      //frameidx = (--I)->getFrameIdx();
      //if( (!sra || !srb) || sra != srb){
        //srb = TRI->getMatchingSuperReg (suba, Epiphany::isub_hi, RC);
        //sra = TRI->getMatchingSuperReg (subb, Epiphany::isub_lo, RC);
        //std::swap(suba,subb);
        //frameidx = (++I)->getFrameIdx();
        //stepForward = false;
      //}
    //}
    //if ((sra && srb) && sra == srb) {
      //Pair = true;
      //const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(sra);
      //TII.storeRegToStackSlot(MBB, MI, sra, IsKill, I->getFrameIdx(), RC, TRI);
      //if (stepForward) {
        //I++;
      //}
    /*}*/

    // Insert the spill to the stack frame.
    if (!Pair) {
      const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
      TII.storeRegToStackSlot(MBB, MI, Reg, IsKill, I->getFrameIdx(), RC, TRI);
    }
  }

  return true;
}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas,
// if it needs dynamic stack realignment, if frame pointer elimination is
// disabled, or if the frame address is taken.
bool EpiphanyFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  DEBUG(
      const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
      dbgs() << "\nMax alignment = " << MFI.getMaxAlignment() << "\n";
      dbgs() << "Current alignment = " << TFI->getStackAlignment() << "\n";
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
      });

  return (MF.getTarget().Options.DisableFramePointerElim(MF) || 
      TRI->needsStackRealignment(MF) ||
      MFI.hasVarSizedObjects() ||
      MFI.isFrameAddressTaken());
}

// Eliminate pseudo ADJCALLSTACKUP/ADJCALLSTACKDOWN instructions
// See EpiphanyInstrInfo.td and EpiphanyInstrInfo.cpp
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

