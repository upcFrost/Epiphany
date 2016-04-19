//===- Epiphany.td - Describe the Epiphany Target Machine -------*- tblgen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the top level entry point for the Epiphany target.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Epiphany Processors
//===----------------------------------------------------------------------===//

include "EpiphanySchedule.td"

def : Processor<"epiphany", EpiphanyGenericItineraries, []>;

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "EpiphanyRegisterInfo.td"

include "EpiphanyCallingConv.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "EpiphanyInstrInfo.td"

def EpiphanyInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

// Will generate EpiphanyGenAsmWrite.inc included by EpiphanyInstPrinter.cpp, contents
//  as follows,
// void EpiphanyInstPrinter::printInstruction(const MCInst *MI, raw_ostream &O) {...}
// const char *EpiphanyInstPrinter::getRegisterName(unsigned RegNo) {...}
def Epiphany : Target {
  let InstructionSet = EpiphanyInstrInfo;
}