
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

// Prologue should save the original stack pointer.
// e-gcc generates it like this:
//   str fp, [sp], -offset
//   mov fp, sp
// So, basically, write old FP to [SP], substract offset, move new SP to FP
//@emitPrologue {
void EpiphanyFrameLowering::emitPrologue(MachineFunction &MF,
    MachineBasicBlock &MBB) const {
  assert(&MF.front() == &MBB && "Shrink-wrapping not yet supported");
  MachineFrameInfo *MFI = MF.getFrameInfo();
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

  const EpiphanyInstrInfo &TII =
    *static_cast<const EpiphanyInstrInfo*>(STI.getInstrInfo());
  const EpiphanyRegisterInfo &RegInfo =
    *static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();
  EpiphanyABIInfo ABI = STI.getABI();
  unsigned SP = Epiphany::SP;
  unsigned FP = Epiphany::FP;
  unsigned STRi32_r32 = Epiphany::STRi32_r32;
  unsigned MOVi32rr = Epiphany::MOVi32rr;
  const TargetRegisterClass *RC = &Epiphany::GPR32RegClass;
  unsigned CFIIndex;

  // First, compute final stack size.
  uint64_t StackSize = MFI->getStackSize();

  // No need to allocate space on the stack.
  if (StackSize == 0 && !MFI->adjustsStack()) return;

  MachineModuleInfo &MMI = MF.getMMI();
  const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();
  MachineLocation DstML, SrcML;

  // if framepointer enabled, set it to point to the stack pointer.
  if (hasFP(MF)) {
    // TODO: Offset 0 for the SP is fixed for now. This is not correct, but at least for testing. 
    // offset is added later
    //
    // Save old FP to stack
    BuildMI(MBB, MBBI, DL, TII.get(STRi32_r32), FP).addReg(SP).addImm(0).setMIFlag(MachineInstr::FrameSetup);

    // Adding offset
    // TODO: Should be merged in STR/POSTMODIFY
    TII.adjustStackPtr(SP, -StackSize, MBB, MBBI);

    // Move new SP to FP
    BuildMI(MBB, MBBI, DL, TII.get(MOVi32rr), FP).addReg(SP).setMIFlag(MachineInstr::FrameSetup);

    // emit ".cfi_def_cfa_register $fp"
    CFIIndex = MMI.addFrameInst(MCCFIInstruction::createDefCfaRegister(nullptr, MRI->getDwarfRegNum(FP, true)));
    BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex);
  } else {
    // Just adjust SP if no frame present
    TII.adjustStackPtr(SP, -StackSize, MBB, MBBI);
  }

  // emit ".cfi_def_cfa_offset StackSize"
  CFIIndex = MMI.addFrameInst(MCCFIInstruction::createDefCfaOffset(nullptr, -StackSize));
  BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex);

  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();

  if (CSI.size()) {
    // Find the instruction past the last instruction that saves a callee-saved
    // register to the stack.
    for (unsigned i = 0; i < CSI.size(); ++i)
      ++MBBI;

    // Iterate over list of callee-saved registers and emit .cfi_offset
    // directives.
    for (std::vector<CalleeSavedInfo>::const_iterator I = CSI.begin(),
        E = CSI.end(); I != E; ++I) {
      int64_t Offset = MFI->getObjectOffset(I->getFrameIdx());
      unsigned Reg = I->getReg();
      {
        // Reg is in CPURegs.
        CFIIndex = MMI.addFrameInst(MCCFIInstruction::createOffset(nullptr, MRI->getDwarfRegNum(Reg, 1), Offset));
        BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex);
      }
    }
  }

}
//}

//@emitEpilogue {
void EpiphanyFrameLowering::emitEpilogue(MachineFunction &MF,
    MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  EpiphanyMachineFunctionInfo *FI = MF.getInfo<EpiphanyMachineFunctionInfo>();

  const EpiphanyInstrInfo &TII =
    *static_cast<const EpiphanyInstrInfo *>(STI.getInstrInfo());
  const EpiphanyRegisterInfo &RegInfo =
    *static_cast<const EpiphanyRegisterInfo *>(STI.getRegisterInfo());

  DebugLoc dl = MBBI->getDebugLoc();
  EpiphanyABIInfo ABI = STI.getABI();
  unsigned SP = Epiphany::SP;
  unsigned FP = Epiphany::FP;
  unsigned LDRi32_r32 = Epiphany::LDRi32_r32;

  // Get the number of bytes from FrameInfo
  uint64_t StackSize = MFI->getStackSize();

  if (!StackSize)
    return;

  // if framepointer enabled, set it to point to the stack pointer.
  if (hasFP(MF)) {
    // Restore old frame pointer as SP + offset
    BuildMI(MBB, MBBI, dl, TII.get(LDRi32_r32), FP).addReg(SP).addImm(StackSize).setMIFlag(MachineInstr::FrameSetup);
  }

  // Adjust stack.
  TII.adjustStackPtr(SP, StackSize, MBB, MBBI);
}
//}

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
  MachineRegisterInfo& MRI = MF.getRegInfo();

  if (MF.getFrameInfo()->hasCalls())
    setAliasRegs(MF, SavedRegs, Epiphany::LR);

  return;
}

bool EpiphanyFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();

  // Reserve call frame if the size of the maximum call frame fits into 16-bit
  // immediate field and there are no variable sized objects on the stack.
  // Make sure the second register scavenger spill slot can be accessed with one
  // instruction.
  return isInt<16>(MFI->getMaxCallFrameSize() + getStackAlignment()) &&
    !MFI->hasVarSizedObjects();
}

bool EpiphanyFrameLowering::spillCalleeSavedRegisters(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, const std::vector<CalleeSavedInfo> &CSI, const TargetRegisterInfo *TRI) const {
  MachineFunction *MF = MBB.getParent();
  MachineBasicBlock *EntryBlock = &MF->front();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();

  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    // Add the callee-saved register as live-in.
    // It's killed at the spill, unless the register is LR and return address
    // is taken.
    unsigned Reg = CSI[i].getReg();
    bool IsRAAndRetAddrIsTaken = (Reg == Epiphany::LR) && MF->getFrameInfo()->isReturnAddressTaken();
    if (!IsRAAndRetAddrIsTaken) {
      EntryBlock->addLiveIn(Reg);
    }

    // Insert the spill to the stack frame.
    bool IsKill = !IsRAAndRetAddrIsTaken;
    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.storeRegToStackSlot(*EntryBlock, MI, Reg, IsKill, CSI[i].getFrameIdx(), RC, TRI);
  }

  return true;
}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas,
// if it needs dynamic stack realignment, if frame pointer elimination is
// disabled, or if the frame address is taken.
bool EpiphanyFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  return (MF.getTarget().Options.DisableFramePointerElim(MF) || 
      TRI->needsStackRealignment(MF) ||
      MFI->hasVarSizedObjects() ||
      MFI->isFrameAddressTaken());
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

