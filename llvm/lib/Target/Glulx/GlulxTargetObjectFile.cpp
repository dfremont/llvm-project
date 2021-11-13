//===-- GlulxTargetObjectFile.cpp - Glulx Object Files --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "GlulxTargetObjectFile.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionGlulx.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

GlulxTargetObjectFile::GlulxTargetObjectFile()
    : TargetLoweringObjectFile() {}

MCSection *GlulxTargetObjectFile::getSectionForJumpTable(
    const Function &F, const TargetMachine &TM) const {
  // Otherwise would use data section, which is in RAM.
  // FIXME once we have a proper ReadOnly section.
  return getContext().getObjectFileInfo()->getTextSection();
}

MCSection *GlulxTargetObjectFile::getExplicitSectionGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  return SelectSectionForGlobal(GO, Kind, TM);
}

MCSection *GlulxTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  if (Kind.isText())
    return getContext().getObjectFileInfo()->getTextSection();

  return getContext().getObjectFileInfo()->getDataSection();
}
