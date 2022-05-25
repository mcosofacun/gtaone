#include "stdafx.h"
#include "TrimeshBuffer.h"
#include "GpuBuffer.h"

TrimeshBuffer::~TrimeshBuffer()
{
    cxx_assert(mIndexBuffer == nullptr);
    cxx_assert(mVertexBuffer == nullptr);
}

void TrimeshBuffer::SetVertices(unsigned int dataLength, const void* dataSource)
{
    if (mVertexBuffer == nullptr)
    {
        mVertexBuffer = gGraphicsDevice.CreateBuffer(eBufferContent_Vertices, eBufferUsage_Stream, dataLength, dataSource);
        cxx_assert(mVertexBuffer);
        return;
    }

    if (!mVertexBuffer->Setup(eBufferUsage_Stream, dataLength, dataSource))
    {
        cxx_assert(false);
    }
}

void TrimeshBuffer::SetIndices(unsigned int dataLength, const void* dataSource)
{
    if (mIndexBuffer == nullptr)
    {
        mIndexBuffer = gGraphicsDevice.CreateBuffer(eBufferContent_Indices, eBufferUsage_Stream, dataLength, dataSource);
        cxx_assert(mIndexBuffer);
        return;
    }

    if (!mIndexBuffer->Setup(eBufferUsage_Stream, dataLength, dataSource))
    {
        cxx_assert(false);
    }
}

void TrimeshBuffer::Bind(const VertexFormat& vertexFormat)
{
    cxx_assert(mVertexBuffer);
    if (mVertexBuffer == nullptr)
        return;

    gGraphicsDevice.BindVertexBuffer(mVertexBuffer, vertexFormat);
    gGraphicsDevice.BindIndexBuffer(mIndexBuffer);
}

void TrimeshBuffer::Deinit()
{
    if (mIndexBuffer)
    {
        gGraphicsDevice.DestroyBuffer(mIndexBuffer);
        mIndexBuffer = nullptr;
    }
    if (mVertexBuffer)
    {
        gGraphicsDevice.DestroyBuffer(mVertexBuffer);
        mVertexBuffer = nullptr;
    }
}