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

// Return
def EpiphanyRet : SDNode<"EpiphanyIDS::Ret", SDTNone, 
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// Load/store instructions
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// Integer arithmetic operations with registers
//===----------------------------------------------------------------------===//
let Defs = [STATUS] in {
  def ADDrr : IntMath32rr<0b0011111, "add", addc>;
  def SUBrr : IntMath32rr<0b0111111, "sub", subc>;
  def ANDrr : IntMath32rr<0b1011111, "and", and>;
  def ORRrr : IntMath32rr<0b1111111, "orr", or>;
  def EORrr : IntMath32rr<0b0001111, "eor", xor>;
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
  def ADDri : IntMath32ri<0b0011011, "add", addc, simm11, immSExt11>;
  def SUBri : IntMath32ri<0b0111011, "sub", subc, simm11, immSExt11>;
}

//===----------------------------------------------------------------------===//
// Floating-point arithmetic operations with registers
//===----------------------------------------------------------------------===//

// All the instructions set STATUS flags
//let Defs = [STATUS] in {
//  def FADDrr : FPMath32rr<0b0001111, "fadd", fadd>;
//  def FSUBrr : FPMath32rr<0b0011111, "fsub", fsub>;
//  def FMULrr : FPMath32rr<0b0101111, "fmul", fmul>;
//}

//===----------------------------------------------------------------------===//
// Move operations: Immediates
//===----------------------------------------------------------------------===//
let isMoveImm = 1, isAsCheapAsAMove = 1, isReMaterializable = 1 in {
  def MOVT32ri : Mov32ri<"movt", (ins GPR32:$src, imm16:$Imm), [(set GPR32:$Rd, (or (and GPR32:$src, 0xffff), immSExt16:$Imm))], 0b01011, /* MOVT = */ 1> {
    let Constraints = "$src = $Rd";
  }
  def MOV32ri : Mov32ri<"mov", (ins imm16:$Imm), [(set GPR32:$Rd, immSExt16:$Imm)], 0b01011, /* MOVT = */ 0>;
}
def : Pat<(or GPR32:$src, 0xffff0000), (MOVT32ri GPR32:$src, 0xffff)>;

//===----------------------------------------------------------------------===//
// Move operations: Registers
//===----------------------------------------------------------------------===//

// TODO: Unsigned comparison
def MOVEQ32rr   : Mov32rr<"moveq",   [(set GPR32:$Rd, (select (i32 (seteq GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_EQ,    GPR32>;
def MOVNE32rr   : Mov32rr<"movne",   [(set GPR32:$Rd, (select (i32 (setne GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_NE,    GPR32>;
def MOVGTU32rr  : Mov32rr<"movgtu",  [(set GPR32:$Rd, (select (i32 (setgt GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_GTU,   GPR32>;
def MOVGTEU32rr : Mov32rr<"movgteu", [(set GPR32:$Rd, (select (i32 (setge GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_GTEU,  GPR32>;
def MOVLTEU32rr : Mov32rr<"movlteu", [(set GPR32:$Rd, (select (i32 (setle GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_LTEU,  GPR32>;
def MOVLTU32rr  : Mov32rr<"movltu",  [(set GPR32:$Rd, (select (i32 (setlt GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_LTU,   GPR32>;
def MOVGT32rr   : Mov32rr<"movgt",   [(set GPR32:$Rd, (select (i32 (setgt GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_GT,    GPR32>;
def MOVGTE32rr  : Mov32rr<"movgte",  [(set GPR32:$Rd, (select (i32 (setge GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_GTE,   GPR32>;
def MOVLT32rr   : Mov32rr<"movlt",   [(set GPR32:$Rd, (select (i32 (setlt GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_LT,    GPR32>;
def MOVLTE32rr  : Mov32rr<"movlte",  [(set GPR32:$Rd, (select (i32 (setle GPR32:$Rn, GPR32:$src)), GPR32:$Rn, GPR32:$src))], COND_LTE,   GPR32>;
//def MOVBEQ32rr  : Mov32rr<"movbeq",  [(set FPR32:$Rd, (select (i32 (seteq FPR32:$Rn, FPR32:$src)), FPR32:$Rn, FPR32:$src))], COND_BEQ,   FPR32>;
//def MOVBNE32rr  : Mov32rr<"movbne",  [(set FPR32:$Rd, (select (i32 (setne FPR32:$Rn, FPR32:$src)), FPR32:$Rn, FPR32:$src))], COND_BNE,   FPR32>;
//def MOVBLT32rr  : Mov32rr<"movblt",  [(set FPR32:$Rd, (select (i32 (setlt FPR32:$Rn, FPR32:$src)), FPR32:$Rn, FPR32:$src))], COND_BLT,   FPR32>;
//def MOVBLTE32rr : Mov32rr<"movblte", [(set FPR32:$Rd, (select (i32 (setle FPR32:$Rn, FPR32:$src)), FPR32:$Rn, FPR32:$src))], COND_BLTE,  FPR32>;



