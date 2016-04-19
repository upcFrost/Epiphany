//===- EpiphanyRegisterInfo.td - ARM Register defs ----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains declarations that describe the Epiphany register file
//
//===----------------------------------------------------------------------===//

// Registers are identified with 5-bit ID numbers.
class EpiphanyReg<bits<16> enc, string n> : Register<n> {
  let HWEncoding = enc;
  let Namespace = "Epiphany";
}

class Bank0Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank1Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank2Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;
class Bank3Reg<bits<16> enc, string n> : EpiphanyReg<enc, n>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//

// Bank 0 GPRs
let Namespace = "Epiphany" in {
  def R0    : Bank0Reg<0,  "A1">,  DwarfRegNum<[0]>;
  def R1    : Bank0Reg<1,  "A2">,  DwarfRegNum<[1]>;
  def R2    : Bank0Reg<2,  "A3">,  DwarfRegNum<[2]>;
  def R3    : Bank0Reg<3,  "A4">,  DwarfRegNum<[3]>;
  def R4    : Bank0Reg<4,  "V1">,  DwarfRegNum<[4]>;
  def R5    : Bank0Reg<5,  "V2">,  DwarfRegNum<[5]>;
  def R6    : Bank0Reg<6,  "V3">,  DwarfRegNum<[6]>;
  def R7    : Bank0Reg<7,  "V4">,  DwarfRegNum<[7]>;
  def R8    : Bank0Reg<8,  "V5">,  DwarfRegNum<[8]>;
  def SB    : Bank0Reg<9,  "SB">,  DwarfRegNum<[9]>;
  def SL    : Bank0Reg<10, "SL">,  DwarfRegNum<[10]>;
  def FP    : Bank0Reg<11, "FP">,  DwarfRegNum<[11]>;
  def R12   : Bank0Reg<12, "R12">, DwarfRegNum<[12]>;
  def SP    : Bank0Reg<13, "SP">,  DwarfRegNum<[13]>;
  def LR    : Bank0Reg<14, "LR">,  DwarfRegNum<[14]>;
  foreach i = 15-63 in {
    def R#i : Bank0Reg<#i, "R#i">,  DwarfRegNum<[#i]>;
  }
}

// Bank1 registers
let Namespace = "Epiphany" in {
  def CONFIG      : Bank1Reg<0,  "CONFIG">,      DwarfRegNum<[100]>;
  def STATUS      : Bank1Reg<1,  "STATUS">,      DwarfRegNum<[101]>;
  def PC          : Bank1Reg<2,  "PC">,          DwarfRegNum<[102]>;
  def DEBUGSTATUS : Bank1Reg<3,  "DEBUGSTATUS">, DwarfRegNum<[103]>;
  def LC          : Bank1Reg<4,  "LC">,          DwarfRegNum<[104]>;
  def LS          : Bank1Reg<5,  "LS">,          DwarfRegNum<[105]>;
  def LE          : Bank1Reg<6,  "LE">,          DwarfRegNum<[106]>;
  def IRET        : Bank1Reg<7,  "IRET">,        DwarfRegNum<[107]>;
  def IMASK       : Bank1Reg<8,  "IMASK">,       DwarfRegNum<[108]>;
  def ILAT        : Bank1Reg<9,  "ILAT">,        DwarfRegNum<[109]>;
  def ILATST      : Bank1Reg<10, "ILATST">,      DwarfRegNum<[110]>;
  def ILATCL      : Bank1Reg<11, "ILATCL">,      DwarfRegNum<[111]>;
  def IPEND       : Bank1Reg<12, "IPEND">,       DwarfRegNum<[112]>;
  def CTIMER0     : Bank1Reg<13, "CTIMER0">,     DwarfRegNum<[113]>;
  def CTIMER1     : Bank1Reg<14, "CTIMER1">,     DwarfRegNum<[114]>;
  def FSTATUS     : Bank1Reg<15, "FSTATUS">,     DwarfRegNum<[115]>;
  // 0xF0444 is empty
  def DEBUGCMD    : Bank1Reg<17, "DEBUGCMD">,    DwarfRegNum<[117]>;
}

// DMA Registers
let Namespace = "Epiphany" in {
  def DMA0CONFIG  : Bank2Reg<0, "DMA0CONFIG">,   DwarfRegNum<[200]>;
}

// 0xF07xx
let Namespace = "Epiphany" in {
  def RESETCORE   : Bank1Reg<1, "RESETCORE">,   DwarfRegNum<[400]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

// General purpose
def GPR32 : RegisterClass<"Epiphany", [i32], 32, (add
  // Arg/result/scratch caller-saved regs
  R0, R1, R2, R3,
  // Variable callee-saved
  R4, R5, R6, R7, R8,
  // Static base
  SB,
  // Stack limit
  SL,
  // Frame pointer
  FP,
  // Intra-procedure call scratch reg
  R12,
  // Stack pointer
  SP,
  // Callee-saved link register
  LR,
  // Callee-saved GPR
  R15,
  // Caller-saved GPR
  (sequence "R%u", 16, 27),
  // Reserved
  R28, R29, R30,
  // Callee-saved GPR
  (sequence "R%u", 32, 43),
  // Caller-saved GPR
  (sequence "R%u", 44, 63))>;

// Status register
def SR : RegisterClass<"Epiphany", [i32], 32, (add STATUS)>;
