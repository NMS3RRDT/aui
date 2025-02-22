// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <AUI/Traits/serializable.h>

namespace AStreams {
    inline void copyAll(aui::no_escape<IInputStream> is, aui::no_escape<IOutputStream> os, size_t bytes) {
        char buf[0x1000];
        while (bytes > 0) {
            size_t r = is->read(buf, (glm::min)(bytes, sizeof(buf)));
            if (r == 0) {
                throw AEOFException();
            }
            os->write(buf, r);
            bytes -= r;
        }
    }
}