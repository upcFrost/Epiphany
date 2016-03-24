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
// ===----------------------------------------------------------------------===//


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
// Arithmetic operations with registers: Integers
//===----------------------------------------------------------------------===//
class IntMath32<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
//  let Inst{31-29} = Rd{5-3};
//  let Inst{28-26} = Rn{5-3};
//  let Inst{25-23} = Rm{5-3};
//  let Inst{19-16} = 0b1010;
//  let Inst{15-13} = Rd{2-0};
//  let Inst{12-10} = Rn{2-0};
//  let Inst{9-7}   = Rm{2-0};
//  let Inst{6-0}   = opcode;
}

class IntMath16<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
//  let Inst{15-13} = Rd{2-0};
//  let Inst{12-10} = Rn{2-0};
//  let Inst{9-7}   = Rm{2-0};
//  let Inst{6-0}   = opcode;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: Floating point
//===----------------------------------------------------------------------===//
class FpMath32<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
//  let Inst{19-16} = 0b0111;
//  let Inst{6-0}   = opcode;
}

class FpMath16<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal<outs, ins, asm, pattern, itin> {
//  let Inst{6-0}   = opcode;
}

//===----------------------------------------------------------------------===//
// Epiphany Operand Definitions.
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