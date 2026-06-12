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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/ShaderModule.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>

namespace bg2e {
namespace gpu {

struct ComputePipelineDescription {
    gpu::ShaderModule*   computeShader = nullptr; // non-owning; stage must be Compute
    gpu::PipelineLayout* layout        = nullptr; // non-owning; caller must keep alive
    std::string          debugName;    // Debug label for validation layers / Xcode GPU Capture
};

class BG2E_API ComputePipeline {
public:
    virtual ~ComputePipeline() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};

}
}
