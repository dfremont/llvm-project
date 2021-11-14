//===-- GlulxTargetStreamer.cpp - Glulx Target Streamer Methods -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides Glulx-specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "GlulxTargetStreamer.h"
#include "GlulxMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

GlulxTargetStreamer::GlulxTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

// This part is for ascii assembly output
GlulxTargetAsmStreamer::GlulxTargetAsmStreamer(MCStreamer &S)
    : GlulxTargetStreamer(S) {
  S.emitRawText(
      "; Preamble:\n"
      "<!include \":glulx\"\n"
      "!include \":veneer\"\n");
}

void GlulxTargetAsmStreamer::changeSection(const MCSection *CurSection,
                                        MCSection *Section,
                                        const MCExpr *SubSection,
                                        raw_ostream &OS) {
  llvm_unreachable(
      "unexpectedly called GlulxTargetAsmStreamer::changeSection");
}

void GlulxTargetAsmStreamer::emitValue(const MCExpr *Value) {
  SmallString<128> Str;
  raw_svector_ostream OS(Str);

  const auto *MAI = Streamer.getContext().getAsmInfo();
  Value->print(OS, MAI);
  if (Value->getKind() == MCExpr::SymbolRef)
    OS << MAI->getLabelSuffix();
  Streamer.emitRawText(OS.str());
}