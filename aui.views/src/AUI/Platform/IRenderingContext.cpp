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

//
// Created by Alex2772 on 12/12/2021.
//

#include <AUI/Render/Render.h>
#include "IRenderingContext.h"
#include <AUI/Platform/AWindow.h>

void IRenderingContext::init(const IRenderingContext::Init& init) {
    init.window.updateDpi();
}

void IRenderingContext::Init::setRenderingContext(_unique<IRenderingContext>&& context) const {
    window.mRenderingContext = std::forward<_unique<IRenderingContext>>(context);
}
