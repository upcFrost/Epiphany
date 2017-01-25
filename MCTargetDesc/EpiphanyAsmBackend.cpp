//===-- EpiphanyAsmBackend.cpp - Epiphany Asm Backend  ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the EpiphanyAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#include "MCTargetDesc/EpiphanyFixupKinds.h"
#include "MCTargetDesc/EpiphanyAsmBackend.h"

#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "mc-dump"

//@adjustFixupValue {
// Prepare value for the target space for it
static unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
		MCContext *Ctx = nullptr) {
	unsigned Kind = Fixup.getKind();

	// Add/subtract and shift
	switch (Kind) {
		default:
			llvm_unreachable("Unimplemented fixup kind: " + Kind);
    case Epiphany::fixup_Epiphany_PCREL24:
      // Sign-extend and shift by 7 bits
      // Shift by 7 as it will be shifted by 1 afterwards, See Arch reference
      // TODO: iPTR is 16 bits, that's why sign-extension is needed in such way
      DEBUG(dbgs() << "PCREL24 Value before adjust "; dbgs().write_hex(Value); dbgs() << "\n");
      Value = (Value & 0x1ffffff) << 7;
      DEBUG(dbgs() << "PCREL24 Value after adjust "; dbgs().write_hex(Value); dbgs() << "\n");
      Value = Value & 0xffffffff;
      break;
		case FK_GPRel_4:
		case FK_Data_4:
		case Epiphany::fixup_Epiphany_LO16:
      DEBUG(dbgs() << "FK_GPREL_4/Data4 value before adjust "; dbgs().write_hex(Value); dbgs() << "\n");
			break;
		case Epiphany::fixup_Epiphany_HI16:
		case Epiphany::fixup_Epiphany_GOT:
			// Get the higher 16-bits. Also add 1 if bit 15 is 1.
			Value = ((Value + 0x8000) >> 16) & 0xffff;
			break;
	}

	return Value;
}
//@adjustFixupValue }

MCObjectWriter *
EpiphanyAsmBackend::createObjectWriter(raw_pwrite_stream &OS) const {
	return createEpiphanyELFObjectWriter(OS,
			MCELFObjectTargetWriter::getOSABI(OSType), IsLittle);
}

/// ApplyFixup - Apply the \p Value for given \p Fixup into the provided
/// data fragment, at the offset specified by the fixup and following the
/// fixup kind as appropriate.
void EpiphanyAsmBackend::applyFixup(const MCFixup &Fixup, char *Data,
		unsigned DataSize, uint64_t Value,
		bool IsPCRel) const {
	MCFixupKind Kind = Fixup.getKind();
	Value = adjustFixupValue(Fixup, Value);

	if (!Value) {
		return; // Doesn't change encoding.
  }

	// Where do we start in the object
	unsigned Offset = Fixup.getOffset();
	// Number of bytes we need to fixup
	unsigned NumBytes = (getFixupKindInfo(Kind).TargetSize + 7) / 8;
	// Used to point to big endian bytes
	unsigned FullSize;

	switch ((unsigned)Kind) {
		default:
			FullSize = 4;
			break;
	}

	// Grab current value, if any, from bits.
	uint64_t CurVal = 0;

	for (unsigned i = 0; i != NumBytes; ++i) {
		unsigned Idx = IsLittle ? i : (FullSize - 1 - i);
		CurVal |= (uint64_t)((uint8_t)Data[Offset + Idx]) << (i*8);
	}

	uint64_t Mask = ((uint64_t)(-1) >>
			(64 - getFixupKindInfo(Kind).TargetSize));
	CurVal |= Value & Mask;

	// Write out the fixed up bytes back to the code/data bits.
	for (unsigned i = 0; i != NumBytes; ++i) {
		unsigned Idx = IsLittle ? i : (FullSize - 1 - i);
		Data[Offset + Idx] = (uint8_t)((CurVal >> (i*8)) & 0xff);
	}
}

//@getFixupKindInfo {
const MCFixupKindInfo &EpiphanyAsmBackend::
getFixupKindInfo(MCFixupKind Kind) const {
	const static MCFixupKindInfo Infos[Epiphany::NumTargetFixupKinds] = {
		// This table *must* be in same the order of fixup_* kinds in
		// EpiphanyFixupKinds.h.
		//
		// name                        offset  bits  flags
		{ "fixup_Epiphany_32",             0,     32,   0 },
		{ "fixup_Epiphany_HI16",           0,     16,   0 },
		{ "fixup_Epiphany_LO16",           0,     16,   0 },
		{ "fixup_Epiphany_GPREL16",        0,     16,   0 },
		{ "fixup_Epiphany_GOT",            0,     16,   0 },
		{ "fixup_Epiphany_GOT_HI16",       0,     16,   0 },
		{ "fixup_Epiphany_GOT_LO16",       0,     16,   0 },
		{ "fixup_Epiphany_PCREL16",        0,     16,   MCFixupKindInfo::FKF_IsPCRel },
		{ "fixup_Epiphany_PCREL24",        0,     32,   MCFixupKindInfo::FKF_IsPCRel }
	};

	if (Kind < FirstTargetFixupKind)
		return MCAsmBackend::getFixupKindInfo(Kind);

	assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
			"Invalid kind!");
	return Infos[Kind - FirstTargetFixupKind];
}
//@getFixupKindInfo }

/// WriteNopData - Write an (optimal) nop sequence of Count bytes
/// to the given output. If the target cannot generate such a sequence,
/// it should return an error.
///
/// \return - True on success.
bool EpiphanyAsmBackend::writeNopData(uint64_t Count, MCObjectWriter *OW) const {

  static const uint16_t Nopcode = 0x1A2, // Hard-coded NOP code
                        InstrSize = 2;   // Minimal instruction size is 2 bytes

  // If alignment is not even, something's wrong. 
  if (Count % InstrSize) {
    llvm_unreachable("Alignment is not a multiple of 2 in writeNopData");
  }

  // Add nops
  while (Count) {
    Count -= InstrSize;
    OW->write16(Nopcode);
  }

	return true;
}

// MCAsmBackend
MCAsmBackend *llvm::createEpiphanyAsmBackendEL32(const Target &T,
		const MCRegisterInfo &MRI,
		const Triple &TT, StringRef CPU) {
	return new EpiphanyAsmBackend(T, TT.getOS(), /*IsLittle*/true);
}
