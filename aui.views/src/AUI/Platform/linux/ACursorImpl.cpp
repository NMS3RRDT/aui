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

#include <AUI/Platform/ACursor.h>
#include "AUI/Traits/callables.h"
#include "AUI/Image/IDrawable.h"
#include "AUI/Platform/AWindow.h"
#include "AUI/Platform/CommonRenderingContext.h"
#include "AUI/Util/kAUI.h"
#include "AUI/Util/ACleanup.h"
#include <AUI/Util/ARaiiHelper.h>
#include <AUI/Logging/ALogger.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>

namespace {
    class NativeCursorHandle: public aui::noncopyable {
    public:
        NativeCursorHandle(Cursor c): mHandle(c) {

        }

        ~NativeCursorHandle() {
            XFreeCursor(CommonRenderingContext::ourDisplay, mHandle);
        }

        [[nodiscard]]
        Cursor handle() const noexcept {
            return mHandle;
        }

    private:
        Cursor mHandle;
    };


    void setCursor(AWindow* window, Cursor cursorHandle) {
        static Cursor prevCursor = -1;
        if (prevCursor == cursorHandle) return;
        prevCursor = cursorHandle;

        XDefineCursor(CommonRenderingContext::ourDisplay, window->getNativeHandle(), cursorHandle);
    }

    void setFontCursor(AWindow* window, int cursor) {
        static std::unordered_map<int, NativeCursorHandle> nativeCursors;
        do_once {
            ACleanup::afterEntry([&] {
                nativeCursors.clear();
            });
        }

        auto it = nativeCursors.find(cursor);
        if (it == nativeCursors.end()) {
            it = nativeCursors.emplace(cursor, XCreateFontCursor(CommonRenderingContext::ourDisplay, cursor)).first;
        }
        setCursor(window, it->second.handle());
    }
}

struct ACursor::Custom: public NativeCursorHandle{
public:
    Custom(AImageView img): NativeCursorHandle([&]() -> Cursor {
        auto image = XcursorImageCreate(img.width(), img.height());
        if (image == nullptr) {
            ALogger::err("XCursor") << "XcursorImageCreate failed";
            return None;
        }
        image->xhot = 0;
        image->yhot = 0;
        image->delay = 0;
        static constexpr auto X_FORMAT = APixelFormat::BGRA | APixelFormat::BYTE;
        AFormattedImageView<X_FORMAT> destination(AByteBufferView(reinterpret_cast<const char*>(image->pixels), img.buffer().size()),
                                                  img.size());

        img.visit([&](const auto& source) {
            using source_image_t      = std::decay_t<decltype(source)>;
            using destination_image_t = std::decay_t<decltype(destination)>;

            static constexpr auto sourceFormat      = (APixelFormat::Value)source_image_t::FORMAT;
            static constexpr auto destinationFormat = (APixelFormat::Value)destination_image_t::FORMAT;

            std::transform(source.begin(), source.end(),
                           const_cast<destination_image_t::Color*>(destination.begin()),
                           aui::pixel_format::convert<sourceFormat, destinationFormat>);
        });


        auto cursor = XcursorImageLoadCursor(CommonRenderingContext::ourDisplay, image);
        XcursorImageDestroy(image);

        return cursor;
    }()) {
    }

    ~Custom() {
    }

private:
};


ACursor::ACursor(aui::no_escape<AImage> image, int size) : mValue(std::make_unique<ACursor::Custom>(*image)), mSize(size) {}

void ACursor::applyNativeCursor(AWindow* pWindow) const {

    std::visit(aui::lambda_overloaded {
            [&](System s) {
                switch (s) {
                    // https://tronche.com/gui/x/xlib/appendix/b/
                    default: {
                        setFontCursor(pWindow, XC_arrow);
                        break;
                    }
                    case ACursor::POINTER: {
                        setFontCursor(pWindow, XC_hand2);
                        break;
                    }
                    case ACursor::TEXT: {
                        setFontCursor(pWindow, XC_xterm);
                        break;
                    }
                }
            },
            [&](const _<Custom>& custom) {
                setCursor(pWindow, custom->handle());
            },
            [&](const _<IDrawable>& drawable) {
                static AMap<_<IDrawable>, AMap<int, _<Custom>>> cache;

                auto custom = cache[drawable].getOrInsert(int(pWindow->getDpiRatio() * 10), [&] {
                    return _new<Custom>(drawable->rasterize(glm::ivec2(mSize * pWindow->getDpiRatio())));
                });

                setCursor(pWindow, custom->handle());
            }
    }, mValue);
}
