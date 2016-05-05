//==-- EpiphanyMCAsmInfo.h - Epiphany asm properties -------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the EpiphanyMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANYTARGETASMINFO_H
#define LLVM_EPIPHANYTARGETASMINFO_H

#include "EpiphanyConfig.h"

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
  class Triple;
  
  class EpiphanyELFMCAsmInfo : public MCAsmInfoELF {
    void anchor() override;
  public:
    explicit EpiphanyELFMCAsmInfo(const Triple &TheTriple);
  };

} // namespace llvm

#endif