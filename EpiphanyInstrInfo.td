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

def LO64 : SDNodeXForm<imm, [{
  return getImm(N, (N->getZExtValue() >> 32) & 0xffff);
}]>;

def HI64 : SDNodeXForm<imm, [{
  return getImm(N, (N->getZExtValue() >> 48) & 0xffff);
}]>;

// Pattern fragments to extract the low and high subregisters from a
// 64-bit value.
def LoReg: OutPatFrag<(ops node:$Rd), (EXTRACT_SUBREG (i64 $Rd), isub_lo)>;
def HiReg: OutPatFrag<(ops node:$Rd), (EXTRACT_SUBREG (i64 $Rd), isub_hi)>;

//===----------------------------------------------------------------------===//
// Bitcast
//===----------------------------------------------------------------------===//
multiclass bitconvert_32<ValueType a, ValueType b> {
  def : Pat <(b (bitconvert (a GPR32:$src))),
             (b GPR32:$src)>;
  def : Pat <(a (bitconvert (b GPR32:$src))),
             (a GPR32:$src)>;
}

// Bit convert vector types.
defm : bitconvert_32<v4i8,  i32>;
defm : bitconvert_32<v2i16, i32>;
defm : bitconvert_32<v2i16, v4i8>;

//===----------------------------------------------------------------------===//
// Load/store instructions
//===----------------------------------------------------------------------===//
multiclass LoadM<PatFrag LoadType, LS_size LoadSize, ValueType Ty> {
    def _r16          : LoadDisp16<0, GPR16, LoadType, LoadSize, Ty>;
    def _r32          : LoadDisp32<0, GPR32, LoadType, LoadSize, Ty>;
    def _idx_add_r16  : LoadIdx16<0, GPR16, LoadType, LoadSize, IndexAdd, Ty>;
    def _idx_add_r32  : LoadIdx32<0, GPR32, LoadType, LoadSize, IndexAdd, Ty>;
    def _idx_sub_r32  : LoadIdx32<0, GPR32, LoadType, LoadSize, IndexSub, Ty>;
    def _pm_add_r16   : LoadPm16<0, GPR16, LoadType, LoadSize, IndexAdd, Ty>;
    def _pm_add_r32   : LoadPm32<0, GPR32, LoadType, LoadSize, IndexAdd, Ty>;
    def _pm_sub_r32   : LoadPm32<0, GPR32, LoadType, LoadSize, IndexSub, Ty>;
    def _pmd_r32      : LoadPmd32<0, GPR32, LoadType, LoadSize, Ty>;
}
multiclass StoreM<PatFrag StoreType, LS_size StoreSize, ValueType Ty> {
  def _r16         : StoreDisp16<0, GPR16, StoreType, StoreSize, Ty>;
  def _r32         : StoreDisp32<0, GPR32, StoreType, StoreSize, Ty>;
  def _idx_add_r16 : StoreIdx16<0, GPR16, StoreType, StoreSize, IndexAdd, Ty>;
  def _idx_add_r32 : StoreIdx32<0, GPR32, StoreType, StoreSize, IndexAdd, Ty>;
  def _idx_sub_r32 : StoreIdx32<0, GPR32, StoreType, StoreSize, IndexSub, Ty>;
  def _pm_add_r16  : StorePm16<0, GPR16, StoreType, StoreSize, IndexAdd, Ty>;
  def _pm_add_r32  : StorePm32<0, GPR32, StoreType, StoreSize, IndexAdd, Ty>;
  def _pm_sub_r32  : StorePm32<0, GPR32, StoreType, StoreSize, IndexSub, Ty>;
  def _pmd_r32     : StorePmd32<0, GPR32, StoreType, StoreSize, Ty>;
}

defm LDRi8s  : LoadM<AlignedLoad<sextloadi8>,      LS_byte,  i32>;
defm LDRi8e  : LoadM<AlignedLoad<extloadi8>,       LS_byte,  i32>;
defm LDRi8z  : LoadM<AlignedLoad<zextloadi8>,      LS_byte,  i32>;
defm LDRi16s : LoadM<AlignedLoad<sextloadi16>,     LS_hword, i32>;
defm LDRi16e : LoadM<AlignedLoad<extloadi16>,      LS_hword, i32>;
defm LDRi16z : LoadM<AlignedLoad<zextloadi16>,     LS_hword, i32>;
defm LDRi32  : LoadM<AlignedLoad<load>,            LS_word,  i32>;
defm LDRi32s : LoadM<AlignedLoad<sextloadi32>,     LS_word,  i32>;
defm LDRi32z : LoadM<AlignedLoad<zextloadi32>,     LS_word,  i32>;
defm STRi8   : StoreM<AlignedStore<truncstorei8>,  LS_byte,  i32>;
defm STRi16  : StoreM<AlignedStore<truncstorei16>, LS_hword, i32>;
defm STRi32  : StoreM<AlignedStore<store>,         LS_word,  i32>;
defm STRi32t : StoreM<AlignedStore<truncstorei32>, LS_word,  i32>;

def LDRf32e  : LoadDisp32<0,  FPR32, AlignedLoad<extloadf32>,     LS_word,  f32>;
def STRf32t  : StoreDisp32<0, FPR32, AlignedStore<truncstoref32>, LS_word,  f32>;
def LDRf32   : LoadDisp32<0,  FPR32, AlignedLoad<load>,   LS_word,  f32>;
def STRf32   : StoreDisp32<0, FPR32, AlignedStore<store>, LS_word,  f32>;

def LDRi64     : LoadDisp32<0, GPR64, AlignedLoad<load>,    LS_dword, i64>;
def LDRi64_pmd : LoadPmd32<0, GPR64, AlignedLoad<load>, LS_dword, i64>;
def STRi64     : StoreDisp32<0, GPR64, AlignedStore<store>, LS_dword, i64>;
def STRi64_pmd : StorePmd32<0, GPR64, AlignedStore<store>, LS_dword, i64>;

def LDRv2i32 : LoadDisp32<0, GPR64, AlignedLoad<load>,    LS_dword, v2i32>;
def STRv2i32 : StoreDisp32<0, GPR64, AlignedStore<store>, LS_dword, v2i32>;

def LDRf64t : LoadDisp32<0, FPR64, AlignedLoad<extloadf64>,      LS_dword, f64>;
def STRf64e : StoreDisp32<0, FPR64, AlignedStore<truncstoref64>, LS_dword, f64>;
def LDRf64  : LoadDisp32<0, FPR64, AlignedLoad<load>,            LS_dword, f64>;
def STRf64  : StoreDisp32<0, FPR64, AlignedStore<store>,         LS_dword, f64>;

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers
//===----------------------------------------------------------------------===//

// Node used in comparisons
def SDT_CMP : SDTypeProfile<1, 2, []>;
def CMP : SDNode<"EpiphanyISD::CMP",   SDT_CMP, [SDNPOptInGlue]>;

multiclass SimpleMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : SimpleMath16rr<opcode16, instr_asm, OpNode, GPR16, i32> { let Defs = [STATUS]; }
  def _r32 : SimpleMath32rr<opcode32, instr_asm, OpNode, GPR32, i32> { let Defs = [STATUS]; }
}

// COPY is used as REG_SEQUENCE does not accept multioutput functions (e.g. those defining STATUS reg)
multiclass SimpleMath64<SDNode OpNode> {
  def _r64 : Pat<(i64 (OpNode GPR64:$Rn, GPR64:$Rm)),
                 (REG_SEQUENCE GPR64, 
                 (i32 (COPY (!cast<Instruction>(NAME # _r32) (LoReg GPR64:$Rn), (LoReg GPR64:$Rm)))), isub_lo, 
                 (i32 (COPY (!cast<Instruction>(NAME # _r32) (HiReg GPR64:$Rn), (HiReg GPR64:$Rm)))), isub_hi)>;
}

let isCompare = 1 in {
  defm ADDrr  : SimpleMath<0b0011010, 0b0011111, "add", add >;
  defm SUBrr  : SimpleMath<0b0111010, 0b0111111, "sub", sub >;
  defm CMPrr  : SimpleMath<0b0111010, 0b0111111, "sub", CMP >;
  defm ADDCrr : SimpleMath<0b0011010, 0b0011111, "add", addc>;
  defm SUBCrr : SimpleMath<0b0111010, 0b0111111, "sub", subc>;
}
defm ANDrr  : SimpleMath<0b1011010, 0b1011111, "and", and >, SimpleMath64<and>;
defm ORRrr  : SimpleMath<0b1111010, 0b1111111, "orr", or  >, SimpleMath64<or >;
defm EORrr  : SimpleMath<0b0001010, 0b0001111, "eor", xor >, SimpleMath64<xor>;
defm ASRrr  : SimpleMath<0b1101010, 0b1101111, "asr", sra >;
defm LSRrr  : SimpleMath<0b1001010, 0b1001111, "lsr", srl >;
defm LSLrr  : SimpleMath<0b0101010, 0b0101111, "lsl", shl >;

// Complex math: f32
multiclass FPMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath16rr<opcode16, instr_asm, OpNode, FPR16, FpuItin, f32>;
  def _r32 : ComplexMath32rr<opcode32, instr_asm, OpNode, FPR32, FpuItin, f32>;
}
multiclass FPMath2<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath2_16rr<opcode16, instr_asm, OpNode, fmul, FPR16, FpuItin, f32>;
  def _r32 : ComplexMath2_32rr<opcode32, instr_asm, OpNode, fmul, FPR32, FpuItin, f32>;
}
let Defs = [STATUS] in {
  defm FADDrr  : FPMath<0b0000111, 0b0001111, "fadd", fadd>;
  defm FSUBrr  : FPMath<0b0010111, 0b0011111, "fsub", fsub>;
  defm FCMPrr  : FPMath<0b0010111, 0b0011111, "fsub", CMP>;
  defm FMULrr  : FPMath<0b0100111, 0b0101111, "fmul", fmul>;
  defm FMADDrr : FPMath2<0b0110111, 0b0111111, "fmadd", fadd>;
  defm FMSUBrr : FPMath2<0b1000111, 0b1001111, "fmsub", fsub>;
}

// Complex math: i32
multiclass Ialu2Math<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath16rr<opcode16, instr_asm, OpNode, GPR16, Ialu2Itin, i32>;
  def _r32 : ComplexMath32rr<opcode32, instr_asm, OpNode, GPR32, Ialu2Itin, i32>;
}
multiclass Ialu2Math_2<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16 : ComplexMath2_16rr<opcode16, instr_asm, mul, OpNode, GPR16, Ialu2Itin, i32>;
  def _r32 : ComplexMath2_32rr<opcode32, instr_asm, mul, OpNode, GPR32, Ialu2Itin, i32>;
}
let Defs = [STATUS], AddedComplexity = 1 in {
  defm IADDrr : Ialu2Math<0b0000111, 0b0001111, "iadd", add>;
  defm ISUBrr : Ialu2Math<0b0010111, 0b0011111, "isub", sub>;
  defm IMULrr : Ialu2Math<0b0100111, 0b0101111, "imul", mul>;
  defm IMADDrr : Ialu2Math_2<0b0110111, 0b0111111, "imadd", add>;
  defm IMSUBrr : Ialu2Math_2<0b1000111, 0b1001111, "imsub", sub>;
}

//===----------------------------------------------------------------------===//
// Integer arithmetic operations with immediates
//===----------------------------------------------------------------------===//
multiclass IntMath<bits<7> opcode16, bits<7> opcode32, string instr_asm, SDNode OpNode> {
  def _r16     : IntMath_r16_i32<opcode16, instr_asm, OpNode, i32imm, i32immSExt3,  i32>;
  def _r32     : IntMath_r32_i32<opcode32, instr_asm, OpNode, i32imm, i32immSExt11, i32>;
}

let Defs = [STATUS] in {
  // Addsub
  let isCompare = 1 in {
    defm ADDri  : IntMath<0b0010011, 0b0011011, "add", add >;
    defm SUBri  : IntMath<0b0110011, 0b0111011, "sub", sub >;
    defm CMPri  : IntMath<0b0110011, 0b0111011, "sub", CMP >;
    defm ADDCri : IntMath<0b0010011, 0b0011011, "add", addc>;
    defm SUBCri : IntMath<0b0110011, 0b0111011, "sub", subc>;
  }

  // Shifts i16
  def LSR16ri  : ShiftMath16ri<0b00110, "lsr",  srl,        imm5, immUExt5, i32>;
  def LSL16ri  : ShiftMath16ri<0b10110, "lsl",  shl,        imm5, immUExt5, i32>;
  def ASR16ri  : ShiftMath16ri<0b01110, "asr",  sra,        imm5, immUExt5, i32>;

  // Shifts i32
  def LSR32ri  : ShiftMath32ri<0b0110, 0b01111, "lsr",  srl,        imm5, immUExt5, i32>;
  def LSL32ri  : ShiftMath32ri<0b0110, 0b11111, "lsl",  shl,        imm5, immUExt5, i32>;
  def ASR32ri  : ShiftMath32ri<0b1110, 0b01111, "asr",  sra,        imm5, immUExt5, i32>;

  def BITR16ri : Bitr16ri<0b11110, i32>;
  def BITR32ri : Bitr32ri<0b1110, 0b11111, i32>;
}

//===----------------------------------------------------------------------===//
// IntToFloat and Abs
//===----------------------------------------------------------------------===//
def SDT_FIX   : SDTypeProfile<1, 1, [SDTCisVT<0, i32>, SDTCisVT<1, f32>]>;
def SDT_FLOAT : SDTypeProfile<1, 1, [SDTCisVT<0, f32>, SDTCisVT<1, i32>]>;
def FIX   : SDNode<"EpiphanyISD::FIX",   SDT_FIX>;
def FLOAT : SDNode<"EpiphanyISD::FLOAT", SDT_FLOAT>;

let Defs = [STATUS] in {
  def FLOAT32rr : IntToFloat32<(outs FPR32:$Rd), (ins GPR32:$Rn), "float\t$Rd, $Rn", 0b1011111, 
    [(set FPR32:$Rd, (FLOAT GPR32:$Rn))], FpuItin>;
  def FIX32rr   : IntToFloat32<(outs GPR32:$Rd), (ins FPR32:$Rn), "fix\t$Rd, $Rn",   0b1101111,
    [(set GPR32:$Rd, (FIX FPR32:$Rn))],   FpuItin>;
}

//===----------------------------------------------------------------------===//
// Move operations: Immediates
//===----------------------------------------------------------------------===//
let Constraints = "$src = $Rd" in {
  def MOVTi32ri : Mov32ri<"movt", (ins GPR32:$src, i32imm:$Imm), [(set (i32 GPR32:$Rd), (or (and (i32 GPR32:$src), 0xffff), (shl i32immSExt16:$Imm, (i32 16))))], 0b01011, /* MOVT = */ 1, GPR32>;
  def MOVTf32ri : Mov32ri<"movt", (ins FPR32:$src, f32imm:$Imm), [],     0b01011, /* MOVT = */ 1, FPR32>;
}
def MOVi16ri     : Mov16ri<"mov", (ins i32imm:$Imm), [(set (i32 GPR16:$Rd), i32immSExt8:$Imm)],  0b00011, GPR16>;
def MOVi32ri     : Mov32ri<"mov", (ins i32imm:$Imm), [(set (i32 GPR32:$Rd), i32immSExt16:$Imm)], 0b01011, /* MOVT = */ 0, GPR32>;
def MOVi32ri_r32 : Pat<(i32immSExt32:$imm), (i32 (MOVTi32ri (MOVi32ri (LO16 $imm)), (HI16 $imm)))>;

def MOVf16ri_r32 : Mov32ri<"mov", (ins f32imm:$Imm), [(set (f32 FPR32:$Rd), f32imm16:$Imm)],      0b01011, /* MOVT = */ 0, FPR32>;
def MOVf32ri_r32 : Pat<(f32imm32:$imm), (f32 (MOVTf32ri (MOVf16ri_r32 (LO16 $imm)), (HI16 $imm)))>;

// Special instruction to move memory pointer to the reg
def MOViPTR  : AddrMath32ri<(outs GPR32:$Rd), (ins smem11:$imm), "add \t$Rd, $imm", [(set GPR32:$Rd, addr11:$imm)],   0b0011011, IaluItin>;
def : Pat<(or (i32 GPR32:$src), 0xffff0000), (MOVTi32ri (i32 GPR32:$src), 0xffff)>;

// SIMD patterns
def MOVri_v2i16_r32 : Pat<(v2i16 (imm:$imm)), (v2i16 (MOVTi32ri (MOVi32ri (LO16 $imm)), (HI16 $imm)))>;

// 64-bit move i64 pattern
def MOVi64ri_i16 : Pat<(i64immSExt32:$Imm),
    (REG_SEQUENCE GPR64, 
    (MOVi32ri 0), isub_hi, 
    (MOVi32ri (LO16 i64:$Imm)), isub_lo)>;

def MOVi64ri_i32 : Pat<(i64immSExt32:$Imm),
    (REG_SEQUENCE GPR64, 
    (MOVi32ri 0), isub_hi,
    (MOVTi32ri (MOVi32ri (LO64 i64:$Imm)), (HI64 i64:$Imm)), isub_lo)>;

def MOVi64ri_i64 : Pat<(i64immSExt64:$Imm), 
    (REG_SEQUENCE GPR64,
    (i32 (MOVTi32ri (i32 (MOVi32ri (i32 (LO16 i64:$Imm)))), (i32 (HI16 i64:$Imm)))), isub_hi, 
    (i32 (MOVTi32ri (i32 (MOVi32ri (i32 (LO64 i64:$Imm)))), (i32 (HI64 i64:$Imm)))), isub_lo)>;

//===----------------------------------------------------------------------===//
// Move operations: Registers
//===----------------------------------------------------------------------===//
def MOVi32rr : Mov32rr<"mov", [], GPR32>;
def MOVf32rr : Mov32rr<"mov", [], FPR32>;

// Special regs
def MOVFS32rr    : MovSpecial<"movfs", (outs GPR32:$Rd),   (ins SPECIAL:$MMR), [], CoreReg, SpecFrom>;
def MOVTS32rr    : MovSpecial<"movts", (outs SPECIAL:$MMR), (ins GPR32:$Rd),   [], CoreReg, SpecTo>;

//===----------------------------------------------------------------------===//
// Move operations: Wrapper, see EpiphanyISelLowering.cpp
//===----------------------------------------------------------------------===//
def SDT_MOV   : SDTypeProfile<1, 1, [SDTCisVT<0, i32>]>;
def SDT_MOVT  : SDTypeProfile<1, 2, [SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_MOVCC : SDTypeProfile<1, 4, [SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def MOV       : SDNode<"EpiphanyISD::MOV",   SDT_MOV>;
def MOVT      : SDNode<"EpiphanyISD::MOVT",  SDT_MOVT>;
def MOVCCsd   : SDNode<"EpiphanyISD::MOVCC", SDT_MOVCC, [SDNPHasChain]>;
def : Pat<(i32 (MOV tglobaladdr:$dst)),   (MOVi32ri tglobaladdr:$dst)>;
def : Pat<(i32 (MOV texternalsym:$dst)),  (MOVi32ri texternalsym:$dst)>;
def : Pat<(i32 (MOV tblockaddress:$dst)), (MOVi32ri tblockaddress:$dst)>;
def : Pat<(i32 (MOV tconstpool:$dst)),    (MOVi32ri tconstpool:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, tglobaladdr:$dst)),   (MOVTi32ri GPR32:$Rd, tglobaladdr:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, texternalsym:$dst)),  (MOVTi32ri GPR32:$Rd, texternalsym:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, tblockaddress:$dst)), (MOVTi32ri GPR32:$Rd, tblockaddress:$dst)>;
def : Pat<(i32 (MOVT GPR32:$Rd, tconstpool:$dst)),    (MOVTi32ri GPR32:$Rd, tconstpool:$dst)>;


let Constraints = "$src = $Rd", Uses = [STATUS] in {
  def MOVCC : MovCond32rr<(outs GPR32:$Rd), (ins GPR32:$Rn, GPR32:$src, cc:$cc), 
      [(set GPR32:$Rd, (MOVCCsd (i32 GPR32:$Rn), (i32 GPR32:$src), (i32 i32immSExt32:$cc), STATUS))]>;
}

//===----------------------------------------------------------------------===//
// Branching
//===----------------------------------------------------------------------===//
def SDT_BRCC  : SDTypeProfile<0, 3, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i32>]>;
def BRCC      : SDNode<"EpiphanyISD::BRCC",  SDT_BRCC, [SDNPHasChain]>;

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

let isTerminator = 1, isBranch = 1, isBarrier = 0, hasDelaySlot = 0, Uses = [STATUS] in {
  def BCC : BranchCC32<(ins branchtarget:$addr, cc:$cc), [(BRCC bb:$addr, i32immSExt32:$cc, STATUS)]>;
}

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
  
  let isBarrier = 0, isTerminator = 0 in {
    def JALR16 : JumpReg16<"jalr",   0b0101010010, [(EpiphanyCall GPR16:$Rn)], COND_NONE>;
    def JALR32 : JumpReg32<"jalr.l", 0b0101011111, [(EpiphanyCall GPR32:$Rn)], COND_NONE>;
  }
}

//===----------------------------------------------------------------------===//
// Additional integer arithmetic patterns
//===----------------------------------------------------------------------===//

// Extended Addsub i32, exploiting the fact that MOVGTEU depend only on the carry
def : Pat<(adde GPR32:$Rn, simm11:$imm), 
          (ADDrr_r32 (ADDri_r32 GPR32:$Rn, simm11:$imm), (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code))>;
def : Pat<(sube GPR32:$Rn, simm11:$imm), 
          (SUBrr_r32 (ADDri_r32 GPR32:$Rn, simm11:$imm), (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code))>;

// COPY is used as REG_SEQUENCE does not accept multioutput functions (e.g. those defining STATUS reg)
def ADDrr_r64 : Pat<(i64 (add GPR64:$Rn, GPR64:$Rm)),
  (REG_SEQUENCE GPR64,
  (i32 (COPY (ADDrr_r32 (LoReg GPR64:$Rn), (LoReg GPR64:$Rm)))), 
  isub_lo,
  (i32 (COPY (ADDrr_r32 (HiReg GPR64:$Rn), 
    (ADDri_r32 (HiReg GPR64:$Rm), 
      (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code))))), 
  isub_hi)>;

// Sub64 patfrag, as we will need it in many places later
def SUBrr_r64_pat : OutPatFrag<(ops node:$Rn, node:$Rm), 
  (REG_SEQUENCE GPR64,
  (i32 (COPY (SUBrr_r32 (LoReg $Rn), (LoReg $Rm)))), isub_lo,
  (i32 (COPY (SUBrr_r32 (HiReg $Rn), 
    (SUBri_r32 (HiReg $Rm), 
      (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code))))), 
  isub_hi)>;

// SUB and SUBC patterns for i64
def SUBrr_r64  : Pat<(i64 (sub GPR64:$Rn, GPR64:$Rm)), (SUBrr_r64_pat GPR64:$Rn, GPR64:$Rm)>;
def SUBCrr_r64 : Pat<(i64 (subc GPR64:$Rn, GPR64:$Rm)), (SUBrr_r64_pat GPR64:$Rn, GPR64:$Rm)>;

// COPY is used as REG_SEQUENCE does not accept multioutput functions (e.g. those defining STATUS reg)
def ADDCrr_r64 : Pat<(i64 (addc GPR64:$Rn, GPR64:$Rm)), 
  (REG_SEQUENCE GPR64,
  (i32 (COPY (ADDrr_r32 (LoReg GPR64:$Rn), (LoReg GPR64:$Rm)))), isub_lo,
  (i32 (COPY (ADDrr_r32 (HiReg GPR64:$Rn), 
    (ADDri_r32 (HiReg GPR64:$Rm), 
      (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code))))), 
  isub_hi)>;

// COPY is used as REG_SEQUENCE does not accept multioutput functions (e.g. those defining STATUS reg)
def ADDri_r64 : Pat<(i64 (add (i64 GPR64:$Rn), i64immSExt11:$imm)),
  (REG_SEQUENCE GPR64,
  (i32 (COPY (ADDri_r32 (LoReg (i64 GPR64:$Rn)), (LO16 i64:$imm)))), isub_lo,
  (i32 (COPY (ADDrr_r32 (HiReg (i64 GPR64:$Rn)), 
    (MOVCC (MOVi32ri 0), (MOVi32ri 1), COND_GTEU.Code)))), isub_hi)>;

def : Pat<(i64 (shl GPR64:$Rn, (i32 32))), 
          (REG_SEQUENCE GPR64,
          (MOVi32rr (EXTRACT_SUBREG GPR64:$Rn, isub_lo)), isub_hi,
          (MOVi32ri 0), isub_lo)>;

def : Pat<(i32 (trunc GPR64:$Rn)), (LoReg GPR64:$Rn)>;

def ZERO_EXTENDi64 : Pat<(i64 (zext GPR32:$Rn)), 
                        (REG_SEQUENCE GPR64, 
                        (MOVi32ri 0), isub_hi, 
                        (MOVi32rr GPR32:$Rn), isub_lo)>;

def SIGN_EXTENDi64 : Pat<(i64 (sext GPR32:$Rn)), 
                        (REG_SEQUENCE GPR64, 
                        (i32 (COPY (ASR32ri GPR32:$Rn, 31))), isub_hi, 
                        (MOVi32rr GPR32:$Rn), isub_lo)>;

def ANY_EXTENDi64 : Pat<(i64 (anyext GPR32:$Rn)), 
                        (REG_SEQUENCE GPR64, 
                        (i32 (COPY (ASR32ri GPR32:$Rn, 31))), isub_hi, 
                        (MOVi32rr GPR32:$Rn), isub_lo)>;

