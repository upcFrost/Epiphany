//=- EpiphanyMachineFuctionInfo.h - Epiphany machine function info -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares Epiphany-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef EPIPHANYMACHINEFUNCTIONINFO_H
#define EPIPHANYMACHINEFUNCTIONINFO_H

#include "EpiphanyConfig.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include <map>
#include <string>
#include <utility>

namespace llvm {

/// \brief A class derived from PseudoSourceValue that represents a GOT entry
/// resolved by lazy-binding.
class EpiphanyCallEntry : public PseudoSourceValue {
public:
  explicit EpiphanyCallEntry(StringRef N);
  explicit EpiphanyCallEntry(const GlobalValue *V);
  bool isConstant(const MachineFrameInfo *) const override;
  bool isAliased(const MachineFrameInfo *) const override;
  bool mayAlias(const MachineFrameInfo *) const override;

private:
  void printCustom(raw_ostream &O) const override;
#ifndef NDEBUG
  std::string Name;
  const GlobalValue *Val;
#endif
};

/// This class is derived from MachineFunctionInfo and contains private Epiphany
/// target-specific information for each MachineFunction.
class EpiphanyMachineFunctionInfo : public MachineFunctionInfo {
public:
  EpiphanyFunctionInfo(MachineFunction& MF)
  : MF(MF), 
    VarArgsFrameIndex(0), 
    MaxCallFrameSize(0)
    {}

  ~EpiphanyFunctionInfo();

private:
  virtual void anchor();

  MachineFunction& MF;

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex;

  unsigned MaxCallFrameSize;

  /// EpiphanyCallEntry maps.
  StringMap<std::unique_ptr<const EpiphanyCallEntry>> ExternalCallEntries;
  ValueMap<const GlobalValue *, std::unique_ptr<const EpiphanyCallEntry>>
      GlobalCallEntries;
};
//@1 }

} // End llvm namespace

#endif