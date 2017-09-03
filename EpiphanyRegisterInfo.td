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

let Namespace = "Epiphany" in {
  def isub_lo  : SubRegIndex<32, 32>;
  def isub_hi  : SubRegIndex<32, 0>;
  def subreg_overflow : SubRegIndex<1, 0>;
}

class EpiphanyReg64<bits<16> enc, string n, list<Register> subregs> : RegisterWithSubRegs<n, subregs> {
  let HWEncoding = enc;
  let Namespace = "Epiphany";
  let SubRegs = subregs;
  let SubRegIndices = [isub_lo, isub_hi];
  let CoveredBySubRegs = 1;
}

class GPReg32<bits<16> enc, string n>:     EpiphanyReg<enc, n>;
class eCoreReg<bits<16> enc, string n>:    EpiphanyReg<enc, n>;
class DmaReg<bits<16> enc, string n>:      EpiphanyReg<enc, n>;
class MemProtReg<bits<16> enc, string n>:  EpiphanyReg<enc, n>;
class MeshNodeReg<bits<16> enc, string n>: EpiphanyReg<enc, n>;

class GPReg64<bits<16> enc, string n, list<Register> subregs> : EpiphanyReg64<enc, n, subregs>;

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//

// General purpose registers
let Namespace = "Epiphany" in {
// 32 bit regs
  foreach i = 0-63 in {
    def R#i : GPReg32<#i, "R"#i>,  DwarfRegNum<[#i]>;
  }

// 64 bit extension
  def D0    : GPReg64<0,  "D0",  [R0, R1]>,   DwarfRegNum<[0]>;
  def D1    : GPReg64<2,  "D1",  [R2, R3]>,   DwarfRegNum<[2]>;
  def D2    : GPReg64<4,  "D2",  [R4, R5]>,   DwarfRegNum<[4]>;
  def D3    : GPReg64<6,  "D3",  [R6, R7]>,   DwarfRegNum<[6]>;
  def D4    : GPReg64<8,  "D4",  [R8, R9]>,   DwarfRegNum<[8]>;
  def D5    : GPReg64<10, "D5",  [R10, R11]>, DwarfRegNum<[10]>;
  def D6    : GPReg64<12, "D6",  [R12, R13]>, DwarfRegNum<[12]>;
  def D7    : GPReg64<14, "D7",  [R14, R15]>, DwarfRegNum<[14]>;
  def D8    : GPReg64<16, "D8",  [R16, R17]>, DwarfRegNum<[16]>;
  def D9    : GPReg64<18, "D9",  [R18, R19]>, DwarfRegNum<[18]>;
  def D10   : GPReg64<20, "D10", [R20, R21]>, DwarfRegNum<[20]>;
  def D11   : GPReg64<22, "D11", [R22, R23]>, DwarfRegNum<[22]>;
  def D12   : GPReg64<24, "D12", [R24, R25]>, DwarfRegNum<[24]>;
  def D13   : GPReg64<26, "D13", [R26, R27]>, DwarfRegNum<[26]>;
  def D14   : GPReg64<28, "D14", [R28, R29]>, DwarfRegNum<[28]>;
  def D15   : GPReg64<30, "D15", [R30, R31]>, DwarfRegNum<[30]>;
  def D16   : GPReg64<32, "D16", [R32, R33]>, DwarfRegNum<[32]>;
  def D17   : GPReg64<34, "D17", [R34, R35]>, DwarfRegNum<[34]>;
  def D18   : GPReg64<36, "D18", [R36, R37]>, DwarfRegNum<[36]>;
  def D19   : GPReg64<38, "D19", [R38, R39]>, DwarfRegNum<[38]>;
  def D20   : GPReg64<40, "D20", [R40, R41]>, DwarfRegNum<[40]>;
  def D21   : GPReg64<42, "D21", [R42, R43]>, DwarfRegNum<[42]>;
  def D22   : GPReg64<44, "D22", [R44, R45]>, DwarfRegNum<[44]>;
  def D23   : GPReg64<46, "D23", [R46, R47]>, DwarfRegNum<[46]>;
  def D24   : GPReg64<48, "D24", [R48, R49]>, DwarfRegNum<[48]>;
  def D25   : GPReg64<50, "D25", [R50, R51]>, DwarfRegNum<[50]>;
  def D26   : GPReg64<52, "D26", [R52, R53]>, DwarfRegNum<[52]>;
  def D27   : GPReg64<54, "D27", [R54, R55]>, DwarfRegNum<[54]>;
  def D28   : GPReg64<56, "D28", [R56, R57]>, DwarfRegNum<[56]>;
  def D29   : GPReg64<58, "D29", [R58, R59]>, DwarfRegNum<[58]>;
  def D30   : GPReg64<60, "D30", [R60, R61]>, DwarfRegNum<[60]>;
  def D31   : GPReg64<62, "D31", [R62, R63]>, DwarfRegNum<[62]>;

// Aliases
  def A1    : GPReg32<0,  "A1">,  DwarfRegAlias<R0>;
  def A2    : GPReg32<1,  "A2">,  DwarfRegAlias<R1>;
  def A3    : GPReg32<2,  "A3">,  DwarfRegAlias<R2>;
  def A4    : GPReg32<3,  "A4">,  DwarfRegAlias<R3>;
  def V1    : GPReg32<4,  "V1">,  DwarfRegAlias<R4>;
  def V2    : GPReg32<5,  "V2">,  DwarfRegAlias<R5>;
  def V3    : GPReg32<6,  "V3">,  DwarfRegAlias<R6>;
  def V4    : GPReg32<7,  "V4">,  DwarfRegAlias<R7>;
  def V5    : GPReg32<8,  "V5">,  DwarfRegAlias<R8>;
  def SB    : GPReg32<9,  "SB">,  DwarfRegAlias<R9>;
  def SL    : GPReg32<10, "SL">,  DwarfRegAlias<R10>;
  def V8    : GPReg32<11, "V8">,  DwarfRegAlias<R11>;
  def IP    : GPReg32<12, "IP">,  DwarfRegAlias<R12>;
  def SP    : GPReg32<13, "SP">,  DwarfRegAlias<R13>;
  def LR    : GPReg32<14, "LR">,  DwarfRegAlias<R14>;
  def FP    : GPReg32<15, "FP">,  DwarfRegAlias<R15>;
  def ZERO  : GPReg32<31, "ZERO">, DwarfRegAlias<R31>;
}

// eCore registers
// 0xF04xx
let Namespace = "Epiphany" in {
  def CONFIG      : eCoreReg<0,  "CONFIG">,      DwarfRegNum<[100]>;
  def STATUS      : eCoreReg<1,  "STATUS">,      DwarfRegNum<[101]>;
  def PC          : eCoreReg<2,  "PC">,          DwarfRegNum<[102]>;
  def DEBUGSTATUS : eCoreReg<3,  "DEBUGSTATUS">, DwarfRegNum<[103]>;
  // 0xF0410 is empty
  def LC          : eCoreReg<5,  "LC">,          DwarfRegNum<[105]>;
  def LS          : eCoreReg<6,  "LS">,          DwarfRegNum<[106]>;
  def LE          : eCoreReg<7,  "LE">,          DwarfRegNum<[107]>;
  def IRET        : eCoreReg<8,  "IRET">,        DwarfRegNum<[108]>;
  def IMASK       : eCoreReg<9,  "IMASK">,       DwarfRegNum<[109]>;
  def ILAT        : eCoreReg<10, "ILAT">,        DwarfRegNum<[110]>;
  def ILATST      : eCoreReg<11, "ILATST">,      DwarfRegNum<[111]>;
  def ILATCL      : eCoreReg<12, "ILATCL">,      DwarfRegNum<[112]>;
  def IPEND       : eCoreReg<13, "IPEND">,       DwarfRegNum<[113]>;
  def CTIMER0     : eCoreReg<14, "CTIMER0">,     DwarfRegNum<[114]>;
  def CTIMER1     : eCoreReg<15, "CTIMER1">,     DwarfRegNum<[115]>;
  def FSTATUS     : eCoreReg<16, "FSTATUS">,     DwarfRegNum<[116]>;
  // 0xF0444 is empty
  def DEBUGCMD    : eCoreReg<18, "DEBUGCMD">,    DwarfRegNum<[118]>;
}

// DMA Registers, 0xF05xx
let Namespace = "Epiphany" in {
  def DMA0CONFIG  : DmaReg<0,  "DMA0CONFIG">,  DwarfRegNum<[200]>;
  def DMA0STRIDE  : DmaReg<1,  "DMA0STRIDE">,  DwarfRegNum<[201]>;
  def DMA0COUNT   : DmaReg<2,  "DMA0COUNT">,   DwarfRegNum<[202]>;
  def DMA0SRCADDR : DmaReg<3,  "DMA0SRCADDR">, DwarfRegNum<[203]>;
  def DMA0DSTADDR : DmaReg<4,  "DMA0DSTADDR">, DwarfRegNum<[204]>;
  def DMA0AUTO0   : DmaReg<5,  "DMA0AUTO0">,   DwarfRegNum<[205]>;
  def DMA0AUTO1   : DmaReg<6,  "DMA0AUTO1">,   DwarfRegNum<[206]>;
  def DMA0STATUS  : DmaReg<7,  "DMA0STATUS">,  DwarfRegNum<[207]>;
  def DMA1CONFIG  : DmaReg<8,  "DMA1CONFIG">,  DwarfRegNum<[208]>;
  def DMA1STRIDE  : DmaReg<9,  "DMA1STRIDE">,  DwarfRegNum<[209]>;
  def DMA1COUNT   : DmaReg<10, "DMA1COUNT">,   DwarfRegNum<[210]>;
  def DMA1SRCADDR : DmaReg<11, "DMA1SRCADDR">, DwarfRegNum<[211]>;
  def DMA1DSTADDR : DmaReg<12, "DMA1DSTADDR">, DwarfRegNum<[212]>;
  def DMA1AUTO0   : DmaReg<13, "DMA1AUTO0">,   DwarfRegNum<[213]>;
  def DMA1AUTO1   : DmaReg<14, "DMA1AUTO1">,   DwarfRegNum<[214]>;
  def DMA1STATUS  : DmaReg<15, "DMA1STATUS">,  DwarfRegNum<[215]>;
}

// Processor control / memory protection regs
// 0xF06xx
let Namespace = "Epiphany" in {
  // 0xF0600 is empty
  def MEMSTATUS  : MemProtReg<1, "MEMSTATUS">,  DwarfRegNum<[301]>;
  def MEMPROTECT : MemProtReg<2, "MEMPROTECT">, DwarfRegNum<[302]>;
}

// Mesh node control
// 0xF07xx
let Namespace = "Epiphany" in {
  def MESHCONFIG  : MeshNodeReg<0, "MESHCONFIG">,  DwarfRegNum<[400]>;
  def COREID      : MeshNodeReg<1, "COREID">,      DwarfRegNum<[401]>;
  def MULTICAST   : MeshNodeReg<2, "MULTICAST">,   DwarfRegNum<[402]>;
  def RESETCORE   : MeshNodeReg<3, "RESETCORE">,   DwarfRegNum<[403]>;
  def CMESHROUTE  : MeshNodeReg<4, "CMESHROUTE">,  DwarfRegNum<[404]>;
  def XMESHROUTE  : MeshNodeReg<5, "XMESHROUTE">,  DwarfRegNum<[405]>;
  def RMESHROUTE  : MeshNodeReg<6, "RMESHROUTE">,  DwarfRegNum<[406]>;
}

//===----------------------------------------------------------------------===//
//@Register Classes
//===----------------------------------------------------------------------===//

// General purpose (first 7 only)
let AllocationPriority = 2, Size = 32, CopyCost = 1 in {
  def GPR16 : RegisterClass<"Epiphany", [i32, v4i8, v2i16], 32, (add (sequence "R%u", 0, 7))>;
  def FPR16 : RegisterClass<"Epiphany", [f32], 32, (add (sequence "R%u", 0, 7))>;
}

// General purpose 32 bit
def GPR32 : RegisterClass<"Epiphany", [i32, v4i8, v2i16], 32, (add
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
  IP,
  // Stack pointer
  SP,
  // Callee-saved link register
  LR,
  // Caller-saved GPR
  (sequence "R%u", 16, 27),
  // Reserved
  R28, R29, R30, ZERO,
  // Caller-saved GPR
  (sequence "R%u", 32, 63))> {
  
  let AllocationPriority = 2;
  let Size = 32;
  let CopyCost = 1;
}

def FPR32 : RegisterClass<"Epiphany", [f32], 32, (add GPR32)>;

// 64 bit
def GPR64 : RegisterClass<"Epiphany", [i64,v2i32], 64, (add (sequence "D%u", 0, 31))> {
  let CopyCost = 4;
  let Size = 64;
}
def FPR64 : RegisterClass<"Epiphany", [f64,v2f32], 64, (add GPR64)> {
  let CopyCost = 4;
  let Size = 64;
}


// We should not be able to copy special regs on a whim
let CopyCost = -1, isAllocatable = 0 in {
  // Status register
  def SR : RegisterClass<"Epiphany", [i32], 32, (add STATUS)>;

  // eCoreRegClass regs
  def eCore : RegisterClass<"Epiphany", [i32], 32, (add 
    CONFIG, STATUS, PC, DEBUGSTATUS,
    LC, LS, LE,
    IRET, IMASK, ILAT, ILATST, ILATCL, IPEND,
    CTIMER0, CTIMER1,
    FSTATUS, DEBUGCMD)>;

  def DMA : RegisterClass<"Epiphany", [i32], 32, (add
    DMA0CONFIG, DMA0STRIDE, DMA0COUNT, DMA0SRCADDR, DMA0DSTADDR, DMA0AUTO0, DMA0AUTO1, DMA0STATUS,
    DMA1CONFIG, DMA1STRIDE, DMA1COUNT, DMA1SRCADDR, DMA1DSTADDR, DMA1AUTO0, DMA1AUTO1, DMA1STATUS)>;
  
  def MemProtect : RegisterClass<"Epiphany", [i32], 32, (add MEMSTATUS, MEMPROTECT)>;

  def MeshNodeControl : RegisterClass<"Epiphany", [i32], 32, (add 
    MESHCONFIG, COREID, MULTICAST, RESETCORE,
    CMESHROUTE, XMESHROUTE, RMESHROUTE)>;

  def Special : RegisterClass<"Epiphany", [i32], 32, (add eCore, DMA, MemProtect, MeshNodeControl)>;
}


