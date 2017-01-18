//===---- EpiphanyInstrInfo.td - Epiphany Instruction Info ---*- tablegen -*-=//
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
// Note for self and anyone who may read it (mini-manual as the web one sucks):
//   MC  - Machine code. In short and in general, binary.
//   DAG - Directed Acyclic Graph. Think of it as if you're solving an equation
//         with pen and paper. Solution steps = DAG
//   Lowering - Changing instructions in DAG. Think of it as solving an equation
//              and substituting one form (i.e. with variables) with another
//              (i.e. with numbers).
// 
// Most of the classes (except some very final multiclasses) are kept in
//   EpiphanyInstrFormats.td. It's my personal preference.
//
// Classes are used to set up main parameters, and usually only require opcode
//   and/or RegClass to be used.
//
// Multiclasses are used to make definitions shorter. For example:
//     multiclass LoadM<> {
//       def _r16: Load16<>;
//       def _r32: Load32<>;
//     }
//     defm LDR : LoadM<>;
//   Is the same as writing to defs for Load16 and Load32. 
//   Names are LDR_r16 and LDR_r32
//
// For how SDNodes are used see EpiphanyISelLowering.cpp, look for "Custom"
//
// Pseudo instructions are used in the same way as SDNodes, i.e. when we need
//   to show that there is an instruction, but we will disclose how it should
//   look later on. They may even have their patterns empty if they are matched
//   from the C++ code.
//
// Patterns (Pat keyword) are used to replace one DAG value with another. For
//   example 
//     def : Pat<(add A, B, C), (mul A, B, C)>;
//   will replace addition with multiplication (yeah, stupid example, i know).
//
// Complex patterns have some function attached, i.e. "SelectAddr" for the 
//   address pattern. See EpiphanyISelDAGToDAG how those are used.
//   Those functions return true if the operand is valid for this pattern,
//   and false otherwise (i.e. if we expect 1 byte value, int 1 is true, and 
//   int 2^62 is false);
//
// Custom operands have additional methods for them, such as EncoderMethod and
//   PrintMethod. Note that Encoder = MC printing, Printer = Asm printing.
//
//
//
// Petr Belyaev <upcfrost@gmail.com>
//===----------------------------------------------------------------------===//

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
// Arithmetic operations with registers
//===----------------------------------------------------------------------===//
multiclass SimpleMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : SimpleMath16rr<opcode16, instr_asm, OpNode, GPR16>;
  def _r32 : SimpleMath32rr<opcode32, instr_asm, OpNode, GPR32>;
}

let Defs = [STATUS] in {
  defm ADDrr : SimpleMath<0b0011010, 0b0011111, "add", addc>;
  defm SUBrr : SimpleMath<0b0111010, 0b0111111, "sub", subc>;
  defm ANDrr : SimpleMath<0b1011010, 0b1011111, "and", and >;
  defm ORRrr : SimpleMath<0b1111010, 0b1111111, "orr", or  >;
  defm EORrr : SimpleMath<0b0001010, 0b0001111, "eor", xor >;
  defm ASRrr : SimpleMath<0b1101010, 0b1101111, "asr", sra >;
  defm LSRrr : SimpleMath<0b1001010, 0b1001111, "lsr", srl >;
  defm LSLrr : SimpleMath<0b0101010, 0b0101111, "lsl", shl >;
}

// Complex math: i32/f32
multiclass ComplexMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNodeI, SDNode OpNodeF> {
  def _i32_r16 : ComplexMath16rr<opcode16, !strconcat("i", instr_asm), OpNodeI, GPR16>;
  def _i32_r32 : ComplexMath32rr<opcode32, !strconcat("i", instr_asm), OpNodeI, GPR32>;
  def _f32_r32 : ComplexMath32rr<opcode32, !strconcat("f", instr_asm), OpNodeF, FPR32>;
}

let Defs = [STATUS] in {
  defm ADDrr : ComplexMath<0b0000111, 0b0011111, "add", add, fadd>;
  defm SUBrr : ComplexMath<0b0010111, 0b0111111, "sub", sub, fsub>;
  defm MULrr : ComplexMath<0b0100111, 0b0101111, "mul", mul, fmul>;
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
// Return
let isReturn=1, isTerminator=1, hasDelaySlot=1, isBarrier=1, hasCtrlDep=1 in {
  def RTS : Pseudo32<(outs), (ins), [(EpiphanyRet)]>;
}

// JR
let isBarrier=1, hasDelaySlot = 1, isIndirectBranch = 1 in {
  def JR16 : JumpReg16<"jr", 0b0101000010, [(brind GPR16:$Rn)], COND_NONE>;
  def JR32 : JumpReg32<"jr", 0b0101001111, [(brind GPR32:$Rn)], COND_NONE>;
}

// Branches are made combining sub (cmp replacement) and b<cond>
let isBarrier = 1, hasDelaySlot = 0 in {
  def BNONE32 : Branch32<(ins jmptarget:$addr), [(br bb:$addr)], COND_NONE>;
  def BEQ32   : BranchCC32<COND_EQ>;
  def BGTU32  : BranchCC32<COND_GTU>;
  def BGTEU32 : BranchCC32<COND_GTEU>;
  def BLTEU32 : BranchCC32<COND_LTEU>;
  def BLTU32  : BranchCC32<COND_LTU>;
  def BGT32   : BranchCC32<COND_GT>;
  def BGTE32  : BranchCC32<COND_GTE>;
  def BLT32   : BranchCC32<COND_LT>;
  def BLTE32  : BranchCC32<COND_LTE>;
}

// Patterns to use while replacing "brcc" (condition branch)
def : Pat<(brcc SETEQ,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BEQ32   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETUGT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTU32  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETUGE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTEU32 (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETULE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTEU32 (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETULT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTU32  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETGT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGT32   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETGE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BGTE32  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETLT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLT32   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;
def : Pat<(brcc SETLE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BLTE32  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn), bb:$addr)>;

//===----------------------------------------------------------------------===//
// Function Calls and JALR
//===----------------------------------------------------------------------===//
// Types
def SDT_CallSeqStart : SDCallSeqStart<[ SDTCisPtrTy<0> ]>;
def SDT_CallSeqEnd   : SDCallSeqEnd<[ SDTCisPtrTy<0>, SDTCisPtrTy<1> ]>; 
def SDT_JmpLink      : SDTypeProfile<0, 1, [SDTCisVT<0, iPTR>]>;

// Nodes
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart, [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_CallSeqEnd, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def EpiphanyCall  : SDNode<"EpiphanyISD::Call",  SDT_JmpLink, [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

// Pseudo instructions (see EpiphanyInstrInfo.cpp)
let Defs = [SP], Uses = [SP] in {
  def ADJCALLSTACKDOWN : Pseudo32<(outs), (ins i32imm:$amt), [(callseq_start timm:$amt)]>;
  def ADJCALLSTACKUP   : Pseudo32<(outs), (ins i32imm:$amt1, i32imm:$amt2), [(callseq_end timm:$amt1, timm:$amt2)]>;
}

let isCall = 1, Defs = [LR], hasDelaySlot = 0 in {
  def BL32 : Branch32<(ins calltarget:$addr), [(EpiphanyCall tglobaladdr:$addr)], COND_L> {
    let isBranch = 1;
  }
  
  let isBarrier = 0 in {
    def JALR16 : JumpReg16<"jalr",   0b0101010010, [(EpiphanyCall GPR16:$Rn)], COND_NONE>;
    def JALR32 : JumpReg32<"jalr.l", 0b0101011111, [(EpiphanyCall GPR32:$Rn)], COND_NONE>;
  }
}
