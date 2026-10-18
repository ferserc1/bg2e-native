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

#pragma once
#include <bg2e/bake/all.hpp>
#include <bg2e/draw/SettingsCodec.hpp>
#include <limits>
#include <stdexcept>

namespace bg2e::bake::tests {
// CPU contract fixture; caller compiles/invokes from an existing temporary harness.
inline void runContractsProbe()
{
    auto settings = std::make_shared<LightmapSettings>();
    settings->validate();
    draw::SettingsCatalog catalog;
    catalog.reset("bake-contract-probe");
    catalog.add({"target",settings->settingsType(),settings->settingsInstance(),settings,{},{},{}});
    const auto sample = catalog.handle("target","samplesPerPixel");
    if (!catalog.apply({{sample,uint32_t(12)}}).applied || settings->samplesPerPixel() != 12)
    {
        throw std::runtime_error("Bake metadata must edit the actual settings object");
    }
    auto saved = draw::SettingsCodec::parse(draw::SettingsCodec::stringify(*draw::SettingsCodec::serialize(catalog)));
    settings->setSamplesPerPixel(1);
    if (!draw::SettingsCodec::validate(*saved,catalog).apply(catalog).applied || settings->samplesPerPixel() != 12)
    {
        throw std::runtime_error("Bake settings failed strict typed codec round trip");
    }
    auto reject = [](const auto& operation)
    {
        bool failed = false;
        try
        {
            operation();
        }
        catch (const std::exception&)
        {
            failed = true;
        }
        if (!failed)
        {
            throw std::runtime_error("Invalid bake contract unexpectedly accepted");
        }
    };
    reject([&]
    {
        settings->setResolution(0);
    });
    reject([&]
    {
        settings->setRayBias(std::numeric_limits<float>::quiet_NaN());
    });
    reject([&]
    {
        settings->setMode(static_cast<LightmapMode>(99));
    });
    settings->setRayBias(30);
    reject([&]
    {
        settings->validate();
    });
    settings->setRayBias(0.0005f);
    settings->setResolution(2);
    reject([&]
    {
        settings->validate();
    }); // Default dilation four exceeds extent.
    settings->setDilationPixels(1);
    settings->validate();
    LightmapPixels bytes{2,1,LightmapPixelFormat::RGB8,std::vector<uint8_t>{255,0,128,0,0,0}};
    bytes.validate();
    LightmapPixels hdr{1,1,LightmapPixelFormat::RGB32F,std::vector<float>{4,0,2}};
    hdr.validate(); // Above-one and valid black are not clipped or rejected.
    reject([&]
    {
        LightmapPixels{}.validate();
    });
    bytes.rgb = std::vector<uint8_t>{255};
    reject([&]
    {
        bytes.validate();
    });
    hdr.rgb = std::vector<uint8_t>{1,2,3};
    reject([&]
    {
        hdr.validate();
    });
    hdr.rgb = std::vector<float>{1,-1,2};
    reject([&]
    {
        hdr.validate();
    });
    hdr.rgb = std::vector<float>{1,std::numeric_limits<float>::infinity(),2};
    reject([&]
    {
        hdr.validate();
    });
    LightmapPixels huge;
    huge.width = huge.height = std::numeric_limits<uint32_t>::max();
    reject([&]
    {
        huge.elementCount();
    });
    reject([&]
    {
        BakerContextConfiguration{}.validate();
    });
    reject([&]
    {
        LightmapBakerConfiguration{}.validate();
    });
}
}
