/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef __WINE_D3D10SHADER_H
#define __WINE_D3D10SHADER_H

#include "d3d10.h"

#define D3D10_SHADER_DEBUG                          0x0001
#define D3D10_SHADER_SKIP_VALIDATION                0x0002
#define D3D10_SHADER_SKIP_OPTIMIZATION              0x0004
#define D3D10_SHADER_PACK_MATRIX_ROW_MAJOR          0x0008
#define D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR       0x0010
#define D3D10_SHADER_PARTIAL_PRECISION              0x0020
#define D3D10_SHADER_FORCE_VS_SOFTWARE_NO_OPT       0x0040
#define D3D10_SHADER_FORCE_PS_SOFTWARE_NO_OPT       0x0080
#define D3D10_SHADER_NO_PRESHADER                   0x0100
#define D3D10_SHADER_AVOID_FLOW_CONTROL             0x0200
#define D3D10_SHADER_PREFER_FLOW_CONTROL            0x0300
#define D3D10_SHADER_ENABLE_STRICTNESS              0x0400
#define D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY 0x0800
#define D3D10_SHADER_IEEE_STRICTNESS                0x1000

typedef enum _D3D10_SHADER_VARIABLE_CLASS
{
    D3D10_SVC_SCALAR,
    D3D10_SVC_VECTOR,
    D3D10_SVC_MATRIX_ROWS,
    D3D10_SVC_MATRIX_COLUMNS,
    D3D10_SVC_OBJECT,
    D3D10_SVC_STRUCT,
    D3D10_SVC_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_CLASS, *LPD3D10_SHADER_VARIABLE_CLASS;

typedef enum _D3D10_SHADER_VARIABLE_TYPE
{
    D3D10_SVT_VOID = 0,
    D3D10_SVT_BOOL = 1,
    D3D10_SVT_INT = 2,
    D3D10_SVT_FLOAT = 3,
    D3D10_SVT_STRING = 4,
    D3D10_SVT_TEXTURE = 5,
    D3D10_SVT_TEXTURE1D = 6,
    D3D10_SVT_TEXTURE2D = 7,
    D3D10_SVT_TEXTURE3D = 8,
    D3D10_SVT_TEXTURECUBE = 9,
    D3D10_SVT_SAMPLER = 10,
    D3D10_SVT_PIXELSHADER = 15,
    D3D10_SVT_VERTEXSHADER = 16,
    D3D10_SVT_UINT = 19,
    D3D10_SVT_UINT8 = 20,
    D3D10_SVT_GEOMETRYSHADER = 21,
    D3D10_SVT_RASTERIZER = 22,
    D3D10_SVT_DEPTHSTENCIL = 23,
    D3D10_SVT_BLEND = 24,
    D3D10_SVT_BUFFER = 25,
    D3D10_SVT_CBUFFER = 26,
    D3D10_SVT_TBUFFER = 27,
    D3D10_SVT_TEXTURE1DARRAY = 28,
    D3D10_SVT_TEXTURE2DARRAY = 29,
    D3D10_SVT_RENDERTARGETVIEW = 30,
    D3D10_SVT_DEPTHSTENCILVIEW = 31,
    D3D10_SVT_TEXTURE2DMS = 32,
    D3D10_SVT_TEXTURE2DMSARRAY = 33,
    D3D10_SVT_TEXTURECUBEARRAY = 34,
    D3D10_SVT_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_TYPE, *LPD3D10_SHADER_VARIABLE_TYPE;

typedef enum D3D10_CBUFFER_TYPE
{
    D3D10_CT_CBUFFER = 0,
    D3D10_CT_TBUFFER = 1
} D3D10_CBUFFER_TYPE, *LPD3D10_CBUFFER_TYPE;

typedef enum D3D10_NAME
{
    D3D10_NAME_UNDEFINED = 0,
    D3D10_NAME_POSITION = 1,
    D3D10_NAME_CLIP_DISTANCE = 2,
    D3D10_NAME_CULL_DISTANCE = 3,
    D3D10_NAME_RENDER_TARGET_ARRAY_INDEX = 4,
    D3D10_NAME_VIEWPORT_ARRAY_INDEX = 5,
    D3D10_NAME_VERTEX_ID = 6,
    D3D10_NAME_PRIMITIVE_ID = 7,
    D3D10_NAME_INSTANCE_ID = 8,
    D3D10_NAME_IS_FRONT_FACE = 9,
    D3D10_NAME_SAMPLE_INDEX = 10,
    D3D10_NAME_TARGET = 64,
    D3D10_NAME_DEPTH = 65,
} D3D10_NAME;

typedef enum D3D10_REGISTER_COMPONENT_TYPE
{
    D3D10_REGISTER_COMPONENT_UNKNOWN = 0,
    D3D10_REGISTER_COMPONENT_UINT32 = 1,
    D3D10_REGISTER_COMPONENT_SINT32 = 2,
    D3D10_REGISTER_COMPONENT_FLOAT32 = 3,
} D3D10_REGISTER_COMPONENT_TYPE;

typedef struct _D3D10_SHADER_MACRO
{
    LPCSTR Name;
    LPCSTR Definition;
} D3D10_SHADER_MACRO, *LPD3D10_SHADER_MACRO;

typedef struct _D3D10_SIGNATURE_PARAMETER_DESC
{
    LPCSTR SemanticName;
    UINT SemanticIndex;
    UINT Register;
    D3D10_NAME SystemValueType;
    D3D10_REGISTER_COMPONENT_TYPE ComponentType;
    BYTE Mask;
    BYTE ReadWriteMask;
} D3D10_SIGNATURE_PARAMETER_DESC;

LPCSTR WINAPI D3D10GetVertexShaderProfile(ID3D10Device *device);
LPCSTR WINAPI D3D10GetGeometryShaderProfile(ID3D10Device *device);
LPCSTR WINAPI D3D10GetPixelShaderProfile(ID3D10Device *device);

#endif /* __WINE_D3D10SHADER_H */
