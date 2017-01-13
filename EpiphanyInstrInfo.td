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
multiclass LoadM<PatFrag LoadType, LS_size LoadSize> {
  def _r16 : Load16<0, GPR16, LoadType, LoadSize>;
  def _r32 : Load32<0, GPR32, LoadType, LoadSize>;
}
multiclass StoreM<PatFrag StoreType, LS_size StoreSize> {
  def _r16 : Store16<0, GPR16, StoreType, StoreSize>;
  def _r32 : Store32<0, GPR32, StoreType, StoreSize>;
}

defm LDRi8   : LoadM<AlignedLoad<sextloadi8>,      LS_byte>;
defm LDRi8u  : LoadM<AlignedLoad<zextloadi8>,      LS_byte>;
defm LDRi16  : LoadM<AlignedLoad<sextloadi16>,     LS_hword>;
defm LDRi16u : LoadM<AlignedLoad<zextloadi16>,     LS_hword>;
defm LDRi32  : LoadM<AlignedLoad<load>,            LS_word>;
defm STRi8   : StoreM<AlignedStore<truncstorei8>,  LS_byte>;
defm STRi16  : StoreM<AlignedStore<truncstorei16>, LS_hword>;
defm STRi32  : StoreM<AlignedStore<store>,         LS_word>;

def LDRf32   : Load16<0, FPR32, AlignedLoad<load>,    LS_word>;
def STRf32   : Store16<0, FPR32, AlignedStore<store>, LS_word>;

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

  // Meta to use with tglobaladdr
  def MOVri : Mov32ri<"mov", (ins imm16:$Imm), [(set GPR32:$Rd, imm:$Imm)], 0b01011, /* MOVT = */ 0, GPR32>;

  // One more for memory
  def MOViPTR : Mov32ri<"mov", (ins mem11:$Imm), [(set GPR32:$Rd, addr11:$Imm)], 0b01011, /* MOVT = */ 0, GPR32>;
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

//===----------------------------------------------------------------------===//
// Move operations: Wrapper
//===----------------------------------------------------------------------===//
def SDT_MOV : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def MOV     : SDNode<"EpiphanyISD::MOV", SDT_MOV>;

def : Pat<(i32 (MOV tglobaladdr:$dst)),   (MOVi32ri tglobaladdr:$dst)>;
def : Pat<(i32 (MOV texternalsym:$dst)),  (MOVi32ri texternalsym:$dst)>;
def : Pat<(i32 (MOV tblockaddress:$dst)), (MOVi32ri tblockaddress:$dst)>;

//===----------------------------------------------------------------------===//
// Branching
//===----------------------------------------------------------------------===//
let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1, hasCtrlDep=1 in {
  def RTS : Pseudo32<(outs), (ins), [(EpiphanyRet)]>;
}

let isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR16 : JumpReg16<"jr", 0b0101000010, [(brind GPR16:$Rn)], COND_NONE>;
}

let isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR32 : JumpReg32<"jr", 0b0101001111, [(brind GPR32:$Rn)], COND_NONE>;
}

// Branches are made combining sub (cmp replacement) and b<cond>
def BNONE32 : Branch32<[(br bb:$addr)]>;
def BEQ32   : BranchCC32<COND_EQ>;
def BGTU32  : BranchCC32<COND_GTU>;
def BGTEU32 : BranchCC32<COND_GTEU>;
def BLTEU32 : BranchCC32<COND_LTEU>;
def BLTU32  : BranchCC32<COND_LTU>;
def BGT32   : BranchCC32<COND_GT>;
def BGTE32  : BranchCC32<COND_GTE>;
def BLT32   : BranchCC32<COND_LT>;
def BLTE32  : BranchCC32<COND_LTE>;

def : Pat<(brcc SETEQ,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BEQ32   (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETUGT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTU32  (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETUGE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTEU32 (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETULE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTEU32 (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETULT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTU32  (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETGT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGT32   (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETGE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTE32  (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETLT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLT32   (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETLE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTE32  (SUBi32rr GPR32:$Rd, GPR32:$Rn), bb:$addr)>;


