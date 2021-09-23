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
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

GlulxTargetStreamer::GlulxTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

// This part is for ascii assembly output
GlulxTargetAsmStreamer::GlulxTargetAsmStreamer(MCStreamer &S,
                                               formatted_raw_ostream &OS)
    : GlulxTargetStreamer(S), OS(OS) {
  OS << "; Preamble:\n";
  OS << "<!include \":glulx\"\n";
  OS << "<!include \":glk\"\n";
  OS << "!include \":veneer\"\n";
  // Set up macros to map non-configurable parts of GAS syntax to glasm syntax.
  OS << "!macro \".file\"\n!endm\n";
  OS << "!macro \".p2align\"\n!alignbss \\1\n!endm\n";
}

void GlulxTargetAsmStreamer::emitLabel(MCSymbol *Symbol) {
  OS << ":";
}

void GlulxTargetAsmStreamer::changeSection(const MCSection *CurSection,
                                        MCSection *Section,
                                        const MCExpr *SubSection,
                                        raw_ostream &OS) {
  SectionKind Kind = Section->getKind();
  assert((Kind.isText() || Kind.isBSS()) && "unexpected Glulx section type");
  OS << "\t" << Section->getName() <<"\n";
}
