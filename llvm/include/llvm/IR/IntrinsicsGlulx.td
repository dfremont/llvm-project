//===- IntrinsicsGlulx.td - Defines Glulx intrinsics --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines all of the Glulx-specific intrinsics.
///
//===----------------------------------------------------------------------===//

let TargetPrefix = "glulx" in {  // All intrinsics start with "llvm.glulx.".

// Integer math

def int_glulx_div : GCCBuiltin<"__div">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem]>;     // ignore side effect of division by zero

def int_glulx_mod : GCCBuiltin<"__mod">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem]>;     // ignore side effect of division by zero

// Continuations

def int_glulx_catch : GCCBuiltin<"__catch">,
    Intrinsic<[llvm_i32_ty], [llvm_ptr_ty],
              [IntrArgMemOnly, WriteOnly<ArgIndex<0>>]>;

def int_glulx_throw : GCCBuiltin<"__throw">,
    Intrinsic<[], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoReturn]>;

// Memory allocation heap

def int_glulx_malloc : GCCBuiltin<"__malloc">,
    Intrinsic<[llvm_ptr_ty], [llvm_i32_ty], [IntrInaccessibleMemOnly]>;

def int_glulx_mfree : GCCBuiltin<"__mfree">,
    Intrinsic<[], [llvm_ptr_ty],
              [IntrInaccessibleMemOrArgMemOnly, NoCapture<ArgIndex<0>>]>;

// Output

def int_glulx_setiosys : GCCBuiltin<"__setiosys">,
    Intrinsic<[], [llvm_i32_ty, llvm_i32_ty],
              [IntrNoMem, IntrHasSideEffects]>;

def int_glulx_streamchar : GCCBuiltin<"__streamchar">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_streamnum : GCCBuiltin<"__streamnum">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_streamstr : GCCBuiltin<"__streamstr">,
    Intrinsic<[], [llvm_anyptr_ty], []>;

def int_glulx_streamunichar : GCCBuiltin<"__streamunichar">,
    Intrinsic<[], [llvm_i32_ty], []>;

// Game state

def int_glulx_quit : GCCBuiltin<"__quit">,
    Intrinsic<[], [], [IntrNoReturn]>;

// Miscellaneous

def int_glulx_gestalt : GCCBuiltin<"__gestalt">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty],
              [IntrReadMem]>;

def int_glulx_debugtrap : GCCBuiltin<"__debugtrap">,
    Intrinsic<[], [llvm_i32_ty], []>;

def int_glulx_glk : GCCBuiltin<"__glk">,
    Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty], []>;

def int_glulx_push : GCCBuiltin<"__push">,
    Intrinsic<[], [llvm_i32_ty], [IntrNoMem, IntrHasSideEffects]>;

def int_glulx_pop : GCCBuiltin<"__pop">,
    Intrinsic<[llvm_i32_ty], [], [IntrNoMem, IntrHasSideEffects]>;

} // TargetPrefix = "glulx"
