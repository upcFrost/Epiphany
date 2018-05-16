//===-- EpiphanyHardwareLoops.cpp - Identify and generate hardware loops --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass identifies loops where we can generate the Epiphany hardware
// loop instruction.  The hardware loop can perform loop branches with a
// zero-cycle overhead.
//
// Criteria for hardware loops:
//  - Countable loops (w/ ind. var for a trip count)
//  - Try inner-most loops first
// 
// HW loop constraints:
//  - All interrupts disabled
//  - 64-bit loop start alignment
//  - 64-bit next-to-last instruction alignment
//  - All instructions should be 32-bit wide
//  - No function calls in loops
//  - Minimum loop length of 8 instructions
//
//===----------------------------------------------------------------------===//
//
#ifndef _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYHWLOOPSPASS_H
#define _LLVM_LIB_TARGET_EPIPHANY_EPIPHANYHWLOOPSPASS_H

#include "Epiphany.h"
#include "EpiphanyConfig.h"
#include "EpiphanyMachineFunction.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include <numeric>

namespace llvm {
  void initializeEpiphanyHardwareLoopsPrePass(PassRegistry &);

  void initializeEpiphanyHardwareLoopsPostPass(PassRegistry &);

  class CountValue {
  private:
    enum CountValueType {
      CV_Register,
      CV_Immediate
    };

    CountValueType Kind;
    union Value {
      unsigned Reg;
      int64_t Imm;
    } Value;

  public:
    CountValue(const MachineOperand &MO) {
      if (MO.isReg()) {
        Value.Reg = MO.getReg();
        Kind = CV_Register;
      } else if (MO.isImm()) {
        Value.Imm = MO.getImm();
        Kind = CV_Immediate;
      } else {
        llvm_unreachable("Unknown MachineOperand type passed");
      }
    }

    bool isReg() const { return Kind == CV_Register; }

    bool isImm() const { return Kind == CV_Immediate; }

    unsigned getReg() const {
      assert(isReg() && "Wrong CountValue accessor");
      return Value.Reg;
    }

    int64_t getImm() const {
      assert(isImm() && "Wrong CountValue accessor");
      return Value.Imm;
    }

    void print(raw_ostream &OS, const TargetRegisterInfo *TRI = nullptr) const {
      if (isReg()) { OS << PrintReg(Value.Reg, TRI); }
      if (isImm()) { OS << Value.Imm; }
    }
  };

  class EpiphanyHardwareLoops {
  protected:
    const EpiphanyInstrInfo *TII;
    const TargetRegisterInfo *TRI;
    const EpiphanySubtarget *Subtarget;
    EpiphanyMachineFunctionInfo *MFF;
    MachineRegisterInfo *MRI;
    MachineFrameInfo *MFI;
    MachineLoopInfo *MLI;

  public:
    bool containsInvalidInstruction(MachineLoop *pLoop);

    bool isInvalidLoopOperation(MachineInstr &instr);

    bool lessThanEightCalls(MachineLoop *pLoop);

    bool isLoopEligible(MachineLoop *L);
  };

  class EpiphanyHardwareLoopsPre : public EpiphanyHardwareLoops, public MachineFunctionPass {
  public:
    static char ID;

    EpiphanyHardwareLoopsPre() : EpiphanyHardwareLoops(), MachineFunctionPass(ID) {
      initializeEpiphanyHardwareLoopsPrePass(*PassRegistry::getPassRegistry());
    }

    StringRef getPassName() const {
      return "Epiphany Hardware Loops Pass Pre-RA";
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<MachineDominatorTree>();
      AU.addRequired<MachineLoopInfo>();
      MachineFunctionPass::getAnalysisUsage(AU);
    }

  private:
    void findControlInstruction(MachineLoop *const &L, MachineInstr *&BranchStartMI, MachineInstr *&BranchExitMI,
                                MachineInstr *&CmpMI, MachineInstr *&BumpMI);

    void addLoopSetInstructions(MachineBasicBlock &Preheader, MachineBasicBlock::iterator &InsertPos,
                                int64_t TripCount, const DebugLoc &DL,
                                const BlockAddress *StartAddress, const BlockAddress *ExitAddress) const;

    bool findCmpValueInner(MachineInstr *CmpMI, int64_t &CmpValue);

    bool findCmpValue(MachineInstr *value, int64_t &CmpValue);

    MachineBasicBlock *findLoopStart(MachineBasicBlock *ExitingBlock, MachineBasicBlock *TopBlock, MachineLoop *L);
  };

  class EpiphanyHardwareLoopsPost : public EpiphanyHardwareLoops, public MachineFunctionPass {
  public:
    static char ID;

    EpiphanyHardwareLoopsPost() : EpiphanyHardwareLoops(), MachineFunctionPass(ID) {
      initializeEpiphanyHardwareLoopsPostPass(*PassRegistry::getPassRegistry());
    }

    StringRef getPassName() const {
      return "Epiphany Hardware Loops Pass Post-RA";
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<MachineDominatorTree>();
      AU.addRequired<MachineLoopInfo>();
      MachineFunctionPass::getAnalysisUsage(AU);
    }

  private:
    void createExitMBB(MachineFunction &MF, MachineLoop *L) const;

    void removeHardwareLoop(MachineBasicBlock *MBB);

    void cleanUpBranch(ConvertableLoopInfo &L) const;

    MachineLoop *getLoop(const ConvertableLoopInfo &L) const;
  };

}

#endif
