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

#include "MCTargetDesc/EpiphanyBaseInfo.h"
#include "MCTargetDesc/EpiphanyAddressingModes.h"
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

static cl::opt<bool> EnableFastMath("ffast-math",
    cl::Hidden, cl::ZeroOrMore, cl::init(false),
    cl::desc("Enable Fast Math processing"));


const char *EpiphanyTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
    case EpiphanyISD::Call:           return "EpiphanyISD::Call";
    case EpiphanyISD::RTI:            return "EpiphanyISD::RTI";
    case EpiphanyISD::RTS:            return "EpiphanyISD::RTS";
    case EpiphanyISD::MOV:            return "EpiphanyISD::MOV";
    case EpiphanyISD::MOVT:           return "EpiphanyISD::MOVT";
    case EpiphanyISD::MOVCC:          return "EpiphanyISD::MOVCC";
    case EpiphanyISD::STORE:          return "EpiphanyISD::STORE";
    case EpiphanyISD::LOAD:           return "EpiphanyISD::LOAD";
    case EpiphanyISD::CMP:            return "EpiphanyISD::CMP";
    case EpiphanyISD::BRCC:           return "EpiphanyISD::BRCC";
    case EpiphanyISD::FIX:            return "EpiphanyISD::FIX";
    case EpiphanyISD::FLOAT:          return "EpiphanyISD::FLOAT";

    default:                          return NULL;
  }
}

//@EpiphanyTargetLowering {
EpiphanyTargetLowering::EpiphanyTargetLowering(const EpiphanyTargetMachine &TM,
    const EpiphanySubtarget &STI)
  : TargetLowering(TM), Subtarget(STI), ABI(TM.getABI()) {

    // Set up the register classes
    addRegisterClass(MVT::i32, &Epiphany::GPR16RegClass);
    addRegisterClass(MVT::i32, &Epiphany::GPR32RegClass);
    addRegisterClass(MVT::f32, &Epiphany::FPR32RegClass);
    addRegisterClass(MVT::i64, &Epiphany::GPR64RegClass);
    addRegisterClass(MVT::f64, &Epiphany::FPR64RegClass);

    //- Set .align 2
    // It will emit .align 2 later
    setMinFunctionAlignment(STI.stackAlignment());

    // Set boolean to i32 for now (as we don't have i1)
    setBooleanContents(ZeroOrOneBooleanContent);
    setBooleanVectorContents(ZeroOrNegativeOneBooleanContent);

    // must, computeRegisterProperties - Once all of the register classes are 
    //  added, this allows us to compute derived properties we expose.
    computeRegisterProperties(STI.getRegisterInfo());

    // Provide all sorts of operation actions
    setStackPointerRegisterToSaveRestore(Epiphany::SP);

    // Provide ops that we don't have
    setOperationAction(ISD::SDIV,       MVT::i32,  Expand);
    setOperationAction(ISD::SREM,       MVT::i32,  Expand);
    setOperationAction(ISD::UDIV,       MVT::i32,  Expand);
    setOperationAction(ISD::UREM,       MVT::i32,  Expand);
    setOperationAction(ISD::SDIVREM,    MVT::i32,  Expand);
    setOperationAction(ISD::UDIVREM,    MVT::i32,  Expand);
    setOperationAction(ISD::MULHS,      MVT::i32,  Expand);
    setOperationAction(ISD::MULHU,      MVT::i32,  Expand);
    setOperationAction(ISD::UMUL_LOHI,  MVT::i32,  Expand);
    setOperationAction(ISD::SMUL_LOHI,  MVT::i32,  Expand);

    for (MVT VT : MVT::fp_valuetypes()) {
      setOperationAction(ISD::FDIV,  VT,  Expand);
      setOperationAction(ISD::FSQRT, VT,  Expand);
      setOperationAction(ISD::FSIN,  VT,  Expand);
      setOperationAction(ISD::FCOS,  VT,  Expand);
      setOperationAction(ISD::FLOG,  VT,  Expand);
      setOperationAction(ISD::FEXP,  VT,  Expand);
      setOperationAction(ISD::FPOW,  VT,  Expand);
      setOperationAction(ISD::FREM,  VT,  Expand);
    }

    // Turn FP truncstore into trunc + store.
    setTruncStoreAction(MVT::f64, MVT::f32, Expand);
    setTruncStoreAction(MVT::i64, MVT::i32, Expand);

    // We don't have conversion from i32/i64 to f64
    setLoadExtAction(ISD::EXTLOAD,  MVT::i64, MVT::i32, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i64, MVT::i32, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i64, MVT::i32, Expand);
    setLoadExtAction(ISD::EXTLOAD,  MVT::f32, MVT::i32, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::f32, MVT::i32, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::f32, MVT::i32, Expand);
    setLoadExtAction(ISD::EXTLOAD,  MVT::f32, MVT::i64, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::f32, MVT::i64, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::f32, MVT::i64, Expand);
    setLoadExtAction(ISD::EXTLOAD,  MVT::f64, MVT::f32, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::f64, MVT::f32, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::f64, MVT::f32, Expand);
    setLoadExtAction(ISD::EXTLOAD,  MVT::f64, MVT::i32, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::f64, MVT::i32, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::f64, MVT::i32, Expand);
    setLoadExtAction(ISD::EXTLOAD,  MVT::f64, MVT::i64, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::f64, MVT::i64, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::f64, MVT::i64, Expand);

    setOperationAction(ISD::EXTLOAD,  MVT::f64, Expand);
    setOperationAction(ISD::SEXTLOAD, MVT::f64, Expand);
    setOperationAction(ISD::ZEXTLOAD, MVT::f64, Expand);

    // For now - expand i64 ops that were not implemented yet
    setOperationAction(ISD::MUL,       MVT::i64,  Expand);
    setOperationAction(ISD::SMUL_LOHI, MVT::i64,  Expand);
    setOperationAction(ISD::UMUL_LOHI, MVT::i64,  Expand);
    setOperationAction(ISD::SDIV,      MVT::i64,  Expand);
    setOperationAction(ISD::SREM,      MVT::i64,  Expand);
    setOperationAction(ISD::UDIV,      MVT::i64,  Expand);
    setOperationAction(ISD::UREM,      MVT::i64,  Expand);
    setOperationAction(ISD::SDIVREM,   MVT::i64,  Expand);
    setOperationAction(ISD::UDIVREM,   MVT::i64,  Expand);

    // Same for f64
    setOperationAction(ISD::FADD,       MVT::f64,  Expand);
    setOperationAction(ISD::FSUB,       MVT::f64,  Expand);
    setOperationAction(ISD::FMUL,       MVT::f64,  Expand);
    setOperationAction(ISD::FDIV,       MVT::f64,  Expand);
    setOperationAction(ISD::SELECT,     MVT::f64,  Expand);
    setOperationAction(ISD::FP_ROUND,   MVT::f64,  Expand);

    // Custom operations, see below
    setOperationAction(ISD::GlobalAddress,  MVT::i32, Custom);
    setOperationAction(ISD::ExternalSymbol, MVT::i32, Custom);
    setOperationAction(ISD::ConstantPool,   MVT::i32, Custom);

    for (MVT Ty : {MVT::i32, MVT::f32, MVT::i64, MVT::f64}) {
      setOperationAction(ISD::BR_CC,  Ty, Custom);
      setOperationAction(ISD::SETCC,  Ty, Custom);
      setOperationAction(ISD::SELECT, Ty, Custom);
    }
    setOperationAction(ISD::BRCOND,    MVT::i32, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
    setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
    setOperationAction(ISD::FP_EXTEND, MVT::f64, Custom);

        // Just expand all conversions, as they're getting on the nerves
    for (MVT VT : MVT::all_valuetypes()) {
      setOperationAction(ISD::FP_TO_UINT, VT, Custom);
      setOperationAction(ISD::FP_TO_SINT, VT, Custom);
      setOperationAction(ISD::UINT_TO_FP, VT, Custom);
      setOperationAction(ISD::SINT_TO_FP, VT, Custom);
    }

    // Libraries for fast math
    if (EnableFastMath) {
      setLibcallName(RTLIB::DIV_F32, "__fast_recipsf2");
      setOperationAction(ISD::FDIV, MVT::f32, Custom);
    }
  }

SDValue EpiphanyTargetLowering::LowerOperation(SDValue Op,
    SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
    case ISD::GlobalAddress:
      return LowerGlobalAddress(Op, DAG);
      break;
    case ISD::ExternalSymbol:
      return LowerExternalSymbol(Op, DAG);
      break;
    case ISD::ConstantPool:
      return LowerConstantPool(Op, DAG);
      break;
    case ISD::SELECT:
      return LowerSelect(Op, DAG);
      break;
    case ISD::SELECT_CC:
      return LowerSelectCC(Op, DAG);
      break;
    case ISD::SETCC:
      return LowerSetCC(Op, DAG);
      break;
    case ISD::FP_EXTEND:
      return LowerFpExtend(Op, DAG);
      break;
    case ISD::BR_CC:
      return LowerBrCC(Op, DAG);
      break;
    case ISD::BRCOND:
      return LowerBrCond(Op, DAG);
      break;
    case ISD::FDIV:
      return LowerFastDiv(Op, DAG);
      break;
    case ISD::FP_TO_SINT:
    case ISD::FP_TO_UINT:
      return LowerFpToInt(Op, DAG);
      break;
    case ISD::UINT_TO_FP:
    case ISD::SINT_TO_FP:
      return LowerIntToFp(Op, DAG);
      break;
  }
  return SDValue();
}

//===----------------------------------------------------------------------===//
//  Fast arithmetics lowering
//===----------------------------------------------------------------------===//
SDValue EpiphanyTargetLowering::LowerFastDiv(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  // Get operands
  SDValue LHS   = Op.getOperand(0);
  SDValue RHS   = Op.getOperand(1);

  // Prepare lib call
  RTLIB::Libcall LC = RTLIB::DIV_F32;
  SDValue Callee = DAG.getExternalSymbol(getLibcallName(LC),
      getPointerTy(DAG.getDataLayout()));

  assert(LHS.getSimpleValueType() == MVT::f32 && RHS.getSimpleValueType() == MVT::f32 && 
      "Wrong value type in float fast division!");

  // Call the library
  SmallVector<SDValue, 2> Ops({RHS, Callee});
  std::pair<SDValue, SDValue> Divisor = makeLibCall(DAG, LC, MVT::f32, Ops, /* isSigned = */ true, DL);

  // Multiply by divident
  return DAG.getNode(ISD::FMUL, DL, MVT::f32, Divisor.first, LHS, Divisor.second);
}

SDValue EpiphanyTargetLowering::LowerIntToFp(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  SDValue arg = Op.getOperand(0);
  EVT ArgVT = arg.getValueType();
  EVT ResVT = Op.getValueType();

  // We have FLOAT op for i32 -> f32 conversion
  if (ArgVT.getSimpleVT() == MVT::i32 && ResVT.getSimpleVT() == MVT::f32) {
    return DAG.getNode(EpiphanyISD::FLOAT, DL, ResVT, arg);
  }

  RTLIB::Libcall LC;
  if (Op.getOpcode() == ISD::SINT_TO_FP) {
    LC = RTLIB::getSINTTOFP(ArgVT, ResVT);
  } else {
    LC = RTLIB::getUINTTOFP(ArgVT, ResVT);
  }

  SmallVector<SDValue, 2> Ops(Op->op_begin(), Op->op_end());
  return makeLibCall(DAG, LC, ResVT, Ops, false, DL).first;
}

SDValue EpiphanyTargetLowering::LowerFpToInt(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  SDValue arg = Op.getOperand(0);
  EVT ArgVT = arg.getValueType();
  EVT ResVT = Op.getValueType();

  // We have FIX op for f32 -> i32 conversion
  if (ArgVT.getSimpleVT() == MVT::f32 && ResVT.getSimpleVT() == MVT::i32) {
    return DAG.getNode(EpiphanyISD::FIX, DL, ResVT, arg);
  }

  RTLIB::Libcall LC;
  if (Op.getOpcode() == ISD::FP_TO_SINT) {
    LC = RTLIB::getFPTOSINT(ArgVT, ResVT);
  } else {
    LC = RTLIB::getFPTOUINT(ArgVT, ResVT);
  }

  SmallVector<SDValue, 2> Ops(Op->op_begin(), Op->op_end());
  return makeLibCall(DAG, LC, ResVT, Ops, false, DL).first;
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//
static EpiphanyCC::CondCodes ConvertCC(SDValue CC, const SDLoc &DL, SDValue &RHS, bool &swap) {
  // Get condition code
  ISD::CondCode code = cast<CondCodeSDNode>(CC)->get();
  // Get used reg class
  MVT Ty = RHS.getSimpleValueType();
  // Choose condition
  switch (code) {
    default:
      llvm_unreachable("Unknown condition code: " + code);
      break;
    case ISD::SETEQ:
    case ISD::SETOEQ:
    case ISD::SETUEQ:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BEQ;
      } else {
        return EpiphanyCC::COND_EQ;
      }
      break;
    case ISD::SETNE:
    case ISD::SETONE:
    case ISD::SETUNE:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BNE;
      } else {
        return EpiphanyCC::COND_NE;
      }
      break;
    case ISD::SETGE:
    case ISD::SETOGE:
      if (Ty.isFloatingPoint()) {
        swap = true;
        return EpiphanyCC::COND_BLT;
      } else {
        return EpiphanyCC::COND_GTE;
      }
      break;
    case ISD::SETUGE:
      if (Ty.isFloatingPoint()) {
        swap = true;
        return EpiphanyCC::COND_BLT;
      } else {
        return EpiphanyCC::COND_GTEU;
      }
      break;
    case ISD::SETGT:
    case ISD::SETOGT:
      if (Ty.isFloatingPoint()) {
        swap = true;
        return EpiphanyCC::COND_BLTE;
      } else {
        return EpiphanyCC::COND_GT;
      }
      break;
    case ISD::SETUGT:
      if (Ty.isFloatingPoint()) {
        swap = true;
        return EpiphanyCC::COND_BLTE;
      } else {
        return EpiphanyCC::COND_GTU;
      }
      break;
    case ISD::SETLE:
    case ISD::SETOLE:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BLTE;
      } else {
        return EpiphanyCC::COND_LTE;
      }
      break;
    case ISD::SETULE:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BLTE;
      } else {
        return EpiphanyCC::COND_LTEU;
      }
      break;
    case ISD::SETLT:
    case ISD::SETOLT:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BLT;
      } else {
        return EpiphanyCC::COND_LT;
      }
      break;
    case ISD::SETULT:
      if (Ty.isFloatingPoint()) {
        return EpiphanyCC::COND_BLT;
      } else {
        return EpiphanyCC::COND_LTU;
      }
      break;
  }
}

static ISD::CondCode getUnsignedToSigned(SDValue cond) {
  // Get condition code
  ISD::CondCode code = cast<CondCodeSDNode>(cond)->get();

  // Choose function
  switch (code) {
    default:
      return code;
    case ISD::SETUEQ:
      return ISD::SETEQ;
    case ISD::SETUGE:
      return ISD::SETGE;
    case ISD::SETUGT:
      return ISD::SETGT;
    case ISD::SETULE:
      return ISD::SETLE;
    case ISD::SETULT:
      return ISD::SETLT;
    case ISD::SETUNE:
      return ISD::SETNE;
  }
}

static RTLIB::Libcall getDoubleCmp(SDValue cond) {
  // Get condition code
  ISD::CondCode code = cast<CondCodeSDNode>(cond)->get();

  // Choose function
  switch (code) {
    default:
      llvm_unreachable("Unknown condition code: " + code);
      break;
    case ISD::SETEQ:
    case ISD::SETOEQ:
    case ISD::SETUEQ:
      return RTLIB::OEQ_F64;
    case ISD::SETGE:
    case ISD::SETUGE:
    case ISD::SETOGE:
      return RTLIB::OGE_F64;
    case ISD::SETGT:
    case ISD::SETOGT:
    case ISD::SETUGT:
      return RTLIB::OGT_F64;
    case ISD::SETLE:
    case ISD::SETOLE:
    case ISD::SETULE:
      return RTLIB::OLE_F64;
    case ISD::SETLT:
    case ISD::SETOLT:
    case ISD::SETULT:
      return RTLIB::OLT_F64;
    case ISD::SETNE:
    case ISD::SETONE:
    case ISD::SETUNE:
      return RTLIB::UNE_F64;
  }
}

bool EpiphanyTargetLowering::isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const {
  return false;
}

SDValue EpiphanyTargetLowering::LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  auto PTY = getPointerTy(DAG.getDataLayout());

  SDValue AddrLow  = DAG.getTargetGlobalAddress(GV, DL, PTY, Offset, EpiphanyII::MO_LOW);
  SDValue AddrHigh = DAG.getTargetGlobalAddress(GV, DL, PTY, Offset, EpiphanyII::MO_HIGH);
  SDValue Low = DAG.getNode(EpiphanyISD::MOV, DL, PTY, AddrLow);
  return DAG.getNode(EpiphanyISD::MOVT, DL, PTY, Low, AddrHigh);
  //}
  }

SDValue EpiphanyTargetLowering::LowerExternalSymbol(SDValue Op,
    SelectionDAG &DAG) const {
  SDLoc DL(Op);

  const char *Sym = cast<ExternalSymbolSDNode>(Op)->getSymbol();
  auto PTY = getPointerTy(DAG.getDataLayout());

  SDValue AddrLow  = DAG.getTargetExternalSymbol(Sym, PTY, EpiphanyII::MO_LOW);
  SDValue AddrHigh = DAG.getTargetExternalSymbol(Sym, PTY, EpiphanyII::MO_HIGH);
  SDValue Low = DAG.getNode(EpiphanyISD::MOV, DL, PTY, AddrLow);
  return DAG.getNode(EpiphanyISD::MOVT, DL, PTY, Low, AddrHigh);
}

SDValue EpiphanyTargetLowering::LowerConstantPool(SDValue Op,
    SelectionDAG &DAG) const {
  SDLoc DL(Op);
  ConstantPoolSDNode *CP = cast<ConstantPoolSDNode>(Op);
  EVT PTY = Op.getValueType();

  // Get constant pool address
  SDValue AddrLow;
  SDValue AddrHigh;
  if (CP->isMachineConstantPoolEntry()) {
    AddrLow  = DAG.getTargetConstantPool(CP->getMachineCPVal(), PTY, CP->getAlignment(), CP->getOffset(), EpiphanyII::MO_LOW);
    AddrHigh = DAG.getTargetConstantPool(CP->getMachineCPVal(), PTY, CP->getAlignment(), CP->getOffset(), EpiphanyII::MO_HIGH);
  } else {
    AddrLow  = DAG.getTargetConstantPool(CP->getConstVal(), PTY, CP->getAlignment(), CP->getOffset(), EpiphanyII::MO_LOW);
    AddrHigh = DAG.getTargetConstantPool(CP->getConstVal(), PTY, CP->getAlignment(), CP->getOffset(), EpiphanyII::MO_HIGH);
  }

  // Move address to the register
  SDValue Low = DAG.getNode(EpiphanyISD::MOV, DL, PTY, AddrLow);
  return DAG.getNode(EpiphanyISD::MOVT, DL, PTY, Low, AddrHigh);
}

/// LowerBrCond
//  Lower conditional branch selection
SDValue EpiphanyTargetLowering::LowerBrCond(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  // Get operands
  SDValue Chain = Op.getOperand(0);
  SDValue Value = Op.getOperand(1);
  SDValue Dest  = Op.getOperand(2);

  // Set flag
  ::EpiphanyCC::CondCodes CC = ::EpiphanyCC::COND_GTU;
  SDVTList VTs = DAG.getVTList(Value.getValueType(), MVT::i32);
  SDValue Flag = DAG.getNode(EpiphanyISD::CMP, DL, VTs, Value, DAG.getConstant(0, DL, MVT::i32));

  // Prepare conditional move
  assert(Flag && "Can't get op for provided type"); 
  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(EpiphanyISD::BRCC, DL, Op.getValueType(), Chain, Dest, TargetCC, Flag.getValue(1));
}

/// LowerBrCC
//  Lower conditional branch selection
SDValue EpiphanyTargetLowering::LowerBrCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  // Get operands
  SDValue Chain = Op.getOperand(0);
  SDValue Cond  = Op.getOperand(1);
  SDValue LHS   = Op.getOperand(2);
  SDValue RHS   = Op.getOperand(3);
  SDValue Dest  = Op.getOperand(4);

  // Get operand types
  MVT RTy = RHS.getSimpleValueType();
  MVT LTy = LHS.getSimpleValueType();

  // Set flag
  SDValue Flag;
  ::EpiphanyCC::CondCodes CCode;
  bool swap            = false;
  if (RTy == MVT::i32 && LTy == MVT::i32) {
    // Simple i32 case
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
    CCode        = ConvertCC(Cond, DL, LHS, swap);
  } else if (RTy == MVT::f32 && LTy == MVT::f32) {
    // Floating point f32 case
    CCode        = ConvertCC(Cond, DL, LHS, swap);
    if (swap) {
      std::swap(LHS, RHS);
    }
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
  } else if (RTy == MVT::i64 && LTy == MVT::i64) {
    SDVTList VTs   = DAG.getVTList(MVT::i32, MVT::i32);
    // Extract subregs
    SDValue LHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, LHS);
    SDValue RHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, RHS);
    SDValue LHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, LHS);
    SDValue RHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, RHS);
    // Sub low and high regs
    SDValue Low    = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_lo, RHS_lo);
    SDValue High   = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_hi, RHS_hi);
    // Sub borrow
    SDValue TrueV  = DAG.getConstant(1, DL, MVT::i32);
    SDValue FalseV = DAG.getConstant(0, DL, MVT::i32);
    SDValue CC     = DAG.getConstant(::EpiphanyCC::COND_LT, DL, MVT::i32);
    SDValue Borrow = DAG.getNode(EpiphanyISD::MOVCC, DL, MVT::i32, TrueV, FalseV, CC, Low.getValue(1));
    Flag           = DAG.getNode(EpiphanyISD::CMP, DL, VTs, High, Borrow);
    CCode          = ConvertCC(DAG.getCondCode(getUnsignedToSigned(Cond)), DL, Flag, swap);
  } else if (RTy == MVT::f64 && LTy == MVT::f64) {
    RTLIB::Libcall LC = getDoubleCmp(Cond);
    SmallVector<SDValue, 2> Ops({LHS, RHS});
    Flag              = makeLibCall(DAG, LC, MVT::i32, Ops, /* isSigned = */ true, DL).first;
    // Use integer sub to set the flag, see GCC Soft-Float Library Routines
    SDVTList VTs      = DAG.getVTList(Flag.getValueType(), MVT::i32);
    Flag              = DAG.getNode(EpiphanyISD::CMP, DL, VTs, Flag, DAG.getConstant(0, DL, MVT::i32));
    CCode             = ConvertCC(Cond, DL, Flag, swap);
  }

  // Prepare conditional move
  assert(Flag && "Can't get op for provided type"); 
  SDValue TargetCC = DAG.getConstant(CCode, DL, MVT::i32);
  return DAG.getNode(EpiphanyISD::BRCC, DL, Op.getValueType(), Chain, Dest, TargetCC, Flag.getValue(1));
}

/// LowerSelectCC
//  Lower conditional selection. Similar to movcc + cmp
SDValue EpiphanyTargetLowering::LowerSelectCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);

  // Get operands
  SDValue LHS    = Op.getOperand(0);
  SDValue RHS    = Op.getOperand(1);
  SDValue TrueV  = Op.getOperand(2);
  SDValue FalseV = Op.getOperand(3);
  SDValue Cond   = Op.getOperand(4);

  // Get operand types
  MVT RTy = RHS.getSimpleValueType();
  MVT LTy = LHS.getSimpleValueType();

  // Set the flag
  SDValue Flag;
  if (RTy == MVT::i32 && LTy == MVT::i32) {
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
  } else if (RTy == MVT::f32 && LTy == MVT::f32) {
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
  } else if (RTy == MVT::i64 && LTy == MVT::i64) {
    SDVTList VTs   = DAG.getVTList(MVT::i32, MVT::i32);
    // Extract subregs
    SDValue LHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, LHS);
    SDValue RHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, RHS);
    SDValue LHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, LHS);
    SDValue RHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, RHS);
    // Sub low and high regs
    SDValue Low    = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_lo, RHS_lo);
    SDValue High   = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_hi, RHS_hi);
    // Sub borrow
    SDValue TrueV  = DAG.getConstant(1, DL, MVT::i32);
    SDValue FalseV = DAG.getConstant(0, DL, MVT::i32);
    SDValue CC     = DAG.getConstant(::EpiphanyCC::COND_LT, DL, MVT::i32);
    SDValue Borrow = DAG.getNode(EpiphanyISD::MOVCC, DL, MVT::i32, TrueV, FalseV, CC, Low.getValue(1));
    Flag           = DAG.getNode(EpiphanyISD::CMP, DL, VTs, High, Borrow);
  } else if (RTy == MVT::f64 && LTy == MVT::f64) {
    RTLIB::Libcall LC = getDoubleCmp(Cond);
    SmallVector<SDValue, 2> Ops({LHS, RHS});
    Flag         = makeLibCall(DAG, LC, MVT::i32, Ops, /* isSigned = */ true, DL).first;
    // Use integer sub to set the flag, see GCC Soft-Float Library Routines
    SDVTList VTs = DAG.getVTList(Flag.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, Flag, DAG.getConstant(0, DL, MVT::i32));
  }

  // Get condition code
  bool swap = false;
  ::EpiphanyCC::CondCodes CC = ConvertCC(Cond, DL, Flag, swap);
  if (swap) {
    std::swap(TrueV, FalseV);
  }

  // Prepare conditional move
  assert(Flag && "Can't get op for provided type"); 
  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(EpiphanyISD::MOVCC, DL, Op.getValueType(), TrueV, FalseV, TargetCC, Flag.getValue(1));
}


/// LowerSelect
//  Select one of two options based on the flag. In general it is equal to movcc
SDValue EpiphanyTargetLowering::LowerSelect(SDValue Op, SelectionDAG &DAG) const {

  SDLoc DL(Op);
  // Get operands
  SDValue Cmp   = Op.getOperand(0);
  SDValue True  = Op.getOperand(1);
  SDValue False = Op.getOperand(2);

  // Get condition from CMP operand
  assert(Cmp.getNumOperands() == 3 && "Strange number of operand in the first SELECT argument");
  SDValue Cond = Cmp.getOperand(2);

  // Get condition code
  bool swap = false;
  ::EpiphanyCC::CondCodes CC = ConvertCC(Cond, DL, True, swap);
  if (swap) {
    std::swap(True, False);
  }
  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(EpiphanyISD::MOVCC, DL, Op.getValueType(), True, False, TargetCC);
}


/// LowerSetCC
//  Lower conditional set operation. In general it is equal to movcc + cmp
SDValue EpiphanyTargetLowering::LowerSetCC(SDValue Op, SelectionDAG &DAG) const {

  SDLoc DL(Op);
  // Get operands
  SDValue LHS    = Op.getOperand(0);
  SDValue RHS    = Op.getOperand(1);
  SDValue Cond   = Op.getOperand(2);

  // Get operand types
  MVT RTy = RHS.getSimpleValueType();
  MVT LTy = LHS.getSimpleValueType();

  // Create result values
  SDValue TrueV  = DAG.getConstant(1, DL, MVT::i32);
  SDValue FalseV = DAG.getConstant(0, DL, MVT::i32);

  // Set the flag
  SDValue Flag;
  if (RTy == MVT::i32 && LTy == MVT::i32) {
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
  } else if (RTy == MVT::f32 && LTy == MVT::f32) {
    SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS, RHS);
  } else if (RTy == MVT::i64 && LTy == MVT::i64) {
    SDVTList VTs   = DAG.getVTList(MVT::i32, MVT::i32);
    // Extract subregs
    SDValue LHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, LHS);
    SDValue RHS_lo = DAG.getTargetExtractSubreg(Epiphany::isub_lo, DL, MVT::i32, RHS);
    SDValue LHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, LHS);
    SDValue RHS_hi = DAG.getTargetExtractSubreg(Epiphany::isub_hi, DL, MVT::i32, RHS);
    // Sub low and high regs
    SDValue Low    = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_lo, RHS_lo);
    SDValue High   = DAG.getNode(EpiphanyISD::CMP, DL, VTs, LHS_hi, RHS_hi);
    // Sub borrow
    SDValue CC     = DAG.getConstant(::EpiphanyCC::COND_LT, DL, MVT::i32);
    SDValue Borrow = DAG.getNode(EpiphanyISD::MOVCC, DL, MVT::i32, TrueV, FalseV, CC, Low.getValue(1));
    Flag           = DAG.getNode(EpiphanyISD::CMP, DL, VTs, High, Borrow);
  } else if (RTy == MVT::f64 && LTy == MVT::f64) {
    RTLIB::Libcall LC = getDoubleCmp(Cond);
    SmallVector<SDValue, 2> Ops({LHS, RHS});
    Flag         = makeLibCall(DAG, LC, MVT::i32, Ops, /* isSigned = */ true, DL).first;
    // Use integer sub to set the flag, see GCC Soft-Float Library Routines
    SDVTList VTs = DAG.getVTList(Flag.getValueType(), MVT::i32);
    Flag         = DAG.getNode(EpiphanyISD::CMP, DL, VTs, Flag, DAG.getConstant(0, DL, MVT::i32));
  }

  // Get condition code
  bool swap = false;
  ::EpiphanyCC::CondCodes CC = ConvertCC(Cond, DL, Flag, swap);
  if (swap) {
    std::swap(TrueV, FalseV);
  }

  // Prepare conditional move
  assert(Flag && "Can't get op for provided type"); 
  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(EpiphanyISD::MOVCC, DL, Op.getValueType(), TrueV, FalseV, TargetCC, Flag.getValue(1));
}

SDValue EpiphanyTargetLowering::LowerFpExtend(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  // Get external extention function
  RTLIB::Libcall LC;
  LC = RTLIB::getFPEXT(Op.getOperand(0).getValueType(), Op.getValueType());
  SDValue SrcVal = Op.getOperand(0);
  return makeLibCall(DAG, LC, Op.getValueType(), SrcVal, /* isSigned = */ false, DL).first;
}

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

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  DEBUG(dbgs() << "\nLowering formal arguments\n");
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_Epiphany_Assign);

  // Create frame index for the start of the first vararg value
  /*if (IsVarArg) {*/
  //unsigned Offset = CCInfo.getNextStackOffset();
  //FI->setVarArgsFrameIndex(MFI.CreateFixedObject(1, Offset, true));
  /*}*/

  DEBUG(dbgs() << "Number of args present: " << ArgLocs.size() << "\n");
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue ArgValue;

    // If assigned to register
    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      DEBUG(errs() << "Arg " << i << " assigned to reg " << VA.getLocReg() << "\n") ;

      // Get register that was assigned
      const TargetRegisterClass *RC = getRegClassFor(RegVT.getSimpleVT());
      unsigned VReg = RegInfo.createVirtualRegister(RC);
      RegInfo.addLiveIn(VA.getLocReg(), VReg);
      ArgValue = DAG.getCopyFromReg(Chain, DL, VReg, RegVT);

      // Check how exactly we should make assignment based on the value type
      switch (VA.getLocInfo()) {
        default: llvm_unreachable("Unknown loc info!");
        case CCValAssign::Full: 
                 break;
        case CCValAssign::BCvt:
                 ArgValue = DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), ArgValue);
                 break;
        case CCValAssign::SExt:
                 ArgValue = DAG.getNode(ISD::AssertSext, DL, RegVT, ArgValue, DAG.getValueType(VA.getValVT()));
                 ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);
                 break;
        case CCValAssign::ZExt:        
                 ArgValue = DAG.getNode(ISD::AssertZext, DL, RegVT, ArgValue, DAG.getValueType(VA.getValVT()));
                 ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);
                 break;
      }

    } else { // VA.isRegLoc()
      assert(VA.isMemLoc());
      DEBUG(dbgs() << "Arg is a memory loc\n");
      int FI = MFI.CreateFixedObject(VA.getLocVT().getSizeInBits()/8, VA.getLocMemOffset() + Subtarget.stackOffset(), true);
      SDValue FIN = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
      ArgValue = DAG.getLoad(VA.getLocVT(), DL, Chain, FIN, MachinePointerInfo::getFixedStack(MF, FI));
    }

    InVals.push_back(ArgValue);
  }

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
    SDValue Val = DAG.getCopyFromReg(Chain, DL, Reg, getPointerTy(DAG.getDataLayout()));
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

  return DAG.getNode(EpiphanyISD::RTS, DL, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
//@            Function Call Calling Convention Implementation
//===----------------------------------------------------------------------===//
// From original Hoenchen implementation

SDValue
EpiphanyTargetLowering::LowerCall(CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const {
  // Init needed parameters
  SelectionDAG &DAG                     = CLI.DAG;
  SDLoc &DL                             = CLI.DL;
  SmallVector<ISD::OutputArg, 32> &Outs = CLI.Outs;
  SmallVector<SDValue, 32> &OutVals     = CLI.OutVals;
  SmallVector<ISD::InputArg, 32> &Ins   = CLI.Ins;
  SDValue Chain                         = CLI.Chain;
  SDValue Callee                        = CLI.Callee;
  bool &IsTailCall                      = CLI.IsTailCall;
  CallingConv::ID CallConv              = CLI.CallConv;
  bool IsVarArg                         = CLI.IsVarArg;

  MachineFunction &MF = DAG.getMachineFunction();
  bool IsStructRet = !Outs.empty() && Outs[0].Flags.isSRet();

  DEBUG(dbgs() << "\nLowering call\n");

  // Check if the call is eligible for tail optimization
  if (IsTailCall) {
    DEBUG(dbgs() << "Optimizing as tail call\n");
    IsTailCall = IsEligibleForTailCallOptimization(Callee, CallConv, IsVarArg, IsStructRet, MF.getFunction()->hasStructRetAttr(), Outs, OutVals, Ins, DAG);
  }

  // Analyze return variables based on EpiphanyCallingConv.td
  DEBUG(dbgs() << "Call has " << Outs.size() << " args\n");
  // TODO: Maybe 16 is not that much considering the stack
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, RetCC_Epiphany);

  // Adjust stack pointer
  unsigned NextStackOffset = CCInfo.getNextStackOffset();
  unsigned StackAlignment = Subtarget.getFrameLowering()->getStackAlignment();
  NextStackOffset = alignTo(NextStackOffset, StackAlignment);
  SDValue NextStackOffsetVal = DAG.getIntPtrConstant(NextStackOffset, DL, true);
  DEBUG(dbgs() << "Next offset value is " << NextStackOffset << "\n");

  // Emit CALLSEQ_START
  Chain = DAG.getCALLSEQ_START(Chain, NextStackOffsetVal, DL);
  SDValue StackPtr = DAG.getCopyFromReg(Chain, DL, Epiphany::SP, getPointerTy(DAG.getDataLayout()));

  // We can have only 4 regs to pass, but we can compensate with stack-based args
  SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;

  // Check each argument if it needs any modifications and push it into the stack
  DEBUG(dbgs() << "After analysis, call has " << ArgLocs.size() << " args\n");
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    ISD::ArgFlagsTy Flags = Outs[i].Flags;
    MVT ValVT = VA.getValVT(), LocVT = VA.getLocVT();
    SDValue Arg = OutVals[i];
    DEBUG(dbgs() << "Analyzing arg: "; Arg.dump());

    // Callee does the actual widening, so all extensions just use an implicit
    // definition of the rest of the Loc. Aesthetically, this would be nicer as
    // an ANY_EXTEND, but that isn't valid for floating-point types and this
    // alternative works on integer types too.
    switch (VA.getLocInfo()) {
      default: 
        llvm_unreachable("Unknown loc info!");
      case CCValAssign::Full: 
        if (VA.isRegLoc()) {
          if ((ValVT == MVT::f32 && LocVT == MVT::i32) ||
              (ValVT == MVT::f64 && LocVT == MVT::i64) ||
              (ValVT == MVT::i64 && LocVT == MVT::f64))
            Arg = DAG.getNode(ISD::BITCAST, DL, LocVT, Arg);
          else if (ValVT == MVT::f64 && LocVT == MVT::i32) {
            llvm_unreachable("Unimplemented yet!");
            continue;
          }
        }
        // Nothing to do
        break;
      case CCValAssign::SExt:
      case CCValAssign::ZExt:
      case CCValAssign::AExt:
        // Floating-point arguments only get extended/truncated if they're going
        // in memory, so using the integer operation is acceptable here.
        Arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Arg);
        break;
      case CCValAssign::BCvt:
        Arg = DAG.getNode(ISD::BITCAST, DL, VA.getLocVT(), Arg);
        break;
    }

    if (VA.isRegLoc()) {
      DEBUG(dbgs() << "Argument will be passed using register\n");
      // A normal register (sub-) argument. For now we just note it down because
      // we want to copy things into registers as late as possible to avoid
      // register-pressure (and possibly worse).
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
      continue;
    }

    // If arg is neither Reg nor Memory - throw error
    assert(VA.isMemLoc() && "unexpected argument location");
    DEBUG(dbgs() << "Argument will be passed using memory loc\n");

    // Deal with memory-stored args
    SDValue PtrOff = DAG.getIntPtrConstant(VA.getLocMemOffset() + Subtarget.stackOffset(), DL, /* isTarget = */ false);
    SDValue DstAddr = DAG.getNode(ISD::ADD, DL, getPointerTy(DAG.getDataLayout()), StackPtr, PtrOff);

    if (Flags.isByVal()) {
      DEBUG(dbgs() << "Argument passed by value");
      SDValue SizeNode = DAG.getConstant(Flags.getByValSize(), DL, MVT::i32);
      SDValue Cpy = DAG.getMemcpy(Chain, DL, DstAddr, Arg, SizeNode, Flags.getByValAlign(), 
          /*isVolatile = */ false, /*alwaysInline = */ false, /*isTailCall = */ false,
          MachinePointerInfo(), MachinePointerInfo());
      MemOpChains.push_back(Cpy);
    } else {
      DEBUG(dbgs() << "Argument passed in stack");
      // Normal stack argument, put it where it's needed.
      SDValue Store = DAG.getStore(Chain, DL, Arg, DstAddr, MachinePointerInfo());
      MemOpChains.push_back(Store);
    }
  }

  // The loads and stores generated above shouldn't clash with each
  // other. Combining them with this TokenFactor notes that fact for the rest of
  // the backend.
  if (!MemOpChains.empty()) {
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);
  }

  // Most of the rest of the instructions need to be glued together; we don't
  // want assignments to actual registers used by a call to be rearranged by a
  // well-meaning scheduler.
  SDValue InFlag;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[i].first, RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // The linker is responsible for inserting veneers when necessary to put a
  // function call destination in range, so we don't need to bother with a
  // wrapper here.
  // For internal linkage we can use BranchAndLink without regs, while for external it'd be better to use JALR
  EVT PTY = getPointerTy(DAG.getDataLayout());
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
    DEBUG(dbgs() << "\nArgument is a global value");
    const GlobalValue *GV = G->getGlobal();
    SDValue AddrLow  = DAG.getTargetGlobalAddress(GV, DL, PTY, 0, EpiphanyII::MO_LOW);
    SDValue AddrHigh = DAG.getTargetGlobalAddress(GV, DL, PTY, 0, EpiphanyII::MO_HIGH);
    Callee = DAG.getNode(EpiphanyISD::MOV, DL, PTY, AddrLow);
    Callee = DAG.getNode(EpiphanyISD::MOVT, DL, PTY, Callee, AddrHigh);
  } else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    DEBUG(dbgs() << "\nArgument is an external symbol");
    const char *Sym = S->getSymbol();
    SDValue AddrLow  = DAG.getTargetExternalSymbol(Sym, PTY, EpiphanyII::MO_LOW);
    SDValue AddrHigh = DAG.getTargetExternalSymbol(Sym, PTY, EpiphanyII::MO_HIGH);
    Callee = DAG.getNode(EpiphanyISD::MOV, DL, PTY, AddrLow);
    Callee = DAG.getNode(EpiphanyISD::MOVT, DL, PTY, Callee, AddrHigh);
  }

  // We produce the following DAG scheme for the actual call instruction:
  //     (EpiphanyCall Chain, Callee, reg1, ..., regn, preserveMask, inflag?
  //
  // Most arguments aren't going to be used and just keep the values live as
  // far as LLVM is concerned. It's expected to be selected as simply "bl
  // callee" (for a direct, non-tail call).
  std::vector<SDValue> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Ops.push_back(DAG.getRegister(RegsToPass[i].first, RegsToPass[i].second.getValueType()));
  }

  // Add a register mask operand representing the call-preserved registers. This
  // is used later in codegen to constrain register-allocation.
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(MF, CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  // If we needed glue, put it in as the last argument.
  if (InFlag.getNode()) {
    Ops.push_back(InFlag);
  }

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(EpiphanyISD::Call, DL, NodeTys, Ops);
  InFlag = Chain.getValue(1);

  // Now we can reclaim the stack, just as well do it before working out where
  // our return value is.
  uint64_t CalleePopBytes = /*DoesCalleeRestoreStack(CallConv, TailCallOpt) ? NumBytes :*/ 0;
  Chain = DAG.getCALLSEQ_END(Chain, NextStackOffsetVal, 
      DAG.getIntPtrConstant(CalleePopBytes, DL, /* isTarget = */ true), InFlag, DL);
  InFlag = Chain.getValue(1);

  DEBUG(dbgs() << "\n");

  return LowerCallResult(Chain, InFlag, CallConv, IsVarArg, Ins, DL, DAG, InVals);
}

//===----------------------------------------------------------------------===//
//@            Call Return Parameters Calling Convention Implementation
//===----------------------------------------------------------------------===//
// From original Hoenchen implementation
SDValue
EpiphanyTargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
    CallingConv::ID CallConv, bool IsVarArg, const SmallVectorImpl<ISD::InputArg> &Ins,
    const SDLoc &DL, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  // Assign locations to each value returned by this call according to EpiphanyCallingConv.td
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs, *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, RetCC_Epiphany);

  // For each argument check if some modification is needed
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign VA = RVLocs[i];

    // Return values that are too big to fit into registers should use an sret
    // pointer, so this can be a lot simpler than the main argument code.
    assert(VA.isRegLoc() && "Memory locations not expected for call return");
    SDValue Val = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), InFlag);
    Chain = Val.getValue(1);
    InFlag = Val.getValue(2);

    switch (VA.getLocInfo()) {
      default: llvm_unreachable("Unknown loc info!");
      case CCValAssign::Full: 
               break;
      case CCValAssign::BCvt:
               Val = DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), Val);
               break;
      case CCValAssign::ZExt:
      case CCValAssign::SExt:
      case CCValAssign::AExt:
               // Floating-point arguments only get extended/truncated if they're going
               // in memory, so using the integer operation is acceptable here.
               Val = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Val);
               break;
    }
    InVals.push_back(Val);
  }
  return Chain;
}

//===----------------------------------------------------------------------===//
//@            Additional Functions For Calling Convention Implementation
//===----------------------------------------------------------------------===//

EpiphanyTargetLowering::EpiphanyCC::EpiphanyCC(
    CallingConv::ID CC, bool IsE16_, CCState &Info,
    EpiphanyCC::SpecialCallingConvType SpecialCallingConv_)
  : CCInfo(Info), CallConv(CC), IsE16(IsE16_) {
    // Pre-allocate reserved argument area.
    CCInfo.AllocateStack(reservedArgArea(), 1);
  }

template<typename Ty> 
void EpiphanyTargetLowering::EpiphanyCC::analyzeReturn(const SmallVectorImpl<Ty> &RetVals, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  CCAssignFn *Fn;

  Fn = RetCC_Epiphany;

  for (unsigned I = 0, E = RetVals.size(); I < E; ++I) {
    MVT VT = RetVals[I].VT;
    ISD::ArgFlagsTy Flags = RetVals[I].Flags;
    MVT RegVT = this->getRegVT(VT, RetTy, CallNode, IsSoftFloat);

    if (Fn(I, VT, RegVT, CCValAssign::Full, Flags, this->CCInfo)) {
      DEBUG(dbgs() << "Call result #" << I << " has unhandled type " << EVT(VT).getEVTString() << '\n');
      llvm_unreachable(nullptr);
    }
  }
}

void EpiphanyTargetLowering::EpiphanyCC::analyzeCallResult(const SmallVectorImpl<ISD::InputArg> &Ins, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  analyzeReturn(Ins, IsSoftFloat, CallNode, RetTy);
}

void EpiphanyTargetLowering::EpiphanyCC::analyzeReturn(const SmallVectorImpl<ISD::OutputArg> &Outs, bool IsSoftFloat,
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


