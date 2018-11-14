#ifndef G80_DEFS_XML
#define G80_DEFS_XML

/* Autogenerated file, DO NOT EDIT manually!

This file was generated by the rules-ng-ng headergen tool in this git repository:
http://github.com/envytools/envytools/
git clone https://github.com/envytools/envytools.git

The rules-ng-ng source files this header was generated from are:
- /home/skeggsb/git/envytools/rnndb/../rnndb/graph/g80_texture.xml (  18837 bytes, from 2016-01-14 23:54:22)
- /home/skeggsb/git/envytools/rnndb/copyright.xml                  (   6456 bytes, from 2015-09-10 02:57:40)
- /home/skeggsb/git/envytools/rnndb/nvchipsets.xml                 (   2908 bytes, from 2016-02-02 23:45:00)
- /home/skeggsb/git/envytools/rnndb/g80_defs.xml                   (  21739 bytes, from 2016-02-04 00:29:42)
- /home/skeggsb/git/envytools/rnndb/nv_defs.xml                    (   5388 bytes, from 2016-01-14 23:54:22)

Copyright (C) 2006-2016 by the following authors:
- Artur Huillet <arthur.huillet@free.fr> (ahuillet)
- Ben Skeggs (darktama, darktama_)
- B. R. <koala_br@users.sourceforge.net> (koala_br)
- Carlos Martin <carlosmn@users.sf.net> (carlosmn)
- Christoph Bumiller <e0425955@student.tuwien.ac.at> (calim, chrisbmr)
- Dawid Gajownik <gajownik@users.sf.net> (gajownik)
- Dmitry Baryshkov
- Dmitry Eremin-Solenikov <lumag@users.sf.net> (lumag)
- EdB <edb_@users.sf.net> (edb_)
- Erik Waling <erikwailing@users.sf.net> (erikwaling)
- Francisco Jerez <currojerez@riseup.net> (curro)
- Ilia Mirkin <imirkin@alum.mit.edu> (imirkin)
- jb17bsome <jb17bsome@bellsouth.net> (jb17bsome)
- Jeremy Kolb <kjeremy@users.sf.net> (kjeremy)
- Laurent Carlier <lordheavym@gmail.com> (lordheavy)
- Luca Barbieri <luca@luca-barbieri.com> (lb, lb1)
- Maarten Maathuis <madman2003@gmail.com> (stillunknown)
- Marcin Kościelnicki <koriakin@0x04.net> (mwk, koriakin)
- Mark Carey <mark.carey@gmail.com> (careym)
- Matthieu Castet <matthieu.castet@parrot.com> (mat-c)
- nvidiaman <nvidiaman@users.sf.net> (nvidiaman)
- Patrice Mandin <patmandin@gmail.com> (pmandin, pmdata)
- Pekka Paalanen <pq@iki.fi> (pq, ppaalanen)
- Peter Popov <ironpeter@users.sf.net> (ironpeter)
- Richard Hughes <hughsient@users.sf.net> (hughsient)
- Rudi Cilibrasi <cilibrar@users.sf.net> (cilibrar)
- Serge Martin
- Simon Raffeiner
- Stephane Loeuillet <leroutier@users.sf.net> (leroutier)
- Stephane Marchesin <stephane.marchesin@gmail.com> (marcheu)
- sturmflut <sturmflut@users.sf.net> (sturmflut)
- Sylvain Munaut <tnt@246tNt.com>
- Victor Stinner <victor.stinner@haypocalc.com> (haypo)
- Wladmir van der Laan <laanwj@gmail.com> (miathan6)
- Younes Manton <younes.m@gmail.com> (ymanton)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#define G80_VSTATUS_IDLE					0x00000000
#define G80_VSTATUS_BUSY					0x00000001
#define G80_VSTATUS_UNK2					0x00000002
#define G80_VSTATUS_WAITING					0x00000003
#define G80_VSTATUS_BLOCKED					0x00000005
#define G80_VSTATUS_FAULTED					0x00000006
#define G80_VSTATUS_PAUSED					0x00000007
#define G80_TIC_SOURCE_ZERO					0x00000000
#define G80_TIC_SOURCE_R					0x00000002
#define G80_TIC_SOURCE_G					0x00000003
#define G80_TIC_SOURCE_B					0x00000004
#define G80_TIC_SOURCE_A					0x00000005
#define G80_TIC_SOURCE_ONE_INT					0x00000006
#define G80_TIC_SOURCE_ONE_FLOAT				0x00000007
#define G80_TIC_TYPE_SNORM					0x00000001
#define G80_TIC_TYPE_UNORM					0x00000002
#define G80_TIC_TYPE_SINT					0x00000003
#define G80_TIC_TYPE_UINT					0x00000004
#define G80_TIC_TYPE_SNORM_FORCE_FP16				0x00000005
#define G80_TIC_TYPE_UNORM_FORCE_FP16				0x00000006
#define G80_TIC_TYPE_FLOAT					0x00000007
#define G80_SURFACE_FORMAT_BITMAP				0x0000001c
#define G80_SURFACE_FORMAT_UNK1D				0x0000001d
#define G80_SURFACE_FORMAT_RGBA32_FLOAT				0x000000c0
#define G80_SURFACE_FORMAT_RGBA32_SINT				0x000000c1
#define G80_SURFACE_FORMAT_RGBA32_UINT				0x000000c2
#define G80_SURFACE_FORMAT_RGBX32_FLOAT				0x000000c3
#define G80_SURFACE_FORMAT_RGBX32_SINT				0x000000c4
#define G80_SURFACE_FORMAT_RGBX32_UINT				0x000000c5
#define G80_SURFACE_FORMAT_RGBA16_UNORM				0x000000c6
#define G80_SURFACE_FORMAT_RGBA16_SNORM				0x000000c7
#define G80_SURFACE_FORMAT_RGBA16_SINT				0x000000c8
#define G80_SURFACE_FORMAT_RGBA16_UINT				0x000000c9
#define G80_SURFACE_FORMAT_RGBA16_FLOAT				0x000000ca
#define G80_SURFACE_FORMAT_RG32_FLOAT				0x000000cb
#define G80_SURFACE_FORMAT_RG32_SINT				0x000000cc
#define G80_SURFACE_FORMAT_RG32_UINT				0x000000cd
#define G80_SURFACE_FORMAT_RGBX16_FLOAT				0x000000ce
#define G80_SURFACE_FORMAT_BGRA8_UNORM				0x000000cf
#define G80_SURFACE_FORMAT_BGRA8_SRGB				0x000000d0
#define G80_SURFACE_FORMAT_RGB10_A2_UNORM			0x000000d1
#define G80_SURFACE_FORMAT_RGB10_A2_UINT			0x000000d2
#define G80_SURFACE_FORMAT_RGBA8_UNORM				0x000000d5
#define G80_SURFACE_FORMAT_RGBA8_SRGB				0x000000d6
#define G80_SURFACE_FORMAT_RGBA8_SNORM				0x000000d7
#define G80_SURFACE_FORMAT_RGBA8_SINT				0x000000d8
#define G80_SURFACE_FORMAT_RGBA8_UINT				0x000000d9
#define G80_SURFACE_FORMAT_RG16_UNORM				0x000000da
#define G80_SURFACE_FORMAT_RG16_SNORM				0x000000db
#define G80_SURFACE_FORMAT_RG16_SINT				0x000000dc
#define G80_SURFACE_FORMAT_RG16_UINT				0x000000dd
#define G80_SURFACE_FORMAT_RG16_FLOAT				0x000000de
#define G80_SURFACE_FORMAT_BGR10_A2_UNORM			0x000000df
#define G80_SURFACE_FORMAT_R11G11B10_FLOAT			0x000000e0
#define G80_SURFACE_FORMAT_R32_SINT				0x000000e3
#define G80_SURFACE_FORMAT_R32_UINT				0x000000e4
#define G80_SURFACE_FORMAT_R32_FLOAT				0x000000e5
#define G80_SURFACE_FORMAT_BGRX8_UNORM				0x000000e6
#define G80_SURFACE_FORMAT_BGRX8_SRGB				0x000000e7
#define G80_SURFACE_FORMAT_B5G6R5_UNORM				0x000000e8
#define G80_SURFACE_FORMAT_BGR5_A1_UNORM			0x000000e9
#define G80_SURFACE_FORMAT_RG8_UNORM				0x000000ea
#define G80_SURFACE_FORMAT_RG8_SNORM				0x000000eb
#define G80_SURFACE_FORMAT_RG8_SINT				0x000000ec
#define G80_SURFACE_FORMAT_RG8_UINT				0x000000ed
#define G80_SURFACE_FORMAT_R16_UNORM				0x000000ee
#define G80_SURFACE_FORMAT_R16_SNORM				0x000000ef
#define G80_SURFACE_FORMAT_R16_SINT				0x000000f0
#define G80_SURFACE_FORMAT_R16_UINT				0x000000f1
#define G80_SURFACE_FORMAT_R16_FLOAT				0x000000f2
#define G80_SURFACE_FORMAT_R8_UNORM				0x000000f3
#define G80_SURFACE_FORMAT_R8_SNORM				0x000000f4
#define G80_SURFACE_FORMAT_R8_SINT				0x000000f5
#define G80_SURFACE_FORMAT_R8_UINT				0x000000f6
#define G80_SURFACE_FORMAT_A8_UNORM				0x000000f7
#define G80_SURFACE_FORMAT_BGR5_X1_UNORM			0x000000f8
#define G80_SURFACE_FORMAT_RGBX8_UNORM				0x000000f9
#define G80_SURFACE_FORMAT_RGBX8_SRGB				0x000000fa
#define G80_SURFACE_FORMAT_BGR5_X1_UNORM_UNKFB			0x000000fb
#define G80_SURFACE_FORMAT_BGR5_X1_UNORM_UNKFC			0x000000fc
#define G80_SURFACE_FORMAT_BGRX8_UNORM_UNKFD			0x000000fd
#define G80_SURFACE_FORMAT_BGRX8_UNORM_UNKFE			0x000000fe
#define G80_SURFACE_FORMAT_Y32_UINT_UNKFF			0x000000ff
#define G80_ZETA_FORMAT_Z32_FLOAT				0x0000000a
#define G80_ZETA_FORMAT_Z16_UNORM				0x00000013
#define G80_ZETA_FORMAT_S8_Z24_UNORM				0x00000014
#define G80_ZETA_FORMAT_Z24_X8_UNORM				0x00000015
#define G80_ZETA_FORMAT_Z24_S8_UNORM				0x00000016
#define G80_ZETA_FORMAT_Z24_C8_UNORM				0x00000018
#define G80_ZETA_FORMAT_Z32_S8_X24_FLOAT			0x00000019
#define G80_ZETA_FORMAT_Z24_X8_S8_C8_X16_UNORM			0x0000001d
#define G80_ZETA_FORMAT_Z32_X8_C8_X16_FLOAT			0x0000001e
#define G80_ZETA_FORMAT_Z32_S8_C8_X16_FLOAT			0x0000001f
#define GK104_IMAGE_FORMAT_RGBA32_FLOAT				0x00000002
#define GK104_IMAGE_FORMAT_RGBA32_SINT				0x00000003
#define GK104_IMAGE_FORMAT_RGBA32_UINT				0x00000004
#define GK104_IMAGE_FORMAT_RGBA16_UNORM				0x00000008
#define GK104_IMAGE_FORMAT_RGBA16_SNORM				0x00000009
#define GK104_IMAGE_FORMAT_RGBA16_SINT				0x0000000a
#define GK104_IMAGE_FORMAT_RGBA16_UINT				0x0000000b
#define GK104_IMAGE_FORMAT_RGBA16_FLOAT				0x0000000c
#define GK104_IMAGE_FORMAT_RG32_FLOAT				0x0000000d
#define GK104_IMAGE_FORMAT_RG32_SINT				0x0000000e
#define GK104_IMAGE_FORMAT_RG32_UINT				0x0000000f
#define GK104_IMAGE_FORMAT_BGRA8_UNORM				0x00000011
#define GK104_IMAGE_FORMAT_RGB10_A2_UNORM			0x00000013
#define GK104_IMAGE_FORMAT_RGB10_A2_UINT			0x00000015
#define GK104_IMAGE_FORMAT_RGBA8_UNORM				0x00000018
#define GK104_IMAGE_FORMAT_RGBA8_SNORM				0x0000001a
#define GK104_IMAGE_FORMAT_RGBA8_SINT				0x0000001b
#define GK104_IMAGE_FORMAT_RGBA8_UINT				0x0000001c
#define GK104_IMAGE_FORMAT_RG16_UNORM				0x0000001d
#define GK104_IMAGE_FORMAT_RG16_SNORM				0x0000001e
#define GK104_IMAGE_FORMAT_RG16_SINT				0x0000001f
#define GK104_IMAGE_FORMAT_RG16_UINT				0x00000020
#define GK104_IMAGE_FORMAT_RG16_FLOAT				0x00000021
#define GK104_IMAGE_FORMAT_R11G11B10_FLOAT			0x00000024
#define GK104_IMAGE_FORMAT_R32_SINT				0x00000027
#define GK104_IMAGE_FORMAT_R32_UINT				0x00000028
#define GK104_IMAGE_FORMAT_R32_FLOAT				0x00000029
#define GK104_IMAGE_FORMAT_RG8_UNORM				0x0000002e
#define GK104_IMAGE_FORMAT_RG8_SNORM				0x0000002f
#define GK104_IMAGE_FORMAT_RG8_SINT				0x00000030
#define GK104_IMAGE_FORMAT_RG8_UINT				0x00000031
#define GK104_IMAGE_FORMAT_R16_UNORM				0x00000032
#define GK104_IMAGE_FORMAT_R16_SNORM				0x00000033
#define GK104_IMAGE_FORMAT_R16_SINT				0x00000034
#define GK104_IMAGE_FORMAT_R16_UINT				0x00000035
#define GK104_IMAGE_FORMAT_R16_FLOAT				0x00000036
#define GK104_IMAGE_FORMAT_R8_UNORM				0x00000037
#define GK104_IMAGE_FORMAT_R8_SNORM				0x00000038
#define GK104_IMAGE_FORMAT_R8_SINT				0x00000039
#define GK104_IMAGE_FORMAT_R8_UINT				0x0000003a
#define G80_PGRAPH_DATA_ERROR_INVALID_OPERATION			0x00000003
#define G80_PGRAPH_DATA_ERROR_INVALID_VALUE			0x00000004
#define G80_PGRAPH_DATA_ERROR_INVALID_ENUM			0x00000005
#define G80_PGRAPH_DATA_ERROR_INVALID_OBJECT			0x00000008
#define G80_PGRAPH_DATA_ERROR_READ_ONLY_OBJECT			0x00000009
#define G80_PGRAPH_DATA_ERROR_SUPERVISOR_OBJECT			0x0000000a
#define G80_PGRAPH_DATA_ERROR_INVALID_ADDRESS_ALIGNMENT		0x0000000b
#define G80_PGRAPH_DATA_ERROR_INVALID_BITFIELD			0x0000000c
#define G80_PGRAPH_DATA_ERROR_BEGIN_END_ACTIVE			0x0000000d
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_COLOR_BACK_OVER_LIMIT	0x0000000e
#define G80_PGRAPH_DATA_ERROR_VIEWPORT_ID_NEEDS_GP		0x0000000f
#define G80_PGRAPH_DATA_ERROR_RT_DOUBLE_BIND			0x00000010
#define G80_PGRAPH_DATA_ERROR_RT_TYPES_MISMATCH			0x00000011
#define G80_PGRAPH_DATA_ERROR_RT_PITCH_WITH_ZETA		0x00000012
#define G80_PGRAPH_DATA_ERROR_FP_TOO_FEW_REGS			0x00000015
#define G80_PGRAPH_DATA_ERROR_ZETA_FORMAT_CSAA_MISMATCH		0x00000016
#define G80_PGRAPH_DATA_ERROR_RT_PITCH_WITH_MSAA		0x00000017
#define G80_PGRAPH_DATA_ERROR_FP_INTERPOLANT_START_OVER_LIMIT	0x00000018
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_LAYER_OVER_LIMIT		0x00000019
#define G80_PGRAPH_DATA_ERROR_RT_INVALID_ALIGNMENT		0x0000001a
#define G80_PGRAPH_DATA_ERROR_SAMPLER_OVER_LIMIT		0x0000001b
#define G80_PGRAPH_DATA_ERROR_TEXTURE_OVER_LIMIT		0x0000001c
#define G80_PGRAPH_DATA_ERROR_GP_TOO_MANY_OUTPUTS		0x0000001e
#define G80_PGRAPH_DATA_ERROR_RT_BPP128_WITH_MS8		0x0000001f
#define G80_PGRAPH_DATA_ERROR_Z_OUT_OF_BOUNDS			0x00000021
#define G80_PGRAPH_DATA_ERROR_XY_OUT_OF_BOUNDS			0x00000023
#define G80_PGRAPH_DATA_ERROR_VP_ZERO_INPUTS			0x00000024
#define G80_PGRAPH_DATA_ERROR_CP_MORE_PARAMS_THAN_SHARED	0x00000027
#define G80_PGRAPH_DATA_ERROR_CP_NO_REG_SPACE_STRIPED		0x00000028
#define G80_PGRAPH_DATA_ERROR_CP_NO_REG_SPACE_PACKED		0x00000029
#define G80_PGRAPH_DATA_ERROR_CP_NOT_ENOUGH_WARPS		0x0000002a
#define G80_PGRAPH_DATA_ERROR_CP_BLOCK_SIZE_MISMATCH		0x0000002b
#define G80_PGRAPH_DATA_ERROR_CP_NOT_ENOUGH_LOCAL_WARPS		0x0000002c
#define G80_PGRAPH_DATA_ERROR_CP_NOT_ENOUGH_STACK_WARPS		0x0000002d
#define G80_PGRAPH_DATA_ERROR_CP_NO_BLOCKDIM_LATCH		0x0000002e
#define G80_PGRAPH_DATA_ERROR_ENG2D_FORMAT_MISMATCH		0x00000031
#define G80_PGRAPH_DATA_ERROR_ENG2D_OPERATION_ILLEGAL_FOR_DST_FORMAT	0x00000033
#define G80_PGRAPH_DATA_ERROR_ENG2D_FORMAT_MISMATCH_B		0x00000034
#define G80_PGRAPH_DATA_ERROR_PRIMITIVE_ID_NEEDS_GP		0x0000003f
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_VIEWPORT_OVER_LIMIT	0x00000044
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_COLOR_FRONT_OVER_LIMIT	0x00000045
#define G80_PGRAPH_DATA_ERROR_LAYER_ID_NEEDS_GP			0x00000046
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_CLIP_OVER_LIMIT		0x00000047
#define G80_PGRAPH_DATA_ERROR_SEMANTIC_PTSZ_OVER_LIMIT		0x00000048
#define G80_PGRAPH_DATA_ERROR_M2MF_LINE_LENGTH_EXCEEDS_PITCH_IN	0x00000051
#define G80_PGRAPH_DATA_ERROR_M2MF_LINE_LENGTH_EXCEEDS_PITCH_OUT	0x00000053
#define G80_PGRAPH_DATA_ERROR_RT_PITCH_WITH_ZETA_GF100		0x00000098
#define G80_PGRAPH_DATA_ERROR_ENG2D_UNALIGNED_PITCH_GF100	0x000000a5
#define G80_CG_IDLE_TIMEOUT__MASK				0x0000003f
#define G80_CG_IDLE_TIMEOUT__SHIFT				0
#define G80_CG_IDLE_TIMEOUT_ENABLE				0x00000040
#define G80_CG_INTERFACE_REENABLE_TIME__MASK			0x000f0000
#define G80_CG_INTERFACE_REENABLE_TIME__SHIFT			16
#define G80_CG_THROTTLE_DUTY_M1__MASK				0x00f00000
#define G80_CG_THROTTLE_DUTY_M1__SHIFT				20
#define G80_CG_DELAY__MASK					0x0f000000
#define G80_CG_DELAY__SHIFT					24
#define G80_CG_CLOCK_THROTTLE_ENABLE				0x10000000
#define G80_CG_THROTTLE_MODE__MASK				0x20000000
#define G80_CG_THROTTLE_MODE__SHIFT				29
#define G80_CG_THROTTLE_MODE_AUTO				0x00000000
#define G80_CG_THROTTLE_MODE_MANUAL				0x20000000
#define G80_CG_INTERFACE_THROTTLE_ENABLE			0x40000000
#define G80_QUERY__SIZE						0x00000010
#define G80_QUERY_COUNTER					0x00000000

#define G80_QUERY_RES						0x00000004

#define G80_QUERY_TIME						0x00000008


#endif /* G80_DEFS_XML */
