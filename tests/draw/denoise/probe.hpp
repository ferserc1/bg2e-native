/*
 *    business grade graphic engine (bg2e engine)
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

#pragma once
#include <bg2e/draw/pipeline/DenoiseFilter.hpp>
#include <bg2e/draw/SettingsCatalog.hpp>
#include <bg2e/draw/Engine.hpp>
#include <cmath>
#include <cstring>

namespace bg2e::draw::tests {
// Stopped initialized Engine. false denotes unavailable native profiles, not pass.
inline bool runDenoiseProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    using namespace pipeline;
    DenoiseConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    struct ObservedFilter : DenoiseFilter {
        using DenoiseFilter::DenoiseFilter;
        uint32_t radius = 1, recorded = 0;

        uint32_t kernelRadius() const override
        {
            return radius;
        }
        void setKernelRadius(uint32_t value) override
        {
            DenoiseFilter::setKernelRadius(value);
            radius = value;
        }

        DenoiseResult record(const DenoiseInput& input) override
        {
            ++recorded;
            return DenoiseFilter::record(input);
        }
    };
    auto filter = std::make_shared<ObservedFilter>(configuration);
    SettingsCatalog catalog;
    catalog.reset("denoise-probe");
    catalog.add({"denoise",filter->settingsType(),filter->settingsInstance(),filter,{},{},{}});
    const auto radiusHandle = catalog.handle("denoise","kernelRadius");
    if (std::any_cast<uint32_t>(catalog.read(radiusHandle)) != 1 ||
        !catalog.apply({{radiusHandle,uint32_t(2)}}).applied || filter->kernelRadius() != 2)
    {
        throw std::runtime_error("Denoise metadata must read/write actual virtual accessors");
    }
    filter->setKernelRadius(1);
    auto& device = *engine.device();
    filter->initialize(device,engine.backendType(),2);
    for (auto profile : {SurfaceProfile::CameraDepth,SurfaceProfile::WorldPositionCoverage})
    {
        for (auto kind : {DenoiseSignal::Visibility,DenoiseSignal::DiffuseRadiance})
        {
            if (!filter->available(kind,profile))
            {
                filter->cleanup();
                return false;
            }
        }
    }
    constexpr uint32_t width = 5, height = 3, pixels = width*height;
    auto upload = [&](gpu::PixelFormat format, gpu::Size2D size, const void* data, size_t bytes)
    {
        gpu::ImageDescription description;
        description.size = size;
        description.format = format;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst | gpu::ImageUsage::TransferSrc;
        auto image = device.createImage(description);
        image->upload({data,bytes,bytes/size.height,0,0});
        return image;
    };
    std::vector<uint16_t> normals(pixels*4), scalar(pixels,0x3c00), hdr(pixels*4), reflected(pixels*4);
    std::vector<float> positions(pixels*4);
    std::vector<uint8_t> coverage(pixels,255), material(pixels*4,0);
    for (uint32_t i = 0; i < pixels; ++i)
    {
        normals[i*4+2] = 0x3c00;
        positions[i*4] = float(i%width)*0.01f;
        positions[i*4+1] = float(i/width)*0.01f;
        positions[i*4+3] = 1;
        hdr[i*4] = 0x4400;
        hdr[i*4+1] = 0x4000;
        hdr[i*4+2] = hdr[i*4+3] = 0x3c00;
    }
    const uint32_t center = width+2;
    scalar[center] = 0;
    coverage[0] = 0;
    auto normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},normals.data(),normals.size()*2);
    auto position = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,{width,height},positions.data(),positions.size()*4);
    auto covered = upload(gpu::PixelFormat::R8_UNORM,{width,height},coverage.data(),coverage.size());
    auto roughness = upload(gpu::PixelFormat::R8G8B8A8_UNORM,{width,height},material.data(),material.size());
    SurfaceInputs surface{SurfaceProfile::WorldPositionCoverage,normal.get(),position.get(),covered.get()};
    uint64_t serial = 0;
    auto render = [&](DenoiseSignal kind, gpu::Image& signal)
    {
        ++serial;
        const uint32_t slot = uint32_t(serial%2);
        filter->beginFrame(slot,serial);
        DenoiseResult output;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            output = filter->record({*command,slot,serial,{kind,4,9},signal,surface,glm::mat4(1),
                kind == DenoiseSignal::ReflectedRadiance ? roughness.get() : nullptr,0.75f});
        });
        return output;
    };
    auto read = [&](const DenoiseResult& result, uint32_t channels)
    {
        std::vector<uint8_t> bytes;
        result.image->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
        if (bytes.size() != size_t(result.image->width())*result.image->height()*channels*2)
        {
            throw std::runtime_error("Denoise output format/extent differs");
        }
        std::vector<float> values(bytes.size()/2);
        for (size_t i = 0; i < values.size(); ++i)
        {
            uint16_t half;
            std::memcpy(&half,bytes.data()+i*2,2);
            const uint32_t exponent = (half>>10)&31u, mantissa = half&1023u;
            if (exponent == 31u)
            {
                throw std::runtime_error("Denoise output must remain finite");
            }
            values[i] = exponent ? std::ldexp(float(1024u+mantissa),int(exponent)-25) : std::ldexp(float(mantissa),-24);
            if (half&0x8000u)
            {
                values[i] = -values[i];
            }
        }
        return values;
    };
    auto scalarImage = upload(gpu::PixelFormat::R16_SFLOAT,{width,height},scalar.data(),scalar.size()*2);
    const auto first = render(DenoiseSignal::Visibility,*scalarImage);
    auto values = read(first,1);
    if (values[center] <= 0 || values[center] >= 1 || values[0] != 1)
    {
        throw std::runtime_error("Bilateral scalar smoothing/coverage differs");
    }
    auto hdrImage = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},hdr.data(),hdr.size()*2);
    values = read(render(DenoiseSignal::DiffuseRadiance,*hdrImage),4);
    for (uint32_t i = 1; i < pixels; ++i)
    {
        if (std::abs(values[i*4]-4) > 0.01f || std::abs(values[i*4+1]-2) > 0.01f ||
            std::abs(values[i*4+2]-1) > 0.01f || values[i*4+3] != 1)
        {
            throw std::runtime_error("Normalized HDR filter must preserve constant energy at odd extents/borders");
        }
    }
    for (uint32_t i = 0; i < pixels; ++i)
    {
        scalar[i] = i%width < 3 ? 0 : 0x3c00;
        positions[i*4+2] = i%width < 3 ? 0 : 1;
    }
    scalarImage = upload(gpu::PixelFormat::R16_SFLOAT,{width,height},scalar.data(),scalar.size()*2);
    position = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,{width,height},positions.data(),positions.size()*4);
    surface.depthOrPosition = position.get();
    values = read(render(DenoiseSignal::Visibility,*scalarImage),1);
    if (values[center] != 0 || values[center+1] != 1)
    {
        throw std::runtime_error("World-distance discontinuity must reject cross-surface filtering");
    }
    for (uint32_t i = 0; i < pixels; ++i)
    {
        positions[i*4+2] = 0;
        normals[i*4] = i%width >= 3 ? 0x3c00 : 0;
        normals[i*4+2] = i%width >= 3 ? 0 : 0x3c00;
    }
    normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},normals.data(),normals.size()*2);
    position = upload(gpu::PixelFormat::R32G32B32A32_SFLOAT,{width,height},positions.data(),positions.size()*4);
    surface.normal = normal.get();
    surface.depthOrPosition = position.get();
    values = read(render(DenoiseSignal::Visibility,*scalarImage),1);
    if (values[center] != 0 || values[center+1] != 1)
    {
        throw std::runtime_error("Normal discontinuity must reject cross-surface filtering");
    }
    for (uint32_t i = 0; i < pixels; ++i)
    {
        normals[i*4] = 0;
        normals[i*4+2] = 0x3c00;
        reflected[i*4] = i%width < 3 ? 0x4000 : 0;
        reflected[i*4+2] = i%width < 3 ? 0 : 0x4000;
        reflected[i*4+3] = i%width < 3 ? 0x3c00 : 0;
    }
    normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},normals.data(),normals.size()*2);
    surface.normal = normal.get();
    auto reflectionImage = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},reflected.data(),reflected.size()*2);
    values = read(render(DenoiseSignal::ReflectedRadiance,*reflectionImage),4);
    if (std::abs(values[center*4]-2) > 0.01f || values[center*4+2] != 0 ||
        values[center*4+3] <= 0 || values[center*4+3] >= 1)
    {
        throw std::runtime_error("Reflection RGB must remain unpremultiplied while confidence averages eligible misses");
    }
    hdr[center*4+3] = 0;
    hdrImage = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},hdr.data(),hdr.size()*2);
    values = read(render(DenoiseSignal::DiffuseRadiance,*hdrImage),4);
    if (values[center*4] != 0 || values[center*4+3] != 0)
    {
        throw std::runtime_error("GI invalid center must not be filled by neighboring valid radiance");
    }
    material[center*4+1] = 255;
    roughness = upload(gpu::PixelFormat::R8G8B8A8_UNORM,{width,height},material.data(),material.size());
    values = read(render(DenoiseSignal::ReflectedRadiance,*reflectionImage),4);
    if (values[center*4] != 0 || values[center*4+3] != 0)
    {
        throw std::runtime_error("Roughness rejection must not revive neighboring reflections");
    }
    // Native camera-depth profile and a differently sized signal.
    for (uint32_t i = 0; i < pixels; ++i)
    {
        normals[i*4] = normals[i*4+1] = 0x3800;
        normals[i*4+2] = 0x3c00;
    }
    normal = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{width,height},normals.data(),normals.size()*2);
    gpu::ImageDescription depthDescription;
    depthDescription.size = {width,height};
    depthDescription.format = gpu::PixelFormat::D32_SFLOAT;
    depthDescription.usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled;
    auto depth = device.createImage(depthDescription);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        command->transition(depth.get(),gpu::ImageLayout::DepthAttachment);
        gpu::RenderingDescription rendering;
        rendering.depth = gpu::DepthAttachmentDescription{depth.get(),gpu::AttachmentLoadOperation::Clear,
            gpu::AttachmentStoreOperation::Store,0};
        command->beginRendering(rendering);
        command->endRendering();
        command->transition(depth.get(),gpu::ImageLayout::ShaderReadOnly);
    });
    surface = {SurfaceProfile::CameraDepth,normal.get(),depth.get(),nullptr,glm::mat4(1)};
    scalar.assign(pixels,0x3c00);
    scalar[center] = 0;
    scalarImage = upload(gpu::PixelFormat::R16_SFLOAT,{width,height},scalar.data(),scalar.size()*2);
    values = read(render(DenoiseSignal::Visibility,*scalarImage),1);
    if (values[center] <= 0 || values[center] >= 1)
    {
        throw std::runtime_error("Camera-depth scalar filtering must smooth a continuous plane");
    }
    const uint16_t smallHDR[] = {0x4400,0x4000,0x3c00,0x3c00,0x4400,0x4000,0x3c00,0x3c00,0x4400,0x4000,0x3c00,0x3c00};
    auto small = upload(gpu::PixelFormat::R16G16B16A16_SFLOAT,{3,1},smallHDR,sizeof(smallHDR));
    values = read(render(DenoiseSignal::DiffuseRadiance,*small),4);
    if (values.size() != 12 || std::abs(values[0]-4) > 0.01f || values[3] != 1)
    {
        throw std::runtime_error("Signal and surface extents must remain independent");
    }
    const auto retained = read(first,1);
    if (retained[center] <= 0 || retained[center] >= 1)
    {
        throw std::runtime_error("Later effect/slot dispatch overwrote an earlier filter output");
    }
    filter->setKernelRadius(0);
    const auto identity = render(DenoiseSignal::Visibility,*scalarImage);
    if (identity.owner || identity.image != scalarImage.get())
    {
        throw std::runtime_error("Zero-radius filter must return the borrowed source");
    }
    filter->setKernelRadius(1);
    filter->setEnabled(false);
    const auto disabled = render(DenoiseSignal::DiffuseRadiance,*small);
    if (disabled.owner || disabled.image != small.get() || filter->recorded < 10)
    {
        throw std::runtime_error("Disabled filter/virtual dispatch differs");
    }
    catalog.reset();
    filter->cleanup();
    return true;
}
}
