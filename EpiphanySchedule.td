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
def FE : FuncUnit;
def IM : FuncUnit;
def DE : FuncUnit;
def RA : FuncUnit;
def E1 : FuncUnit;
def E2 : FuncUnit;
def E3 : FuncUnit;
def E4 : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for Epiphany (p58, epiphany_arch_ref.pdf)
//===----------------------------------------------------------------------===//
def IaluItin    : InstrItinClass;
def Ialu2Itin   : InstrItinClass;
def FpuItin     : InstrItinClass;
def LoadItin    : InstrItinClass;
def StoreItin   : InstrItinClass;
def ControlItin : InstrItinClass;
def BranchItin  : InstrItinClass;

def EpiphanyGenericItineraries : ProcessorItineraries<[FE, IM, DE, RA, E1, E2, E3, E4], [], [
  // IALU instructions take steps FE-E1, 1 cycle per step
  // Read on cycle 3, result at cycle 4 (after issue)
  InstrItinData<IaluItin    , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>],
                               [4, 3]>,

  // IALU2 instructions take steps FE-E1, 1 cycle per step
  // Read on cycle 3, result at cycle 4 (after issue)
  InstrItinData<Ialu2Itin   , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>],
                               [4, 3]>,

  // FPU instructions take steps FE-E4, 1 cycle per step
  // Read on cycle 3, result at cycle 7 (after issue)
  InstrItinData<FpuItin     , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>, 
                               InstrStage<1, [E2]>, 
                               InstrStage<1, [E3]>, 
                               InstrStage<1, [E4]>],
                               [7, 3]>,

  // LOAD instructions take steps FE-E2, 1 cycle per step 
  // Read on cycle 3, result at cycle 6 (after issue)
  InstrItinData<LoadItin    , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>,
                               InstrStage<1, [E2]>],
                               [6, 3]>,

  // STORE instructions take steps FE-E1, 1 cycle per step
  // Read on cycle 3, result at cycle 4 (after issue)
  InstrItinData<StoreItin   , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>],
                               [4, 3]>,

  // CONTROL instructions take steps FE-E1, 1 cycle per step
  // Read on cycle 3, result at cycle 4 (after issue)
  InstrItinData<ControlItin , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<1, [E1]>],
                               [4, 3]>,

  // BRANCH instructions take steps FE-E1, 1 cycle per step plus 3 cycle fixed penalty in worst case
  // Read on cycle 3, result at cycle 4 (after issue)
  InstrItinData<BranchItin  , [InstrStage<1, [FE]>, 
                               InstrStage<1, [IM]>,
                               InstrStage<1, [DE]>, 
                               InstrStage<1, [RA]>, 
                               InstrStage<4, [E1]>],
                               [4, 3]>
]>;
