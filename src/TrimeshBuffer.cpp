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
        mVertexBuffer = gSystem.mGfxDevice.CreateBuffer(eBufferContent_Vertices, eBufferUsage_Stream, dataLength, dataSource);
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
        mIndexBuffer = gSystem.mGfxDevice.CreateBuffer(eBufferContent_Indices, eBufferUsage_Stream, dataLength, dataSource);
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

    gSystem.mGfxDevice.BindVertexBuffer(mVertexBuffer, vertexFormat);
    gSystem.mGfxDevice.BindIndexBuffer(mIndexBuffer);
}

void TrimeshBuffer::Deinit()
{
    if (mIndexBuffer)
    {
        gSystem.mGfxDevice.DestroyBuffer(mIndexBuffer);
        mIndexBuffer = nullptr;
    }
    if (mVertexBuffer)
    {
        gSystem.mGfxDevice.DestroyBuffer(mVertexBuffer);
        mVertexBuffer = nullptr;
    }
}