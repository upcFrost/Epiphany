//===- EpiphanyInstrFormats.td - Epiphany Instruction Formats --*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This file describes Epiphany instruction formats, down to the level of the
// instruction's overall class.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Epiphany Instruction Format Definitions.
//===----------------------------------------------------------------------===//

class Format<bits<2> val> {
  bits<2> Value = val;
}

def PseudoFrm  : Format<0>;
def NormalFrm  : Format<1>;

// Epiphany instruction format (general)
class EpiphanyInst<Format f, string cstr> : Instruction {
  field bits<32>    Inst; // Instruction encoding
  let Namespace     = "Epiphany";
  Format F          = f;
  bits<2> Form      = F.Value;
  let Pattern       = [];
  let Constraints   = cstr;
  let isCodeGenOnly = !eq(!cast<string>(f), "PseudoFrm");
}

// Pseudo instructions (without encoding info)
class Pseudo<dag outs, dag ins, list<dag> pattern, InstrItinClass itin = NoItinerary, string cstr = ""> 
    : EpiphanyInst<PseudoFrm, cstr> {
  let OutOperandList  = outs;
  let InOperandList   = ins;
  let Pattern         = pattern;
  let isCodeGenOnly   = 1;
  let isPseudo        = 1;
  let Itinerary       = itin;
}

// Real instructions (with encoding info)
class Encoded<string cstr, list<dag> pattern> : EpiphanyInst<NormalFrm, cstr> {
  let Pattern = pattern;
  let Size = 4;
}

// Normal instructions
class Normal<dag outs, dag ins, string asm, list<dag> pattern, InstrItinClass itin, string cstr = ""> 
    : Encoded<cstr, pattern> {
  dag OutOperandList = outs;
  dag InOperandList  = ins;
  let AsmString      = asm;
  let Itinerary      = itin;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Epiphany Operand Definitions.
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
class ImmAsmOperand : AsmOperandClass { let RenderMethod = "addImmOperands"; }

// Immediate classes
def Imm3_Operand   : ImmAsmOperand { let Name = "imm3";   }
def Imm8_Operand   : ImmAsmOperand { let Name = "imm8";   }
def Imm11_Operand  : ImmAsmOperand { let Name = "imm11";  }
def Imm16_Operand  : ImmAsmOperand { let Name = "imm16";  }
def Simm3_Operand  : ImmAsmOperand { let Name = "simm3";  }
def Simm8_Operand  : ImmAsmOperand { let Name = "simm8";  }
def Simm11_Operand : ImmAsmOperand { let Name = "simm11"; }
def Simm24_Operand : ImmAsmOperand { let Name = "simm24"; }
// Immediates
let OperandType = "OPERAND_IMMEDIATE" in {
  def imm3   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 8);     }]> { let ParserMatchClass = Imm3_Operand;   }
  def imm8   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 255);   }]> { let ParserMatchClass = Imm8_Operand;   }
  def imm11  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 2047);  }]> { let ParserMatchClass = Imm11_Operand;  }
  def imm16  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 65535); }]> { let ParserMatchClass = Imm16_Operand;  }
  def simm3  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -4       && Imm < 3);       }]> { let ParserMatchClass = Simm3_Operand;  }
  def simm8  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -128     && Imm < 127);     }]> { let ParserMatchClass = Simm8_Operand;  }
  def simm11 : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -1024    && Imm < 1023);    }]> { let ParserMatchClass = Simm11_Operand; }
  def simm24 : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -8388608 && Imm < 8388607); }]> { let ParserMatchClass = Simm24_Operand; }
}
def immSExt3   : PatLeaf<(imm), [{ return isInt<3>(N->getSExtValue()); }]>;
def immSExt8   : PatLeaf<(imm), [{ return isInt<8>(N->getSExtValue()); }]>;
def immSExt11  : PatLeaf<(imm), [{ return isInt<11>(N->getSExtValue()); }]>;
def immSExt16  : PatLeaf<(imm), [{ return isInt<16>(N->getSExtValue()); }]>;


//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Arithmetic operations with registers
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

class Math16rr<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
  bits<3> Rd;
  bits<3> Rn;
  bits<3> Rm;

  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

class Math32rr<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
  bits<6> Rd;
  bits<6> Rn;
  bits<6> Rm;

  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = Rm{5-3};
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: Integer
//===----------------------------------------------------------------------===//

class IntMath16rr<bits<7> opcode, string instr_asm, SDNode OpNode> :
  Math16rr<(outs GPR32:$Rd), (ins GPR32:$Rn, GPR32:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, GPR32:$Rm))], opcode, NoItinerary> {}

class IntMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode> :
  Math32rr<(outs GPR32:$Rd), (ins GPR32:$Rn, GPR32:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, GPR32:$Rm))], opcode, NoItinerary> {
  let Inst{19-16} = 0b1010;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: Floating point
//===----------------------------------------------------------------------===//

class FPMath16rr<bits<7> opcode, string instr_asm, SDNode OpNode> :
  Math16rr<(outs FPR32:$Rd), (ins FPR32:$Rn, FPR32:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"),
          [(set FPR32:$Rd, (OpNode FPR32:$Rn, FPR32:$Rm))], opcode, NoItinerary> {}
          
class FPMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode> :
  Math32rr<(outs FPR32:$Rd), (ins FPR32:$Rn, FPR32:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"),
          [(set FPR32:$Rd, (OpNode FPR32:$Rn, FPR32:$Rm))], opcode, NoItinerary> {
  let Inst{19-16} = 0b0111;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Arithmetic operations with immediates
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

class Math16ri<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
  bits<3> Rd;
  bits<3> Rn;
  bits<3> Imm;

  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Imm{2-0};
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

class Math32ri<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
  bits<6>  Rd;
  bits<6>  Rn;
  bits<11> Imm;

  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = Rn{5-3};
  let Inst{23-16} = Imm{10-3};
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Imm{2-0};
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with immediates: Integer
//===----------------------------------------------------------------------===//

class IntMath16ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type> :
  Math16ri<(outs GPR32:$Rd), (ins GPR32:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], opcode, NoItinerary> {}

class IntMath32ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type> :
  Math32ri<(outs GPR32:$Rd), (ins GPR32:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], opcode, NoItinerary> {
  let Inst{19-16} = 0b1010;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Memory-related operations
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Mov operations with immediates
//===----------------------------------------------------------------------===//

class Mov16ri<string instr_asm, list<dag> pattern, bits<5> opcode>
    : Normal<(outs GPR32:$Rd), (ins GPR32:$src, imm8:$Imm), !strconcat(instr_asm, "\t$Rd, $Imm"), 
             pattern, NoItinerary> {
  bits<8> Imm;
  bits<3> Rd;
  
  let Inst{15-13} = Rd;
  let Inst{12-5} = Imm;
  let Inst{4-0} = opcode;
}

class Mov32ri<string instr_asm, dag ins, list<dag> pattern, bits<5> opcode, bits<1> MOVT>
    : Normal<(outs GPR32:$Rd), ins, !strconcat(instr_asm, "\t$Rd, $Imm"), 
             pattern, NoItinerary> {
  bits<16> Imm;
  bits<6> Rd;
  
  let Inst{31-29} = Rd{5-3};
  let Inst{28} = MOVT;
  let Inst{27-20} = Imm{15-8};
  let Inst{15-13} = Rd{2-0};
  let Inst{12-5} = Imm{7-0};
  let Inst{4-0} = opcode;
}

//===----------------------------------------------------------------------===//
// Mov operations with registers
//===----------------------------------------------------------------------===//
class ConditionCode<bits<4> cond_code, string cond_asm> {
  bits<4> Code = cond_code;
  string Asm = cond_asm;
}

def COND_EQ   : ConditionCode<0x0, "eq">;
def COND_NE   : ConditionCode<0x1, "ne">;
def COND_GTU  : ConditionCode<0x2, "gtu">;
def COND_GTEU : ConditionCode<0x3, "gteu">;
def COND_LTEU : ConditionCode<0x4, "lteu">;
def COND_LTU  : ConditionCode<0x5, "ltu">;
def COND_GT   : ConditionCode<0x6, "gt">;
def COND_GTE  : ConditionCode<0x7, "gte">;
def COND_LT   : ConditionCode<0x8, "lt">;
def COND_LTE  : ConditionCode<0x9, "lte">;
def COND_BEQ  : ConditionCode<0xA, "beq">;
def COND_BNE  : ConditionCode<0xB, "bne">;
def COND_BLT  : ConditionCode<0xC, "blt">;
def COND_BLTE : ConditionCode<0xD, "blte">;
def COND_NONE : ConditionCode<0xE, "">;
def COND_L    : ConditionCode<0xF, "l">;

class Mov16rr<string instr_asm, list<dag> pattern, ConditionCode cond, RegisterClass RegClass>
    : Normal<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rd, $Rn"),
             pattern, NoItinerary> {
    bits<3> Rd;
    bits<3> Rn;
    
    let Inst{15-13} = Rd;
    let Inst{12-10} = Rn;
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cond.Code;
    let Inst{3-0} = 0b0010;
}

class Mov32rr<string instr_asm, list<dag> pattern, ConditionCode cond, RegisterClass RegClass>
    : Normal<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rd, $Rn"),
             pattern, NoItinerary> {
    bits<3> Rd;
    bits<3> Rn;
    
    let Inst{15-13} = Rd;
    let Inst{12-10} = Rn;
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cond.Code;
    let Inst{3-0} = 0b0010;
}










// As above, this will be a single A64 instruction, but we can actually give the
// expansion in TableGen.
class A64PseudoExpand<dag outs, dag ins, list<dag> patterns, dag Result>
  : Pseudo<outs, ins, patterns>,
    PseudoInstExpansion<Result>;


class EP1INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}

class EP2INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}
class EP3INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}

class EP4INST<dag outs, dag ins, string asmstr, list<dag> patterns, InstrItinClass itin>  : Instruction {

field bits<32> Inst =0 ;
  // LLVM-level model of the Epiphany/A64 distinction.
  let Namespace = "Epiphany";
  let Size = 4;

  // Set the templated fields
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = patterns;
  let Itinerary = itin;
}