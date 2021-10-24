/*
 * Copyright © 2016 Intel Corporation
 * Copyright © 2019 Google LLC
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

#include "vk_format.h"

/* Note that for packed formats, VK_FORMAT_ lists channels from high to low
 * bits occupied by the channel, while MESA_FORMAT_* and PIPE_FORMAT_* are
 * low-to-high.
 *
 * Also, missing entries are zero-filled, which happens to be
 * PIPE_FORMAT_NONE.
 */
static const enum pipe_format vk_format_map[] = {
   /* Missing R4G4 */
   [VK_FORMAT_R4G4B4A4_UNORM_PACK16] = PIPE_FORMAT_A4B4G4R4_UNORM,
   [VK_FORMAT_B4G4R4A4_UNORM_PACK16] = PIPE_FORMAT_A4R4G4B4_UNORM,
   [VK_FORMAT_R5G6B5_UNORM_PACK16] = PIPE_FORMAT_B5G6R5_UNORM,
   [VK_FORMAT_B5G6R5_UNORM_PACK16] = PIPE_FORMAT_R5G6B5_UNORM,
   [VK_FORMAT_R5G5B5A1_UNORM_PACK16] = PIPE_FORMAT_A1B5G5R5_UNORM,
   [VK_FORMAT_B5G5R5A1_UNORM_PACK16] = PIPE_FORMAT_A1R5G5B5_UNORM,
   [VK_FORMAT_A1R5G5B5_UNORM_PACK16] = PIPE_FORMAT_B5G5R5A1_UNORM,

   [VK_FORMAT_R8_UNORM] = PIPE_FORMAT_R8_UNORM,
   [VK_FORMAT_R8_SNORM] = PIPE_FORMAT_R8_SNORM,
   [VK_FORMAT_R8_USCALED] = PIPE_FORMAT_R8_USCALED,
   [VK_FORMAT_R8_SSCALED] = PIPE_FORMAT_R8_SSCALED,
   [VK_FORMAT_R8_UINT] = PIPE_FORMAT_R8_UINT,
   [VK_FORMAT_R8_SINT] = PIPE_FORMAT_R8_SINT,
   [VK_FORMAT_R8_SRGB] = PIPE_FORMAT_R8_SRGB,

   [VK_FORMAT_R8G8_UNORM] = PIPE_FORMAT_R8G8_UNORM,
   [VK_FORMAT_R8G8_SNORM] = PIPE_FORMAT_R8G8_SNORM,
   [VK_FORMAT_R8G8_USCALED] = PIPE_FORMAT_R8G8_USCALED,
   [VK_FORMAT_R8G8_SSCALED] = PIPE_FORMAT_R8G8_SSCALED,
   [VK_FORMAT_R8G8_UINT] = PIPE_FORMAT_R8G8_UINT,
   [VK_FORMAT_R8G8_SINT] = PIPE_FORMAT_R8G8_SINT,
   [VK_FORMAT_R8G8_SRGB] = PIPE_FORMAT_R8G8_SRGB,

   [VK_FORMAT_R8G8B8_UNORM] = PIPE_FORMAT_R8G8B8_UNORM,
   [VK_FORMAT_R8G8B8_SNORM] = PIPE_FORMAT_R8G8B8_SNORM,
   [VK_FORMAT_R8G8B8_USCALED] = PIPE_FORMAT_R8G8B8_USCALED,
   [VK_FORMAT_R8G8B8_SSCALED] = PIPE_FORMAT_R8G8B8_SSCALED,
   [VK_FORMAT_R8G8B8_UINT] = PIPE_FORMAT_R8G8B8_UINT,
   [VK_FORMAT_R8G8B8_SINT] = PIPE_FORMAT_R8G8B8_SINT,
   [VK_FORMAT_R8G8B8_SRGB] = PIPE_FORMAT_R8G8B8_SRGB,

   [VK_FORMAT_B8G8R8_UNORM] = PIPE_FORMAT_B8G8R8_UNORM,
   [VK_FORMAT_B8G8R8_SNORM] = PIPE_FORMAT_B8G8R8_SNORM,
   [VK_FORMAT_B8G8R8_USCALED] = PIPE_FORMAT_B8G8R8_USCALED,
   [VK_FORMAT_B8G8R8_SSCALED] = PIPE_FORMAT_B8G8R8_SSCALED,
   [VK_FORMAT_B8G8R8_UINT] = PIPE_FORMAT_B8G8R8_UINT,
   [VK_FORMAT_B8G8R8_SINT] = PIPE_FORMAT_B8G8R8_SINT,
   [VK_FORMAT_B8G8R8_SRGB] = PIPE_FORMAT_B8G8R8_SRGB,

   [VK_FORMAT_R8G8B8A8_UNORM] = PIPE_FORMAT_R8G8B8A8_UNORM,
   [VK_FORMAT_R8G8B8A8_SNORM] = PIPE_FORMAT_R8G8B8A8_SNORM,
   [VK_FORMAT_R8G8B8A8_USCALED] = PIPE_FORMAT_R8G8B8A8_USCALED,
   [VK_FORMAT_R8G8B8A8_SSCALED] = PIPE_FORMAT_R8G8B8A8_SSCALED,
   [VK_FORMAT_R8G8B8A8_UINT] = PIPE_FORMAT_R8G8B8A8_UINT,
   [VK_FORMAT_R8G8B8A8_SINT] = PIPE_FORMAT_R8G8B8A8_SINT,
   [VK_FORMAT_R8G8B8A8_SRGB] = PIPE_FORMAT_R8G8B8A8_SRGB,

   [VK_FORMAT_B8G8R8A8_UNORM] = PIPE_FORMAT_B8G8R8A8_UNORM,
   [VK_FORMAT_B8G8R8A8_SNORM] = PIPE_FORMAT_B8G8R8A8_SNORM,
   [VK_FORMAT_B8G8R8A8_USCALED] = PIPE_FORMAT_B8G8R8A8_USCALED,
   [VK_FORMAT_B8G8R8A8_SSCALED] = PIPE_FORMAT_B8G8R8A8_SSCALED,
   [VK_FORMAT_B8G8R8A8_UINT] = PIPE_FORMAT_B8G8R8A8_UINT,
   [VK_FORMAT_B8G8R8A8_SINT] = PIPE_FORMAT_B8G8R8A8_SINT,
   [VK_FORMAT_B8G8R8A8_SRGB] = PIPE_FORMAT_B8G8R8A8_SRGB,

   [VK_FORMAT_A8B8G8R8_UNORM_PACK32] = PIPE_FORMAT_RGBA8888_UNORM,
   [VK_FORMAT_A8B8G8R8_SNORM_PACK32] = PIPE_FORMAT_RGBA8888_SNORM,
   [VK_FORMAT_A8B8G8R8_USCALED_PACK32] = PIPE_FORMAT_RGBA8888_USCALED,
   [VK_FORMAT_A8B8G8R8_SSCALED_PACK32] = PIPE_FORMAT_RGBA8888_SSCALED,
   [VK_FORMAT_A8B8G8R8_UINT_PACK32] = PIPE_FORMAT_RGBA8888_UINT,
   [VK_FORMAT_A8B8G8R8_SINT_PACK32] = PIPE_FORMAT_RGBA8888_SINT,
   [VK_FORMAT_A8B8G8R8_SRGB_PACK32] = PIPE_FORMAT_RGBA8888_SRGB,

   [VK_FORMAT_A2R10G10B10_UNORM_PACK32] = PIPE_FORMAT_B10G10R10A2_UNORM,
   [VK_FORMAT_A2R10G10B10_SNORM_PACK32] = PIPE_FORMAT_B10G10R10A2_SNORM,
   [VK_FORMAT_A2R10G10B10_USCALED_PACK32] = PIPE_FORMAT_B10G10R10A2_USCALED,
   [VK_FORMAT_A2R10G10B10_SSCALED_PACK32] = PIPE_FORMAT_B10G10R10A2_SSCALED,
   [VK_FORMAT_A2R10G10B10_UINT_PACK32] = PIPE_FORMAT_B10G10R10A2_UINT,
   [VK_FORMAT_A2R10G10B10_SINT_PACK32] = PIPE_FORMAT_B10G10R10A2_SINT,

   [VK_FORMAT_A2B10G10R10_UNORM_PACK32] = PIPE_FORMAT_R10G10B10A2_UNORM,
   [VK_FORMAT_A2B10G10R10_SNORM_PACK32] = PIPE_FORMAT_R10G10B10A2_SNORM,
   [VK_FORMAT_A2B10G10R10_USCALED_PACK32] = PIPE_FORMAT_R10G10B10A2_USCALED,
   [VK_FORMAT_A2B10G10R10_SSCALED_PACK32] = PIPE_FORMAT_R10G10B10A2_SSCALED,
   [VK_FORMAT_A2B10G10R10_UINT_PACK32] = PIPE_FORMAT_R10G10B10A2_UINT,
   [VK_FORMAT_A2B10G10R10_SINT_PACK32] = PIPE_FORMAT_R10G10B10A2_SINT,

   [VK_FORMAT_R16_UNORM] = PIPE_FORMAT_R16_UNORM,
   [VK_FORMAT_R16_SNORM] = PIPE_FORMAT_R16_SNORM,
   [VK_FORMAT_R16_USCALED] = PIPE_FORMAT_R16_USCALED,
   [VK_FORMAT_R16_SSCALED] = PIPE_FORMAT_R16_SSCALED,
   [VK_FORMAT_R16_UINT] = PIPE_FORMAT_R16_UINT,
   [VK_FORMAT_R16_SINT] = PIPE_FORMAT_R16_SINT,
   [VK_FORMAT_R16_SFLOAT] = PIPE_FORMAT_R16_FLOAT,

   [VK_FORMAT_R16G16_UNORM] = PIPE_FORMAT_R16G16_UNORM,
   [VK_FORMAT_R16G16_SNORM] = PIPE_FORMAT_R16G16_SNORM,
   [VK_FORMAT_R16G16_USCALED] = PIPE_FORMAT_R16G16_USCALED,
   [VK_FORMAT_R16G16_SSCALED] = PIPE_FORMAT_R16G16_SSCALED,
   [VK_FORMAT_R16G16_UINT] = PIPE_FORMAT_R16G16_UINT,
   [VK_FORMAT_R16G16_SINT] = PIPE_FORMAT_R16G16_SINT,
   [VK_FORMAT_R16G16_SFLOAT] = PIPE_FORMAT_R16G16_FLOAT,

   [VK_FORMAT_R16G16B16_UNORM] = PIPE_FORMAT_R16G16B16_UNORM,
   [VK_FORMAT_R16G16B16_SNORM] = PIPE_FORMAT_R16G16B16_SNORM,
   [VK_FORMAT_R16G16B16_USCALED] = PIPE_FORMAT_R16G16B16_USCALED,
   [VK_FORMAT_R16G16B16_SSCALED] = PIPE_FORMAT_R16G16B16_SSCALED,
   [VK_FORMAT_R16G16B16_UINT] = PIPE_FORMAT_R16G16B16_UINT,
   [VK_FORMAT_R16G16B16_SINT] = PIPE_FORMAT_R16G16B16_SINT,
   [VK_FORMAT_R16G16B16_SFLOAT] = PIPE_FORMAT_R16G16B16_FLOAT,

   [VK_FORMAT_R16G16B16A16_UNORM] = PIPE_FORMAT_R16G16B16A16_UNORM,
   [VK_FORMAT_R16G16B16A16_SNORM] = PIPE_FORMAT_R16G16B16A16_SNORM,
   [VK_FORMAT_R16G16B16A16_USCALED] = PIPE_FORMAT_R16G16B16A16_USCALED,
   [VK_FORMAT_R16G16B16A16_SSCALED] = PIPE_FORMAT_R16G16B16A16_SSCALED,
   [VK_FORMAT_R16G16B16A16_UINT] = PIPE_FORMAT_R16G16B16A16_UINT,
   [VK_FORMAT_R16G16B16A16_SINT] = PIPE_FORMAT_R16G16B16A16_SINT,
   [VK_FORMAT_R16G16B16A16_SFLOAT] = PIPE_FORMAT_R16G16B16A16_FLOAT,

   [VK_FORMAT_R32_UINT] = PIPE_FORMAT_R32_UINT,
   [VK_FORMAT_R32_SINT] = PIPE_FORMAT_R32_SINT,
   [VK_FORMAT_R32_SFLOAT] = PIPE_FORMAT_R32_FLOAT,

   [VK_FORMAT_R32G32_UINT] = PIPE_FORMAT_R32G32_UINT,
   [VK_FORMAT_R32G32_SINT] = PIPE_FORMAT_R32G32_SINT,
   [VK_FORMAT_R32G32_SFLOAT] = PIPE_FORMAT_R32G32_FLOAT,

   [VK_FORMAT_R32G32B32_UINT] = PIPE_FORMAT_R32G32B32_UINT,
   [VK_FORMAT_R32G32B32_SINT] = PIPE_FORMAT_R32G32B32_SINT,
   [VK_FORMAT_R32G32B32_SFLOAT] = PIPE_FORMAT_R32G32B32_FLOAT,

   [VK_FORMAT_R32G32B32A32_UINT] = PIPE_FORMAT_R32G32B32A32_UINT,
   [VK_FORMAT_R32G32B32A32_SINT] = PIPE_FORMAT_R32G32B32A32_SINT,
   [VK_FORMAT_R32G32B32A32_SFLOAT] = PIPE_FORMAT_R32G32B32A32_FLOAT,

   [VK_FORMAT_R64_UINT] = PIPE_FORMAT_R64_UINT,
   [VK_FORMAT_R64_SINT] = PIPE_FORMAT_R64_SINT,
   /* Missing rest of 64-bit uint/sint formats */
   [VK_FORMAT_R64_SFLOAT] = PIPE_FORMAT_R64_FLOAT,
   [VK_FORMAT_R64G64_SFLOAT] = PIPE_FORMAT_R64G64_FLOAT,
   [VK_FORMAT_R64G64B64_SFLOAT] = PIPE_FORMAT_R64G64B64_FLOAT,
   [VK_FORMAT_R64G64B64A64_SFLOAT] = PIPE_FORMAT_R64G64B64A64_FLOAT,

   [VK_FORMAT_B10G11R11_UFLOAT_PACK32] = PIPE_FORMAT_R11G11B10_FLOAT,
   [VK_FORMAT_E5B9G9R9_UFLOAT_PACK32] = PIPE_FORMAT_R9G9B9E5_FLOAT,

   [VK_FORMAT_D16_UNORM] = PIPE_FORMAT_Z16_UNORM,
   [VK_FORMAT_X8_D24_UNORM_PACK32] = PIPE_FORMAT_Z24X8_UNORM,
   [VK_FORMAT_D32_SFLOAT] = PIPE_FORMAT_Z32_FLOAT,
   [VK_FORMAT_S8_UINT] = PIPE_FORMAT_S8_UINT,
   [VK_FORMAT_D16_UNORM_S8_UINT] = PIPE_FORMAT_Z16_UNORM_S8_UINT,
   [VK_FORMAT_D24_UNORM_S8_UINT] = PIPE_FORMAT_Z24_UNORM_S8_UINT,
   [VK_FORMAT_D32_SFLOAT_S8_UINT] = PIPE_FORMAT_Z32_FLOAT_S8X24_UINT,

   [VK_FORMAT_BC1_RGB_UNORM_BLOCK] = PIPE_FORMAT_DXT1_RGB,
   [VK_FORMAT_BC1_RGB_SRGB_BLOCK] = PIPE_FORMAT_DXT1_SRGB,
   [VK_FORMAT_BC1_RGBA_UNORM_BLOCK] = PIPE_FORMAT_DXT1_RGBA,
   [VK_FORMAT_BC1_RGBA_SRGB_BLOCK] = PIPE_FORMAT_DXT1_SRGBA,
   [VK_FORMAT_BC2_UNORM_BLOCK] = PIPE_FORMAT_DXT3_RGBA,
   [VK_FORMAT_BC2_SRGB_BLOCK] = PIPE_FORMAT_DXT3_SRGBA,
   [VK_FORMAT_BC3_UNORM_BLOCK] = PIPE_FORMAT_DXT5_RGBA,
   [VK_FORMAT_BC3_SRGB_BLOCK] = PIPE_FORMAT_DXT5_SRGBA,
   [VK_FORMAT_BC4_UNORM_BLOCK] = PIPE_FORMAT_RGTC1_UNORM,
   [VK_FORMAT_BC4_SNORM_BLOCK] = PIPE_FORMAT_RGTC1_SNORM,
   [VK_FORMAT_BC5_UNORM_BLOCK] = PIPE_FORMAT_RGTC2_UNORM,
   [VK_FORMAT_BC5_SNORM_BLOCK] = PIPE_FORMAT_RGTC2_SNORM,
   [VK_FORMAT_BC6H_UFLOAT_BLOCK] = PIPE_FORMAT_BPTC_RGB_UFLOAT,
   [VK_FORMAT_BC6H_SFLOAT_BLOCK] = PIPE_FORMAT_BPTC_RGB_FLOAT,
   [VK_FORMAT_BC7_UNORM_BLOCK] = PIPE_FORMAT_BPTC_RGBA_UNORM,
   [VK_FORMAT_BC7_SRGB_BLOCK] = PIPE_FORMAT_BPTC_SRGBA,

   [VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK] = PIPE_FORMAT_ETC2_RGB8,
   [VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK] = PIPE_FORMAT_ETC2_SRGB8,
   [VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK] = PIPE_FORMAT_ETC2_RGB8A1,
   [VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK] = PIPE_FORMAT_ETC2_SRGB8A1,
   [VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK] = PIPE_FORMAT_ETC2_RGBA8,
   [VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK] = PIPE_FORMAT_ETC2_SRGBA8,
   [VK_FORMAT_EAC_R11_UNORM_BLOCK] = PIPE_FORMAT_ETC2_R11_UNORM,
   [VK_FORMAT_EAC_R11_SNORM_BLOCK] = PIPE_FORMAT_ETC2_R11_SNORM,
   [VK_FORMAT_EAC_R11G11_UNORM_BLOCK] = PIPE_FORMAT_ETC2_RG11_UNORM,
   [VK_FORMAT_EAC_R11G11_SNORM_BLOCK] = PIPE_FORMAT_ETC2_RG11_SNORM,

   [VK_FORMAT_ASTC_4x4_UNORM_BLOCK] = PIPE_FORMAT_ASTC_4x4,
   [VK_FORMAT_ASTC_4x4_SRGB_BLOCK] = PIPE_FORMAT_ASTC_4x4_SRGB,
   [VK_FORMAT_ASTC_5x4_UNORM_BLOCK] = PIPE_FORMAT_ASTC_5x4,
   [VK_FORMAT_ASTC_5x4_SRGB_BLOCK] = PIPE_FORMAT_ASTC_5x4_SRGB,
   [VK_FORMAT_ASTC_5x5_UNORM_BLOCK] = PIPE_FORMAT_ASTC_5x5,
   [VK_FORMAT_ASTC_5x5_SRGB_BLOCK] = PIPE_FORMAT_ASTC_5x5_SRGB,
   [VK_FORMAT_ASTC_6x5_UNORM_BLOCK] = PIPE_FORMAT_ASTC_6x5,
   [VK_FORMAT_ASTC_6x5_SRGB_BLOCK] = PIPE_FORMAT_ASTC_6x5_SRGB,
   [VK_FORMAT_ASTC_6x6_UNORM_BLOCK] = PIPE_FORMAT_ASTC_6x6,
   [VK_FORMAT_ASTC_6x6_SRGB_BLOCK] = PIPE_FORMAT_ASTC_6x6_SRGB,
   [VK_FORMAT_ASTC_8x5_UNORM_BLOCK] = PIPE_FORMAT_ASTC_8x5,
   [VK_FORMAT_ASTC_8x5_SRGB_BLOCK] = PIPE_FORMAT_ASTC_8x5_SRGB,
   [VK_FORMAT_ASTC_8x6_UNORM_BLOCK] = PIPE_FORMAT_ASTC_8x6,
   [VK_FORMAT_ASTC_8x6_SRGB_BLOCK] = PIPE_FORMAT_ASTC_8x6_SRGB,
   [VK_FORMAT_ASTC_8x8_UNORM_BLOCK] = PIPE_FORMAT_ASTC_8x8,
   [VK_FORMAT_ASTC_8x8_SRGB_BLOCK] = PIPE_FORMAT_ASTC_8x8_SRGB,
   [VK_FORMAT_ASTC_10x5_UNORM_BLOCK] = PIPE_FORMAT_ASTC_10x5,
   [VK_FORMAT_ASTC_10x5_SRGB_BLOCK] = PIPE_FORMAT_ASTC_10x5_SRGB,
   [VK_FORMAT_ASTC_10x6_UNORM_BLOCK] = PIPE_FORMAT_ASTC_10x6,
   [VK_FORMAT_ASTC_10x6_SRGB_BLOCK] = PIPE_FORMAT_ASTC_10x6_SRGB,
   [VK_FORMAT_ASTC_10x8_UNORM_BLOCK] = PIPE_FORMAT_ASTC_10x8,
   [VK_FORMAT_ASTC_10x8_SRGB_BLOCK] = PIPE_FORMAT_ASTC_10x8_SRGB,
   [VK_FORMAT_ASTC_10x10_UNORM_BLOCK] = PIPE_FORMAT_ASTC_10x10,
   [VK_FORMAT_ASTC_10x10_SRGB_BLOCK] = PIPE_FORMAT_ASTC_10x10_SRGB,
   [VK_FORMAT_ASTC_12x10_UNORM_BLOCK] = PIPE_FORMAT_ASTC_12x10,
   [VK_FORMAT_ASTC_12x10_SRGB_BLOCK] = PIPE_FORMAT_ASTC_12x10_SRGB,
   [VK_FORMAT_ASTC_12x12_UNORM_BLOCK] = PIPE_FORMAT_ASTC_12x12,
   [VK_FORMAT_ASTC_12x12_SRGB_BLOCK] = PIPE_FORMAT_ASTC_12x12_SRGB,

   /* Missing planes */

   /* Missing PVRTC */

   /* Missing ASTC SFLOAT */

   /* Missing more planes */
};

enum pipe_format
vk_format_to_pipe_format(enum VkFormat vkformat)
{
   if (vkformat >= ARRAY_SIZE(vk_format_map)) {
      switch (vkformat) {
      case VK_FORMAT_G8B8G8R8_422_UNORM:
         return PIPE_FORMAT_YUYV;
      case VK_FORMAT_B8G8R8G8_422_UNORM:
         return PIPE_FORMAT_UYVY;
      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
         return PIPE_FORMAT_IYUV;
      case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
         return PIPE_FORMAT_NV12;
      case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
         return PIPE_FORMAT_Y8_U8_V8_422_UNORM;
      case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
         return PIPE_FORMAT_Y8_U8V8_422_UNORM;
      case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
         return PIPE_FORMAT_Y8_U8_V8_444_UNORM;
      case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
         return PIPE_FORMAT_Y16_U16_V16_420_UNORM;
      case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
         return PIPE_FORMAT_P016;
      case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
         return PIPE_FORMAT_Y16_U16_V16_422_UNORM;
      case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
         return PIPE_FORMAT_Y16_U16V16_422_UNORM;
      case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
         return PIPE_FORMAT_Y16_U16_V16_444_UNORM;
      case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT:
         return PIPE_FORMAT_B4G4R4A4_UNORM;
      case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:
         return PIPE_FORMAT_R4G4B4A4_UNORM;
      default:
         return PIPE_FORMAT_NONE;
      }
   }

   /* Unpopulated entries in the table must be PIPE_FORMAT_NONE */
   STATIC_ASSERT(PIPE_FORMAT_NONE == 0);

   return vk_format_map[vkformat];
}

VkImageAspectFlags
vk_format_aspects(VkFormat format)
{
   switch (format) {
   case VK_FORMAT_UNDEFINED:
      return 0;

   case VK_FORMAT_S8_UINT:
      return VK_IMAGE_ASPECT_STENCIL_BIT;

   case VK_FORMAT_D16_UNORM_S8_UINT:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D32_SFLOAT:
      return VK_IMAGE_ASPECT_DEPTH_BIT;

   case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
   case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
   case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
   case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
   case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
   case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
      return (VK_IMAGE_ASPECT_PLANE_0_BIT |
              VK_IMAGE_ASPECT_PLANE_1_BIT |
              VK_IMAGE_ASPECT_PLANE_2_BIT);

   case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
   case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
   case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
   case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
      return (VK_IMAGE_ASPECT_PLANE_0_BIT |
              VK_IMAGE_ASPECT_PLANE_1_BIT);

   default:
      return VK_IMAGE_ASPECT_COLOR_BIT;
   }
}

void
vk_component_mapping_to_pipe_swizzle(VkComponentMapping mapping,
                                     unsigned char out_swizzle[4])
{
   VkComponentSwizzle swizzle[4] = { mapping.r, mapping.g, mapping.b, mapping.a };
   for (unsigned i = 0; i < 4; i++) {
      switch (swizzle[i]) {
      case VK_COMPONENT_SWIZZLE_R:
         out_swizzle[i] = PIPE_SWIZZLE_X;
         break;
      case VK_COMPONENT_SWIZZLE_G:
         out_swizzle[i] = PIPE_SWIZZLE_Y;
         break;
      case VK_COMPONENT_SWIZZLE_B:
         out_swizzle[i] = PIPE_SWIZZLE_Z;
         break;
      case VK_COMPONENT_SWIZZLE_A:
         out_swizzle[i] = PIPE_SWIZZLE_W;
         break;
      case VK_COMPONENT_SWIZZLE_IDENTITY:
         out_swizzle[i] = PIPE_SWIZZLE_X + i;
         break;
      case VK_COMPONENT_SWIZZLE_ZERO:
         out_swizzle[i] = PIPE_SWIZZLE_0;
         break;
      case VK_COMPONENT_SWIZZLE_ONE:
         out_swizzle[i] = PIPE_SWIZZLE_1;
         break;
      default:
         unreachable("unknown swizzle");
      }
   }
}
