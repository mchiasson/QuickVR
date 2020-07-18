#ifndef VRTYPES_H
#define VRTYPES_H

#include <cstdint>

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }

enum BufferUsage
{
    BufferUsage_Unknown  = 0x00,
    BufferUsage_Vertex   = 0x01,
    BufferUsage_Index    = 0x02,
    BufferUsage_Uniform  = 0x04,
    BufferUsage_Feedback = 0x08,
    BufferUsage_Compute  = 0x10,
    BufferUsage_Static = 0x80
};

enum MapFlags
{
    Map_Default        = 0x00,
    Map_Discard        = 0x01,
    Map_Read           = 0x02,
    Map_Unsynchronized = 0x04
};

enum SampleMode
{
    Sample_Linear       = 0,
    Sample_Nearest      = 1,
    Sample_Anisotropic  = 2,
    Sample_FilterMask   = 3,

    Sample_Repeat       = 0,
    Sample_Clamp        = 4,
    Sample_ClampBorder  = 8, // If unsupported Clamp is used instead.
    Sample_AddressMask  =12,

    Sample_Count        =13
};

enum ShaderStage
{
    ShaderStage_Vertex   = 0,
    ShaderStage_Geometry,
    ShaderStage_Fragment,
    ShaderStage_Pixel,
    ShaderStage_Compute,

    ShaderStage_Count
};

enum TextureFormat : uint64_t
{
    //////////////////////////////////////////////////////////////////////////
    // These correspond to the available OVR_FORMAT enum except sRGB versions
    // which are enabled via the Texture_SRGB flag
    Texture_B5G6R5          = 0x10, // not supported as requires feature level DirectX 11.1
    Texture_BGR5A1          = 0x20, // not supported as requires feature level DirectX 11.1
    Texture_BGRA4           = 0x30, // not supported as requires feature level DirectX 11.1
    Texture_RGBA8           = 0x40, // allows sRGB
    Texture_BGRA8           = 0x50, // allows sRGB
    Texture_BGRX            = 0x60, // allows sRGB
    Texture_RGBA16f         = 0x70,
    Texture_R11G11B10f      = 0x80,
    // End of OVR_FORMAT corresponding formats
    //////////////////////////////////////////////////////////////////////////

    Texture_RGBA            = Texture_RGBA8,
    Texture_BGRA            = Texture_BGRA8,
    Texture_R               = 0x100,
    Texture_A               = 0x110,
    Texture_BC1             = 0x210,
    Texture_BC2             = 0x220,
    Texture_BC3             = 0x230,
    Texture_BC6S            = 0x240,
    Texture_BC6U            = 0x241,
    Texture_BC7             = 0x250,

    Texture_Depth32f        = 0x1000,   // aliased as default Texture_Depth
    Texture_Depth24Stencil8 = 0x2000,
    Texture_Depth32fStencil8= 0x4000,
    Texture_Depth16         = 0x8000,

    Texture_DepthMask       = 0xf000,
    Texture_TypeMask        = 0xfff0,
    Texture_Compressed      = 0x200,
    Texture_SamplesMask     = 0x000f,

    Texture_RenderTarget    = 0x10000,
    Texture_SampleDepth     = 0x20000,
    Texture_GenMipmaps      = 0x40000,
    Texture_SRGB            = 0x80000,
    Texture_Mirror          = 0x100000,
    Texture_SwapTextureSet  = 0x200000,
    Texture_SwapTextureSetStatic  = 0x400000,
    Texture_Hdcp            = 0x800000,
    Texture_CpuDynamic      = 0x1000000,    // only used by D3D11
    Texture_GenMipmapsBySdk = 0x2000000,    // not compatible with Texture_GenMipmaps

    Texture_Cubemap         = 0x4000000,

    Texture_MirrorPostDistortion        = 0x10000000,
    Texture_MirrorLeftEyeOnly           = 0x20000000,
    Texture_MirrorRightEyeOnly          = 0x40000000,
    Texture_MirrorIncludeGuardian       = 0x80000000,
    Texture_MirrorIncludeNotifications  = 0x100000000,
    Texture_MirrorIncludeSystemGui      = 0x200000000,
    Texture_MirrorForceSymmetricFov     = 0x400000000
};


#endif // VRTYPES_H
