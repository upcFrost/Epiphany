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

def NormalFrm  : Format<0>;
def PseudoFrm  : Format<1>;

class EpiphanyInst16<Format f, string cstr> : Instruction {
  field bits<16>    Inst; // Instruction encoding
  let Namespace     = "Epiphany";
  Format F          = f;
  bits<2> Form      = F.Value;
  let Pattern       = [];
  let Constraints   = cstr;
}

// Epiphany instruction format (general)
class EpiphanyInst32<Format f, string cstr> : Instruction {
  field bits<32>    Inst; // Instruction encoding
  let Namespace     = "Epiphany";
  Format F          = f;
  bits<2> Form      = F.Value;
  let Pattern       = [];
  let Constraints   = cstr;
}

class Pseudo16<dag outs, dag ins, list<dag> pattern, InstrItinClass itin = NoItinerary, string cstr = ""> 
    : EpiphanyInst16<PseudoFrm, cstr> {
  let OutOperandList  = outs;
  let InOperandList   = ins;
  let Pattern         = pattern;
  let isCodeGenOnly   = 1;
  let isPseudo        = 1;
  let Itinerary       = itin;
}

// Pseudo instructions (without encoding info)
class Pseudo32<dag outs, dag ins, list<dag> pattern, InstrItinClass itin = NoItinerary, string cstr = ""> 
    : EpiphanyInst32<PseudoFrm, cstr> {
  let OutOperandList  = outs;
  let InOperandList   = ins;
  let Pattern         = pattern;
  let isCodeGenOnly   = 1;
  let isPseudo        = 1;
  let Itinerary       = itin;
}

// Real instructions (with encoding info)
class Encoded16<string cstr, list<dag> pattern> : EpiphanyInst16<NormalFrm, cstr> {
  let Pattern = pattern;
  let Size = 2;
}

class Encoded32<string cstr, list<dag> pattern> : EpiphanyInst32<NormalFrm, cstr> {
  let Pattern = pattern;
  let Size = 4;
}

// Normal instructions
class Normal16<dag outs, dag ins, string asm, list<dag> pattern, InstrItinClass itin, string cstr = ""> 
    : Encoded16<cstr, pattern> {
  dag OutOperandList = outs;
  dag InOperandList  = ins;
  let AsmString      = asm;
  let Itinerary      = itin;
}
class Normal32<dag outs, dag ins, string asm, list<dag> pattern, InstrItinClass itin, string cstr = ""> 
    : Encoded32<cstr, pattern> {
  dag OutOperandList = outs;
  dag InOperandList  = ins;
  let AsmString      = asm;
  let Itinerary      = itin;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Interrupts control
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
class Interrupt<bits<10> opcode, list<dag> pattern, string asm>
    : Normal32<(outs), (ins), asm, pattern, NoItinerary> {
  let Inst{9-0} = opcode;
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
def Imm5_Operand   : ImmAsmOperand { let Name = "imm5";   }
def Imm8_Operand   : ImmAsmOperand { let Name = "imm8";   }
def Imm11_Operand  : ImmAsmOperand { let Name = "imm11";  }
def Imm16_Operand  : ImmAsmOperand { let Name = "imm16";  }
def Imm24_Operand  : ImmAsmOperand { let Name = "imm24";  }
def Simm3_Operand  : ImmAsmOperand { let Name = "simm3";  }
def Simm8_Operand  : ImmAsmOperand { let Name = "simm8";  }
def Simm11_Operand : ImmAsmOperand { let Name = "simm11"; }
def Simm12_Operand : ImmAsmOperand { let Name = "simm12"; }
def Simm24_Operand : ImmAsmOperand { let Name = "simm24"; }
def Fimm16_Operand : ImmAsmOperand { let Name = "fimm16"; }

// Immediates
let OperandType = "OPERAND_IMMEDIATE" in {
  def imm3   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 8);       }]> { let ParserMatchClass = Imm3_Operand;   }
  def imm5   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 32);      }]> { let ParserMatchClass = Imm5_Operand;   }
  def imm8   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 255);     }]> { let ParserMatchClass = Imm8_Operand;   }
  def imm11  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 2047);    }]> { let ParserMatchClass = Imm11_Operand;  }
  def imm16  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 65535);   }]> { let ParserMatchClass = Imm16_Operand;  }
  def imm24  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 16777215; }]> { let ParserMatchClass = Imm24_Operand;  }
  def simm3  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -4       && Imm < 3);       }]> { let ParserMatchClass = Simm3_Operand;  }
  def simm8  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -128     && Imm < 127);     }]> { let ParserMatchClass = Simm8_Operand;  }
  def simm11 : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -1024    && Imm < 1023);    }]> { let ParserMatchClass = Simm11_Operand; }
  def simm12 : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -2048    && Imm < 2047);    }]> { let ParserMatchClass = Simm12_Operand; }
  def simm24 : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= -8388608 && Imm < 8388607); }]> { let ParserMatchClass = Simm24_Operand; }
}

// Floating-point immediate.
def fpimm16 : Operand<f32>, PatLeaf<(f32 fpimm), [{ return Epiphany_AM::getFP16Imm(N->getValueAPF()) != -1; }], 
    SDNodeXForm<fpimm, [{
      APFloat InVal = N->getValueAPF();
      uint32_t enc = Epiphany_AM::getFP16Imm(InVal);
      return CurDAG->getTargetConstant(enc, SDLoc(N), MVT::i32);
    }]>> {
  let ParserMatchClass = Fimm16_Operand;
}

// Node immediate fits as N-bit sign extended on target immediate.
def immUExt5   : PatLeaf<(imm), [{ return isInt<5>(N->getZExtValue()); }]>;
def immUExt8   : PatLeaf<(imm), [{ return isInt<8>(N->getZExtValue()); }]>;
def immUExt24  : PatLeaf<(imm), [{ return isInt<24>(N->getZExtValue()); }]>;

def immSExt3   : PatLeaf<(imm), [{ return isInt<3>(N->getSExtValue()); }]>;
def immSExt8   : PatLeaf<(imm), [{ return isInt<8>(N->getSExtValue()); }]>;
def immSExt11  : PatLeaf<(imm), [{ return isInt<11>(N->getSExtValue()); }]>;
def immSExt16  : PatLeaf<(imm), [{ return isInt<16>(N->getSExtValue()); }]>;

// Memory address operand
def mem3 : Operand<iPTR> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR16:$Rn, imm3:$imm);
  let EncoderMethod = "getMemEncoding";
}
def mem11 : Operand<iPTR> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR32:$Rn, simm12:$imm);
  let EncoderMethod = "getMemEncoding";
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Load/Store operations
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
class AlignedLoad<PatFrag Node> :
  PatFrag<(ops node:$ptr), (Node node:$ptr), [{
  LoadSDNode *LD = cast<LoadSDNode>(N);
  return LD->getMemoryVT().getSizeInBits()/8 <= LD->getAlignment();
}]>;

class AlignedStore<PatFrag Node> :
  PatFrag<(ops node:$val, node:$ptr), (Node node:$val, node:$ptr), [{
  StoreSDNode *SD = cast<StoreSDNode>(N);
  return SD->getMemoryVT().getSizeInBits()/8 <= SD->getAlignment();
}]>;

// Load/Store PatFrags.
def load_a          : AlignedLoad<load>;
def store_a         : AlignedStore<store>;

class LS_bit<bits<1> LS, string asm> {
  bits<1> Value = LS;
  string Asm = asm;
}

def LoadBit   : LS_bit<0, "ldr">;
def StoreBit  : LS_bit<1, "str">;

class LS_size<bits<2> opcode, string asm> {
  bits<2> Value = opcode;
  string Asm = asm;
}

def LS_byte  : LS_size<0b00, "b">;
def LS_hword : LS_size<0b01, "h">;
def LS_word  : LS_size<0b10, "">;
def LS_dword : LS_size<0b11, "d">;

class LS_addsub<bits<1> opcode, string asm> {
  bits<1> Value = opcode;
  string Asm = asm;
}

def LS_add : LS_addsub<0, "+">;
def LS_sub : LS_addsub<1, "-">;

class LS16_general<dag outs, dag ins, string asm, list<dag> pattern, bits<4> opcode, LS_bit LS, LS_size opsize, InstrItinClass itin>
    : Normal16<outs, ins, asm, pattern, itin> {
  bits<3> Rd;
  bits<3> Rn;
  
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{6-5} = opsize.Value;
  let Inst{4} = LS.Value;
  let Inst{3-0} = opcode;
}

class LS32_general<dag outs, dag ins, string asm, list<dag> pattern, bits<4> opcode, LS_bit LS, LS_size opsize, InstrItinClass itin>
    : Normal32<outs, ins, asm, pattern, itin> {
  bits<6> Rd;
  bits<6> Rn;
  
  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = Rn{5-3};
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{6-5} = opsize.Value;
  let Inst{4} = LS.Value;
  let Inst{3-0} = opcode;
}

// Memory Load/Store
def addr3  : ComplexPattern<iPTR, 2, "SelectAddr<true>",  [frameindex], [SDNPWantParent]>;
def addr11 : ComplexPattern<iPTR, 2, "SelectAddr<false>", [frameindex], [SDNPWantParent]>;

let canFoldAsLoad = 1 in
class Load16<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize>
    : LS16_general<(outs RegClass:$Rd), (ins mem3:$addr), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $addr"), [(set RegClass:$Rd, (LoadType addr3:$addr))], 0b0100, LoadBit, LoadSize, NoItinerary> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{12-10} = addr{18-16}; // Rn
  let Inst{9-7}   = addr{2-0}; // imm3
  let isPseudo    = Pseudo;
}

class Store16<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize>
    : LS16_general<(outs), (ins RegClass:$Rd, mem3:$addr), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $addr"), [(StoreType RegClass:$Rd, addr3:$addr)], 0b0100, StoreBit, StoreSize, NoItinerary> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{12-10} = addr{18-16}; // Rn
  let Inst{9-7}   = addr{2-0}; // imm3
  let isPseudo    = Pseudo;
}


let canFoldAsLoad = 1 in
class Load32<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize>
    : LS32_general<(outs RegClass:$Rd), (ins mem11:$addr), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $addr"), [(set RegClass:$Rd, (LoadType addr11:$addr))], 0b1100, LoadBit, LoadSize, NoItinerary> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{28-26} = addr{21-19}; // Rn{5-3}
  let Inst{16-23} = addr{3-10};  // imm11{10-3}
  let Inst{12-10} = addr{18-16}; // Rn{2-0}
  let Inst{7-9}   = addr{0-2};   // imm11{2-0}
  let Inst{25}    = 0b0;
  let Inst{24}    = addr{11};    // imm sign bit
  let isPseudo    = Pseudo;
}


class Store32<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize>
    : LS32_general<(outs), (ins RegClass:$Rd, mem11:$addr), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $addr"), [(StoreType RegClass:$Rd, addr11:$addr)], 0b1100, StoreBit, StoreSize, NoItinerary> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{28-26} = addr{21-19}; // Rn{5-3}
  let Inst{16-23} = addr{3-10};  // imm11{10-3}
  let Inst{12-10} = addr{18-16}; // Rn{2-0}
  let Inst{7-9}   = addr{0-2};   // imm11{2-0}
  let Inst{25}    = 0b0;
  let Inst{24}    = addr{11};    // imm sign bit
  let isPseudo    = Pseudo;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Arithmetic operations with registers
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

class Math16rr<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal16<outs, ins, asm, pattern, itin> {
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
    : Normal32<outs, ins, asm, pattern, itin> {
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
// Arithmetic operations with registers: Floating/Integer
//===----------------------------------------------------------------------===//

class SimpleMath16rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass> :
  Math16rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"),
          [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, NoItinerary> {}

class SimpleMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass> :
  Math32rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, ".l", "\t$Rd, $Rn, $Rm"),
          [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, NoItinerary> {
  let Inst{19-16} = 0b1010;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: Integer
//===----------------------------------------------------------------------===//

class ComplexMath16rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass> :
  Math16rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, NoItinerary> {}

class ComplexMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass> :
  Math32rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, ".l", "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, NoItinerary> {
  let Inst{19-16} = 0b0111;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: IntToFloat and Abs
//===----------------------------------------------------------------------===//

class IntToFloat32<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal32<outs, ins, asm, pattern, itin> {
  bits<6> Rd;
  bits<6> Rn;
 
  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = 0b000;
  let Inst{19-16} = 0b0111;
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = 0b000;
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Arithmetic operations with immediates
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

class Math16ri<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal16<outs, ins, asm, pattern, itin> {
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
    : Normal32<outs, ins, asm, pattern, itin> {
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

class IntMath16ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type>
    : Math16ri<(outs GPR16:$Rd), (ins GPR16:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR16:$Rd, (OpNode GPR16:$Rn, imm_type:$Imm))], opcode, NoItinerary> {}

class IntMath32ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type> 
    : Math32ri<(outs GPR32:$Rd), (ins GPR32:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], opcode, NoItinerary> {
}

class ShiftMath16ri<bits<5> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type>
    : Normal16<(outs GPR16:$Rd), (ins GPR16:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"), 
            [(set GPR16:$Rd, (OpNode GPR16:$Rn, imm_type:$Imm))], NoItinerary> {
  bits<3> Rd;
  bits<3> Rn;
  bits<5> Imm;

  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-5}   = Imm;
  let Inst{4-0}   = opcode;
}

class ShiftMath32ri<bits<4> leftcode, bits<5> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type>
    : Normal32<(outs GPR32:$Rd), (ins GPR32:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"), 
            [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], NoItinerary> {
  bits<6> Rd;
  bits<6> Rn;
  bits<5> Imm;

  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = Rn{5-3};
  let Inst{19-16} = leftcode;
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = Rn{2-0};
  let Inst{9-5}   = Imm;
  let Inst{4-0}   = opcode;
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

class Mov16ri<string instr_asm, dag ins, list<dag> pattern, bits<5> opcode, RegisterClass RegClass>
    : Normal16<(outs RegClass:$Rd), ins, !strconcat(instr_asm, "\t$Rd, $Imm"), 
             pattern, NoItinerary> {
  bits<8> Imm;
  bits<3> Rd;
  
  let Inst{15-13} = Rd;
  let Inst{12-5} = Imm;
  let Inst{4-0} = opcode;
}

class Mov32ri<string instr_asm, dag ins, list<dag> pattern, bits<5> opcode, bits<1> MOVT, RegisterClass RegClass>
    : Normal32<(outs RegClass:$Rd), ins, !strconcat(instr_asm, "\t$Rd, $Imm"), 
             pattern, NoItinerary> {
  bits<16> Imm;
  bits<6> Rd;
  
  let Inst{31-29} = Rd{5-3};
  let Inst{28} = MOVT;
  let Inst{27-20} = Imm{15-8};
  let Inst{19-16} = 0b0010;
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
    : Normal16<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rd, $Rn"),
             pattern, NoItinerary> {
    bits<3> Rd;
    bits<3> Rn;
    
    let Inst{15-13} = Rd;
    let Inst{12-10} = Rn;
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cond.Code;
    let Inst{3-0} = 0b0010;
}

class Mov32rr<string instr_asm, list<dag> pattern, RegisterClass RegClass>
    : Normal32<(outs RegClass:$Rd), (ins RegClass:$Rn), !strconcat(instr_asm, "\t$Rd, $Rn"),
             pattern, NoItinerary> {
    bits<6> Rd;
    bits<6> Rn;
    
    let Inst{31-29} = Rd{5-3};
    let Inst{28-26} = Rn{5-3};
    let Inst{19-16} = 0b0010;
    let Inst{15-13} = Rd{2-0};
    let Inst{12-10} = Rn{2-0};
    let Inst{9-8} = 0b00;
    let Inst{7-4} = COND_NONE.Code;
    let Inst{3-0} = 0b1111;
}

class MovCond32rr<string instr_asm, list<dag> pattern, ConditionCode cond, RegisterClass RegClass>
    : Normal32<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rd, $Rn"),
             pattern, NoItinerary> {
    bits<6> Rd;
    bits<6> Rn;
    
    let Inst{31-29} = Rd{5-3};
    let Inst{28-26} = Rn{5-3};
    let Inst{19-16} = 0b0010;
    let Inst{15-13} = Rd{2-0};
    let Inst{12-10} = Rn{2-0};
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cond.Code;
    let Inst{3-0} = 0b0010;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Flow-related operations
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Branching/Jumps
//===----------------------------------------------------------------------===//
class IsTailCall {
  bit isCall = 1;
  bit isTerminator = 1;
  bit isReturn = 1;
  bit isBarrier = 1;
  bit hasExtraSrcRegAllocReq = 1;
  bit isCodeGenOnly = 1;
}

let OperandType = "OPERAND_PCREL" in {
  def jmptarget   : Operand<OtherVT> { let EncoderMethod = "getJumpTargetOpValue"; }
  def calltarget  : Operand<iPTR> { let EncoderMethod = "getJumpTargetOpValue"; }
}
def cc : Operand<i8> {}

class JumpReg16<string instr_asm, bits<10> opcode, list<dag> pattern, ConditionCode cond>
    : Normal16<(outs), (ins GPR16:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rn"),
             pattern, NoItinerary> {
  bits<3> Rn;

  let Inst{12-10} = Rn{2-0};
  let Inst{9-0}   = opcode;

  let isBranch    = 1;
  let isTerminator = 1;
}

class JumpReg32<string instr_asm, bits<10> opcode, list<dag> pattern, ConditionCode cond>
    : Normal32<(outs), (ins GPR32:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rn"),
             pattern, NoItinerary> {
  bits<6> Rn;

  let Inst{28-26} = Rn{5-3};
  let Inst{19-16} = 0b0010;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-0}   = opcode;

  let isBranch     = 1;
  let isTerminator = 1;
}

class Branch16<list<dag> pattern>
    : Normal16<(outs), (ins jmptarget:$addr), !strconcat("b", "\t$addr"),
             pattern, NoItinerary> {
  bits<8> addr;
  let Inst{15-8}   = addr;
  let Inst{7-4}    = COND_NONE.Code;
  let Inst{3-0}    = 0;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class Branch32<dag ins, list<dag> pattern, ConditionCode cond>
    : Normal32<(outs), ins, !strconcat("b", cond.Asm, "\t$addr"),
             pattern, NoItinerary> {
  bits<24> addr;

  let Inst{31-8}   = addr;
  let Inst{7-4}    = cond.Code;
  let Inst{3-0}    = 0b1000;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class BranchCC16<ConditionCode cond>
    : Normal16<(outs), (ins GPR16:$Rd, GPR16:$Rn, jmptarget:$addr), !strconcat("b", cond.Asm, "\t$addr"),
             [], NoItinerary> {
  bits<8> addr;
  let Inst{15-8}   = addr;
  let Inst{7-4}    = cond.Code;
  let Inst{3-0}    = 0;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class BranchCC32<ConditionCode cond>
    : Normal32<(outs), (ins GPR32:$Rd, jmptarget:$addr), !strconcat("b", cond.Asm, "\t$addr"),
             [], NoItinerary> {
  bits<24> addr;

  let Inst{31-8}   = addr;
  let Inst{7-4}    = cond.Code;
  let Inst{3-0}    = 0b1000;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

