//===-- EpiphanyMCAsmInfo.cpp - Epiphany asm properties ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the EpiphanyMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyMCAsmInfo.h"

using namespace llvm;

void EpiphanyELFMCAsmInfo::anchor() { }

EpiphanyELFMCAsmInfo::EpiphanyELFMCAsmInfo(const Triple &TheTriple) {
  PointerSize = 32;

  // ".comm align is in bytes but .align is pow-2."
  AlignmentIsInBytes = false;
  Data16bitsDirective = "\t.hword\t";
  Data32bitsDirective = "\t.word\t";
  Data64bitsDirective = "\t.dword\t";
  PrivateGlobalPrefix = ".L";
// PrivateLabelPrefix: display $BB for the labels of basic block
  PrivateLabelPrefix = ".L";
  CommentString = "//";
  Code32Directive = ".code\t32";
  UseDataRegionDirectives = true;
  WeakRefDirective = "\t.weak\t";
  SupportsDebugInformation = true;

  // Exceptions handling
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
  DwarfRegNumForCFI = true;
}
