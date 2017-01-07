//===- EpiphanyInstrInfo.cpp - Epiphany Instruction Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyInstrInfo.h"

#include "MCTargetDesc/EpiphanyAddressingModes.h"
#include "EpiphanyTargetMachine.h"
#include "EpiphanyMachineFunction.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "EpiphanyGenInstrInfo.inc"

// Pin the vtable to this file.
void EpiphanyInstrInfo::anchor() {}

//@EpiphanyInstrInfo {
EpiphanyInstrInfo::EpiphanyInstrInfo(const EpiphanySubtarget &STI)
  : Subtarget(STI), RI(STI) {}

  const EpiphanyRegisterInfo &EpiphanyInstrInfo::getRegisterInfo() const {
    return RI;
  }

//@expandPostRAPseudo
/// Expand Pseudo instructions into real backend instructions
bool EpiphanyInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  //@expandPostRAPseudo-body
  MachineBasicBlock &MBB = *MI.getParent();

  switch (MI.getDesc().getOpcode()) {
    case Epiphany::RTS:
      expandRTS(MBB, MI);
      break;
    default:
      return false;
  }

  MBB.erase(MI);
  return true;
}
// }

void EpiphanyInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned SrcReg, bool KillSrc, int FrameIdx,
    const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  // Get instruction, only STRi32 for now
  unsigned STRi32_r16 = Epiphany::STRi32_r16;
  unsigned STRi32_r32 = Epiphany::STRi32_r32;

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();

  // Get mem operand where to store
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOStore, 
      MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  if (Rd == &Epiphany::GPR16RegClass) {
    BuildMI(MBB, MI, DL, get(STRi32_r16)).addFrameIndex(FrameIdx).addImm(0)
      .addReg(SrcReg, getKillRegState(KillSrc)).addMemOperand(MMO);
  } else if (Rd == &Epiphany::GPR32RegClass) {
    BuildMI(MBB, MI, DL, get(STRi32_r32)).addFrameIndex(FrameIdx).addImm(0)
      .addReg(SrcReg, getKillRegState(KillSrc)).addMemOperand(MMO);
  } else {
    llvm_unreachable("Cannot store this register to stack slot!");
  }
}

void EpiphanyInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned DestReg, int FrameIdx,
    const TargetRegisterClass *Rd, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  // Get instruction, only LDRi32 for now
  unsigned LDRi32_r16 = Epiphany::LDRi32_r16;
  unsigned LDRi32_r32 = Epiphany::LDRi32_r32;

  // Get function and frame info
  if (MI != MBB.end()) DL = MI->getDebugLoc();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = *MF.getFrameInfo();

  // Get mem operand from where to load
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlignment(FrameIdx));

  // Build instruction
  if (Rd == &Epiphany::GPR16RegClass) {
    BuildMI(MBB, MI, DL, get(LDRi32_r16)).addReg(DestReg, getDefRegState(true))
      .addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
  } else if (Rd == &Epiphany::GPR32RegClass) {
    BuildMI(MBB, MI, DL, get(LDRi32_r32)).addReg(DestReg, getDefRegState(true))
      .addFrameIndex(FrameIdx).addImm(0).addMemOperand(MMO);
  } else {
    llvm_unreachable("Cannot store this register to stack slot!");
  }
}

void EpiphanyInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    const DebugLoc &DL, unsigned DestReg,
    unsigned SrcReg, bool KillSrc) const {
  unsigned Opc = 0;

  // TODO: Should make it work for all 4 ways (i32 <-> f32)
  if (Epiphany::GPR32RegClass.contains(DestReg, SrcReg)) { // Copy between regs
    Opc = Epiphany::MOVi32rr;
  } else if (Epiphany::FPR32RegClass.contains(DestReg, SrcReg)) {
    Opc = Epiphany::MOVf32rr;
  }
  assert(Opc && "Cannot copy registers");

  BuildMI(MBB, I, DL, get(Opc), DestReg).addReg(SrcReg, getKillRegState(KillSrc));
}

//@adjustStackPtr
/// Adjusts stack pointer
void EpiphanyInstrInfo::adjustStackPtr(unsigned SP, int64_t Amount,
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned A1 = Epiphany::A1;
  unsigned ADD16ri = Epiphany::ADD16ri;
  unsigned ADD32ri = Epiphany::ADD32ri;
  unsigned IADDrr = Epiphany::IADDrr;
  unsigned MOVi32ri = Epiphany::MOVi32ri;
  unsigned MOVTi32ri = Epiphany::MOVTi32ri;

  if (isInt<11>(Amount)) {
    // add sp, sp, amount
    BuildMI(MBB, I, DL, get(ADD32ri), SP).addReg(SP).addImm(Amount);
  } else { // Expand immediate that doesn't fit in 11-bit.
    // Set lower 16 bits
    BuildMI(MBB, I, DL, get(MOVi32ri), A1).addImm(Amount & 0xffff);
    // Set upper 16 bits
    BuildMI(MBB, I, DL, get(MOVTi32ri), A1).addReg(A1).addImm(Amount >> 16);
    // iadd sp, sp, amount
    BuildMI(MBB, I, DL, get(IADDrr), SP).addReg(SP).addReg(A1, RegState::Kill);
  }
}

void EpiphanyInstrInfo::expandRTS(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  BuildMI(MBB, I, I->getDebugLoc(), get(Epiphany::JR32)).addReg(Epiphany::LR);
}
// }

// Return the number of bytes of code the specified instruction may be.
unsigned EpiphanyInstrInfo::GetInstSizeInBytes(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
    default:
      return MI.getDesc().getSize();
  }
}
// }
