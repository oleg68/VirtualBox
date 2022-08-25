/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include "compiler/nir/nir.h"
#include "compiler/nir/nir_builder.h"
#include "gl_nir.h"
#include "ir_uniform.h"
#include "main/config.h"
#include "main/mtypes.h"
#include <assert.h>

/*
 * replace atomic counter intrinsics that use a variable with intrinsics
 * that directly store the buffer index and byte offset
 */

static bool
lower_deref_instr(nir_builder *b, nir_intrinsic_instr *instr,
                  const struct gl_shader_program *shader_program,
                  nir_shader *shader, bool use_binding_as_idx)
{
   nir_intrinsic_op op;
   switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_read_deref:
      op = nir_intrinsic_atomic_counter_read;
      break;

   case nir_intrinsic_atomic_counter_inc_deref:
      op = nir_intrinsic_atomic_counter_inc;
      break;

   case nir_intrinsic_atomic_counter_pre_dec_deref:
      op = nir_intrinsic_atomic_counter_pre_dec;
      break;

   case nir_intrinsic_atomic_counter_post_dec_deref:
      op = nir_intrinsic_atomic_counter_post_dec;
      break;

   case nir_intrinsic_atomic_counter_add_deref:
      op = nir_intrinsic_atomic_counter_add;
      break;

   case nir_intrinsic_atomic_counter_min_deref:
      op = nir_intrinsic_atomic_counter_min;
      break;

   case nir_intrinsic_atomic_counter_max_deref:
      op = nir_intrinsic_atomic_counter_max;
      break;

   case nir_intrinsic_atomic_counter_and_deref:
      op = nir_intrinsic_atomic_counter_and;
      break;

   case nir_intrinsic_atomic_counter_or_deref:
      op = nir_intrinsic_atomic_counter_or;
      break;

   case nir_intrinsic_atomic_counter_xor_deref:
      op = nir_intrinsic_atomic_counter_xor;
      break;

   case nir_intrinsic_atomic_counter_exchange_deref:
      op = nir_intrinsic_atomic_counter_exchange;
      break;

   case nir_intrinsic_atomic_counter_comp_swap_deref:
      op = nir_intrinsic_atomic_counter_comp_swap;
      break;

   default:
      return false;
   }

   nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
   nir_variable *var = nir_deref_instr_get_variable(deref);

   if (var->data.mode != nir_var_uniform &&
       var->data.mode != nir_var_mem_ssbo &&
       var->data.mode != nir_var_mem_shared)
      return false; /* atomics passed as function arguments can't be lowered */

   const unsigned uniform_loc = var->data.location;
   const unsigned idx = use_binding_as_idx ? var->data.binding :
      shader_program->data->UniformStorage[uniform_loc].opaque[shader->info.stage].index;

   b->cursor = nir_before_instr(&instr->instr);

   nir_ssa_def *offset = nir_imm_int(b, var->data.offset);
   for (nir_deref_instr *d = deref; d->deref_type != nir_deref_type_var;
        d = nir_deref_instr_parent(d)) {
      assert(d->deref_type == nir_deref_type_array);
      assert(d->arr.index.is_ssa);

      unsigned array_stride = ATOMIC_COUNTER_SIZE;
      if (glsl_type_is_array(d->type))
         array_stride *= glsl_get_aoa_size(d->type);

      offset = nir_iadd(b, offset, nir_imul(b, d->arr.index.ssa,
                                            nir_imm_int(b, array_stride)));
   }

   /* Since the first source is a deref and the first source in the lowered
    * instruction is the offset, we can just swap it out and change the
    * opcode.
    */
   instr->intrinsic = op;
   nir_instr_rewrite_src(&instr->instr, &instr->src[0],
                         nir_src_for_ssa(offset));
   nir_intrinsic_set_base(instr, idx);

   nir_deref_instr_remove_if_unused(deref);

   return true;
}

bool
gl_nir_lower_atomics(nir_shader *shader,
                     const struct gl_shader_program *shader_program,
                     bool use_binding_as_idx)
{
   bool progress = false;

   nir_foreach_function(function, shader) {
      if (!function->impl)
         continue;

      bool impl_progress = false;

      nir_builder build;
      nir_builder_init(&build, function->impl);

      nir_foreach_block(block, function->impl) {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type != nir_instr_type_intrinsic)
               continue;

            impl_progress |= lower_deref_instr(&build,
                                               nir_instr_as_intrinsic(instr),
                                               shader_program, shader,
                                               use_binding_as_idx);
         }
      }

      if (impl_progress) {
         nir_metadata_preserve(function->impl, nir_metadata_block_index |
                                               nir_metadata_dominance);
         progress = true;
      }
   }

   return progress;
}
