//===-- Epiphany.h - Top-level interface for Epiphany representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Epiphany back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANY_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANY_H

#include "EpiphanyConfig.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class EpiphanyTargetMachine;
  class FunctionPass;

} // end namespace llvm;

#endif