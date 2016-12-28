//===-- EpiphanyTargetStreamer.cpp - Epiphany Target Streamer Methods -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Epiphany specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "InstPrinter/EpiphanyInstPrinter.h"
#include "EpiphanyMCTargetDesc.h"
#include "EpiphanyTargetObjectFile.h"
#include "EpiphanyTargetStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

EpiphanyTargetStreamer::EpiphanyTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {
}

EpiphanyTargetAsmStreamer::EpiphanyTargetAsmStreamer(MCStreamer &S,
                                             formatted_raw_ostream &OS)
    : EpiphanyTargetStreamer(S), OS(OS) {}
