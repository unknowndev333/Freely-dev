#include "graphics.h"
#include <stdlib.h>
#include <string.h>

Scene2D::Scene2D(int w, int h, int pixelDepth)
    : width(w), height(h), depth(pixelDepth), video(-1),
      frameBufferSize(w*h*pixelDepth), frameBufferCount(0),
      activeFrameBufferIndex(0), directMemoryOffset(0),
      directMemorySize(0), videoMemoryStack(0), videoMemory(NULL),
      frameBuffers(NULL), flipQueue(0) {}

bool Scene2D::allocateVideoMemory(size_t size, int alignment) {
    directMemorySize = (size + alignment - 1) / alignment * alignment;
    int rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(),
        directMemorySize, alignment, 3, &directMemoryOffset);
    if (rc < 0) return false;

    rc = sceKernelMapDirectMemory(&videoMemory, directMemorySize, 0x33, 0,
        directMemoryOffset, alignment);
    if (rc < 0) {
        sceKernelReleaseDirectMemory(directMemoryOffset, directMemorySize);
        directMemorySize = 0;
        return false;
    }
    videoMemoryStack = (uintptr_t)videoMemory;
    return true;
}

char *Scene2D::allocateDisplayMemory(size_t size) {
    char *result = (char *)videoMemoryStack;
    videoMemoryStack += size;
    return result;
}

bool Scene2D::Init(size_t memorySize, int bufferCount) {
    video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
    if (video < 0) return false;

    if (sceKernelCreateEqueue(&flipQueue, "Freely Dev flip queue") < 0)
        return false;
    if (sceVideoOutAddFlipEvent(flipQueue, video, 0) < 0)
        return false;
    if (!allocateVideoMemory(memorySize, 0x200000))
        return false;

    frameBufferCount = bufferCount;
    frameBuffers = new char*[bufferCount];
    if (!frameBuffers) return false;

    for (int i=0; i<bufferCount; ++i)
        frameBuffers[i] = allocateDisplayMemory((size_t)frameBufferSize);

    sceVideoOutSetBufferAttribute(&bufferAttribute, 0x80000000, 1, 0,
        width, height, width);
    if (sceVideoOutRegisterBuffers(video, 0, (void **)frameBuffers,
        bufferCount, &bufferAttribute) < 0)
        return false;

    if (sceVideoOutSetFlipRate(video, 0) < 0)
        return false;
    return true;
}

void Scene2D::SetActiveFrameBuffer(int index) {
    if (index >= 0 && index < frameBufferCount)
        activeFrameBufferIndex = index;
}

void Scene2D::SubmitFlip(int frameId) {
    sceVideoOutSubmitFlip(video, activeFrameBufferIndex,
        ORBIS_VIDEO_OUT_FLIP_VSYNC, frameId);
}

void Scene2D::FrameWait(int frameId) {
    OrbisKernelEvent event;
    int count = 0;
    for (;;) {
        OrbisVideoOutFlipStatus status;
        if (sceVideoOutGetFlipStatus(video, &status) == 0 &&
            status.flipArg == frameId)
            break;
        if (sceKernelWaitEqueue(flipQueue, &event, 1, &count, 0) != 0)
            break;
    }
}

void Scene2D::FrameBufferSwap() {
    if (frameBufferCount > 0)
        activeFrameBufferIndex = (activeFrameBufferIndex + 1) % frameBufferCount;
}

void Scene2D::DrawPixel(int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= width || y >= height || !frameBuffers)
        return;
    size_t pixel = (size_t)y * (size_t)width + (size_t)x;
    uint32_t value = 0x80000000u | ((uint32_t)color.r << 16) |
        ((uint32_t)color.g << 8) | (uint32_t)color.b;
    ((uint32_t *)frameBuffers[activeFrameBufferIndex])[pixel] = value;
}

void Scene2D::DrawRectangle(int x, int y, int w, int h, Color color) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > width ? width : x + w;
    int y1 = y + h > height ? height : y + h;
    for (int py=y0; py<y1; ++py)
        for (int px=x0; px<x1; ++px)
            DrawPixel(px,py,color);
}
