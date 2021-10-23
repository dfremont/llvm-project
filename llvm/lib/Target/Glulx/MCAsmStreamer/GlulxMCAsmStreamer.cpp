//===- lib/MC/MCAsmStreamer.cpp - Text Assembly Output ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#include "GlulxMCAsmStreamer.h"
#include "llvm/MC/MCAsmInfo.h"

void GlulxMCAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc Loc) {
  MCStreamer::emitLabel(Symbol, Loc);

  OS << ":";
  Symbol->print(OS, MAI);
  OS << MAI->getLabelSuffix();

  // Suppress end of line for variable allocations, since an
  // intervening EOL breaks glasm.
  if (!CommentToEmit.empty()) {
    OS << '\n';   // extra newline to separate BB labels from comments for glasm
    EmitEOL();
  }
}


