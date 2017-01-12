//==- EpiphanyRegisterInfo.h - Epiphany Register Information Impl -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the MCRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_EPIPHANYREGISTERINFO_H
#define LLVM_TARGET_EPIPHANYREGISTERINFO_H

#include "EpiphanyConfig.h"

#include "Epiphany.h"
#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "EpiphanyGenRegisterInfo.inc"

namespace llvm {

class EpiphanySubtarget;
class EpiphanyInstrInfo;
class Type;

struct EpiphanyRegisterInfo : public EpiphanyGenRegisterInfo {
protected:
  const EpiphanySubtarget &Subtarget;

public:
  EpiphanyRegisterInfo(const EpiphanySubtarget &Subtarget);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  
  bool requiresRegisterScavenging(const MachineFunction &MF) const override;
  
  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *Rs = nullptr) const override;

  unsigned getFrameRegister(const MachineFunction &MF) const override;
  unsigned getBaseRegister() const;
  bool hasBasePointer(const MachineFunction &MF) const;

  const TargetRegisterClass *GPR16(unsigned Size) const;
  const TargetRegisterClass *GPR32(unsigned Size) const;
};

} // end namespace llvm

#endif // LLVM_TARGET_EPIPHANYREGISTERINFO_H
