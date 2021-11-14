//===- lib/MC/MCAsmStreamer.cpp - Text Assembly Output ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#include "GlulxMCAsmStreamer.h"
#include "GlulxTargetObjectFile.h"
#include "llvm/MC/MCAsmInfo.h"

void GlulxMCAsmStreamer::changeSection(MCSection *Section,
                                       const MCExpr *Subsection) {
  SectionKind Kind = Section->getKind();
  auto *GOS = static_cast<GlulxStreamMultiplexer*>(&OS);
  if (Kind.isText() || Kind.isReadOnly())
    GOS->switchToROM();
  else if (Kind.isData())
    GOS->switchToRAM();
  else if (Kind.isBSS())
    GOS->switchToBSS();
  else
    llvm_unreachable("unexpected Glulx section type");
}

void GlulxMCAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc Loc) {
  MCStreamer::emitLabel(Symbol, Loc);

  if (EOLPending) {   // glasm can't handle multiple labels on the same line
    EOLPending = false;
    OS << "\n";
  }

  OS << ":";
  Symbol->print(OS, MAI);
  OS << MAI->getLabelSuffix();

  // Delay end of line for variable allocations, since an
  // intervening EOL breaks glasm.
  if (!CommentToEmit.empty()) {
    OS << '\n';   // extra newline to separate BB labels from comments for glasm
    EmitEOL();
  } else {
    EOLPending = true;
  }
}

bool GlulxMCAsmStreamer::emitSymbolAttribute(MCSymbol *Symbol,
                                             MCSymbolAttr Attribute) {
  switch (Attribute) {
  case MCSA_Global: OS << MAI->getGlobalDirective(); break;
  case MCSA_Local:  OS << "\t; local "; break;
  default:
    report_fatal_error("unsupported symbol attribute for Glulx");
  }

  Symbol->print(OS, MAI);
  EmitEOL();

  return true;
}

void GlulxMCAsmStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                          unsigned ByteAlignment) {
  assert(static_cast<GlulxStreamMultiplexer*>(&OS)->inBSS() &&
         "tried to emit common Glulx symbol outside of BSS");

  if (ByteAlignment > 1) {
    OS << "\t!alignbss " << ByteAlignment;
    EmitEOL();
  }

  OS << ":";
  Symbol->print(OS, MAI);
  OS << MAI->getLabelSuffix();
  OS << "\t!allot\t" << Size;
  EmitEOL();
}

void GlulxMCAsmStreamer::emitValueToAlignment(unsigned ByteAlignment,
                                              int64_t Value, unsigned ValueSize,
                                              unsigned MaxBytesToEmit) {
  if (!isPowerOf2_32(ByteAlignment))
    report_fatal_error("Only power-of-two alignments are supported "
                       "with .align.");
  OS << "\t!align\t";
  OS << ByteAlignment;
  EmitEOL();
}

void GlulxMCAsmStreamer::finishImpl() {
  auto *GOS = static_cast<GlulxStreamMultiplexer*>(&OS);
  GOS->finish();

  MCAsmStreamer::finishImpl();
}
