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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "bufferobj.h"
#include "compute.h"
#include "context.h"

static bool
check_valid_to_compute(struct gl_context *ctx, const char *function)
{
   if (!_mesa_has_compute_shaders(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "unsupported function (%s) called",
                  function);
      return false;
   }

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_OPERATION error is generated if there is no active program
    *  for the compute shader stage."
    */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE] == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(no active compute shader)",
                  function);
      return false;
   }

   return true;
}

static bool
validate_DispatchCompute(struct gl_context *ctx, const GLuint *num_groups)
{
   if (!check_valid_to_compute(ctx, "glDispatchCompute"))
      return GL_FALSE;

   for (int i = 0; i < 3; i++) {
      /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
       *
       * "An INVALID_VALUE error is generated if any of num_groups_x,
       *  num_groups_y and num_groups_z are greater than or equal to the
       *  maximum work group count for the corresponding dimension."
       *
       * However, the "or equal to" portions appears to be a specification
       * bug. In all other areas, the specification appears to indicate that
       * the number of workgroups can match the MAX_COMPUTE_WORK_GROUP_COUNT
       * value. For example, under DispatchComputeIndirect:
       *
       * "If any of num_groups_x, num_groups_y or num_groups_z is greater than
       *  the value of MAX_COMPUTE_WORK_GROUP_COUNT for the corresponding
       *  dimension then the results are undefined."
       *
       * Additionally, the OpenGLES 3.1 specification does not contain "or
       * equal to" as an error condition.
       */
      if (num_groups[i] > ctx->Const.MaxComputeWorkGroupCount[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchCompute(num_groups_%c)", 'x' + i);
         return GL_FALSE;
      }
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated by DispatchCompute if the active
    *  program for the compute shader stage has a variable work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (prog->info.cs.local_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDispatchCompute(variable work group size forbidden)");
      return GL_FALSE;
   }

   return GL_TRUE;
}

static bool
validate_DispatchComputeGroupSizeARB(struct gl_context *ctx,
                                     const GLuint *num_groups,
                                     const GLuint *group_size)
{
   if (!check_valid_to_compute(ctx, "glDispatchComputeGroupSizeARB"))
      return GL_FALSE;

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated by
    *  DispatchComputeGroupSizeARB if the active program for the compute
    *  shader stage has a fixed work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (!prog->info.cs.local_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDispatchComputeGroupSizeARB(fixed work group size "
                  "forbidden)");
      return GL_FALSE;
   }

   for (int i = 0; i < 3; i++) {
      /* The ARB_compute_variable_group_size spec says:
       *
       * "An INVALID_VALUE error is generated if any of num_groups_x,
       *  num_groups_y and num_groups_z are greater than or equal to the
       *  maximum work group count for the corresponding dimension."
       */
      if (num_groups[i] > ctx->Const.MaxComputeWorkGroupCount[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchComputeGroupSizeARB(num_groups_%c)", 'x' + i);
         return GL_FALSE;
      }

      /* The ARB_compute_variable_group_size spec says:
       *
       * "An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
       *  any of <group_size_x>, <group_size_y>, or <group_size_z> is less than
       *  or equal to zero or greater than the maximum local work group size
       *  for compute shaders with variable group size
       *  (MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB) in the corresponding
       *  dimension."
       *
       * However, the "less than" is a spec bug because they are declared as
       * unsigned integers.
       */
      if (group_size[i] == 0 ||
          group_size[i] > ctx->Const.MaxComputeVariableGroupSize[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchComputeGroupSizeARB(group_size_%c)", 'x' + i);
         return GL_FALSE;
      }
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
    *  the product of <group_size_x>, <group_size_y>, and <group_size_z> exceeds
    *  the implementation-dependent maximum local work group invocation count
    *  for compute shaders with variable group size
    *  (MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB)."
    */
   uint64_t total_invocations = group_size[0] * group_size[1];
   if (total_invocations <= UINT32_MAX) {
      /* Only bother multiplying the third value if total still fits in
       * 32-bit, since MaxComputeVariableGroupInvocations is also 32-bit.
       */
      total_invocations *= group_size[2];
   }
   if (total_invocations > ctx->Const.MaxComputeVariableGroupInvocations) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDispatchComputeGroupSizeARB(product of local_sizes "
                  "exceeds MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB "
                  "(%u * %u * %u > %u))",
                  group_size[0], group_size[1], group_size[2],
                  ctx->Const.MaxComputeVariableGroupInvocations);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static bool
valid_dispatch_indirect(struct gl_context *ctx,  GLintptr indirect)
{
   GLsizei size = 3 * sizeof(GLuint);
   const uint64_t end = (uint64_t) indirect + size;
   const char *name = "glDispatchComputeIndirect";

   if (!check_valid_to_compute(ctx, name))
      return GL_FALSE;

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_VALUE error is generated if indirect is negative or is not a
    *  multiple of four."
    */
   if (indirect & (sizeof(GLuint) - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(indirect is not aligned)", name);
      return GL_FALSE;
   }

   if (indirect < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(indirect is less than zero)", name);
      return GL_FALSE;
   }

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_OPERATION error is generated if no buffer is bound to the
    *  DRAW_INDIRECT_BUFFER binding, or if the command would source data
    *  beyond the end of the buffer object."
    */
   if (!_mesa_is_bufferobj(ctx->DispatchIndirectBuffer)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s: no buffer bound to DISPATCH_INDIRECT_BUFFER", name);
      return GL_FALSE;
   }

   if (_mesa_check_disallowed_mapping(ctx->DispatchIndirectBuffer)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(DISPATCH_INDIRECT_BUFFER is mapped)", name);
      return GL_FALSE;
   }

   if (ctx->DispatchIndirectBuffer->Size < end) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(DISPATCH_INDIRECT_BUFFER too small)", name);
      return GL_FALSE;
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated if the active program for the
    *  compute shader stage has a variable work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (prog->info.cs.local_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(variable work group size forbidden)", name);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static ALWAYS_INLINE void
dispatch_compute(GLuint num_groups_x, GLuint num_groups_y,
                 GLuint num_groups_z, bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint num_groups[3] = { num_groups_x, num_groups_y, num_groups_z };

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDispatchCompute(%d, %d, %d)\n",
                  num_groups_x, num_groups_y, num_groups_z);

   if (!no_error && !validate_DispatchCompute(ctx, num_groups))
      return;

   if (num_groups_x == 0u || num_groups_y == 0u || num_groups_z == 0u)
       return;

   ctx->Driver.DispatchCompute(ctx, num_groups);
}

void GLAPIENTRY
_mesa_DispatchCompute_no_error(GLuint num_groups_x, GLuint num_groups_y,
                               GLuint num_groups_z)
{
   dispatch_compute(num_groups_x, num_groups_y, num_groups_z, true);
}

void GLAPIENTRY
_mesa_DispatchCompute(GLuint num_groups_x,
                      GLuint num_groups_y,
                      GLuint num_groups_z)
{
   dispatch_compute(num_groups_x, num_groups_y, num_groups_z, false);
}

static ALWAYS_INLINE void
dispatch_compute_indirect(GLintptr indirect, bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDispatchComputeIndirect(%ld)\n", (long) indirect);

   if (!no_error && !valid_dispatch_indirect(ctx, indirect))
      return;

   ctx->Driver.DispatchComputeIndirect(ctx, indirect);
}

extern void GLAPIENTRY
_mesa_DispatchComputeIndirect_no_error(GLintptr indirect)
{
   dispatch_compute_indirect(indirect, true);
}

extern void GLAPIENTRY
_mesa_DispatchComputeIndirect(GLintptr indirect)
{
   dispatch_compute_indirect(indirect, false);
}

static ALWAYS_INLINE void
dispatch_compute_group_size(GLuint num_groups_x, GLuint num_groups_y,
                            GLuint num_groups_z, GLuint group_size_x,
                            GLuint group_size_y, GLuint group_size_z,
                            bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint num_groups[3] = { num_groups_x, num_groups_y, num_groups_z };
   const GLuint group_size[3] = { group_size_x, group_size_y, group_size_z };

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
                  "glDispatchComputeGroupSizeARB(%d, %d, %d, %d, %d, %d)\n",
                  num_groups_x, num_groups_y, num_groups_z,
                  group_size_x, group_size_y, group_size_z);

   if (!no_error &&
       !validate_DispatchComputeGroupSizeARB(ctx, num_groups, group_size))
      return;

   if (num_groups_x == 0u || num_groups_y == 0u || num_groups_z == 0u)
       return;

   ctx->Driver.DispatchComputeGroupSize(ctx, num_groups, group_size);
}

void GLAPIENTRY
_mesa_DispatchComputeGroupSizeARB_no_error(GLuint num_groups_x,
                                           GLuint num_groups_y,
                                           GLuint num_groups_z,
                                           GLuint group_size_x,
                                           GLuint group_size_y,
                                           GLuint group_size_z)
{
   dispatch_compute_group_size(num_groups_x, num_groups_y, num_groups_z,
                               group_size_x, group_size_y, group_size_z,
                               true);
}

void GLAPIENTRY
_mesa_DispatchComputeGroupSizeARB(GLuint num_groups_x, GLuint num_groups_y,
                                  GLuint num_groups_z, GLuint group_size_x,
                                  GLuint group_size_y, GLuint group_size_z)
{
   dispatch_compute_group_size(num_groups_x, num_groups_y, num_groups_z,
                               group_size_x, group_size_y, group_size_z,
                               false);
}
