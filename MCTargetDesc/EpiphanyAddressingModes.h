#ifndef LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYADDRESSINGMODES_H
#define LLVM_LIB_TARGET_EPIPHANY_MCTARGETDESC_EPIPHANYADDRESSINGMODES_H

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>

namespace llvm {

  /// Epiphany_AM - Epiphany Addressing Mode Stuff
  namespace Epiphany_AM {
    /// getFP16Imm - Return an 8-bit floating-point version of the 16-bit
    /// floating-point value. If the value cannot be represented as an 8-bit
    /// floating-point value, then return -1.
    static inline int getFP16Imm(const APInt &Imm) {
      uint32_t Sign = Imm.lshr(15).getZExtValue() & 1;
      int32_t Exp = (Imm.lshr(10).getSExtValue() & 0x1f) - 15;  // -14 to 15
      int32_t Mantissa = Imm.getZExtValue() & 0x3ff;  // 10 bits

      // We can handle 4 bits of mantissa.
      // mantissa = (16+UInt(e:f:g:h))/16.
      if (Mantissa & 0x3f)
        return -1;
      Mantissa >>= 6;

      // We can handle 3 bits of exponent: exp == UInt(NOT(b):c:d)-3
      if (Exp < -3 || Exp > 4)
        return -1;
      Exp = ((Exp+3) & 0x7) ^ 4;

      return ((int)Sign << 7) | (Exp << 4) | Mantissa;
    }

    static inline int getFP16Imm(const APFloat &FPImm) {
      return getFP16Imm(FPImm.bitcastToAPInt());
    }

  }

}

#endif
