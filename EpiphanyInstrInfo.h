//===- EpiphanyInstrInfo.h - Epiphany Instruction Information -----*- C++ -*-===//
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

#ifndef LLVM_TARGET_EPIPHANYINSTRINFO_H
#define LLVM_TARGET_EPIPHANYINSTRINFO_H

#include "EpiphanyConfig.h"

#include "Epiphany.h"
#include "EpiphanyRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "EpiphanyGenInstrInfo.inc"

namespace llvm {

class EpiphanyInstrInfo : public EpiphanyGenInstrInfo {
  virtual void anchor();
protected:
  const EpiphanyRegisterInfo RI;
  const EpiphanySubtarget &Subtarget;
public:
  explicit EpiphanyInstrInfo(const EpiphanySubtarget &STI);

  static const EpiphanyInstrInfo *create(EpiphanySubtarget &STI);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const TargetRegisterInfo &getRegisterInfo() const { return RI; }

  /// Return the number of bytes of code the specified instruction may be.
  unsigned getInstSizeInBytes(const MachineInstr &MI) const;

};

} // End of namespace llvm
#endif