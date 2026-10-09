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
#include <bg2e/gpu/detail/SubmissionState.hpp>
#include <vector>

namespace bg2e {
namespace gpu {

class Image;
namespace vk { class Queue; class WindowSurface; class OffscreenSurface; }
namespace metal { class Queue; class WindowSurface; class OffscreenSurface; }

class BG2E_API SurfaceFrame {
public:
    virtual ~SurfaceFrame() = default;

    virtual gpu::Image* colorImage() const = 0;
    virtual gpu::Image* depthImage() const = 0;

    virtual bool isValid() const = 0;

private:
    friend class vk::Queue;
    friend class vk::WindowSurface;
    friend class vk::OffscreenSurface;
    friend class metal::Queue;
    friend class metal::WindowSurface;
    friend class metal::OffscreenSurface;
    // Backend-only lifetime tracking. Multiple sends may occupy a frame slot.
    void trackSubmission(const std::shared_ptr<detail::CompletionRecord>& record) { _submissions.push_back(record); }
    bool hasSubmissions() const { return !_submissions.empty(); }
    void waitForSubmissions() {
        std::exception_ptr error;
        for (const auto& record : _submissions) {
            try { record->wait(); }
            catch (...) { if (!error) error = std::current_exception(); }
        }
        _submissions.clear();
        if (error) std::rethrow_exception(error);
    }

private:
    std::vector<std::shared_ptr<detail::CompletionRecord>> _submissions;
};

}
}