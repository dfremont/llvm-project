//===-- GlulxTargetStreamer.h - Glulx Target Streamer ----------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_GLULX_GLULXTARGETSTREAMER_H
#define LLVM_LIB_TARGET_GLULX_GLULXTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

class formatted_raw_ostream;

class GlulxTargetStreamer : public MCTargetStreamer {
public:
  GlulxTargetStreamer(MCStreamer &S);
};

class GlulxTargetAsmStreamer : public GlulxTargetStreamer {
public:
  GlulxTargetAsmStreamer(MCStreamer &S);

  void changeSection(const MCSection *CurSection, MCSection *Section,
                     const MCExpr *SubSection, raw_ostream &OS) override;
  void emitValue(const MCExpr *Value) override;
};

} // namespace llvm
#endif
