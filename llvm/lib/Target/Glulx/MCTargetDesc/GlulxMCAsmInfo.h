//===-- GlulxMCAsmInfo.h - Glulx Asm Info ------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the GlulxMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCASMINFO_H
#define LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
  class Triple;

class GlulxMCAsmInfo : public MCAsmInfo {

public:
  explicit GlulxMCAsmInfo(const Triple &TheTriple);
};

} // namespace llvm

#endif // end LLVM_LIB_TARGET_Glulx_MCTARGETDESC_GlulxMCASMINFO_H
