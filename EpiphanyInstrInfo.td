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

//===----------------------------------------------------------------------===//
// Interrupts and core control
//===----------------------------------------------------------------------===//
def NOP  : Interrupt<0b0110100010, [], "nop">;
def IDLE : Interrupt<0b0110110010, [], "idle">;
def GID  : Interrupt<0b1110010010, [], "gid">;
let isTerminator = 1, isBarrier = 1 in {
  def GIE  : Interrupt<0b0110010010, [], "gie">;
}

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
  def _r16 : LoadDisp16<0, GPR16, LoadType, LoadSize>;
  def _r32 : LoadDisp32<0, GPR32, LoadType, LoadSize>;
  def _idx_add_r16  : LoadIdx16<0, GPR16, LoadType, LoadSize, IndexAdd>;
  def _idx_add_r32  : LoadIdx32<0, GPR32, LoadType, LoadSize, IndexAdd>;
  def _idx_sub_r32  : LoadIdx32<0, GPR32, LoadType, LoadSize, IndexSub>;
  def _pm_add_r16   : LoadPm16<0, GPR16, LoadType, LoadSize, IndexAdd>;
  def _pm_add_r32   : LoadPm32<0, GPR32, LoadType, LoadSize, IndexAdd>;
  def _pm_sub_r32   : LoadPm32<0, GPR32, LoadType, LoadSize, IndexSub>;
  def _pmd_r32  : LoadPmd32<0, GPR32, LoadType, LoadSize>;
}
multiclass StoreM<PatFrag StoreType, LS_size StoreSize> {
  def _r16 : StoreDisp16<0, GPR16, StoreType, StoreSize>;
  def _r32 : StoreDisp32<0, GPR32, StoreType, StoreSize>;
  def _idx_add_r16 : StoreIdx16<0, GPR16, StoreType, StoreSize, IndexAdd>;
  def _idx_add_r32 : StoreIdx32<0, GPR32, StoreType, StoreSize, IndexAdd>;
  def _idx_sub_r32 : StoreIdx32<0, GPR32, StoreType, StoreSize, IndexSub>;
  def _pm_add_r16  : StorePm16<0, GPR16, StoreType, StoreSize, IndexAdd>;
  def _pm_add_r32  : StorePm32<0, GPR32, StoreType, StoreSize, IndexAdd>;
  def _pm_sub_r32  : StorePm32<0, GPR32, StoreType, StoreSize, IndexSub>;
  def _pmd_r32  : StorePmd32<0, GPR32, StoreType, StoreSize>;
}

defm LDRi8   : LoadM<AlignedLoad<sextloadi8>,      LS_byte>;
defm LDRi8u  : LoadM<AlignedLoad<extloadi8>,       LS_byte>;
defm LDRi8z  : LoadM<AlignedLoad<zextloadi8>,      LS_byte>;
defm LDRi16  : LoadM<AlignedLoad<sextloadi16>,     LS_hword>;
defm LDRi16u : LoadM<AlignedLoad<extloadi16>,      LS_hword>;
defm LDRi16z : LoadM<AlignedLoad<zextloadi16>,     LS_hword>;
defm LDRi32  : LoadM<AlignedLoad<load>,            LS_word>;
defm STRi8   : StoreM<AlignedStore<truncstorei8>,  LS_byte>;
defm STRi16  : StoreM<AlignedStore<truncstorei16>, LS_hword>;
defm STRi32  : StoreM<AlignedStore<store>,         LS_word>;

def LDRf32   : LoadDisp32<0, FPR32, AlignedLoad<load>,    LS_word>;
def STRf32   : StoreDisp32<0, FPR32, AlignedStore<store>, LS_word>;

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers
//===----------------------------------------------------------------------===//
multiclass SimpleMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : SimpleMath16rr<opcode16, instr_asm, OpNode, GPR16>;
  def _r32 : SimpleMath32rr<opcode32, instr_asm, OpNode, GPR32>;
}

let Defs = [STATUS] in {
  defm ADDrr  : SimpleMath<0b0011010, 0b0011111, "add", add >;
  defm SUBrr  : SimpleMath<0b0111010, 0b0111111, "sub", sub >;
  defm ADDCrr : SimpleMath<0b0011010, 0b0011111, "add", addc>;
  defm SUBCrr : SimpleMath<0b0111010, 0b0111111, "sub", subc>;
  defm ANDrr  : SimpleMath<0b1011010, 0b1011111, "and", and >;
  defm ORRrr  : SimpleMath<0b1111010, 0b1111111, "orr", or  >;
  defm EORrr  : SimpleMath<0b0001010, 0b0001111, "eor", xor >;
  defm ASRrr  : SimpleMath<0b1101010, 0b1101111, "asr", sra >;
  defm LSRrr  : SimpleMath<0b1001010, 0b1001111, "lsr", srl >;
  defm LSLrr  : SimpleMath<0b0101010, 0b0101111, "lsl", shl >;
}

// Complex math: f32
multiclass FPMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath16rr<opcode16, instr_asm, OpNode, FPR16, FpuItin>;
  def _r32 : ComplexMath32rr<opcode32, instr_asm, OpNode, FPR32, FpuItin>;
}
multiclass FPMath2<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath2_16rr<opcode16, instr_asm, OpNode, fmul, FPR16, FpuItin>;
  def _r32 : ComplexMath2_32rr<opcode32, instr_asm, OpNode, fmul, FPR32, FpuItin>;
}
let Defs = [STATUS] in {
  defm FADDrr : FPMath<0b0000111, 0b0001111, "fadd", fadd>;
  defm FSUBrr : FPMath<0b0010111, 0b0011111, "fsub", fsub>;
  defm FMULrr : FPMath<0b0100111, 0b0101111, "fmul", fmul>;
  defm FMADDrr : FPMath2<0b0110111, 0b0111111, "fmadd", fadd>;
  defm FMSUBrr : FPMath2<0b1000111, 0b1001111, "fmsub", fsub>;
}

// Complex math: i32
multiclass Ialu2Math<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath16rr<opcode16, instr_asm, OpNode, GPR16, Ialu2Itin>;
  def _r32 : ComplexMath32rr<opcode32, instr_asm, OpNode, GPR32, Ialu2Itin>;
}
multiclass Ialu2Math_2<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath2_16rr<opcode16, instr_asm, OpNode, mul, GPR16, Ialu2Itin>;
  def _r32 : ComplexMath2_32rr<opcode32, instr_asm, OpNode, mul, GPR32, Ialu2Itin>;
}
let Defs = [STATUS], AddedComplexity = -1 in {
  defm IADDrr : Ialu2Math<0b0000111, 0b0001111, "iadd", add>;
  defm ISUBrr : Ialu2Math<0b0010111, 0b0011111, "isub", sub>;
  defm IMULrr : Ialu2Math<0b0100111, 0b0101111, "imul", mul>;
  defm IMADDrr : Ialu2Math_2<0b0110111, 0b0111111, "imadd", add>;
  defm IMSUBrr : Ialu2Math_2<0b1000111, 0b1001111, "imsub", sub>;
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
// IntToFloat and Abs
//===----------------------------------------------------------------------===//
let Defs = [STATUS] in {
  def FLOAT32rr : IntToFloat32<(outs FPR32:$Rd), (ins GPR32:$Rn), "float\t$Rd, $Rn", [(set FPR32:$Rd, (sint_to_fp GPR32:$Rn))], 0b1011111, FpuItin>;
  def FIX32rr   : IntToFloat32<(outs GPR32:$Rd), (ins FPR32:$Rn), "fix\t$Rd, $Rn",   [(set GPR32:$Rd, (fp_to_sint FPR32:$Rn))], 0b1101111, FpuItin>;
}

//===----------------------------------------------------------------------===//
// Move operations: Immediates
//===----------------------------------------------------------------------===//
let Constraints = "$src = $Rd" in {
  def MOVTi32ri : Mov32ri<"movt", (ins GPR32:$src, imm16:$Imm), [(set GPR32:$Rd, (or (and GPR32:$src, 0xffff), (shl immSExt16:$Imm, (i32 16))))], 0b01011, /* MOVT = */ 1, GPR32>;
}
def MOVi16ri : Mov16ri<"mov", (ins imm8:$Imm),    [(set GPR16:$Rd, immSExt8:$Imm)], 0b00011, GPR16>;
def MOVi32ri : Mov32ri<"mov", (ins imm16:$Imm),   [(set GPR32:$Rd, imm:$Imm)],      0b01011, /* MOVT = */ 0, GPR32>;
def MOVf32ri : Mov32ri<"mov", (ins fpimm16:$Imm), [(set FPR32:$Rd, fpimm16:$Imm)],  0b01011, /* MOVT = */ 0, FPR32>;

// Special instruction to move memory pointer to the reg
def MOViPTR  : AddrMath32ri<(outs GPR32:$Rd), (ins mem11:$imm), "add \t$Rd, $imm", [(set GPR32:$Rd, addr11:$imm)],   0b0011011, IaluItin>;
def : Pat<(or GPR32:$src, 0xffff0000), (MOVTi32ri GPR32:$src, 0xffff)>;

//===----------------------------------------------------------------------===//
// Move operations: Registers
//===----------------------------------------------------------------------===//
def MOVi32rr     : Mov32rr<"mov", [], GPR32>;
def MOVf32rr     : Mov32rr<"mov", [], FPR32>;

def MOVFS32rr    : MovSpecial<"movfs", (outs GPR32:$Rd),   (ins SPECIAL:$MMR), [], CoreReg, SpecFrom>;
def MOVTS32rr    : MovSpecial<"movts", (outs SPECIAL:$MMR), (ins GPR32:$Rd),   [], CoreReg, SpecTo>;

let Uses = [STATUS], Constraints = "$src = $Rd" in {
  def MOVCC32rr : MovCond32rr<(outs GPR32:$Rd), (ins GPR32:$Rn, GPR32:$src, cc:$cc, GPR32:$sub), []>;
}

// Patterns to replace select_cc
// Converting select to movcc
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETEQ), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_EQ.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETUGT), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_GTU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETUGE), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_GTEU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETULE), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_LTEU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETULT), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_LTU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETGT), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_GT.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETGE), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_GTE.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETLT), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_LT.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(selectcc GPR32:$lhs, GPR32:$rhs, GPR32:$Rn, GPR32:$src, SETLE), 
          (MOVCC32rr GPR32:$src, GPR32:$Rn, COND_LTE.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;

// Patterns to replace setcc
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETEQ), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_EQ.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETUGT), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GTU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETUGE), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETULE), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_LTEU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETULT), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_LTU.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETGT), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GT.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETGE), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GTE.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETLT), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_LT.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;
def : Pat<(setcc GPR32:$lhs, GPR32:$rhs, SETLE), 
          (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_LTE.Code, (SUBrr_r32 GPR32:$lhs, GPR32:$rhs))>;

//===----------------------------------------------------------------------===//
// Move operations: Wrapper
//===----------------------------------------------------------------------===//
def SDT_MOV  : SDTypeProfile<1, 1, [SDTCisPtrTy<0>]>;
def SDT_MOVT : SDTypeProfile<1, 2, [SDTCisPtrTy<1>]>;
def MOV      : SDNode<"EpiphanyISD::MOV",  SDT_MOV>;
def MOVT     : SDNode<"EpiphanyISD::MOVT", SDT_MOVT>;
def : Pat<(i32 (MOV tglobaladdr:$dst)),   (MOVi32ri tglobaladdr:$dst)>;
def : Pat<(i32 (MOV texternalsym:$dst)),  (MOVi32ri texternalsym:$dst)>;
def : Pat<(i32 (MOV tblockaddress:$dst)), (MOVi32ri tblockaddress:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, tglobaladdr:$dst)),   (MOVTi32ri GPR32:$Rd, tglobaladdr:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, texternalsym:$dst)),  (MOVTi32ri GPR32:$Rd, texternalsym:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, tblockaddress:$dst)), (MOVTi32ri GPR32:$Rd, tblockaddress:$dst)>;

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
  def BNONE32 : Branch32<(ins branchtarget:$addr), [(br bb:$addr)], COND_NONE>;
}

let isBarrier = 1, hasDelaySlot = 0, Uses = [STATUS] in {
  def BCC32    : BranchCC32<(ins branchtarget:$addr, cc:$cc, GPR32:$Rd), []>;
}

// Patterns to use while replacing "brcc" (condition branch)
def : Pat<(brcc SETEQ,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_EQ.Code,   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETUGT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_GTU.Code,  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETUGE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_GTEU.Code, (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETULE, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_LTEU.Code, (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETULT, GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_LTU.Code,  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETGT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_GT.Code,   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETGE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_GTE.Code,  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETLT,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_LT.Code,   (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;
def : Pat<(brcc SETLE,  GPR32:$Rd, GPR32:$Rn, bb:$addr), (BCC32 bb:$addr, COND_LTE.Code,  (SUBrr_r32 GPR32:$Rd, GPR32:$Rn))>;

// Patterns to use while replacing "brcond"
// brcond takes only 1 arg, which should be true or false
def : Pat<(brcond GPR32:$Rd, bb:$addr), (BCC32 bb:$addr, COND_GTU.Code, (SUB32ri GPR32:$Rd, 0))>;

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

let isCall = 1, Defs = [LR], hasDelaySlot = 0, isBarrier = 0 in {
  def BL32 : Branch32<(ins branchlinktarget:$addr), [(EpiphanyCall tglobaladdr:$addr)], COND_L> {
    let isBranch = 1;
  }
  
  let isBarrier = 0 in {
    def JALR16 : JumpReg16<"jalr",   0b0101010010, [(EpiphanyCall GPR16:$Rn)], COND_NONE>;
    def JALR32 : JumpReg32<"jalr.l", 0b0101011111, [(EpiphanyCall GPR32:$Rn)], COND_NONE>;
  }
}

//===----------------------------------------------------------------------===//
// Additional integer arithmetic patterns
//===----------------------------------------------------------------------===//

// Extended Addsub i32, exploiting the fact that MOVGTEU depend only on the carry
def : Pat<(adde GPR32:$Rn, simm11:$imm), 
          (ADDrr_r32 (ADD32ri GPR32:$Rn, simm11:$imm), (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code, GPR32:$Rn))>;
def : Pat<(sube GPR32:$Rn, simm11:$imm), 
          (SUBrr_r32 (ADD32ri GPR32:$Rn, simm11:$imm), (MOVCC32rr (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code, GPR32:$Rn))>;

