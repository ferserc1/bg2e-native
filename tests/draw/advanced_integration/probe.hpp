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
#include <bg2e/draw/pipeline/SMAAProcessor.hpp>
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>
#include <bg2e/draw/pipeline/RTReflections.hpp>
#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/draw/pipeline/DenoiseFilter.hpp>
#include <bg2e/draw/SettingsCodec.hpp>
#include <bg2e/draw/Engine.hpp>
#include <cmath>
#include <cstring>

namespace bg2e::draw::tests {
// No initialized Device required: requested preferences survive unsupported effects.
inline void runAdvancedSettingsProbe()
{
    SettingsCatalog catalog;
    catalog.reset("advanced-integration-probe");
    auto ao = std::make_shared<pipeline::RTAmbientOcclusion>();
    auto gi = std::make_shared<pipeline::RTGlobalIllumination>();
    auto reflected = std::make_shared<pipeline::RTReflections>();
    auto temporal = std::make_shared<pipeline::TemporalAccumulator>();
    auto denoise = std::make_shared<pipeline::DenoiseFilter>();
    auto smaa = std::make_shared<pipeline::SMAAProcessor>();
    auto add = [&](const std::string& id, const auto& effect)
    {
        catalog.add({id,effect->settingsType(),effect->settingsInstance(),effect,{},{},{}});
    };
    add("ambientOcclusion",ao);
    add("globalIllumination",gi);
    add("reflections",reflected);
    add("temporal",temporal);
    add("denoise",denoise);
    add("finalProcessor",smaa);
    if (ao->available() || gi->available() || reflected->available() || smaa->available())
    {
        throw std::runtime_error("Uninitialized effects must not advertise availability");
    }
    auto result = catalog.apply({{catalog.handle("ambientOcclusion","enabled"),true},
        {catalog.handle("globalIllumination","sampleCount"),uint32_t(7)},
        {catalog.handle("reflections","maxRoughness"),0.65f},
        {catalog.handle("temporal","maxFrames"),uint32_t(19)},
        {catalog.handle("denoise","kernelRadius"),uint32_t(3)},
        {catalog.handle("finalProcessor","enabled"),true}});
    if (!result.applied)
    {
        throw std::runtime_error("Requested settings must remain editable without RT/SMAA support");
    }
    auto saved = SettingsCodec::parse(SettingsCodec::stringify(*SettingsCodec::serialize(catalog)));
    ao->setEnabled(false);
    gi->setSampleCount(1);
    reflected->setMaxRoughness(0.1f);
    temporal->setMaxFrames(2);
    denoise->setKernelRadius(0);
    smaa->setEnabled(false);
    result = SettingsCodec::validate(*saved,catalog).apply(catalog);
    if (!result.applied || !ao->enabled() || gi->sampleCount() != 7 || reflected->maxRoughness() != 0.65f ||
        temporal->settings().maxFrames != 19 || denoise->kernelRadius() != 3 || !smaa->enabled())
    {
        throw std::runtime_error("Advanced settings strict round trip lost live requested values");
    }
    if (catalog.apply({{catalog.handle("temporal","maxFrames"),uint32_t(1025)}}).valid() ||
        catalog.apply({{catalog.handle("finalProcessor","searchSteps"),uint32_t(33)}}).valid() ||
        catalog.apply({{catalog.handle("ambientOcclusion","available"),true}}).valid())
    {
        throw std::runtime_error("Advanced setting bounds/read-only capabilities were not enforced");
    }
}

// Stopped initialized Engine; false means native SMAA profiles unavailable, not pass.
inline bool runSMAAProbe(Engine& engine, const std::filesystem::path& shaderRoot)
{
    auto& device = *engine.device();
    pipeline::SMAAConfiguration configuration;
    configuration.shaderRoot = shaderRoot;
    pipeline::SMAAProcessor filter(configuration);
    filter.initialize(device,engine.backendType(),2);
    if (!filter.available())
    {
        filter.cleanup();
        return false;
    }
    constexpr uint32_t width = 33, height = 17, pixels = width*height;
    std::vector<uint8_t> bytes(pixels*4,255);
    for (uint32_t i = 0; i < pixels; ++i)
    {
        bytes[i*4+1] = 128;
        bytes[i*4+2] = 64;
    }
    gpu::ImageDescription description;
    description.size = {width,height};
    description.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
    auto source = device.createImage(description);
    source->upload({bytes.data(),bytes.size(),width*4,0,0});
    uint64_t serial = 0;
    auto render = [&]
    {
        ++serial;
        pipeline::SMAAResult result;
        device.immediateSubmit([&](gpu::CommandBuffer* command)
        {
            result = filter.process({*command,uint32_t(serial%2),serial,*source});
        });
        return result;
    };
    auto bypass = render();
    if (bypass.owner || bypass.image != source.get())
    {
        throw std::runtime_error("Disabled SMAA must borrow the original color");
    }
    filter.setEnabled(true);
    auto constant = render();
    auto read = [&](const pipeline::SMAAResult& result)
    {
        std::vector<uint8_t> raw;
        result.image->readPixels(raw,{},gpu::ImageLayout::ShaderReadOnly);
        if (!result.owner || result.image->size() != source->size() || raw.size() != pixels*8)
        {
            throw std::runtime_error("SMAA output ownership/format/extent differs");
        }
        std::vector<uint16_t> halves(pixels*4);
        std::memcpy(halves.data(),raw.data(),raw.size());
        std::vector<float> values;
        for (auto half : halves)
        {
            const uint32_t exponent = (half>>10)&31u;
            if (exponent == 31)
            {
                throw std::runtime_error("SMAA produced nonfinite output");
            }
            const float value = exponent ? std::ldexp(1.0f+float(half&1023)/1024,int(exponent)-15) :
                std::ldexp(float(half&1023),-24);
            values.push_back((half&0x8000) ? -value : value);
        }
        return values;
    };
    const auto original = read(constant);
    const float expected[] = {1,128.0f/255,64.0f/255,1};
    for (uint32_t i = 0; i < original.size(); ++i)
    {
        if (std::abs(original[i]-expected[i%4]) > 0.002f)
        {
            throw std::runtime_error("SMAA changed constant color at an odd extent/border");
        }
    }
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            for (uint32_t c = 0; c < 3; ++c)
            {
                bytes[(y*width+x)*4+c] = x > y*2 ? 255 : 0;
            }
        }
    }
    // immediateSubmit completed before replacing the borrowed source contents.
    source->upload({bytes.data(),bytes.size(),width*4,0,0});
    const auto edge = read(render());
    bool blended = false;
    for (uint32_t i = 0; i < pixels; ++i)
    {
        if (edge[i*4] < 0 || edge[i*4] > 1 || edge[i*4+3] != 1)
        {
            throw std::runtime_error("SMAA edge output escaped normalized opaque color");
        }
        blended = blended || (edge[i*4] > 0.001f && edge[i*4] < 0.999f);
    }
    render(); // Reuse a completed slot while retaining its earlier output owner.
    if (!blended || read(constant) != original)
    {
        throw std::runtime_error("SMAA did not smooth the stair edge or overwrote retained output");
    }
    filter.cleanup();
    return true;
}
}
