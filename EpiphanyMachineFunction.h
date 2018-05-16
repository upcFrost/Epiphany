//=- EpiphanyMachineFuctionInfo.h - Epiphany machine function info -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares Epiphany-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef EPIPHANYMACHINEFUNCTIONINFO_H
#define EPIPHANYMACHINEFUNCTIONINFO_H

#include "EpiphanyConfig.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include <map>
#include <string>
#include <utility>

namespace llvm {

  class ConvertableLoopInfo {
  public:
    ConvertableLoopInfo(MachineInstr *StartMI, MachineInstr *EndMI, MachineInstr *BranchExitMI,
                    MachineInstr *BranchForwardMI, MachineInstr *CmpMI)
        : SetLoopStart(StartMI),
          SetLoopEnd(EndMI),
          BranchExitInstr(BranchExitMI),
          BranchForwardInstr(BranchForwardMI),
          CompareInstr(CmpMI) {}

    MachineInstr *SetLoopStart;
    MachineInstr *SetLoopEnd;
    MachineInstr *BranchExitInstr;
    MachineInstr *BranchForwardInstr;
    MachineInstr *CompareInstr;
  };

/// This class is derived from MachineFunctionInfo and contains private Epiphany
/// target-specific information for each MachineFunction.
  class EpiphanyMachineFunctionInfo : public MachineFunctionInfo {
  public:
    EpiphanyMachineFunctionInfo(MachineFunction &MF)
        : MF(MF),
          VarArgsFrameIndex(0),
          SRetReturnReg(0),
          MaxCallFrameSize(0),
          CallsEhReturn(false),
          CallsEhDwarf(false),
          GlobalBaseReg(0),
          EmitNOAT(false),
          HasFpuInst(false),
          HasIalu2Inst(false),
          ConvertableLoopsInfo() {}

    ~EpiphanyMachineFunctionInfo();

    bool getEmitNOAT() const { return EmitNOAT; }

    void setEmitNOAT() { EmitNOAT = true; }

    unsigned getSRetReturnReg() const { return SRetReturnReg; }

    void setSRetReturnReg(unsigned Reg) { SRetReturnReg = Reg; }

    bool hasByvalArg() const { return HasByvalArg; }

    void setFormalArgInfo(unsigned Size, bool HasByval) {
      IncomingArgSize = Size;
      HasByvalArg = HasByval;
    }

    unsigned getIncomingArgSize() const { return IncomingArgSize; }

    unsigned getGlobalBaseReg();

    bool callsEhReturn() const { return CallsEhReturn; }

    void setCallsEhReturn() { CallsEhReturn = true; }

    bool callsEhDwarf() const { return CallsEhDwarf; }

    void setCallsEhDwarf() { CallsEhDwarf = true; }

    void createEhDataRegsFI();

    int getEhDataRegFI(unsigned Reg) const { return EhDataRegFI[Reg]; }

    unsigned getMaxCallFrameSize() const { return MaxCallFrameSize; }

    void setMaxCallFrameSize(unsigned S) { MaxCallFrameSize = S; }

    void setHasFpuInst(bool hasInst) { HasFpuInst = hasInst; }

    bool getHasFpuInst() { return HasFpuInst; }

    void setHasIalu2Inst(bool hasInst) { HasIalu2Inst = hasInst; }

    bool getHasIalu2Inst() { return HasFpuInst; }

    std::vector<ConvertableLoopInfo> &getConvertableLoopsInfo() { return ConvertableLoopsInfo; }

  private:
    virtual void anchor();

    MachineFunction &MF;

    /// VarArgsFrameIndex - FrameIndex for start of varargs area.
    int VarArgsFrameIndex;

    /// SRetReturnReg - Some subtargets require that sret lowering includes
    /// returning the value of the returned struct in a register. This field
    /// holds the virtual register into which the sret argument is passed.
    unsigned SRetReturnReg;

    unsigned MaxCallFrameSize;

    /// True if function has a byval argument.
    bool HasByvalArg;

    /// Size of incoming argument area.
    unsigned IncomingArgSize;

    /// CallsEhReturn - Whether the function calls llvm.eh.return.
    bool CallsEhReturn;

    /// CallsEhDwarf - Whether the function calls llvm.eh.dwarf.
    bool CallsEhDwarf;

    /// Frame objects for spilling eh data registers.
    int EhDataRegFI[2];

    /// Global base reg virtual register
    unsigned GlobalBaseReg;

    bool EmitNOAT;

    // Boolean flags to use in custom optimization passes
    bool HasFpuInst;
    bool HasIalu2Inst;

    // Possible hardware loops storage
    std::vector<ConvertableLoopInfo> ConvertableLoopsInfo;
  };
//@1 }

} // End llvm namespace

#endif
