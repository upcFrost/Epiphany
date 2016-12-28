//===-- EpiphanyTargetStreamer.h - Epiphany Target Streamer ------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EPIPHANY_EPIPHANYTARGETSTREAMER_H
#define LLVM_LIB_TARGET_EPIPHANY_EPIPHANYTARGETSTREAMER_H

#include "EpiphanyConfig.h"

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {

class EpiphanyTargetStreamer : public MCTargetStreamer {
public:
  EpiphanyTargetStreamer(MCStreamer &S);
};

// This part is for ascii assembly output
class EpiphanyTargetAsmStreamer : public EpiphanyTargetStreamer {
  formatted_raw_ostream &OS;

public:
  EpiphanyTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
};

}

#endif

