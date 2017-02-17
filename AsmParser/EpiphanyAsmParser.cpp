//===-- EpiphanyAsmParser.cpp - Parse Epiphany assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"

#include "MCTargetDesc/EpiphanyMCExpr.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "EpiphanyRegisterInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "epiphany-asm-parser"

namespace {
  class EpiphanyAssemblerOptions {
    public:
      EpiphanyAssemblerOptions():
        reorder(true), macro(true) {
        }

      bool isReorder() {return reorder;}
      void setReorder() {reorder = true;}
      void setNoreorder() {reorder = false;}

      bool isMacro() {return macro;}
      void setMacro() {macro = true;}
      void setNomacro() {macro = false;}

    private:
      bool reorder;
      bool macro;
  };
}

namespace {
  class EpiphanyAsmParser : public MCTargetAsmParser {
    MCAsmParser &Parser;
    EpiphanyAssemblerOptions Options;

#define GET_ASSEMBLER_HEADER
#include "EpiphanyGenAsmMatcher.inc"

    bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
        OperandVector &Operands, MCStreamer &Out,
        uint64_t &ErrorInfo,
        bool MatchingInlineAsm) override;

    bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

    bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
        SMLoc NameLoc, OperandVector &Operands) override;

    bool parseMathOperation(StringRef Name, SMLoc NameLoc,
        OperandVector &Operands);

    bool ParseDirective(AsmToken DirectiveID) override;

    OperandMatchResultTy parseMemOperand(OperandVector &);

    bool ParseOperand(OperandVector &Operands, StringRef Mnemonic);

    int tryParseRegister(StringRef Mnemonic);

    bool tryParseRegisterOperand(OperandVector &Operands,
        StringRef Mnemonic);

    bool needsExpansion(MCInst &Inst);

    void expandInstruction(MCInst &Inst, SMLoc IDLoc,
        SmallVectorImpl<MCInst> &Instructions);
    void expandLoadImm(MCInst &Inst, SMLoc IDLoc,
        SmallVectorImpl<MCInst> &Instructions);
    void expandLoadAddressImm(MCInst &Inst, SMLoc IDLoc,
        SmallVectorImpl<MCInst> &Instructions);
    void expandLoadAddressReg(MCInst &Inst, SMLoc IDLoc,
        SmallVectorImpl<MCInst> &Instructions);
    bool reportParseError(StringRef ErrorMsg);

    bool parseMemOffset(const MCExpr *&Res);
    bool parseRelocOperand(const MCExpr *&Res);

    const MCExpr *evaluateRelocExpr(const MCExpr *Expr, StringRef RelocStr);

    bool parseDirectiveSet();

    bool parseSetAtDirective();
    bool parseSetNoAtDirective();
    bool parseSetMacroDirective();
    bool parseSetNoMacroDirective();
    bool parseSetReorderDirective();
    bool parseSetNoReorderDirective();

    int matchRegisterName(StringRef Symbol);

    int matchRegisterByNumber(unsigned RegNum, StringRef Mnemonic);

    unsigned getReg(int RC,int RegNo);

    public:
    EpiphanyAsmParser(const MCSubtargetInfo &sti, MCAsmParser &parser,
        const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, sti), Parser(parser) {
        // Initialize the set of available features.
        setAvailableFeatures(ComputeAvailableFeatures(getSTI().getFeatureBits()));
      }

    MCAsmParser &getParser() const { return Parser; }
    MCAsmLexer &getLexer() const { return Parser.getLexer(); }

  };
}

namespace {

  /// EpiphanyOperand - Instances of this class represent a parsed Epiphany machine
  /// instruction.
  class EpiphanyOperand : public MCParsedAsmOperand {

    enum KindTy {
      k_CondCode,
      k_CoprocNum,
      k_Immediate,
      k_Memory,
      k_PostIndexRegister,
      k_Register,
      k_Token
    } Kind;

    public:
    EpiphanyOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

    struct Token {
      const char *Data;
      unsigned Length;
    };
    struct PhysRegOp {
      unsigned RegNum; /// Register Number
    };
    struct ImmOp {
      const MCExpr *Val;
    };
    struct MemOp {
      unsigned Base;
      const MCExpr *Off;
    };

    union {
      struct Token Tok;
      struct PhysRegOp Reg;
      struct ImmOp Imm;
      struct MemOp Mem;
    };

    SMLoc StartLoc, EndLoc;

    public:
    void addRegOperands(MCInst &Inst, unsigned N) const {
      assert(N == 1 && "Invalid number of operands!");
      Inst.addOperand(MCOperand::createReg(getReg()));
    }

    void addExpr(MCInst &Inst, const MCExpr *Expr) const{
      // Add as immediate when possible.  Null MCExpr = 0.
      if (Expr == 0)
        Inst.addOperand(MCOperand::createImm(0));
      else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
        Inst.addOperand(MCOperand::createImm(CE->getValue()));
      else
        Inst.addOperand(MCOperand::createExpr(Expr));
    }

    void addImmOperands(MCInst &Inst, unsigned N) const {
      assert(N == 1 && "Invalid number of operands!");
      const MCExpr *Expr = getImm();
      addExpr(Inst,Expr);
    }

    void addMemOperands(MCInst &Inst, unsigned N) const {
      assert(N == 2 && "Invalid number of operands!");

      Inst.addOperand(MCOperand::createReg(getMemBase()));

      const MCExpr *Expr = getMemOff();
      addExpr(Inst,Expr);
    }

    bool isReg() const { return Kind == k_Register; }
    bool isImm() const { return Kind == k_Immediate; }
    bool isConstantImm() const {
      return isImm() && isa<MCConstantExpr>(getImm());
    }

    // Additional templates for different int sizes
    template <unsigned Bits, int Offset = 0> bool isConstantUImm() const {
      return isConstantImm() && isUInt<Bits>(getConstantImm() - Offset);
    }
    template <unsigned Bits> bool isSImm() const {
      return isConstantImm() ? isInt<Bits>(getConstantImm()) : isImm();
    }
    template <unsigned Bits> bool isUImm() const {
      return isConstantImm() ? isUInt<Bits>(getConstantImm()) : isImm();
    }
    template <unsigned Bits> bool isAnyImm() const {
      return isConstantImm() ? (isInt<Bits>(getConstantImm()) ||
          isUInt<Bits>(getConstantImm()))
        : isImm();
    }
    template <unsigned Bits, int Offset = 0> bool isConstantSImm() const {
      return isConstantImm() && isInt<Bits>(getConstantImm() - Offset);
    }
    template <unsigned Bottom, unsigned Top> bool isConstantUImmRange() const {
      return isConstantImm() && getConstantImm() >= Bottom &&
        getConstantImm() <= Top;
    }

    bool isToken() const { return Kind == k_Token; }
    bool isMem() const { return Kind == k_Memory; }

    StringRef getToken() const {
      assert(Kind == k_Token && "Invalid access!");
      return StringRef(Tok.Data, Tok.Length);
    }

    unsigned getReg() const {
      assert((Kind == k_Register) && "Invalid access!");
      return Reg.RegNum;
    }

    const MCExpr *getImm() const {
      assert((Kind == k_Immediate) && "Invalid access!");
      return Imm.Val;
    }

    int64_t getConstantImm() const {
      const MCExpr *Val = getImm();
      return static_cast<const MCConstantExpr *>(Val)->getValue();
    }

    unsigned getMemBase() const {
      assert((Kind == k_Memory) && "Invalid access!");
      return Mem.Base;
    }

    const MCExpr *getMemOff() const {
      assert((Kind == k_Memory) && "Invalid access!");
      return Mem.Off;
    }

    static std::unique_ptr<EpiphanyOperand> CreateToken(StringRef Str, SMLoc S) {
      auto Op = make_unique<EpiphanyOperand>(k_Token);
      Op->Tok.Data = Str.data();
      Op->Tok.Length = Str.size();
      Op->StartLoc = S;
      Op->EndLoc = S;
      return Op;
    }

    /// Internal constructor for register kinds
    static std::unique_ptr<EpiphanyOperand> CreateReg(unsigned RegNum, SMLoc S, 
        SMLoc E) {
      auto Op = make_unique<EpiphanyOperand>(k_Register);
      Op->Reg.RegNum = RegNum;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    static std::unique_ptr<EpiphanyOperand> CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
      auto Op = make_unique<EpiphanyOperand>(k_Immediate);
      Op->Imm.Val = Val;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    static std::unique_ptr<EpiphanyOperand> CreateMem(unsigned Base, const MCExpr *Off,
        SMLoc S, SMLoc E) {
      auto Op = make_unique<EpiphanyOperand>(k_Memory);
      Op->Mem.Base = Base;
      Op->Mem.Off = Off;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    /// getStartLoc - Get the location of the first token of this operand.
    SMLoc getStartLoc() const { return StartLoc; }
    /// getEndLoc - Get the location of the last token of this operand.
    SMLoc getEndLoc() const { return EndLoc; }

    virtual void print(raw_ostream &OS) const {
      llvm_unreachable("unimplemented!");
    }
  };
}

//@1 {
// Some ops may need expansion
bool EpiphanyAsmParser::needsExpansion(MCInst &Inst) {

  switch(Inst.getOpcode()) {
    default:
      return false;
  }
}

void EpiphanyAsmParser::expandInstruction(MCInst &Inst, SMLoc IDLoc,
    SmallVectorImpl<MCInst> &Instructions){
  switch(Inst.getOpcode()) {
  }
}
//@1 }

//@2 {
bool EpiphanyAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
    OperandVector &Operands,
    MCStreamer &Out,
    uint64_t &ErrorInfo,
    bool MatchingInlineAsm) {
  MCInst Inst;
  unsigned MatchResult = MatchInstructionImpl(Operands, Inst, ErrorInfo,
      MatchingInlineAsm);
  switch (MatchResult) {
    default: 
      break;
    case Match_Success: 
      {
        if (needsExpansion(Inst)) {
          SmallVector<MCInst, 4> Instructions;
          expandInstruction(Inst, IDLoc, Instructions);
          for(unsigned i =0; i < Instructions.size(); i++){
            Out.EmitInstruction(Instructions[i], getSTI());
          }
        } else {
          Inst.setLoc(IDLoc);
          Out.EmitInstruction(Inst, getSTI());
        }
        return false;
      }
      //@2 }
  case Match_MissingFeature:
      Error(IDLoc, "instruction requires a CPU feature not currently enabled");
      return true;
  case Match_InvalidOperand: 
      {
        SMLoc ErrorLoc = IDLoc;
        if (ErrorInfo != ~0U) {
          if (ErrorInfo >= Operands.size())
            return Error(IDLoc, "too few operands for instruction");

          ErrorLoc = ((EpiphanyOperand &)*Operands[ErrorInfo]).getStartLoc();
          if (ErrorLoc == SMLoc()) ErrorLoc = IDLoc;
        }

        return Error(ErrorLoc, "invalid operand for instruction");
      }
  case Match_MnemonicFail:
      return Error(IDLoc, "invalid instruction");
}
return true;
}

// Match register by Alias name (not just r#num)
int EpiphanyAsmParser::matchRegisterName(StringRef Name) {

  int CC;
  CC = StringSwitch<unsigned>(Name)
    .Case("a1",  Epiphany::A1)
    .Case("a2",  Epiphany::A2)
    .Case("a3",  Epiphany::A3)
    .Case("a4",  Epiphany::A4)
    .Case("v1",  Epiphany::V1)
    .Case("v2",  Epiphany::V2)
    .Case("v3",  Epiphany::V3)
    .Case("v4",  Epiphany::V4)
    .Case("v5",  Epiphany::V5)
    .Case("sb",  Epiphany::SB)
    .Case("sl",  Epiphany::SL)
    .Case("v8",  Epiphany::V8)
    .Case("ip",  Epiphany::IP)
    .Case("sp",  Epiphany::SP)
    .Case("lr",  Epiphany::LR)
    .Case("fp",  Epiphany::FP)
    .Case("zero",  Epiphany::ZERO)
    .Default(-1);

  if (CC != -1)
    return CC;

  return -1;
}

// Get register by number
unsigned EpiphanyAsmParser::getReg(int RC,int RegNo) {
  return *(getContext().getRegisterInfo()->getRegClass(RC).begin() + RegNo);
}

// Match register by number
int EpiphanyAsmParser::matchRegisterByNumber(unsigned RegNum, StringRef Mnemonic) {
  if (RegNum > 63)
    return -1;

  return getReg(Epiphany::CPURegsRegClassID, RegNum);
}

int EpiphanyAsmParser::tryParseRegister(StringRef Mnemonic) {
  const AsmToken &Tok = Parser.getTok();
  int RegNum = -1;

  if (Tok.is(AsmToken::Identifier)) {
    std::string lowerCase = Tok.getString().lower();
    RegNum = matchRegisterName(lowerCase);
  } else if (Tok.is(AsmToken::Integer))
    RegNum = matchRegisterByNumber(static_cast<unsigned>(Tok.getIntVal()),
        Mnemonic.lower());
  else
    return RegNum;  //error
  return RegNum;
}

bool EpiphanyAsmParser::tryParseRegisterOperand(OperandVector &Operands,
    StringRef Mnemonic){

  SMLoc S = Parser.getTok().getLoc();
  int RegNo = -1;

  RegNo = tryParseRegister(Mnemonic);
  if (RegNo == -1)
    return true;

  Operands.push_back(EpiphanyOperand::CreateReg(RegNo, S,
        Parser.getTok().getLoc()));
  Parser.Lex(); // Eat register token.
  return false;
}

bool EpiphanyAsmParser::ParseOperand(OperandVector &Operands,
    StringRef Mnemonic) {
  DEBUG(dbgs() << "ParseOperand\n");
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);
  if (ResTy == MatchOperand_Success)
    return false;
  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_ParseFail)
    return true;

  DEBUG(dbgs() << ".. Generic Parser\n");

  switch (getLexer().getKind()) {
    default:
      Error(Parser.getTok().getLoc(), "unexpected token in operand");
      return true;
    case AsmToken::Dollar: 
      {
        // parse register
        SMLoc S = Parser.getTok().getLoc();
        Parser.Lex(); // Eat dollar token.
        // parse register operand
        if (!tryParseRegisterOperand(Operands, Mnemonic)) {
          if (getLexer().is(AsmToken::LParen)) {
            // check if it is indexed addressing operand
            Operands.push_back(EpiphanyOperand::CreateToken("(", S));
            Parser.Lex(); // eat parenthesis
            if (getLexer().isNot(AsmToken::Dollar))
              return true;

            Parser.Lex(); // eat dollar
            if (tryParseRegisterOperand(Operands, Mnemonic))
              return true;

            if (!getLexer().is(AsmToken::RParen))
              return true;

            S = Parser.getTok().getLoc();
            Operands.push_back(EpiphanyOperand::CreateToken(")", S));
            Parser.Lex();
          }
          return false;
        }
        // maybe it is a symbol reference
        StringRef Identifier;
        if (Parser.parseIdentifier(Identifier))
          return true;

        SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

        MCSymbol *Sym = getContext().getOrCreateSymbol("$" + Identifier);

        // Otherwise create a symbol ref.
        const MCExpr *Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None,
            getContext());

        Operands.push_back(EpiphanyOperand::CreateImm(Res, S, E));
        return false;
      }
    case AsmToken::Identifier:
    case AsmToken::LParen:
    case AsmToken::Minus:
    case AsmToken::Plus:
    case AsmToken::Integer:
    case AsmToken::String: 
      {
        // quoted label names
        const MCExpr *IdVal;
        SMLoc S = Parser.getTok().getLoc();
        if (getParser().parseExpression(IdVal))
          return true;
        SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
        Operands.push_back(EpiphanyOperand::CreateImm(IdVal, S, E));
        return false;
      }
    case AsmToken::Percent: 
      {
        // it is a symbol reference or constant expression
        const MCExpr *IdVal;
        SMLoc S = Parser.getTok().getLoc(); // start location of the operand
        if (parseRelocOperand(IdVal))
          return true;

        SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

        Operands.push_back(EpiphanyOperand::CreateImm(IdVal, S, E));
        return false;
      } // case AsmToken::Percent
  } // switch(getLexer().getKind())
  return true;
}

///@evaluateRelocExpr
// This function parses expressions like %high(imm) and transforms them to reloc
const MCExpr *EpiphanyAsmParser::evaluateRelocExpr(const MCExpr *Expr,
    StringRef RelocStr) {
  EpiphanyMCExpr::EpiphanyExprKind Kind =
    StringSwitch<EpiphanyMCExpr::EpiphanyExprKind>(RelocStr)
    .Case("high", EpiphanyMCExpr::CEK_HIGH)
    .Case("low",  EpiphanyMCExpr::CEK_LOW)
    .Default(EpiphanyMCExpr::CEK_None);

  assert(Kind != EpiphanyMCExpr::CEK_None);
  return EpiphanyMCExpr::create(Kind, Expr, getContext());
}

// Parse relocation expression
// For example, %high(smth)
bool EpiphanyAsmParser::parseRelocOperand(const MCExpr *&Res) {

  Parser.Lex(); // eat % token
  const AsmToken &Tok = Parser.getTok(); // get next token, operation
  if (Tok.isNot(AsmToken::Identifier))
    return true;

  std::string Str = Tok.getIdentifier().str();

  Parser.Lex(); // eat identifier
  // now make expression from the rest of the operand
  const MCExpr *IdVal;
  SMLoc EndLoc;

  if (getLexer().getKind() == AsmToken::LParen) {
    while (1) {
      Parser.Lex(); // eat '(' token
      if (getLexer().getKind() == AsmToken::Percent) {
        Parser.Lex(); // eat % token
        const AsmToken &nextTok = Parser.getTok();
        if (nextTok.isNot(AsmToken::Identifier))
          return true;
        Str += "(%";
        Str += nextTok.getIdentifier();
        Parser.Lex(); // eat identifier
        if (getLexer().getKind() != AsmToken::LParen)
          return true;
      } else
        break;
    }
    if (getParser().parseParenExpression(IdVal,EndLoc))
      return true;

    while (getLexer().getKind() == AsmToken::RParen)
      Parser.Lex(); // eat ')' token

  } else
    return true; // parenthesis must follow reloc operand

  Res = evaluateRelocExpr(IdVal, Str);
  return false;
}

bool EpiphanyAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
    SMLoc &EndLoc) {

  StartLoc = Parser.getTok().getLoc();
  RegNo = tryParseRegister("");
  EndLoc = Parser.getTok().getLoc();
  return (RegNo == (unsigned)-1);
}

bool EpiphanyAsmParser::parseMemOffset(const MCExpr *&Res) {

  SMLoc S;

  switch(getLexer().getKind()) {
    default:
      return true;
    case AsmToken::Integer:
    case AsmToken::Minus:
    case AsmToken::Plus:
      return (getParser().parseExpression(Res));
    case AsmToken::Percent:
      return parseRelocOperand(Res);
    case AsmToken::LParen:
      return false;  // it's probably assuming 0
  }
  return true;
}

// eg, 12($sp) or 12(la)
EpiphanyAsmParser::OperandMatchResultTy EpiphanyAsmParser::parseMemOperand(
    OperandVector &Operands) {

  const MCExpr *IdVal = 0;
  SMLoc S;
  // first operand is the offset
  S = Parser.getTok().getLoc();

  if (parseMemOffset(IdVal))
    return MatchOperand_ParseFail;

  const AsmToken &Tok = Parser.getTok(); // get next token
  if (Tok.isNot(AsmToken::LParen)) {
    EpiphanyOperand &Mnemonic = static_cast<EpiphanyOperand &>(*Operands[0]);
    if (Mnemonic.getToken() == "la") {
      SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer()-1);
      Operands.push_back(EpiphanyOperand::CreateImm(IdVal, S, E));
      return MatchOperand_Success;
    }
    Error(Parser.getTok().getLoc(), "'(' expected");
    return MatchOperand_ParseFail;
  }

  Parser.Lex(); // Eat '(' token.

  const AsmToken &Tok1 = Parser.getTok(); // get next token
  if (Tok1.is(AsmToken::Dollar)) {
    Parser.Lex(); // Eat '$' token.
    if (tryParseRegisterOperand(Operands,"")) {
      Error(Parser.getTok().getLoc(), "unexpected token in operand");
      return MatchOperand_ParseFail;
    }

  } else {
    Error(Parser.getTok().getLoc(), "unexpected token in operand");
    return MatchOperand_ParseFail;
  }

  const AsmToken &Tok2 = Parser.getTok(); // get next token
  if (Tok2.isNot(AsmToken::RParen)) {
    Error(Parser.getTok().getLoc(), "')' expected");
    return MatchOperand_ParseFail;
  }

  SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

  Parser.Lex(); // Eat ')' token.

  if (!IdVal)
    IdVal = MCConstantExpr::create(0, getContext());

  // Replace the register operand with the memory operand.
  std::unique_ptr<EpiphanyOperand> op(
      static_cast<EpiphanyOperand *>(Operands.back().release()));
  int RegNo = op->getReg();
  // remove register from operands
  Operands.pop_back();
  // and add memory operand
  Operands.push_back(EpiphanyOperand::CreateMem(RegNo, IdVal, S, E));
  return MatchOperand_Success;
}

bool EpiphanyAsmParser::
parseMathOperation(StringRef Name, SMLoc NameLoc,
    OperandVector &Operands) {
  // split the format
  size_t Start = Name.find('.'), Next = Name.rfind('.');
  StringRef Format1 = Name.slice(Start, Next);
  // and add the first format to the operands
  Operands.push_back(EpiphanyOperand::CreateToken(Format1, NameLoc));
  // now for the second format
  StringRef Format2 = Name.slice(Next, StringRef::npos);
  Operands.push_back(EpiphanyOperand::CreateToken(Format2, NameLoc));

  // set the format for the first register
  //  setFpFormat(Format1);

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }

    if (getLexer().isNot(AsmToken::Comma)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");

    }
    Parser.Lex();  // Eat the comma.

    // Parse and remember the operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.eatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool EpiphanyAsmParser::
ParseInstruction(ParseInstructionInfo &Info, StringRef Name, SMLoc NameLoc,
    OperandVector &Operands) {

  // Create the leading tokens for the mnemonic, split by '.' characters.
  size_t Start = 0, Next = Name.find('.');
  StringRef Mnemonic = Name.slice(Start, Next);
  // Refer to the explanation in source code of function DecodeJumpFR(...) in 
  // EpiphanyDisassembler.cpp
  if (Mnemonic == "ret")
    Mnemonic = "jr";

  Operands.push_back(EpiphanyOperand::CreateToken(Mnemonic, NameLoc));

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }

    while (getLexer().is(AsmToken::Comma) ) {
      Parser.Lex();  // Eat the comma.

      // Parse and remember the operand.
      if (ParseOperand(Operands, Name)) {
        SMLoc Loc = getLexer().getLoc();
        Parser.eatToEndOfStatement();
        return Error(Loc, "unexpected token in argument list");
      }
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.eatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool EpiphanyAsmParser::reportParseError(StringRef ErrorMsg) {
  SMLoc Loc = getLexer().getLoc();
  Parser.eatToEndOfStatement();
  return Error(Loc, ErrorMsg);
}

bool EpiphanyAsmParser::parseSetReorderDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Options.setReorder();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool EpiphanyAsmParser::parseSetNoReorderDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Options.setNoreorder();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool EpiphanyAsmParser::parseSetMacroDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("unexpected token in statement");
    return false;
  }
  Options.setMacro();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool EpiphanyAsmParser::parseSetNoMacroDirective() {
  Parser.Lex();
  // if this is not the end of the statement, report error
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    reportParseError("`noreorder' must be set before `nomacro'");
    return false;
  }
  if (Options.isReorder()) {
    reportParseError("`noreorder' must be set before `nomacro'");
    return false;
  }
  Options.setNomacro();
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}
bool EpiphanyAsmParser::parseDirectiveSet() {

  // get next token
  const AsmToken &Tok = Parser.getTok();

  if (Tok.getString() == "reorder") {
    return parseSetReorderDirective();
  } else if (Tok.getString() == "noreorder") {
    return parseSetNoReorderDirective();
  } else if (Tok.getString() == "macro") {
    return parseSetMacroDirective();
  } else if (Tok.getString() == "nomacro") {
    return parseSetNoMacroDirective();
  }
  return true;
}

bool EpiphanyAsmParser::ParseDirective(AsmToken DirectiveID) {

  if (DirectiveID.getString() == ".ent") {
    // ignore this directive for now
    Parser.Lex();
    return false;
  }

  if (DirectiveID.getString() == ".end") {
    // ignore this directive for now
    Parser.Lex();
    return false;
  }

  if (DirectiveID.getString() == ".frame") {
    // ignore this directive for now
    Parser.eatToEndOfStatement();
    return false;
  }

  if (DirectiveID.getString() == ".set") {
    return parseDirectiveSet();
  }

  if (DirectiveID.getString() == ".fmask") {
    // ignore this directive for now
    Parser.eatToEndOfStatement();
    return false;
  }

  if (DirectiveID.getString() == ".mask") {
    // ignore this directive for now
    Parser.eatToEndOfStatement();
    return false;
  }

  if (DirectiveID.getString() == ".gpword") {
    // ignore this directive for now
    Parser.eatToEndOfStatement();
    return false;
  }

  return true;
}

extern "C" void LLVMInitializeEpiphanyAsmParser() {
  RegisterMCAsmParser<EpiphanyAsmParser> X(TheEpiphanyTarget);
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "EpiphanyGenAsmMatcher.inc"

