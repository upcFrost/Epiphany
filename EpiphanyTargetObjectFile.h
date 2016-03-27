//===-- EpiphanyTargetObjectFile.h - Epiphany Object Info ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file deals with any Epiphany specific requirements on object files.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_EPIPHANY_TARGETOBJECTFILE_H
#define LLVM_TARGET_EPIPHANY_TARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

namespace llvm {

  /// EpiphanyLinuxTargetObjectFile - This implementation is used for linux
  /// Epiphany.
  class EpiphanyLinuxTargetObjectFile : public TargetLoweringObjectFileELF {
    void Initialize(MCContext &Ctx, const TargetMachine &TM) override;
  };

} // end namespace llvm

#endif
