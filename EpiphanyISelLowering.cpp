//===-- EpiphanyISelLowering.cpp - Epiphany DAG Lowering Implementation -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Epiphany uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#include "EpiphanyISelLowering.h"

#include "EpiphanyMachineFunction.h"
#include "EpiphanyTargetMachine.h"
#include "EpiphanyTargetObjectFile.h"
#include "EpiphanySubtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-lower"

const char *EpiphanyTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
    case EpiphanyISD::BR_CC:          return "EpiphanyISD::BR_CC";
    case EpiphanyISD::Call:           return "EpiphanyISD::Call";
    case EpiphanyISD::RTI:            return "EpiphanyISD::RTI";
    case EpiphanyISD::SELECT_CC:      return "EpiphanyISD::SELECT_CC";
    case EpiphanyISD::SETCC:          return "EpiphanyISD::SETCC";
    case EpiphanyISD::WrapperSmall:   return "EpiphanyISD::WrapperSmall";
    case EpiphanyISD::FM_A_S:         return "EpiphanyISD::FM_A_S";

    default:                          return NULL;
  }
}

//@EpiphanyTargetLowering {
EpiphanyTargetLowering::EpiphanyTargetLowering(const EpiphanyTargetMachine &TM,
    const EpiphanySubtarget &STI)
  : TargetLowering(TM), Subtarget(STI), ABI(TM.getABI()) {

    // Set up the register classes
    addRegisterClass(MVT::i32, &Epiphany::GPR32RegClass);

    //- Set .align 4
    // It will emit .align 4 later
    setMinFunctionAlignment(4);

    // must, computeRegisterProperties - Once all of the register classes are 
    //  added, this allows us to compute derived properties we expose.
    computeRegisterProperties(STI.getRegisterInfo());
  }

SDValue EpiphanyTargetLowering::LowerOperation(SDValue Op,
    SelectionDAG &DAG) const {
  return EpiphanyTargetLowering::LowerOperation(Op, DAG);
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

#include "EpiphanyGenCallingConv.inc"

//===----------------------------------------------------------------------===//
//@            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//
//@LowerFormalArguments {
/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue
EpiphanyTargetLowering::LowerFormalArguments(SDValue Chain,
    CallingConv::ID CallConv,
    bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    const SDLoc &DL, SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals) const {
  return Chain;
}
// @LowerFormalArguments }

//===----------------------------------------------------------------------===//
//@              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue
EpiphanyTargetLowering::LowerReturn(SDValue Chain,
    CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    const SDLoc &DL, SelectionDAG &DAG) const {

  // CCValAssign - represent the assignment of
  // the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;
  MachineFunction &MF = DAG.getMachineFunction();

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs,
      *DAG.getContext());
  EpiphanyCC EpiphanyCCInfo(CallConv, ABI.IsE16(), 
      CCInfo);

  // Analyze return values.
  EpiphanyCCInfo.analyzeReturn(Outs, Subtarget.abiUsesSoftFloat(),
      MF.getFunction()->getReturnType());

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    SDValue Val = OutVals[i];
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    if (RVLocs[i].getValVT() != RVLocs[i].getLocVT())
      Val = DAG.getNode(ISD::BITCAST, DL, RVLocs[i].getLocVT(), Val);

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Flag);

    // Guarantee that all emitted copies are stuck together with flags.
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  //@Ordinary struct type: 2 {
  // The epiphany ABIs for returning structs by value requires that we copy
  // the sret argument into $v0 for the return. We saved the argument into
  // a virtual register in the entry block, so now we copy the value out
  // and into $v0.
  if (MF.getFunction()->hasStructRetAttr()) {
    EpiphanyMachineFunctionInfo *MFI = MF.getInfo<EpiphanyMachineFunctionInfo>();
    unsigned Reg = MFI->getSRetReturnReg();

    if (!Reg)
      llvm_unreachable("sret virtual register not created in the entry block");
    SDValue Val =
      DAG.getCopyFromReg(Chain, DL, Reg, getPointerTy(DAG.getDataLayout()));
    unsigned A1 = Epiphany::A1;

    Chain = DAG.getCopyToReg(Chain, DL, A1, Val, Flag);
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(A1, getPointerTy(DAG.getDataLayout())));
  }
  //@Ordinary struct type: 2 }

  RetOps[0] = Chain;  // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  return DAG.getNode(EpiphanyISD::RTI, DL, MVT::Other,
      Chain, DAG.getRegister(Epiphany::LR, MVT::i32));
}

EpiphanyTargetLowering::EpiphanyCC::EpiphanyCC(
    CallingConv::ID CC, bool IsE16_, CCState &Info,
    EpiphanyCC::SpecialCallingConvType SpecialCallingConv_)
  : CCInfo(Info), CallConv(CC), IsE16(IsE16_) {
    // Pre-allocate reserved argument area.
    CCInfo.AllocateStack(reservedArgArea(), 1);
  }

template<typename Ty>
void EpiphanyTargetLowering::EpiphanyCC::
analyzeReturn(const SmallVectorImpl<Ty> &RetVals, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  CCAssignFn *Fn;

  Fn = RetCC_Epiphany;

  for (unsigned I = 0, E = RetVals.size(); I < E; ++I) {
    MVT VT = RetVals[I].VT;
    ISD::ArgFlagsTy Flags = RetVals[I].Flags;
    MVT RegVT = this->getRegVT(VT, RetTy, CallNode, IsSoftFloat);

    if (Fn(I, VT, RegVT, CCValAssign::Full, Flags, this->CCInfo)) {
#ifndef NDEBUG
      dbgs() << "Call result #" << I << " has unhandled type "
        << EVT(VT).getEVTString() << '\n';
#endif
      llvm_unreachable(nullptr);
    }
  }
}

void EpiphanyTargetLowering::EpiphanyCC::
analyzeCallResult(const SmallVectorImpl<ISD::InputArg> &Ins, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  analyzeReturn(Ins, IsSoftFloat, CallNode, RetTy);
}

void EpiphanyTargetLowering::EpiphanyCC::
analyzeReturn(const SmallVectorImpl<ISD::OutputArg> &Outs, bool IsSoftFloat,
    const Type *RetTy) const {
  analyzeReturn(Outs, IsSoftFloat, nullptr, RetTy);
}

unsigned EpiphanyTargetLowering::EpiphanyCC::reservedArgArea() const {
  return (IsE16 && (CallConv != CallingConv::Fast)) ? 8 : 0;
}

MVT EpiphanyTargetLowering::EpiphanyCC::getRegVT(MVT VT, const Type *OrigTy,
    const SDNode *CallNode,
    bool IsSoftFloat) const {
  if (IsSoftFloat || IsE16)
    return VT;

  return VT;
}


