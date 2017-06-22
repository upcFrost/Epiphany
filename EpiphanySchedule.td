//===- EpiphanySchedule.td - Epiphany Scheduling Definitions -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Epiphany pipeline is shown below
// +---------+------+------------------------+----------------------------------------------------------------------+
// | STEP NO | STEP | NAME                   | ACTION                                                               |
// |---------|------|------------------------|----------------------------------------------------------------------|
// | 1       | FE   | Fetch Address          | Fetch addr sent to instr memory                                      |
// | 2       | IM   | Instruction Mem Access | Instr returns from core mem                                          |
// | 3       | DE   | Decode                 | Decode instr                                                         |
// | 4       | RA   | Reg Access             | Read operands from regs                                              |
// | 5       | E1   | Execution              | Load/store address calc Reg read for store Status flag set Branching |
// | 6       | E2   | Execution              | Load complete                                                        |
// | 7       | E3   | Execution              | FP result written in trunc mode                                      |
// | 8       | E4   | Execution              | FP result in round-to-near mode                                      |
// +---------+------+------------------------+----------------------------------------------------------------------+
//
// Epiphany can dual-issue when FPU/IALU2 is used.
// In addition, it might be a good idea to group stores and loads to widen them.
//

//===----------------------------------------------------------------------===//
// Functional units across Epiphany chips sets
//===----------------------------------------------------------------------===//
// IALU units
def FE : FuncUnit;
def IM : FuncUnit;
def DE : FuncUnit;
def RA : FuncUnit;
def E1 : FuncUnit;
def E2 : FuncUnit;
def E3 : FuncUnit;
def E4 : FuncUnit;

// IALU2/FPU units
def FE_2 : FuncUnit;
def IM_2 : FuncUnit;
def DE_2 : FuncUnit;
def RA_2 : FuncUnit;
def E1_2 : FuncUnit;
def E2_2 : FuncUnit;
def E3_2 : FuncUnit;
def E4_2 : FuncUnit;

// LoadStore grouping bypass
def LoadStoreBypass : Bypass;
// IALU2 bypass
def IaluBypass : Bypass;

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

def EpiphanyGenericItineraries : ProcessorItineraries<[FE, IM, DE, RA, E1, E2, E3, E4, FE_2, IM_2, DE_2, RA_2, E1_2, E2_2, E3_2, E4_2], [], [
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
  InstrItinData<Ialu2Itin   , [InstrStage<1, [FE_2]>, 
                               InstrStage<1, [IM_2]>,
                               InstrStage<1, [DE_2]>, 
                               InstrStage<1, [RA_2]>, 
                               InstrStage<1, [E1_2]>],
                               [4, 3]>,

  // FPU instructions take steps FE-E4, 1 cycle per step
  // Read on cycle 3, result at cycle 7 (after issue)
  InstrItinData<FpuItin     , [InstrStage<1, [FE_2]>, 
                               InstrStage<1, [IM_2]>,
                               InstrStage<1, [DE_2]>, 
                               InstrStage<1, [RA_2]>, 
                               InstrStage<1, [E1_2]>, 
                               InstrStage<1, [E2_2]>, 
                               InstrStage<1, [E3_2]>, 
                               InstrStage<1, [E4_2]>],
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

def EpiphanyModel : SchedMachineModel {
  let IssueWidth = 2; // At max we can dual-issue
  let Itineraries = EpiphanyGenericItineraries;
  let LoadLatency = 2;
  let CompleteModel = 0;
  let MispredictPenalty = 0;
  let PostRAScheduler = 1;
}
