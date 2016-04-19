//===- EpiphanySchedule.td - Epiphany Scheduling Definitions -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Functional units across Epiphany chips sets
//===----------------------------------------------------------------------===//
def ALU     : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for Epiphany (p58, epiphany_arch_ref.pdf)
//===----------------------------------------------------------------------===//
def IALU      : InstrItinClass;
def IALU2     : InstrItinClass;
def FPU       : InstrItinClass;
def LOADSTORE : InstrItinClass;
def CONTROL   : InstrItinClass;

def EpiphanyGenericItineraries : ProcessorItineraries<[ALU], [], [
  InstrItinData<IALU        , [InstrStage<1,  [ALU]>]>,
  InstrItinData<IALU2       , [InstrStage<1,  [ALU]>]>,
  InstrItinData<FPU         , [InstrStage<1,  [ALU]>]>,
  InstrItinData<LOADSTORE   , [InstrStage<1,  [ALU]>]>,
  InstrItinData<CONTROL     , [InstrStage<1,  [ALU]>]>
]>;
