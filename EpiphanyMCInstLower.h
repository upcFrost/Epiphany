//===-- EpiphanyMCInstLower.h - Lower MachineInstr to MCInst ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANYMCINSTLOWER_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANYMCINSTLOWER_H

#include "EpiphanyConfig.h"

#include "MCTargetDesc/EpiphanyMCExpr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
  class EpiphanyAsmPrinter;
  class MCContext;
  class MCInst;
  class MCOperand;
  class MachineInstr;
  class MachineFunction;

  /// EpiphanyMCInstLower - This class is used to lower an MachineInstr
  /// into an MCInst.
class LLVM_LIBRARY_VISIBILITY EpiphanyMCInstLower {
  typedef MachineOperand::MachineOperandType MachineOperandType;
  MCContext *Ctx;
  EpiphanyAsmPrinter &AsmPrinter;
public:
  EpiphanyMCInstLower(EpiphanyAsmPrinter &asmprinter);
  void Initialize(MCContext* C);
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;
  MCOperand LowerOperand(const MachineOperand &MO, unsigned offset = 0) const;
};

} // namespace llvm

#endif
