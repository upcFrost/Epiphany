//===----- EpiphanyInstrInfo.td - Epiphany Instruction Info ----*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Epiphany scalar instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

include "EpiphanyInstrFormats.td"

//===----------------------------------------------------------------------===//
// Target-specific ISD nodes and profiles
//===----------------------------------------------------------------------===//

def SDT_EpiphanyRet : SDTypeProfile<0, 1, [SDTCisInt<0>]>;

// Return instruction
def EpiphanyRet : SDNode<"EpiphanyISD::RTS", SDTNone, 
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def NOP : Interrupt<0b0110100010, [], "nop">;

//let isReturn = 1, isTerminator = 1, isBarrier = 1 in {
//  def RTI : Interrupt<0b0111010010, [(EpiphanyRet)], "rti">;
//}


//===----------------------------------------------------------------------===//
// Branching
//===----------------------------------------------------------------------===//
let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1, hasCtrlDep=1 in {
  def RTS : Pseudo32<(outs), (ins), [(EpiphanyRet)]>;
}

let isBranch=1, isTerminator=1, isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR16 : JumpReg16<"jr", 0b0101000010, [(brind GPR16:$Rn)], COND_NONE>;
}

let isBranch=1, isTerminator=1, isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR32 : JumpReg32<"jr", 0b0101001111, [(brind GPR32:$Rn)], COND_NONE>;
}

//===----------------------------------------------------------------------===//
// Transformation functions
//===----------------------------------------------------------------------===//
def LO16 : SDNodeXForm<imm, [{
  return getImm(N, N->getZExtValue() & 0xffff);
}]>;

def HI16 : SDNodeXForm<imm, [{
  return getImm(N, (N->getZExtValue() >> 16) & 0xffff);
}]>;

//===----------------------------------------------------------------------===//
// Load/store instructions
//===----------------------------------------------------------------------===//
multiclass LoadM<PatFrag LoadType> {
  def _r16 : Load16<LS_add,  0, GPR16, LoadType>;
  def _r32 : Load32<LS_add,  0, GPR32, LoadType>;
}
multiclass StoreM<PatFrag StoreType> {
  def _r16 : Store16<LS_add,  0, GPR16, StoreType>;
  def _r32 : Store32<LS_add,  0, GPR32, StoreType>;
}

defm LDRi8   : LoadM<AlignedLoad<sextloadi8>>;
defm LDRi8u  : LoadM<AlignedLoad<zextloadi8>>;
defm LDRi16  : LoadM<AlignedLoad<sextloadi16>>;
defm LDRi16u : LoadM<AlignedLoad<zextloadi16>>;
defm LDRi32  : LoadM<AlignedLoad<load>>;
defm STRi8   : StoreM<AlignedStore<truncstorei8>>;
defm STRi16  : StoreM<AlignedStore<truncstorei16>>;
defm STRi32  : StoreM<AlignedStore<store>>;

def LDRf32   : Load16<LS_add,  0, FPR32, AlignedLoad<load>>;
def STRf32   : Store16<LS_add, 0, FPR32, AlignedStore<store>>;

//===----------------------------------------------------------------------===//
// Integer arithmetic operations with registers
//===----------------------------------------------------------------------===//
let Defs = [STATUS] in {
  // Integer
  def ADDi32rr : AnyMath32rr<0b0011111, "add", addc, GPR32>;
  def SUBi32rr : AnyMath32rr<0b0111111, "sub", subc, GPR32>;
  def ANDi32rr : AnyMath32rr<0b1011111, "and", and,  GPR32>;
  def ORRi32rr : AnyMath32rr<0b1111111, "orr", or,   GPR32>;
  def EORi32rr : AnyMath32rr<0b0001111, "eor", xor,  GPR32>;
  def LSLi32rr : AnyMath32rr<0b0101111, "lsl", shl,  GPR32>;
  def LSRi32rr : AnyMath32rr<0b1001111, "lsr", srl,  GPR32>;
  def ASRi32rr : AnyMath32rr<0b1101111, "asr", sra,  GPR32>;
}

let Defs = [STATUS] in {
  def IADDrr : IntMath32rr<0b0011111, "iadd", add>;
  def IMULrr : IntMath32rr<0b0101111, "imul", mul>;
  def ISUBrr : IntMath32rr<0b0111111, "isub", sub>;
}

//===----------------------------------------------------------------------===//
// Integer arithmetic operations with immediates
//===----------------------------------------------------------------------===//
let Defs = [STATUS] in {
  // Addsub i16
  def ADD16ri : IntMath16ri<0b0010011, "add", addc, simm3, immSExt3>;
  def SUB16ri : IntMath16ri<0b0110011, "sub", subc, simm3, immSExt3>;

  // Shifts i16
  def LSR16ri : ShiftMath16ri<0b00110, "lsr", srl, imm5, immUExt5>;
  def LSL16ri : ShiftMath16ri<0b10110, "lsl", shl, imm5, immUExt5>;
  def ASR16ri : ShiftMath16ri<0b01110, "asr", sra, imm5, immUExt5>;

  // Addsub i32
  def ADD32ri : IntMath32ri<0b0011011, "add", addc, simm11, immSExt11>;
  def SUB32ri : IntMath32ri<0b0111011, "sub", subc, simm11, immSExt11>;

  // Shifts i32
  def LSR32ri : ShiftMath32ri<0b0110, 0b01111, "lsr", srl, imm5, immUExt5>;
  def LSL32ri : ShiftMath32ri<0b0110, 0b11111, "lsl", shl, imm5, immUExt5>;
  def ASR32ri : ShiftMath32ri<0b1110, 0b01111, "asr", sra, imm5, immUExt5>;
}

//===----------------------------------------------------------------------===//
// Additional integer arithmetic patterns
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Floating-point arithmetic operations with registers
//===----------------------------------------------------------------------===//

// All the instructions set STATUS flags
let Defs = [STATUS] in {
  def FADDrr : FPMath32rr<0b0001111, "fadd", fadd>;
  def FSUBrr : FPMath32rr<0b0011111, "fsub", fsub>;
  def FMULrr : FPMath32rr<0b0101111, "fmul", fmul>;
}

//===----------------------------------------------------------------------===//
// IntToFloat and Abs
//===----------------------------------------------------------------------===//
let Defs = [STATUS] in {
  def FLOAT32rr : IntToFloat32<(outs FPR32:$Rd), (ins GPR32:$Rn), "float\t$Rd, $Rn", [(set FPR32:$Rd, (sint_to_fp GPR32:$Rn))], 0b1011111, NoItinerary>;
  def FIX32rr   : IntToFloat32<(outs GPR32:$Rd), (ins FPR32:$Rn), "fix\t$Rd, $Rn",   [(set GPR32:$Rd, (fp_to_sint FPR32:$Rn))], 0b1101111, NoItinerary>;
}

//===----------------------------------------------------------------------===//
// Move operations: Immediates
//===----------------------------------------------------------------------===//
// TODO: 32-bit loading
let isMoveImm = 1, isAsCheapAsAMove = 1, isReMaterializable = 1 in {
  let Constraints = "$src = $Rd" in {
    def MOVTi32ri : Mov32ri<"movt", (ins GPR32:$src, imm16:$Imm), [(set GPR32:$Rd, (or (and GPR32:$src, 0xffff), (shl immSExt16:$Imm, 16)))], 0b01011, /* MOVT = */ 1, GPR32>;
  }
  def MOVi16ri : Mov16ri<"mov", (ins imm8:$Imm), [(set GPR16:$Rd, immSExt8:$Imm)], 0b00011, GPR16>;
  def MOVi32ri : Mov32ri<"mov", (ins imm16:$Imm), [(set GPR32:$Rd, immSExt16:$Imm)], 0b01011, /* MOVT = */ 0, GPR32>;
  def MOVf32ri : Mov32ri<"mov", (ins fpimm16:$Imm), [(set FPR32:$Rd, fpimm16:$Imm)], 0b01011, /* MOVT = */ 0, FPR32>;

  // One more for memory
  def MOViPTR : Mov32ri<"mov", (ins mem11:$Imm), [(set GPR32:$Rd, addr:$Imm)], 0b01011, /* MOVT = */ 0, GPR32>;
}
def : Pat<(or GPR32:$src, 0xffff0000), (MOVTi32ri GPR32:$src, 0xffff)>;

//===----------------------------------------------------------------------===//
// Move operations: Registers
//===----------------------------------------------------------------------===//

// TODO: Unsigned comparison
def MOVi32rr     : Mov32rr<"mov", [], GPR32>;
def MOVf32rr     : Mov32rr<"mov", [], FPR32>;

let Constraints = "$src = $Rd" in {
  def MOVEQ32rr   : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETEQ))],  COND_EQ,    GPR32>;
  def MOVNE32rr   : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETNE))],  COND_NE,    GPR32>;
  def MOVGTU32rr  : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETUGT))], COND_GTU,   GPR32>;
  def MOVGTEU32rr : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETUGE))], COND_GTEU,  GPR32>;
  def MOVLTEU32rr : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETULE))], COND_LTEU,  GPR32>;
  def MOVLTU32rr  : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETULT))], COND_LTU,   GPR32>;
  def MOVGT32rr   : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETGT))],  COND_GT,    GPR32>;
  def MOVGTE32rr  : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETGE))], COND_GTE,   GPR32>;
  def MOVLT32rr   : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETLT))],  COND_LT,    GPR32>;
  def MOVLTE32rr  : MovCond32rr<"mov", [(set GPR32:$Rd, (setcc GPR32:$Rn, GPR32:$src, SETLE))], COND_LTE,   GPR32>;
}

