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
  case EpiphanyISD::Ret:            return "EpiphanyISD::Ret";
  case EpiphanyISD::SELECT_CC:      return "EpiphanyISD::SELECT_CC";
  case EpiphanyISD::SETCC:          return "EpiphanyISD::SETCC";
  case EpiphanyISD::WrapperSmall:   return "EpiphanyISD::WrapperSmall";
  case EpiphanyISD::FM_A_S:         return "EpiphanyISD::FM_A_S";

  default:                          return NULL;
  }
}

//@EpiphanyTargetLowering {
EpiphanyTargetLowering::EpiphanyTargetLowering(const TargetMachine &TM,
                                               const EpiphanySubtarget &STI)
    : TargetLowering(TM), Subtarget(&STI), ABI(TM.getABI()) {

  // Set up the register classes
  addRegisterClass(MVT::i32, &Epiphany::GPR32);
  
  // must, computeRegisterProperties - Once all of the register classes are 
  //  added, this allows us to compute derived properties we expose.
  computeRegisterProperties(Subtarget.getRegisterInfo());
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
                                         SDLoc DL, SelectionDAG &DAG,
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
                                SDLoc DL, SelectionDAG &DAG) const {
  return DAG.getNode(Epiphany::Ret, DL, MVT::Other,
                     Chain, DAG.getRegister(Epiphany::LR, MVT::i32));
}


