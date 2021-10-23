//===-- GlulxMCExpr.cpp - Glulx-specific MC expression classes ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "GlulxMCExpr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCObjectStreamer.h"

using namespace llvm;

#define DEBUG_TYPE "glulxmcexpr"

const GlulxMCExpr *GlulxMCExpr::create(VariantKind Kind, const MCExpr *Expr,
                                       MCContext &Ctx) {
  return new (Ctx) GlulxMCExpr(Kind, Expr);
}

void GlulxMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  if (Kind == VK_GLULX_DEREFERENCE)
    OS << "@";
  getSubExpr()->print(OS, MAI);
}

bool
GlulxMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                       const MCAsmLayout *Layout,
                                       const MCFixup *Fixup) const {
  return getSubExpr()->evaluateAsRelocatable(Res, Layout, Fixup);
}

void GlulxMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}
