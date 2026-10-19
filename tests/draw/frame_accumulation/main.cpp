/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/draw/pipeline/FrameAccumulator.hpp>
#include <glm/packing.hpp>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace bg2e;
namespace {
void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Frame accumulation assertion failed");
    }
}

template<class Operation> void rejects(Operation operation)
{
    bool rejected = false;
    try
    {
        operation();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    require(rejected);
}

// Tight RGBA16F transfer fixture exercises the public transfer contract without
// allocating a device. Native transfer synchronization remains a GPU lead check.
class HostImage final : public gpu::Image {
  public:
    HostImage() : Image(nullptr)
    {
        _size = {1,1};
        _pixelFormat = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        set(0);
    }

    bool isValid() const override
    {
        return true;
    }
    void cleanup() override
    {
    }
    gpu::ImageUsage usage() const override
    {
        return gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    }

    void set(float value)
    {
        const uint32_t packed[] = {glm::packHalf2x16({value,value}), glm::packHalf2x16({value,1})};
        bytes.resize(sizeof(packed));
        std::memcpy(bytes.data(), packed, sizeof(packed));
    }
    void readPixels(std::vector<uint8_t>& output, const gpu::ImageSubresourceRange&, gpu::ImageLayout) override
    {
        output = bytes;
    }
    void upload(const gpu::ImageUploadDescription& data, gpu::ImageLayout) override
    {
        require(data.byteSize == bytes.size());
        std::memcpy(bytes.data(), data.data, bytes.size());
    }

    std::vector<uint8_t> bytes;
};
}

int main()
{
    try
    {
        draw::pipeline::FrameAccumulator average;
        rejects([&] { average.reset({0,1},1); });
        rejects([&] { average.reset({1,1},0); });
        rejects([&] { average.reset({1,1},1048577); });
        average.reset({1,1},2);
        const float first[] = {1,-1,100,0};
        const float second[] = {3,3,300,0};
        average.add(first);
        require(average.completedFrames() == 1 && !average.complete());
        rejects([&] { average.resolve(); });
        const float invalid[] = {1,std::numeric_limits<float>::infinity(),1,1};
        rejects([&] { average.add(invalid); });
        require(average.completedFrames() == 1);
        average.add(second);
        auto result = average.resolve();
        require(result == std::vector<float>({2,1,200,1}));
        rejects([&] { average.add(second); });
        average.reset({1,1},1);
        average.add(first);
        require(average.resolve() == std::vector<float>({1,-1,100,1}));

        HostImage sample;
        average.reset({1,1},2);
        sample.set(1);
        average.add(sample);
        sample.set(3);
        average.add(sample);
        average.resolveInto(sample);
        draw::pipeline::FrameAccumulator copy;
        copy.reset({1,1},1);
        copy.add(sample);
        require(copy.resolve() == std::vector<float>({2,2,2,1}));
        average.reset({1,1},1);
        sample.set(std::numeric_limits<float>::infinity());
        rejects([&] { average.add(sample); });
        require(average.completedFrames() == 0);
        std::cout << "Frame accumulation probe passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
