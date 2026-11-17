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

#include <bg2e/base/Log.hpp>
#include <exception>

namespace bg2e::gpu::detail {
// Owned by backend objects. No allocation or identity work while disabled.
struct DiagnosticIdentity {
    uint64_t value = base::Log::deepDebugEnabled() ? base::Log::nextDiagnosticIdentity() : 0;
};

template <typename Result>
Result diagnosticResult(const char* backend, const char* operation, Result result) noexcept
{
    bg2e_deep_trace(backend, "native.result", "operation=" << operation << " result=" << result);
    return result;
}

class DiagnosticScope {
  public:
    DiagnosticScope(const char* backend, const char* event, uint64_t identity)
        : _backend(backend), _event(event), _identity(identity), _exceptions(base::Log::deepDebugEnabled() ? std::uncaught_exceptions() : 0)
    {
        bg2e_deep_trace(_backend, _event, "phase=enter object=" << _identity);
    }
    ~DiagnosticScope()
    {
        bg2e_deep_trace(_backend, _event, "phase=return object=" << _identity
            << " exception=" << (std::uncaught_exceptions() > _exceptions));
    }

  private:
    const char* _backend;
    const char* _event;
    uint64_t _identity;
    int _exceptions;
};
}
