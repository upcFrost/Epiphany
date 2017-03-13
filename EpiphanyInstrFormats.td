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
  let Size          = 2;
  let CodeSize      = 2;
}

// Epiphany instruction format (general)
class EpiphanyInst32<Format f, string cstr> : Instruction {
  field bits<32>    Inst; // Instruction encoding
  let Namespace     = "Epiphany";
  Format F          = f;
  bits<2> Form      = F.Value;
  let Pattern       = [];
  let Constraints   = cstr;
  let Size          = 4;
  let CodeSize      = 4;
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
    : Normal16<(outs), (ins), asm, pattern, NoItinerary> {
  let Inst{9-0} = opcode;
  let hasSideEffects = 1;
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
//let ParserMethod = "ParseImmOperand" in {
  def Imm3_Operand   : ImmAsmOperand { let Name = "UImm3";  let PredicateMethod = "isUImm<3>"; }
  def Imm5_Operand   : ImmAsmOperand { let Name = "UImm5";  let PredicateMethod = "isUImm<5>"; }
  def Imm8_Operand   : ImmAsmOperand { let Name = "UImm8";  let PredicateMethod = "isUImm<8>"; }
  def Imm11_Operand  : ImmAsmOperand { let Name = "UImm11"; let PredicateMethod = "isUImm<11>"; }
  def Imm16_Operand  : ImmAsmOperand { let Name = "UImm16"; let PredicateMethod = "isUImm<16>"; }
  def Imm24_Operand  : ImmAsmOperand { let Name = "UImm24"; let PredicateMethod = "isUImm<24>"; }
//}
//let ParserMethod = "ParseSimmOperand" in {
  def Simm3_Operand  : ImmAsmOperand { let Name = "SImm3";  let PredicateMethod = "isSImm<3>";  }
  def Simm8_Operand  : ImmAsmOperand { let Name = "SImm8";  let PredicateMethod = "isSImm<8>";  }
  def Simm11_Operand : ImmAsmOperand { let Name = "SImm11"; let PredicateMethod = "isSImm<11>"; }
  def Simm12_Operand : ImmAsmOperand { let Name = "SImm12"; let PredicateMethod = "isSImm<12>"; }
  def Simm24_Operand : ImmAsmOperand { let Name = "SImm24"; let PredicateMethod = "isSImm<24>"; }
//}
//let ParserMethod = "ParseFimmOperand" in {
  def Fimm16_Operand : ImmAsmOperand { let Name = "Fimm16"; let PredicateMethod = "isSImm<16>"; }
//}

// Immediates
let OperandType = "OPERAND_IMMEDIATE" in {
  def imm3   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 8);        }]> { let ParserMatchClass = Imm3_Operand;   }
  def imm5   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 32);       }]> { let ParserMatchClass = Imm5_Operand;   }
  def imm8   : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 255);      }]> { let ParserMatchClass = Imm8_Operand;   }
  def imm11  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 2047);     }]> { let ParserMatchClass = Imm11_Operand;  }
  def imm16  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 65535);    }]> { let ParserMatchClass = Imm16_Operand;  }
  def imm24  : Operand<i32>, ImmLeaf<i32, [{ return (Imm >= 0 && Imm < 16777215); }]> { let ParserMatchClass = Imm24_Operand;  }
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

// Memory operand for asm parser
def EpiphanyMemAsmOperand : AsmOperandClass {
  let Name = "Mem";
  let ParserMethod = "parseMemOperand";
}
// Memory address operand
def mem3 : Operand<iPTR> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR16:$Rn, imm3:$imm);
  let EncoderMethod = "getMemEncoding";
  let ParserMatchClass = EpiphanyMemAsmOperand;
}
def mem11 : Operand<iPTR> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR32:$Rn, simm12:$imm);
  let EncoderMethod = "getMemEncoding";
  let ParserMatchClass = EpiphanyMemAsmOperand;
}
// Postmodify operand
def pmem11 : Operand<iPTR> {
  let PrintMethod = "printPostModifyOperand";
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

class IndexAddSub<SDNode addsub, bits<1> opcode, string asm> {
  SDNode Op = addsub;
  bits<1> Opcode = opcode;
  string Asm = asm;
}
def IndexAdd : IndexAddSub<add, 0, "+">;
def IndexSub : IndexAddSub<sub, 1, "-">;

//-----------General classes----------//

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
def addr3  : ComplexPattern<iPTR, 2, "SelectAddr</* is16bit = */true>",  [frameindex], [SDNPWantParent]>;
def addr11 : ComplexPattern<iPTR, 2, "SelectAddr</* is16bit = */false>", [frameindex], [SDNPWantParent]>;

//-----------Displacement (Rd <-> (Rn + imm)) ----------//

class LoadDisp16<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize>
    : LS16_general<(outs RegClass:$Rd), (ins mem3:$addr), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $addr"), [(set RegClass:$Rd, (LoadType addr3:$addr))], 0b0100, LoadBit, LoadSize, LoadItin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{12-10} = addr{18-16}; // Rn
  let Inst{9-7}   = addr{2-0}; // imm3
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
}

class StoreDisp16<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize>
    : LS16_general<(outs), (ins RegClass:$Rd, mem3:$addr), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $addr"), [(StoreType RegClass:$Rd, addr3:$addr)], 0b0100, StoreBit, StoreSize, StoreItin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{12-10} = addr{18-16}; // Rn
  let Inst{9-7}   = addr{2-0}; // imm3
  let isPseudo    = Pseudo;
}

class LoadDisp32<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize>
    : LS32_general<(outs RegClass:$Rd), (ins mem11:$addr), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $addr"), [(set RegClass:$Rd, (LoadType addr11:$addr))], 0b1100, LoadBit, LoadSize, LoadItin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> addr;

  let Inst{28-26} = addr{21-19}; // Rn{5-3}
  let Inst{16-23} = addr{3-10};  // imm11{10-3}
  let Inst{12-10} = addr{18-16}; // Rn{2-0}
  let Inst{7-9}   = addr{0-2};   // imm11{2-0}
  let Inst{25}    = 0b0;
  let Inst{24}    = addr{11};    // imm sign bit
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
}

class StoreDisp32<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize>
    : LS32_general<(outs), (ins RegClass:$Rd, mem11:$addr), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $addr"), [(StoreType RegClass:$Rd, addr11:$addr)], 0b1100, StoreBit, StoreSize, StoreItin> {
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


//-----------Index (Rd <-> (Rn + Rm)) ----------//
class LoadIdx16<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize, IndexAddSub AddSub>
    : LS16_general<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, [$Rn,", AddSub.Asm, "$Rm]"),
                   [(set RegClass:$Rd, (LoadType (AddSub.Op RegClass:$Rn, RegClass:$Rm)))], 0b0001, LoadBit, LoadSize, LoadItin> {
  bits<3> Rn;
  bits<3> Rm;

  let Inst{12-10} = Rn;
  let Inst{9-7}   = Rm;
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
}

class StoreIdx16<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize, IndexAddSub AddSub>
    : LS16_general<(outs), (ins RegClass:$Rd, RegClass:$Rn, RegClass:$Rm), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, [$Rn,", AddSub.Asm, "$Rm]"),
                   [(StoreType RegClass:$Rd, (AddSub.Op RegClass:$Rn, RegClass:$Rm))], 0b0001, StoreBit, StoreSize, StoreItin> {
  bits<3> Rn;
  bits<3> Rm;

  let Inst{12-10} = Rn;
  let Inst{9-7}   = Rm;
  let isPseudo    = Pseudo;
}

class LoadIdx32<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize, IndexAddSub AddSub>
    : LS32_general<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, [$Rn,", AddSub.Asm, "$Rm]"), 
                   [(set RegClass:$Rd, (LoadType (AddSub.Op RegClass:$Rn, RegClass:$Rm)))], 0b1001, LoadBit, LoadSize, LoadItin> {
  bits<6> Rn;
  bits<6> Rm;

  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = Rm{5-3};
  let Inst{22-21} = 0;
  let Inst{20}    = AddSub.Opcode;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
}

class StoreIdx32<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize, IndexAddSub AddSub>
    : LS32_general<(outs), (ins RegClass:$Rd, RegClass:$Rn, RegClass:$Rm), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, [$Rn,", AddSub.Asm, "$Rm]"), 
                   [(StoreType RegClass:$Rd, (AddSub.Op RegClass:$Rn, RegClass:$Rm))], 0b1001, StoreBit, StoreSize, StoreItin> {
  bits<6> Rn;
  bits<6> Rm;

  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = Rm{5-3};
  let Inst{22-21} = 0;
  let Inst{20}    = AddSub.Opcode;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let isPseudo    = Pseudo;
}

//----------- Postmodify (Rd <-> [Rn] -> Rd + Rm) ----------//
// TODO: Add patterns
class LoadPm16<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize, IndexAddSub AddSub>
    : LS16_general<(outs RegClass:$Rd, RegClass:$Rn), (ins RegClass:$base, RegClass:$Rm), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, [$Rn],", AddSub.Asm, "$Rm"), 
                   [], 0b0101, LoadBit, LoadSize, LoadItin> {
  bits<3> Rn;
  bits<3> Rm;

  let Inst{12-10} = Rn;
  let Inst{9-7}   = Rm;
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
  let Constraints = "$base = $Rn";
}

class StorePm16<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize, IndexAddSub AddSub>
    : LS16_general<(outs RegClass:$Rn), (ins RegClass:$Rd, RegClass:$base, RegClass:$Rm), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $Rn,", AddSub.Asm, "$Rm"), 
                   [], 0b0101, StoreBit, StoreSize, StoreItin> {
  bits<3> Rn;
  bits<3> Rm;

  let Inst{12-10} = Rn;
  let Inst{9-7}   = Rm;
  let isPseudo    = Pseudo;
  let Constraints = "$base = $Rn";
}

class LoadPm32<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize, IndexAddSub AddSub>
    : LS32_general<(outs RegClass:$Rd, RegClass:$Rn), (ins RegClass:$base, RegClass:$Rm), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $Rn,", AddSub.Asm, "$Rm"), 
                   [], 0b1101, LoadBit, LoadSize, LoadItin> {
  bits<6> Rn;
  bits<6> Rm;

  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = Rm{5-3};
  let Inst{22-21} = 0;
  let Inst{20}    = AddSub.Opcode;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
  let Constraints = "$base = $Rn";
}

class StorePm32<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize, IndexAddSub AddSub>
    : LS32_general<(outs RegClass:$Rn), (ins RegClass:$Rd, RegClass:$base, RegClass:$Rm), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $Rn,", AddSub.Asm, "$Rm"), 
                   [], 0b1101, StoreBit, StoreSize, StoreItin> {
  bits<6> Rn;
  bits<6> Rm;

  let Inst{28-26} = Rn{5-3};
  let Inst{25-23} = Rm{5-3};
  let Inst{22-21} = 0;
  let Inst{20}    = AddSub.Opcode;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-7}   = Rm{2-0};
  let isPseudo    = Pseudo;
  let Constraints = "$base = $Rn";
}

//----------- Postmodify-Disp (Rd <-> [Rn] -> Rd + imm) ----------//
// TODO: Add patterns
class LoadPmd32<bit Pseudo, RegisterClass RegClass, PatFrag LoadType, LS_size LoadSize>
    : LS32_general<(outs RegClass:$Rd, RegClass:$Rn), (ins pmem11:$imm), !strconcat(LoadBit.Asm, LoadSize.Asm, "\t$Rd, $imm"), [], 0b1100, LoadBit, LoadSize, LoadItin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> imm;

  let Inst{28-26} = imm{21-19}; // Rn{5-3}
  let Inst{16-23} = imm{3-10};  // imm11{10-3}
  let Inst{12-10} = imm{18-16}; // Rn{2-0}
  let Inst{7-9}   = imm{0-2};   // imm11{2-0}
  let Inst{25}    = 0b1;
  let Inst{24}    = imm{11};    // imm sign bit
  let isPseudo    = Pseudo;
  let canFoldAsLoad = 1;
}

class StorePmd32<bit Pseudo, RegisterClass RegClass, PatFrag StoreType, LS_size StoreSize>
    : LS32_general<(outs RegClass:$Rn), (ins RegClass:$Rd, pmem11:$imm), !strconcat(StoreBit.Asm, StoreSize.Asm, "\t$Rd, $imm"), [], 0b1100, StoreBit, StoreSize, StoreItin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> imm;

  let Inst{28-26} = imm{21-19}; // Rn{5-3}
  let Inst{16-23} = imm{3-10};  // imm11{10-3}
  let Inst{12-10} = imm{18-16}; // Rn{2-0}
  let Inst{7-9}   = imm{0-2};   // imm11{2-0}
  let Inst{25}    = 0b1;
  let Inst{24}    = imm{11};    // imm sign bit
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
          [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, IaluItin> {}

class SimpleMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass> :
  Math32rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, ".l", "\t$Rd, $Rn, $Rm"),
          [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, IaluItin> {
  let Inst{19-16} = 0b1010;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with registers: Float
//===----------------------------------------------------------------------===//

class ComplexMath16rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass, InstrItinClass itin> :
  Math16rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, itin> {}

class ComplexMath32rr<bits<7> opcode, string instr_asm, SDNode OpNode, RegisterClass RegClass, InstrItinClass itin> :
  Math32rr<(outs RegClass:$Rd), (ins RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, ".l", "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$Rn, RegClass:$Rm))], opcode, itin> {
  let Inst{19-16} = 0b0111;
}

class ComplexMath2_16rr<bits<7> opcode, string instr_asm, SDNode OpNode, SDNode OpNode2, RegisterClass RegClass, InstrItinClass itin> :
  Math16rr<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$src, (OpNode2 RegClass:$Rn, RegClass:$Rm)))], opcode, itin> {
  let Constraints = "$src = $Rd";
}

class ComplexMath2_32rr<bits<7> opcode, string instr_asm, SDNode OpNode, SDNode OpNode2, RegisterClass RegClass, InstrItinClass itin> :
  Math32rr<(outs RegClass:$Rd), (ins RegClass:$src, RegClass:$Rn, RegClass:$Rm), !strconcat(instr_asm, ".l", "\t$Rd, $Rn, $Rm"), 
           [(set RegClass:$Rd, (OpNode RegClass:$src, (OpNode2 RegClass:$Rn, RegClass:$Rm)))], opcode, itin> {
  let Constraints = "$src = $Rd";
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

// Special class to handle indirect addresses (usually just adding up FP and offset)
class AddrMath32ri<dag outs, dag ins, string asm, list<dag> pattern, bits<7> opcode, InstrItinClass itin>
    : Normal32<outs, ins, asm, pattern, itin> {
  // mem = Rd<21-16> + Imm<15-0> (see getMemEncoding)
  bits<22> imm;
  bits<6>  Rd;

  let Inst{31-29} = Rd{5-3};
  let Inst{28-26} = imm{21-19}; // Rn{5-3};
  let Inst{23-16} = imm{10-3};
  let Inst{15-13} = Rd{2-0};
  let Inst{12-10} = imm{18-16}; // Rn{2-0};
  let Inst{9-7}   = imm{2-0};
  let Inst{6-0}   = opcode;
  
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Arithmetic operations with immediates: Integer
//===----------------------------------------------------------------------===//

class IntMath16ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type>
    : Math16ri<(outs GPR16:$Rd), (ins GPR16:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR16:$Rd, (OpNode GPR16:$Rn, imm_type:$Imm))], opcode, IaluItin> {}

class IntMath32ri<bits<7> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type> 
    : Math32ri<(outs GPR32:$Rd), (ins GPR32:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"),
          [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], opcode, IaluItin> {}

class ShiftMath16ri<bits<5> opcode, string instr_asm, SDNode OpNode, Operand Od, PatLeaf imm_type>
    : Normal16<(outs GPR16:$Rd), (ins GPR16:$Rn, Od:$Imm), !strconcat(instr_asm, "\t$Rd, $Rn, $Imm"), 
            [(set GPR16:$Rd, (OpNode GPR16:$Rn, imm_type:$Imm))], IaluItin> {
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
            [(set GPR32:$Rd, (OpNode GPR32:$Rn, imm_type:$Imm))], IaluItin> {
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
             pattern, IaluItin> {
  bits<8> Imm;
  bits<3> Rd;
  
  let Inst{15-13} = Rd;
  let Inst{12-5} = Imm;
  let Inst{4-0} = opcode;
}

class Mov32ri<string instr_asm, dag ins, list<dag> pattern, bits<5> opcode, bits<1> MOVT, RegisterClass RegClass>
    : Normal32<(outs RegClass:$Rd), ins, !strconcat(instr_asm, "\t$Rd, $Imm"), 
             pattern, IaluItin> {
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
             pattern, IaluItin> {
    bits<3> Rd;
    bits<3> Rn;
    
    let Inst{15-13} = Rd;
    let Inst{12-10} = Rn;
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cond.Code;
    let Inst{3-0} = 0b0010;

    let isMoveImm = 1;
    let isAsCheapAsAMove = 1;
    let isReMaterializable = 1;
}

class Mov32rr<string instr_asm, list<dag> pattern, RegisterClass RegClass>
    : Normal32<(outs RegClass:$Rd), (ins RegClass:$Rn), !strconcat(instr_asm, "\t$Rd, $Rn"),
             pattern, IaluItin> {
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

    let isMoveImm = 1;
    let isAsCheapAsAMove = 1;
    let isReMaterializable = 1;
}

class MovCond32rr<dag outs, dag ins, list<dag> pattern>
    : Normal32<outs, ins, !strconcat("mov$cc", "\t$Rd, $Rn"), pattern, IaluItin> {
    bits<6> Rd;
    bits<6> Rn;
    bits<4> cc;
    
    let Inst{31-29} = Rd{5-3};
    let Inst{28-26} = Rn{5-3};
    let Inst{19-16} = 0b0010;
    let Inst{15-13} = Rd{2-0};
    let Inst{12-10} = Rn{2-0};
    let Inst{9-8} = 0b00;
    let Inst{7-4} = cc;
    let Inst{3-0} = 0b0010;
}

// Special MOVFS/MOVTS
class SpecRegClass<bits<2> group_code> {
  bits<2> Code = group_code;
}
def CoreReg    : SpecRegClass<0>;
def DmaReg     : SpecRegClass<1>;
def MemProtReg : SpecRegClass<2>;
def ConfReg    : SpecRegClass<3>;

class SpecRegFromTo<bits<1> ft_code> {
  bits<1> Code = ft_code;
}
def SpecTo   : SpecRegFromTo<0>;
def SpecFrom : SpecRegFromTo<1>;

class MovSpecial<string instr_asm, dag outs, dag ins, list<dag> pattern, SpecRegClass regclass, SpecRegFromTo FromTo>
    : Normal32<outs, ins, !strconcat(instr_asm, "\t$Rd, $MMR"), pattern, ControlItin> {
    bits<6> Rd;
    bits<6> MMR;
    
    let Inst{31-29} = Rd{5-3};
    let Inst{28-26} = MMR{5-3};
    let Inst{21-20} = regclass.Code;
    let Inst{19-16} = 0b0010;
    let Inst{15-13} = Rd{2-0};
    let Inst{12-10} = MMR{2-0};
    let Inst{9-8}   = 0b01;
    let Inst{7-5}   = 0;
    let Inst{4}     = FromTo.Code;
    let Inst{3-0}   = 0b1111;
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
  def jmptarget        : Operand<iPTR>    { let EncoderMethod = "getJumpTargetOpValue"; }
  def branchtarget     : Operand<OtherVT> { let EncoderMethod = "getBranchTargetOpValue"; }
  def branchlinktarget : Operand<iPTR>    { let EncoderMethod = "getBranchTargetOpValue"; }
}
def cc : Operand<i8> {
  let PrintMethod = "printCondCode";
}

class JumpReg16<string instr_asm, bits<10> opcode, list<dag> pattern, ConditionCode cond>
    : Normal16<(outs), (ins GPR16:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rn"), pattern, BranchItin> {
  bits<3> Rn;

  let Inst{12-10} = Rn{2-0};
  let Inst{9-0}   = opcode;

  let isBranch    = 1;
  let isTerminator = 1;
}

class JumpReg32<string instr_asm, bits<10> opcode, list<dag> pattern, ConditionCode cond>
    : Normal32<(outs), (ins GPR32:$Rn), !strconcat(instr_asm, cond.Asm, "\t$Rn"), pattern, BranchItin> {
  bits<6> Rn;

  let Inst{28-26} = Rn{5-3};
  let Inst{19-16} = 0b0010;
  let Inst{12-10} = Rn{2-0};
  let Inst{9-0}   = opcode;

  let isBranch     = 1;
  let isTerminator = 1;
}

class Branch16<list<dag> pattern>
    : Normal16<(outs), (ins branchtarget:$addr), !strconcat("b", "\t$addr"), pattern, BranchItin> {
  bits<8> addr;
  let Inst{15-8}   = addr;
  let Inst{7-4}    = COND_NONE.Code;
  let Inst{3-0}    = 0;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class Branch32<dag ins, list<dag> pattern, ConditionCode cond>
    : Normal32<(outs), ins, !strconcat("b", cond.Asm, "\t$addr"), pattern, BranchItin> {
  bits<24> addr;

  let Inst{31-8}   = addr;
  let Inst{7-4}    = cond.Code;
  let Inst{3-0}    = 0b1000;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class BranchCC16<ConditionCode cond>
    : Normal16<(outs), (ins GPR16:$Rd, GPR16:$Rn, branchtarget:$addr), !strconcat("b", cond.Asm, "\t$addr"), [], BranchItin> {
  bits<8> addr;
  let Inst{15-8}   = addr;
  let Inst{7-4}    = cond.Code;
  let Inst{3-0}    = 0;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

class BranchCC32<dag ins, list<dag> pattern>
    : Normal32<(outs), ins, !strconcat("b$cc", "\t$addr"), pattern, BranchItin> {
  bits<24> addr;
  bits<4>  cc;

  let Inst{31-8}   = addr;
  let Inst{7-4}    = cc;
  let Inst{3-0}    = 0b1000;

  let isBranch     = 1;
  let isTerminator = 1;
  let Uses         = [STATUS];
}

