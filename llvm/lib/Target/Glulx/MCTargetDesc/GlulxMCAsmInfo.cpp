//===-- GlulxMCAsmInfo.cpp - Glulx Asm Properties -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the GlulxMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "GlulxMCAsmInfo.h"

#include "llvm/ADT/Triple.h"

using namespace llvm;

GlulxMCAsmInfo::GlulxMCAsmInfo(const Triple &TheTriple) {
  // This architecture is big endian only
  IsLittleEndian = false;

  AlignmentIsInBytes          = false;
  Data8bitsDirective          = "\t!datab\t";
  Data16bitsDirective         = "\t!datas\t";
  Data32bitsDirective         = "\t!data\t";

  PrivateGlobalPrefix         = "_L";
  PrivateLabelPrefix          = "_L";

  // to prevent subsequent comments breaking glasm
  LabelSuffix                 = "\n";

  CommentString               = ";";

  GlobalDirective             = "\t; global ";
  HasDotTypeDotSizeDirective  = false;
  ZeroDirective               = "\t!allot ";
}
