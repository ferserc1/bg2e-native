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

#include <bg2e/gpu/detail/DeepDebug.hpp>
#include <bg2e/gpu/metal/MetalFXBridge.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#import <MetalFX/MetalFX.h>
#include "Diagnostics.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace bg2e::gpu::metal {

void logCommandBufferCompletion(MTL::CommandBuffer* command, uint64_t submission, uint64_t identity) noexcept
{
    if (!base::Log::deepDebugEnabled())
    {
        return;
    }
    @autoreleasepool
    {
        id<MTLCommandBuffer> native = reinterpret_cast<id<MTLCommandBuffer>>(command);
        base::Log::trace("metal", "submission.complete", [&](std::ostream& trace)
        {
            trace << "submission=" << submission << " command=" << identity << " status=" << native.status;
            NSError* error = native.error;
            if (error)
            {
                trace << " domain=" << (error.domain.UTF8String ?: "") << " code=" << error.code
                    << " description=" << (error.localizedDescription.UTF8String ?: "");
                NSArray<id<MTLCommandBufferEncoderInfo>>* encoders = error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
                const auto count = std::min<NSUInteger>(encoders.count, 16);
                for (NSUInteger index = 0; index < count; ++index)
                {
                    id<MTLCommandBufferEncoderInfo> encoder = encoders[index];
                    trace << " encoder[" << index << "]=" << (encoder.label.UTF8String ?: "")
                        << " state=" << encoder.errorState;
                    NSArray<NSString*>* signposts = encoder.debugSignposts;
                    const auto signpostCount = std::min<NSUInteger>(signposts.count, 8);
                    for (NSUInteger signpost = 0; signpost < signpostCount; ++signpost)
                    {
                        trace << " signpost=" << (signposts[signpost].UTF8String ?: "");
                    }
                    if (signpostCount < signposts.count)
                    {
                        trace << " [signposts truncated]";
                    }
                }
                if (count < encoders.count)
                {
                    trace << " [encoder list truncated]";
                }
            }
        });
    }
}

struct MetalFXBridge::State {
    const uint64_t generation = base::Log::nextDiagnosticIdentity();
    struct Slot {
        id<MTLTexture> color = nil, depth = nil, motion = nil, output = nil;
        std::shared_ptr<const detail::RecordingCompletion> recording;

        ~Slot()
        {
            [color release];
            [depth release];
            [motion release];
            [output release];
        }
    };
    gpu::Device* device = nullptr;
    id<MTLFXTemporalScaler> scaler = nil;
    Size2D internal, output;
    std::vector<std::unique_ptr<Slot>> slots;
    bool encodingFailed = false;
    std::shared_ptr<std::atomic_bool> failed = std::make_shared<std::atomic_bool>(false);

    ~State()
    {
        bg2e_deep_trace("metal", "metalfx.destroy", "generation=" << generation);
        [scaler release];
    }
};

namespace {
id<MTLTexture> makeTexture(id<MTLDevice> device, Size2D size, MTLPixelFormat format, MTLTextureUsage usage, const std::string& name)
{
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
        width:size.width height:size.height mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeTracked;
    descriptor.usage = usage;
    id<MTLTexture> result = [device newTextureWithDescriptor:descriptor];
    result.label = [NSString stringWithUTF8String:name.c_str()];
    return result;
}

id<MTLTexture> texture(gpu::Image& image, gpu::Device* device, Size2D size, gpu::PixelFormat format)
{
    auto* native = dynamic_cast<metal::Image*>(&image);
    if (!native || !native->isValid() || image.device() != device || image.size() != size ||
        image.pixelFormat() != format || image.imageType() != ImageType::Image2D ||
        image.arrayLayers() != 1 || image.mipLevels() != 1)
    {
        throw std::invalid_argument("MetalFX requires same-device, matching single-level 2D textures");
    }
    return reinterpret_cast<id<MTLTexture>>(native->texture());
}

void copy(id<MTLBlitCommandEncoder> encoder, id<MTLTexture> source, id<MTLTexture> destination, Size2D size)
{
    [encoder copyFromTexture:source sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
        sourceSize:MTLSizeMake(size.width,size.height,1) toTexture:destination destinationSlice:0
        destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
}
}

MetalFXBridge::MetalFXBridge() = default;

MetalFXBridge::~MetalFXBridge() = default;

bool MetalFXBridge::sdkAvailable()
{
    if (@available(macOS 13.0, *))
    {
        return NSClassFromString(@"MTLFXTemporalScalerDescriptor") != Nil;
    }
    return false;
}

bool MetalFXBridge::supportsDevice(const gpu::Device& device)
{
    const auto* native = dynamic_cast<const metal::Device*>(&device);
    if (!native || !native->isValid() || !sdkAvailable())
    {
        return false;
    }
    if (@available(macOS 13.0, *))
    {
        return [MTLFXTemporalScalerDescriptor supportsDevice:reinterpret_cast<id<MTLDevice>>(native->handle())];
    }
    return false;
}

bool MetalFXBridge::initialize(gpu::Device& device, Size2D internal, Size2D output, uint32_t frameSlots)
{
    cleanup();
    struct InitializationTrace {
        MetalFXBridge* bridge;

        ~InitializationTrace()
        {
            bg2e_deep_trace("metal", "metalfx.initialize.return", "status=" << bridge->status());
        }
    } initializationTrace{this};
    if (!supportsDevice(device))
    {
        _status = "MetalFX temporal scaling requires macOS 13+ and a supported Metal device; bilinear reconstruction active";
        return false;
    }
    if (internal.isZero() || output.isZero() || !frameSlots || internal.width > output.width || internal.height > output.height)
    {
        throw std::invalid_argument("MetalFX requires nonzero input/output extents, frame slots and no downscaling");
    }
    if (@available(macOS 13.0, *))
    {
        @autoreleasepool
        {
            @try
            {
                auto state = std::make_unique<State>();
                state->device = &device;
                state->internal = internal;
                state->output = output;
                bg2e_deep_trace("metal", "metalfx.initialize", "generation=" << state->generation
                    << " input=" << internal.width << 'x' << internal.height << " output=" << output.width << 'x' << output.height
                    << " slots=" << frameSlots << " autoExposure=1 inputContentProperties=0 formats=RGBA16F,R32F,RG16F,RGBA16F");
                id<MTLDevice> native = reinterpret_cast<id<MTLDevice>>(dynamic_cast<metal::Device&>(device).handle());
                // Scale is output / input. Older OS versions expose no limits;
                // admit the conservative 1x..2x range and still require creation to succeed.
                float minimum = 1.0f, maximum = 2.0f;
                if (@available(macOS 14.0, *))
                {
                    minimum = [MTLFXTemporalScalerDescriptor supportedInputContentMinScaleForDevice:native];
                    maximum = [MTLFXTemporalScalerDescriptor supportedInputContentMaxScaleForDevice:native];
                }
                const double x = double(output.width) / internal.width;
                const double y = double(output.height) / internal.height;
                if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum <= 0 || maximum < minimum ||
                    x < minimum || y < minimum || x > maximum || y > maximum)
                {
                    _status = "MetalFX requested scale (" + std::to_string(x) + "x, " + std::to_string(y) +
                        "x) is outside device limits [" + std::to_string(minimum) + ", " + std::to_string(maximum) +
                        "]; selected extent retained, unjittered bilinear reconstruction active";
                    return false;
                }
                MTLFXTemporalScalerDescriptor* descriptor = [[MTLFXTemporalScalerDescriptor alloc] init];
                descriptor.inputWidth = internal.width;
                descriptor.inputHeight = internal.height;
                descriptor.outputWidth = output.width;
                descriptor.outputHeight = output.height;
                descriptor.colorTextureFormat = MTLPixelFormatRGBA16Float;
                descriptor.depthTextureFormat = MTLPixelFormatR32Float;
                descriptor.motionTextureFormat = MTLPixelFormatRG16Float;
                descriptor.outputTextureFormat = MTLPixelFormatRGBA16Float;
                descriptor.autoExposureEnabled = YES;
                // Fixed extents; no dynamic input-content range is required.
                descriptor.inputContentPropertiesEnabled = NO;
                @try
                {
                    bg2e_deep_trace("metal", "metalfx.create.enter", "generation=" << state->generation);
                    state->scaler = [descriptor newTemporalScalerWithDevice:native];
                    bg2e_deep_trace("metal", "metalfx.create.return", "generation=" << state->generation << " valid=" << (state->scaler != nil));
                }
                @finally
                {
                    [descriptor release];
                }
                if (!state->scaler)
                {
                    _status = "MetalFX rejected the requested fixed extent/format configuration (including native AA); bilinear reconstruction active";
                    return false;
                }
                for (uint32_t index = 0; index < frameSlots; ++index)
                {
                    auto slot = std::make_unique<State::Slot>();
                    const auto prefix = "metalfx-generation-" + std::to_string(state->generation) + "-slot-" + std::to_string(index);
                    // Query actual native requirements instead of imposing them
                    // on every renderer-owned texture. All copies stay on GPU.
                    slot->color = makeTexture(native,internal,MTLPixelFormatRGBA16Float,state->scaler.colorTextureUsage,prefix + "-input-color");
                    slot->depth = makeTexture(native,internal,MTLPixelFormatR32Float,state->scaler.depthTextureUsage,prefix + "-input-depth");
                    slot->motion = makeTexture(native,internal,MTLPixelFormatRG16Float,state->scaler.motionTextureUsage,prefix + "-input-motion");
                    slot->output = makeTexture(native,output,MTLPixelFormatRGBA16Float,state->scaler.outputTextureUsage,prefix + "-output-color");
                    if (!slot->color || !slot->depth || !slot->motion || !slot->output)
                    {
                        _status = "MetalFX private texture allocation failed; bilinear reconstruction active";
                        return false;
                    }
                    bg2e_deep_trace("metal", "metalfx.slot.create", "generation=" << state->generation << " slot=" << index << " name=" << prefix);
                    state->slots.push_back(std::move(slot));
                }
                _state = std::move(state);
                _status = "MetalFX temporal reconstruction available: linear HDR, auto exposure, unjittered input-pixel motion";
                return true;
            }
            @catch (NSException* exception)
            {
                _status = std::string("MetalFX initialization failed: ") + ([[exception reason] UTF8String] ?: "Unknown native error") +
                    "; bilinear reconstruction active";
            }
        }
    }
    return false;
}

void MetalFXBridge::cleanup()
{
    _state.reset();
    _status = "Not initialized";
}

bool MetalFXBridge::available() const
{
    return _state && _state->scaler && !_state->encodingFailed && !_state->failed->load();
}

const std::string& MetalFXBridge::status() const
{
    static const std::string failed = "MetalFX command submission failed; bilinear reconstruction active until synchronized rebuild";
    return _state && _state->failed->load() ? failed : _status;
}

bool MetalFXBridge::record(const MetalFXDispatch& dispatch)
{
    if (!available())
    {
        return false;
    }
    auto* command = dynamic_cast<metal::CommandBuffer*>(&dispatch.command);
    const auto recording = dispatch.command.recordingCompletion();
    if (!command || !command->isValid() || !recording || command->hasActiveScope() ||
        dispatch.slot >= _state->slots.size() || !std::isfinite(dispatch.jitterX) || !std::isfinite(dispatch.jitterY))
    {
        throw std::invalid_argument("MetalFX requires a valid recording outside encoder scopes and a valid acquired slot/jitter");
    }
    const auto generation = _state->generation;
    const auto slotIndex = dispatch.slot;
    auto& slot = *_state->slots.at(dispatch.slot);
    bg2e_deep_trace("metal", "metalfx.slot.inspect", "generation=" << generation << " slot=" << slotIndex
        << " previousRecording=" << (slot.recording != nullptr) << " sameRecording=" << (slot.recording == recording));
    if (slot.recording && slot.recording != recording && !slot.recording->retired())
    {
        throw std::logic_error("MetalFX slot is still in flight");
    }
    const auto internal = _state->internal;
    const auto output = _state->output;
    id<MTLTexture> color = texture(dispatch.color,_state->device,internal,PixelFormat::R16G16B16A16_SFLOAT);
    id<MTLTexture> depth = texture(dispatch.depth,_state->device,internal,PixelFormat::R32_SFLOAT);
    id<MTLTexture> motion = texture(dispatch.motion,_state->device,internal,PixelFormat::R16G16_SFLOAT);
    id<MTLTexture> result = texture(dispatch.output,_state->device,output,PixelFormat::R16G16B16A16_SFLOAT);
    id<MTLCommandBuffer> native = reinterpret_cast<id<MTLCommandBuffer>>(command->handle());
    if ([native device] != [color device])
    {
        throw std::invalid_argument("MetalFX command buffer and textures must use the same device");
    }
    const auto commandIdentity = command->_diagnosticIdentity.value;
    slot.recording = recording;
    if (@available(macOS 13.0, *))
    {
        @autoreleasepool
        {
            @try
            {
                dispatch.command.transition(&dispatch.color,ImageLayout::TransferSrc);
                dispatch.command.transition(&dispatch.depth,ImageLayout::TransferSrc);
                dispatch.command.transition(&dispatch.motion,ImageLayout::TransferSrc);
                id<MTLBlitCommandEncoder> upload = [native blitCommandEncoder];
                if (!upload)
                {
                    _status = "MetalFX input copy encoder unavailable; bilinear reconstruction active";
                    _state->encodingFailed = true;
                    return false;
                }
                @try
                {
                    upload.label = @"metalfx-input-copy-encoder";
                    copy(upload,color,slot.color,internal);
                    copy(upload,depth,slot.depth,internal);
                    copy(upload,motion,slot.motion,internal);
                }
                @finally
                {
                    [upload endEncoding];
                }
                id<MTLFXTemporalScaler> scaler = _state->scaler;
                scaler.colorTexture = slot.color;
                scaler.depthTexture = slot.depth;
                scaler.motionTexture = slot.motion;
                scaler.outputTexture = slot.output;
                scaler.inputContentWidth = internal.width;
                scaler.inputContentHeight = internal.height;
                scaler.preExposure = 1.0f; // Input is linear HDR, with no pre-exposure multiplier.
                scaler.jitterOffsetX = dispatch.jitterX;
                scaler.jitterOffsetY = dispatch.jitterY;
                scaler.motionVectorScaleX = 1.0f;
                scaler.motionVectorScaleY = 1.0f;
                scaler.depthReversed = NO;
                scaler.reset = dispatch.reset;
                bg2e_deep_trace("metal", "metalfx.encode.enter", "generation=" << generation << " slot=" << slotIndex
                    << " command=" << commandIdentity << " reset=" << dispatch.reset << " jitter=" << dispatch.jitterX << ',' << dispatch.jitterY);
                [scaler encodeToCommandBuffer:native];
                bg2e_deep_trace("metal", "metalfx.encode.return", "generation=" << generation << " slot=" << slotIndex << " command=" << commandIdentity);
                dispatch.command.transition(&dispatch.output,ImageLayout::TransferDst);
                id<MTLBlitCommandEncoder> download = [native blitCommandEncoder];
                if (!download)
                {
                    _status = "MetalFX output copy encoder unavailable; bilinear reconstruction active";
                    _state->encodingFailed = true;
                    return false;
                }
                @try
                {
                    download.label = @"metalfx-output-copy-encoder";
                    copy(download,slot.output,result,output);
                }
                @finally
                {
                    [download endEncoding];
                }
                dispatch.command.transition(&dispatch.color,ImageLayout::ShaderReadOnly);
                dispatch.command.transition(&dispatch.depth,ImageLayout::ShaderReadOnly);
                dispatch.command.transition(&dispatch.motion,ImageLayout::ShaderReadOnly);
                dispatch.command.transition(&dispatch.output,ImageLayout::ShaderReadOnly);
                auto failed = _state->failed;
                [native addCompletedHandler:^(id<MTLCommandBuffer> completed)
                {
                    if (completed.status == MTLCommandBufferStatusError)
                    {
                        failed->store(true);
                        bg2e_deep_trace("metal", "metalfx.completion.failure", "generation=" << generation << " slot=" << slotIndex
                            << " command=" << commandIdentity << " domain=" << (completed.error.domain.UTF8String ?: "")
                            << " code=" << completed.error.code << " description=" << (completed.error.localizedDescription.UTF8String ?: ""));
                    }
                }];
                return true;
            }
            @catch (NSException* exception)
            {
                _status = std::string("MetalFX encoding failed: ") + ([[exception reason] UTF8String] ?: "Unknown native error");
                _state->encodingFailed = true;
                bg2e_deep_trace("metal", "metalfx.encode.exception", "generation=" << generation << " slot=" << slotIndex << " command=" << commandIdentity << " status=" << _status);
            }
        }
    }
    return false;
}
}
