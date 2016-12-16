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
  case Epiphany::RetLR:
    expandRetLR(MBB, MI);
    break;
  default:
    return false;
  }

  MBB.erase(MI);
  return true;
}
// }

//@adjustStackPtr
/// Adjusts stack pointer
void EpiphanyInstrInfo::adjustStackPtr(unsigned SP, int64_t Amount,
                                     MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I) const {
  DebugLoc DL = I != MBB.end() ? I->getDebugLoc() : DebugLoc();
  unsigned A1 = Epiphany::A1;
  unsigned ADDri = Epiphany::ADDri;
  unsigned IADDrr = Epiphany::IADDrr;
  unsigned MOV32ri = Epiphany::MOV32ri;
  unsigned MOVT32ri = Epiphany::MOVT32ri;

  if (isInt<11>(Amount)) {
    // add sp, sp, amount
    BuildMI(MBB, I, DL, get(ADDri), SP).addReg(SP).addImm(Amount);
  }
  else { // Expand immediate that doesn't fit in 16-bit.
    // Set lower 16 bits
    BuildMI(MBB, I, DL, get(MOV32ri), A1).addImm(Amount & 0xffff);
    // Set upper 16 bits
    BuildMI(MBB, I, DL, get(MOVT32ri), A1).addImm(Amount >> 16);
    // iadd sp, sp, amount
    BuildMI(MBB, I, DL, get(IADDrr), SP).addReg(SP).addReg(A1, RegState::Kill);
  }
}

void EpiphanyInstrInfo::expandRetLR(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const {
  BuildMI(MBB, I, I->getDebugLoc(), get(Epiphany::RTI)).addReg(Epiphany::LR);
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
