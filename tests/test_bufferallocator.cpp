/**
 * @file test_bufferallocator.cpp
 * @brief Unit tests for BufferAllocator
 */

#include <iostream>
#include <cmath>
#include <subcollider/BufferAllocator.h>

using namespace subcollider;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

// Use smaller pool size for tests to avoid large memory usage
using TestAllocator = BufferAllocator<48000, 16>;  // 1 second at 48kHz, 16 blocks max

int test_bufferallocator() {
    int failures = 0;

    // Test uninitialized allocator
    {
        TestAllocator alloc;
        TEST("Allocator: not initialized by default", alloc.isInitialized() == false);
        
        Buffer buf = alloc.allocate(100, 1);
        TEST("Allocator: allocate fails when not initialized", buf.isValid() == false);
    }

    // Test initialization
    {
        TestAllocator alloc;
        alloc.init(44100.0f);
        
        TEST("Allocator: initialized after init()", alloc.isInitialized() == true);
        TEST("Allocator: sample rate is set", alloc.sampleRate() == 44100.0f);
        TEST("Allocator: pool size is correct", TestAllocator::poolSize() == 48000);
        TEST("Allocator: block count is 1 after init", alloc.blockCount() == 1);
        TEST("Allocator: free space equals pool size", alloc.freeSpace() == 48000);
        TEST("Allocator: used space is 0", alloc.usedSpace() == 0);
    }

    // Test mono allocation
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(1000, 1);
        TEST("Allocator mono: buffer is valid", buf.isValid() == true);
        TEST("Allocator mono: buffer is mono", buf.isMono() == true);
        TEST("Allocator mono: numSamples is correct", buf.numSamples == 1000);
        TEST("Allocator mono: sampleRate is correct", buf.sampleRate == 48000.0f);
        TEST("Allocator mono: totalFloats is correct", buf.totalFloats() == 1000);
        TEST("Allocator mono: data is not nullptr", buf.data != nullptr);
        TEST("Allocator mono: used space is correct", alloc.usedSpace() == 1000);
        TEST("Allocator mono: free space is correct", alloc.freeSpace() == 47000);
    }

    // Test stereo allocation
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(1000, 2);
        TEST("Allocator stereo: buffer is valid", buf.isValid() == true);
        TEST("Allocator stereo: buffer is stereo", buf.isStereo() == true);
        TEST("Allocator stereo: numSamples is correct", buf.numSamples == 1000);
        TEST("Allocator stereo: totalFloats is correct", buf.totalFloats() == 2000);
        TEST("Allocator stereo: used space is correct", alloc.usedSpace() == 2000);
        TEST("Allocator stereo: free space is correct", alloc.freeSpace() == 46000);
    }

    // Test multiple allocations
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf1 = alloc.allocate(1000, 1);
        Buffer buf2 = alloc.allocate(2000, 1);
        Buffer buf3 = alloc.allocate(500, 2);  // 500 samples * 2 channels = 1000 floats
        
        TEST("Allocator multi: buf1 is valid", buf1.isValid() == true);
        TEST("Allocator multi: buf2 is valid", buf2.isValid() == true);
        TEST("Allocator multi: buf3 is valid", buf3.isValid() == true);
        TEST("Allocator multi: block count is 4", alloc.blockCount() == 4);
        TEST("Allocator multi: used space is correct", alloc.usedSpace() == 4000);
        TEST("Allocator multi: buffers have different data pointers", 
             buf1.data != buf2.data && buf2.data != buf3.data);
    }

    // Test allocation failure when pool is exhausted
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf1 = alloc.allocate(48000, 1);  // Use entire pool
        TEST("Allocator exhaust: first allocation succeeds", buf1.isValid() == true);
        
        Buffer buf2 = alloc.allocate(1, 1);  // Should fail
        TEST("Allocator exhaust: second allocation fails", buf2.isValid() == false);
    }

    // Test release
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(1000, 1);
        TEST("Allocator release: buffer is valid before release", buf.isValid() == true);
        TEST("Allocator release: used space before release", alloc.usedSpace() == 1000);
        
        bool released = alloc.release(buf);
        TEST("Allocator release: release returns true", released == true);
        TEST("Allocator release: used space after release is 0", alloc.usedSpace() == 0);
        TEST("Allocator release: free space after release", alloc.freeSpace() == 48000);
    }

    // Test release and reallocate
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf1 = alloc.allocate(1000, 1);
        Sample* originalData = buf1.data;
        alloc.release(buf1);
        
        Buffer buf2 = alloc.allocate(1000, 1);
        TEST("Allocator realloc: reuses released memory", buf2.data == originalData);
    }

    // Test block merging
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf1 = alloc.allocate(1000, 1);
        Buffer buf2 = alloc.allocate(1000, 1);
        Buffer buf3 = alloc.allocate(1000, 1);
        
        // Release middle block
        alloc.release(buf2);
        TEST("Allocator merge: block count after releasing middle", alloc.blockCount() == 4);
        
        // Release first block - should merge with middle
        alloc.release(buf1);
        TEST("Allocator merge: block count after releasing first", alloc.blockCount() == 3);
        
        // Release last - should merge all free blocks
        alloc.release(buf3);
        TEST("Allocator merge: block count after releasing all", alloc.blockCount() == 1);
        TEST("Allocator merge: all space is free", alloc.freeSpace() == 48000);
    }

    // Test fillMono
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 1);
        Sample srcData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        
        bool filled = TestAllocator::fillMono(buf, srcData, 4);
        TEST("Allocator fillMono: returns true", filled == true);
        TEST("Allocator fillMono: data[0] is correct", std::abs(buf.data[0] - 0.1f) < 0.0001f);
        TEST("Allocator fillMono: data[1] is correct", std::abs(buf.data[1] - 0.2f) < 0.0001f);
        TEST("Allocator fillMono: data[2] is correct", std::abs(buf.data[2] - 0.3f) < 0.0001f);
        TEST("Allocator fillMono: data[3] is correct", std::abs(buf.data[3] - 0.4f) < 0.0001f);
    }

    // Test fillMono with count < numSamples
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 1);
        Sample srcData[2] = {0.5f, 0.6f};
        
        bool filled = TestAllocator::fillMono(buf, srcData, 2);
        TEST("Allocator fillMono partial: returns true", filled == true);
        TEST("Allocator fillMono partial: data[0] is correct", std::abs(buf.data[0] - 0.5f) < 0.0001f);
        TEST("Allocator fillMono partial: data[1] is correct", std::abs(buf.data[1] - 0.6f) < 0.0001f);
    }

    // Test fillMono fails for stereo buffer
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 2);
        Sample srcData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        
        bool filled = TestAllocator::fillMono(buf, srcData, 4);
        TEST("Allocator fillMono stereo: returns false", filled == false);
    }

    // Test fillStereo
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 2);
        Sample leftData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Sample rightData[4] = {0.5f, 0.6f, 0.7f, 0.8f};
        
        bool filled = TestAllocator::fillStereo(buf, leftData, rightData, 4);
        TEST("Allocator fillStereo: returns true", filled == true);
        TEST("Allocator fillStereo: left[0] is correct", std::abs(buf.data[0] - 0.1f) < 0.0001f);
        TEST("Allocator fillStereo: right[0] is correct", std::abs(buf.data[1] - 0.5f) < 0.0001f);
        TEST("Allocator fillStereo: left[1] is correct", std::abs(buf.data[2] - 0.2f) < 0.0001f);
        TEST("Allocator fillStereo: right[1] is correct", std::abs(buf.data[3] - 0.6f) < 0.0001f);
        TEST("Allocator fillStereo: left[2] is correct", std::abs(buf.data[4] - 0.3f) < 0.0001f);
        TEST("Allocator fillStereo: right[2] is correct", std::abs(buf.data[5] - 0.7f) < 0.0001f);
        TEST("Allocator fillStereo: left[3] is correct", std::abs(buf.data[6] - 0.4f) < 0.0001f);
        TEST("Allocator fillStereo: right[3] is correct", std::abs(buf.data[7] - 0.8f) < 0.0001f);
    }

    // Test fillStereo fails for mono buffer
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 1);
        Sample leftData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Sample rightData[4] = {0.5f, 0.6f, 0.7f, 0.8f};
        
        bool filled = TestAllocator::fillStereo(buf, leftData, rightData, 4);
        TEST("Allocator fillStereo mono: returns false", filled == false);
    }

    // Test fillStereoInterleaved
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 2);
        Sample interleaved[8] = {0.1f, 0.5f, 0.2f, 0.6f, 0.3f, 0.7f, 0.4f, 0.8f};
        
        bool filled = TestAllocator::fillStereoInterleaved(buf, interleaved, 4);
        TEST("Allocator fillStereoInterleaved: returns true", filled == true);
        TEST("Allocator fillStereoInterleaved: data[0] is correct", std::abs(buf.data[0] - 0.1f) < 0.0001f);
        TEST("Allocator fillStereoInterleaved: data[1] is correct", std::abs(buf.data[1] - 0.5f) < 0.0001f);
        TEST("Allocator fillStereoInterleaved: data[2] is correct", std::abs(buf.data[2] - 0.2f) < 0.0001f);
        TEST("Allocator fillStereoInterleaved: data[7] is correct", std::abs(buf.data[7] - 0.8f) < 0.0001f);
    }

    // Test reset
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        alloc.allocate(1000, 1);
        alloc.allocate(2000, 1);
        TEST("Allocator reset: used space before reset", alloc.usedSpace() == 3000);
        
        alloc.reset();
        TEST("Allocator reset: used space after reset is 0", alloc.usedSpace() == 0);
        TEST("Allocator reset: block count is 1", alloc.blockCount() == 1);
        TEST("Allocator reset: still initialized", alloc.isInitialized() == true);
    }

    // Test invalid channel count
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf0 = alloc.allocate(100, 0);
        TEST("Allocator invalid channels 0: returns invalid buffer", buf0.isValid() == false);
        
        Buffer buf3 = alloc.allocate(100, 3);
        TEST("Allocator invalid channels 3: returns invalid buffer", buf3.isValid() == false);
    }

    // Test zero sample allocation
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(0, 1);
        TEST("Allocator zero samples: returns invalid buffer", buf.isValid() == false);
    }

    // Test release with nullptr
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf;  // default constructor, data is nullptr
        bool released = alloc.release(buf);
        TEST("Allocator release nullptr: returns false", released == false);
    }

    // Test release with buffer not from this pool
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Sample externalData[100];
        Buffer buf(externalData, 1, 48000.0f, 100);
        bool released = alloc.release(buf);
        TEST("Allocator release external: returns false", released == false);
    }

    // Test fillMono with nullptr data
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 1);
        bool filled = TestAllocator::fillMono(buf, nullptr, 4);
        TEST("Allocator fillMono nullptr: returns false", filled == false);
    }

    // Test fillStereo with nullptr data
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 2);
        Sample data[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        
        bool filled1 = TestAllocator::fillStereo(buf, nullptr, data, 4);
        TEST("Allocator fillStereo nullptr left: returns false", filled1 == false);
        
        bool filled2 = TestAllocator::fillStereo(buf, data, nullptr, 4);
        TEST("Allocator fillStereo nullptr right: returns false", filled2 == false);
    }

    // Test getSample and getStereoSample on allocated buffers
    {
        TestAllocator alloc;
        alloc.init(48000.0f);
        
        Buffer buf = alloc.allocate(4, 2);
        Sample leftData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Sample rightData[4] = {0.5f, 0.6f, 0.7f, 0.8f};
        TestAllocator::fillStereo(buf, leftData, rightData, 4);
        
        Stereo s0 = buf.getStereoSample(0);
        Stereo s1 = buf.getStereoSample(1);
        TEST("Allocator buffer getStereoSample(0).left", std::abs(s0.left - 0.1f) < 0.0001f);
        TEST("Allocator buffer getStereoSample(0).right", std::abs(s0.right - 0.5f) < 0.0001f);
        TEST("Allocator buffer getStereoSample(1).left", std::abs(s1.left - 0.2f) < 0.0001f);
        TEST("Allocator buffer getStereoSample(1).right", std::abs(s1.right - 0.6f) < 0.0001f);
    }

    return failures;
}
