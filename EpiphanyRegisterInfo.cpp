//===- EpiphanyRegisterInfo.cpp - Epiphany Register Information -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyRegisterInfo.h"

#include "Epiphany.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyMachineFunctionInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "EpiphanyGenRegisterInfo.inc"

EpiphanyRegisterInfo::EpiphanyRegisterInfo(const EpiphanySubtarget &ST)
  : EpiphanyGenRegisterInfo(Epiphany::LR), Subtarget(ST) {}
  
//===----------------------------------------------------------------------===//
// Callee Saved Registers methods
//===----------------------------------------------------------------------===//
/// Epiphany Callee Saved Registers
// In EpiphanyCallConv.td,
// def CSR32 : CalleeSavedRegs<(add V1, V2, V3, V4, V5, SB, SL, FP, LR, R15,
//                             (sequence "R%u", 32, 43))>;
// llc create CSR32_SaveList and CSR32_RegMask from above defined.
const MCPhysReg *
EpiphanyRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR32_SaveList;
}

const uint32_t*
EpiphanyRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                           CallingConv::ID) const {
  return CSR32_RegMask;
}

// pure virtual method
BitVector EpiphanyRegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  // Stack base, limit and pointer
  Reserved.set(Epiphany::SB);
  Reserved.set(Epiphany::SL);
  Reserved.set(Epiphany::SP);
  // Frame pointer
  Reserved.set(Epiphany::FP);
  // Link register
  Reserved.set(Epiphany::LR);
  // Constants
  Reserved.set(Epiphany::R28);
  Reserved.set(Epiphany::R29);
  Reserved.set(Epiphany::R30);
  Reserved.set(Epiphany::R31);

  Reserved.set(Epiphany::STATUS);

  return Reserved;
}


//- If no eliminateFrameIndex(), it will hang on run. 
// pure virtual method
// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void EpiphanyRegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator MBBI, int SPAdj,
                    unsigned FIOperandNum, RegScavenger *RS) const {
}

bool
EpiphanyRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool
EpiphanyRegisterInfo::trackLivenessAfterRegAlloc(const MachineFunction &MF) const {
  return true;
}

unsigned
EpiphanyRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  return TFI->hasFP(MF) ? (Epiphany::FP) :
                          (Epiphany::SP);
}

const TargetRegisterClass *
EpiphanyRegisterInfo::GPR32(unsigned Size) const {
  return &Epiphany::GPR32RegClass;
}
