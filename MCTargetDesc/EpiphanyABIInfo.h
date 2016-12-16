//===---- EpiphanyABIInfo.h - Information about Epiphany ABI's --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Epiphany_MCTARGETDESC_EpiphanyABIINFO_H
#define LLVM_LIB_TARGET_Epiphany_MCTARGETDESC_EpiphanyABIINFO_H

#include "EpiphanyConfig.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/MC/MCRegisterInfo.h"

namespace llvm {

class MCTargetOptions;
class StringRef;
class TargetRegisterClass;

class EpiphanyABIInfo {
public:
  enum class ABI { Unknown, E16 };

protected:
  ABI ThisABI;

public:
  EpiphanyABIInfo(ABI ThisABI) : ThisABI(ThisABI) {}

  static EpiphanyABIInfo Unknown() { return EpiphanyABIInfo(ABI::Unknown); }
  static EpiphanyABIInfo E16() { return EpiphanyABIInfo(ABI::E16); }
  static EpiphanyABIInfo computeTargetABI();

  bool IsKnown() const { return ThisABI != ABI::Unknown; }
  bool IsE16() const { return ThisABI == ABI::E16; }
  ABI GetEnumValue() const { return ThisABI; }

  /// The registers to use for byval arguments.
  const ArrayRef<MCPhysReg> GetByValArgRegs() const;

  /// The registers to use for the variable argument list.
  const ArrayRef<MCPhysReg> GetVarArgRegs() const;

  /// Obtain the size of the area allocated by the callee for arguments.
  unsigned GetCalleeAllocdArgSizeInBytes(CallingConv::ID CC) const;

  /// Ordering of ABI's
  /// EpiphanyGenSubtargetInfo.inc will use this to resolve conflicts when given
  /// multiple ABI options.
  bool operator<(const EpiphanyABIInfo Other) const {
    return ThisABI < Other.GetEnumValue();
  }

  unsigned GetStackPtr() const;
  unsigned GetFramePtr() const;
  unsigned GetNullPtr() const;

  unsigned GetEhDataReg(unsigned I) const;
  int EhDataRegSize() const;
};

} // namespace llvm

#endif
