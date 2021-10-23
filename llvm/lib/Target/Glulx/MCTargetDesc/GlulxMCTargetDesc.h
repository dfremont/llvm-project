//===-- GlulxMCTargetDesc.h - Glulx Target Descriptions ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Glulx specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCTARGETDESC_H
#define LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCTARGETDESC_H

// Defines symbolic names for Glulx registers. This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "GlulxGenRegisterInfo.inc"

// Defines symbolic names for the Glulx instructions.
#define GET_INSTRINFO_ENUM
#include "GlulxGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "GlulxGenSubtargetInfo.inc"

namespace llvm {
namespace Glulx {
inline bool isArgument(unsigned Opc) {
  switch (Opc) {
  case Glulx::ARGUMENT_I32:
  case Glulx::ARGUMENT_F32:
    return true;
  default:
    return false;
  }
}
}

// GlulxII - This namespace holds all of the target-specific flags that
// instruction info tracks.
namespace GlulxII {
// Target Operand Flag enum.
enum TOF {
  //===------------------------------------------------------------------===//
  // Glulx-specific MachineOperand flags.
  MO_NO_FLAG,

  // MO_DEREFERENCE - Represents load/store from a global address operand.
  MO_DEREFERENCE,
};
} // namespace GlulxII
}

#endif // end LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCTARGETDESC_H
