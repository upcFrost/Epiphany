//==-- EpiphanyCallingConv.td - Calling Conventions for ARM ----*- tblgen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Epiphany architecture.
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// Epiphany Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_Epiphany_Assign : CallingConv<[
  // Put ByVal arguments directly on the stack. 
  CCIfByVal<CCPassByVal<4, 4>>,

  // Promote all ints to natural i32
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  // First try to assign to the first 8 regs so that we will be able to use 16-bit instr
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,
  // Then, try to use any general purpose caller-saved regs
  CCIfType<[i32,f32], CCAssignToReg<[R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27]>>,
  CCIfType<[i32,f32], CCAssignToReg<[R32, R33, R34, R35, R36, R37, R38, R39, R40, R41, R42, R43]>>,
  CCIfType<[i32,f32], CCAssignToReg<[R44, R45, R46, R47, R48, R49, R50, R51, R52, R53, R54, R55, R56, R57, R58, R59, R60, R61, R62, R63]>>,
  // Then, callee-saved regs
  CCIfType<[i32,f32], CCAssignToReg<[R4, R5, R6, R7, R8]>>,

  // If still no luck - save to stack
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>
]>;

//===----------------------------------------------------------------------===//
// Epiphany Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Epiphany : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  // Result to be returned in first 4 regs
  CCIfType<[i32,f32], CCAssignToReg<[R0, R1, R2, R3]>>,
  // Alternatively, they are assigned to the stack in 4-byte aligned units.
  CCAssignToStack<4, 4>
]>;

def CSR32 : CalleeSavedRegs<(add R4, R5, R6, R7, R8, SB, SL, FP, LR, R15)>;
