#ifndef FREELY_GRAPHICS_H
#define FREELY_GRAPHICS_H
#include <stdint.h>
#include <stddef.h>
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
struct Color { uint8_t r; uint8_t g; uint8_t b; };
class Scene2D {
public:
    Scene2D(int width, int height, int pixelDepth);
    bool Init(size_t memorySize, int bufferCount);
    void SetActiveFrameBuffer(int index);
    void SubmitFlip(int frameId);
    void FrameWait(int frameId);
    void FrameBufferSwap();
    void DrawPixel(int x, int y, Color color);
    void DrawRectangle(int x, int y, int width, int height, Color color);
private:
    bool allocateVideoMemory(size_t size, int alignment);
    char *allocateDisplayMemory(size_t size);
    int width, height, depth, video, frameBufferSize, frameBufferCount;
    int activeFrameBufferIndex;
    off_t directMemoryOffset;
    size_t directMemorySize;
    uintptr_t videoMemoryStack;
    void *videoMemory;
    char **frameBuffers;
    OrbisKernelEqueue flipQueue;
    OrbisVideoOutBufferAttribute bufferAttribute;
};
#endif
