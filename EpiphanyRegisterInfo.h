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

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "EpiphanyGenRegisterInfo.inc"

namespace llvm {

class EpiphanyInstrInfo;
class EpiphanySubtarget;

/// Register allocation hints.
namespace EPIPHANYRI {
  enum {
    RegPairOdd = 1,
    RegPairEven = 2
  };
}

struct EpiphanyRegisterInfo : public EpiphanyGenRegisterInfo {
private:
  const EpiphanyInstrInfo &TII;

public:
  EpiphanyRegisterInfo(const EpiphanyInstrInfo &tii,
                      const EpiphanySubtarget &sti);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF = 0) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  const uint32_t *getTLSDescCallPreservedMask() const;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  unsigned getFrameRegister(const MachineFunction &MF) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *Rs = nullptr) const override;

  /// getCrossCopyRegClass - Returns a legal register class to copy a register
  /// in the specified class to or from. Returns original class if it is
  /// possible to copy between a two registers of the specified class.
  const TargetRegisterClass *
  getCrossCopyRegClass(const TargetRegisterClass *RC) const override;

  /// getLargestLegalSuperClass - Returns the largest super class of RC that is
  /// legal to use in the current sub-target and has the same spill size.
  const TargetRegisterClass*
  getLargestLegalSuperClass(const TargetRegisterClass *RC) const {
    //if (RC == &Epiphany::tcGPR32RegClass)
    //  return &Epiphany::GPR32RegClass;

    return RC;
  }

  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool useFPForScavengingIndex(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif // LLVM_TARGET_EPIPHANYREGISTERINFO_H
