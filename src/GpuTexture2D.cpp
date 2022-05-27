#include "stdafx.h"
#include "GpuTexture2D.h"
#include "OpenGLDefs.h"
#include "GraphicsContext.h"

//////////////////////////////////////////////////////////////////////////

class ScopedTexture2DBinder
{
public:
    ScopedTexture2DBinder(GraphicsContext& renderContext, GpuTexture2D* gpuTexture)
        : mRenderContext(renderContext)
        , mPreviousTexture(renderContext.mCurrentTextures[renderContext.mCurrentTextureUnit].mTexture2D)
        , mTexture(gpuTexture)
    {
        cxx_assert(gpuTexture);
        if (mTexture != mPreviousTexture)
        {
            ::glBindTexture(GL_TEXTURE_2D, mTexture->mResourceHandle);
            glCheckError();
        }
    }
    ~ScopedTexture2DBinder()
    {
        if (mTexture != mPreviousTexture)
        {
            ::glBindTexture(GL_TEXTURE_2D, mPreviousTexture ? mPreviousTexture->mResourceHandle : 0);
            glCheckError();
        }
    }
private:
    GraphicsContext& mRenderContext;
    GpuTexture2D* mPreviousTexture;
    GpuTexture2D* mTexture;
};

//////////////////////////////////////////////////////////////////////////

GpuTexture2D::GpuTexture2D(GraphicsContext& renderContext)
    : mGraphicsContext(renderContext)
    , mResourceHandle()
    , mFiltering()
    , mRepeating()
    , mSize()
    , mFormat()
{
    ::glGenTextures(1, &mResourceHandle);
    glCheckError();
}

GpuTexture2D::~GpuTexture2D()
{
    SetUnbound();

    ::glDeleteTextures(1, &mResourceHandle);
    glCheckError();
}

bool GpuTexture2D::Setup(eTextureFormat textureFormat, int sizex, int sizey, const void* sourceData)
{
    GLuint formatGL = GetTextureInputFormatGL(textureFormat);
    GLint internalFormatGL = GetTextureInternalFormatGL(textureFormat);
    GLenum dataType = GetTextureDataTypeGL(textureFormat);
    if (formatGL == 0 || internalFormatGL == 0 || dataType == 0)
    {
        cxx_assert(false);
        return false;
    }

    mFormat = textureFormat;
    mSize.x = sizex;
    mSize.y = sizey;
    
    ScopedTexture2DBinder scopedBind(mGraphicsContext, this);
    ::glTexImage2D(GL_TEXTURE_2D, 0, internalFormatGL, mSize.x, mSize.y, 0, formatGL, dataType, sourceData);
    glCheckError();

    // set default filter and repeat mode for texture
    SetSamplerStateImpl(gSystem.mGfxDevice.mDefaultTextureFilter, gSystem.mGfxDevice.mDefaultTextureWrap);
    return true;
}

void GpuTexture2D::SetSamplerState(eTextureFilterMode filtering, eTextureWrapMode repeating)
{
    if (!IsTextureInited())
    {
        cxx_assert(false);
        return;
    }

    // already set
    if (mFiltering == filtering && mRepeating == repeating)
        return;

    ScopedTexture2DBinder scopedBind(mGraphicsContext, this);

    SetSamplerStateImpl(filtering, repeating);
}

bool GpuTexture2D::IsTextureBound(eTextureUnit textureUnit) const
{
    if (!IsTextureInited())
        return false;

    cxx_assert(textureUnit < eTextureUnit_COUNT);
    return this == mGraphicsContext.mCurrentTextures[textureUnit].mTexture2D;
}

bool GpuTexture2D::IsTextureBound() const
{
    if (!IsTextureInited())
        return false;

    for (int itexunit = 0; itexunit < eTextureUnit_COUNT; ++itexunit)
    {
        if (this == mGraphicsContext.mCurrentTextures[itexunit].mTexture2D)
            return true;
    }

    return false;
}

bool GpuTexture2D::Upload(int mipLevel, int xoffset, int yoffset, int sizex, int sizey, const void* sourceData)
{
    if (!IsTextureInited())
        return false;

    cxx_assert(sourceData);
    GLuint formatGL = GetTextureInputFormatGL(mFormat);
    GLenum dataType = GetTextureDataTypeGL(mFormat);
    if (formatGL == 0 || dataType == 0)
    {
        cxx_assert(false);
        return false;
    }

    ScopedTexture2DBinder scopedBind(mGraphicsContext, this);
    ::glTexSubImage2D(GL_TEXTURE_2D, mipLevel, xoffset, yoffset, sizex, sizey, formatGL, dataType, sourceData);
    glCheckError();
    return true;
}

bool GpuTexture2D::Upload(const void* sourceData)
{
    return Upload(0, 0, 0, mSize.x, mSize.y, sourceData);
}

bool GpuTexture2D::IsTextureInited() const
{
    return mFormat != eTextureFormat_Null;
}

void GpuTexture2D::SetSamplerStateImpl(eTextureFilterMode filtering, eTextureWrapMode repeating)
{
    mFiltering = filtering;
    mRepeating = repeating;

    // set filtering
    GLint magFilterGL = GL_NEAREST;
    GLint minFilterGL = GL_NEAREST;
    switch (filtering)
    {
        case eTextureFilterMode_Nearest: 
        break;
        case eTextureFilterMode_Bilinear:
            magFilterGL = GL_LINEAR;
            minFilterGL = GL_LINEAR;
        break;
        case eTextureFilterMode_Trilinear:
            magFilterGL = GL_LINEAR;
            minFilterGL = GL_LINEAR;
        break;
        default:
        {
            cxx_assert(filtering == eTextureFilterMode_Nearest);
        }
        break;
    }

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterGL);
    glCheckError();

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterGL);
    glCheckError();

    // set repeating
    GLint wrapSGL = GL_CLAMP_TO_EDGE;
    GLint wrapTGL = GL_CLAMP_TO_EDGE;
    switch (repeating)
    {
        case eTextureWrapMode_Repeat:
            wrapSGL = GL_REPEAT;
            wrapTGL = GL_REPEAT;
        break;
        case eTextureWrapMode_ClampToEdge:
        break;
        default:
        {
            cxx_assert(repeating == eTextureWrapMode_ClampToEdge);
        }
        break;
    }

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapSGL);
    glCheckError();

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapTGL);
    glCheckError();
}

void GpuTexture2D::SetUnbound()
{
    for (int iTextureUnit = 0; iTextureUnit < eTextureUnit_COUNT; ++iTextureUnit)
    {
        if (this == mGraphicsContext.mCurrentTextures[iTextureUnit].mTexture2D)
        {
            mGraphicsContext.mCurrentTextures[iTextureUnit].mTexture2D = nullptr;
        }
    }
}
