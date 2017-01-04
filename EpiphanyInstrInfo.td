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
  def RTS : Pseudo<(outs), (ins), [(EpiphanyRet)]>;
}

let isBranch=1, isTerminator=1, isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR32 : JumpReg<"jr", 0b0101011111, [(brind GPR32:$Rn)], COND_NONE>;
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
def LDRi8   : LoadM32<LS_add,  0, GPR32, AlignedLoad<sextloadi8>>;
def LDRi8u  : LoadM32<LS_add,  0, GPR32, AlignedLoad<zextloadi8>>;
def STRi8   : StoreM32<LS_add, 0, GPR32, AlignedStore<truncstorei8>>;
def LDRi16  : LoadM32<LS_add,  0, GPR32, AlignedLoad<sextloadi16>>;
def LDRi16u : LoadM32<LS_add,  0, GPR32, AlignedLoad<zextloadi16>>;
def STRi16  : StoreM32<LS_add, 0, GPR32, AlignedStore<truncstorei16>>;
def LDRi32  : LoadM32<LS_add,  0, GPR32, AlignedLoad<load>>;
def STRi32  : StoreM32<LS_add, 0, GPR32, AlignedStore<store>>;

def LDRf32 : LoadM32<LS_add,  0, FPR32, AlignedLoad<load>>;
def STRf32 : StoreM32<LS_add, 0, FPR32, AlignedStore<store>>;

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
  // Addsub
  def ADDri : IntMath32ri<0b0011011, "add", addc, simm11, immSExt11>;
  def SUBri : IntMath32ri<0b0111011, "sub", subc, simm11, immSExt11>;

  // Shifts
  def LSRri : ShiftMath32ri<0b0110, 0b01111, "lsr", srl, imm5, immUExt5>;
  def LSLri : ShiftMath32ri<0b0110, 0b01111, "lsl", shl, imm5, immUExt5>;
  def ASRri : ShiftMath32ri<0b0110, 0b01111, "asr", sra, imm5, immUExt5>;
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

