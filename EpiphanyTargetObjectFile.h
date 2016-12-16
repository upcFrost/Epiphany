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

#include "EpiphanyConfig.h"

#include "EpiphanyTargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
  
  class EpiphanyTargetMachine;
  
  class EpiphanyTargetObjectFile : public TargetLoweringObjectFileELF {
    MCSection *SmallDataSection;
    MCSection *SmallBSSSection;
    const EpiphanyTargetMachine *TM;
    
   public:
    void Initialize(MCContext &Ctx, const TargetMachine &TM) override;
  };

} // end namespace llvm

#endif
