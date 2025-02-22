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

#include <AUI/View/AViewContainer.h>
#include <AUI/Util/kAUI.h>
#include <AUI/Traits/callables.h>
#include <AUI/Traits/parameter_pack.h>
#include <AUI/ASS/ASS.h>


namespace aui::ui_building {

    using View = _<AView>;
    using ViewContainer = _<AViewContainer>;
    using ViewGroup = AVector<_<AView>>;
    using ViewOrViewGroup = std::variant<_<AView>, AVector<_<AView>>>;

    template<typename ViewFactory>
    struct view_helper {
    public:
        view_helper(ViewFactory& factory): mFactory(factory) {}

        operator View() const {
            return mFactory();
        }
        operator ViewContainer() const {
            return mFactory();
        }
        auto operator<<(const AString& assEntry) const {
            return mFactory() << assEntry;
        }
        template<typename T>
        auto operator^(const T& t) const {
            return mFactory() ^ t;
        }
        template<typename T>
        auto operator+(const T& t) const {
            return mFactory() + t;
        }

        template<typename T>
        auto operator^(T&& t) const {
            return mFactory() ^ std::forward<T>(t);
        }
        template<typename T>
        auto operator&&(T&& t) const {
            return mFactory() && std::forward<T>(t);
        }
        template<typename T>
        auto operator+(T&& t) const {
            return mFactory() + std::forward<T>(t);
        }

        auto operator->() const {
            return mFactory();
        }


        template<typename SignalField, typename Object, typename Function>
        auto connect(SignalField&& signalField, Object&& object, Function&& function) {
            return mFactory().connect(std::forward<SignalField>(signalField), std::forward<Object>(object), std::forward<Function>(function));
        }

        template<typename Object, typename Function>
        auto clicked(Object&& object, Function&& function) {
            return connect(&AView::clicked, std::forward<Object>(object), std::forward<Function>(function));
        }

        template<typename SignalField, typename Function>
        auto connect(SignalField&& signalField, Function&& function) {
            return mFactory().connect(std::forward<SignalField>(signalField), std::forward<Function>(function));
        }

    private:
        ViewFactory& mFactory;
    };

    template<typename View>
    struct view: view_helper<view<View>> {

    public:
        template<typename... Args>
        view(Args&&... args): view_helper<view<View>>(*this), mView(_new<View>(std::forward<Args>(args)...)) {}

        _<View> operator()() {
            return std::move(mView);
        }

        operator _<View>() {
            return std::move(mView);
        }

    private:
        _<View> mView;
    };

    static_assert(std::is_convertible_v<view<AView>, View>, "declarative view wrapper is not convertible to _<AView>");

    template<typename Layout, aui::derived_from<AViewContainer> Container = AViewContainer>
    struct layouted_container_factory_impl {
    private:
        AVector<View> mViews;

    public:
        struct Expanding: layouted_container_factory_impl<Layout, Container>, view_helper<Expanding> {
        public:
            template<typename... Views>
            Expanding(Views&&... views): layouted_container_factory_impl<Layout>(std::forward<Views>(views)...),
                                         view_helper<Expanding>(*this) {

            }

            ViewContainer operator()() {
                return layouted_container_factory_impl<Layout>::operator()() let {
                    it->setExpanding();
                };
            }
        };

        template<typename... Views>
        layouted_container_factory_impl(Views&&... views) {
            mViews.reserve(sizeof...(views));
            aui::parameter_pack::for_each([this](auto&& item) {
                using type = decltype(item);
                constexpr bool isViewGroup = std::is_convertible_v<type, ViewGroup>;
                constexpr bool isView = std::is_convertible_v<type, View>;
                constexpr bool isInvokable = std::is_invocable_v<type>;

                static_assert(isViewGroup || isView || isInvokable, "the item is neither convertible to View nor ViewGroup, nor invokable");

                if constexpr (isViewGroup) {
                    auto asViewGroup = ViewGroup(item);
                    mViews << std::move(asViewGroup);
                } else if constexpr (isView) {
                    auto asView = View(item);
                    mViews << std::move(asView);
                } else if constexpr(isInvokable) {
                    mViews << item();
                }
            }, std::forward<Views>(views)...);
        }

        ViewContainer operator()() {
            auto c = _new<Container>();
            if constexpr(!std::is_same_v<Layout, std::nullopt_t>) {
                c->setLayout(_new<Layout>());
            }
            c->setViews(std::move(mViews));
            return c;
        }
    };


    template<typename Layout, aui::derived_from<AViewContainer> Container = AViewContainer>
    struct layouted_container_factory: layouted_container_factory_impl<Layout, Container>, view_helper<layouted_container_factory<Layout, Container>> {

        template<typename... Views>
        layouted_container_factory(Views&&... views): layouted_container_factory_impl<Layout, Container>(std::forward<Views>(views)...),
                                                      view_helper<layouted_container_factory<Layout, Container>>(*this) {

        }

    };
}

namespace declarative {

    /**
     * @brief Extra styles wrapper.
     * @ingroup declarative
     * @details
     * @code{cpp}
     * Button { "Default button" },
     * Style{
     *     {
     *         c(".btn"),
     *         BackgroundSolid { 0xff0000_rgb },
     *     },
     * } ({
     *     Button { "Red button" },
     *     Button { "Another red button" },
     * }),
     * @endcode
     */
    struct Style {
    public:
        Style(std::initializer_list<Rule> rules): mStylesheet(_new<AStylesheet>(AStylesheet(rules))) {

        }

        Style& operator()(AVector<_<AView>> views) {
            for (const auto& view : views) {
                assert(("extra stylesheet already specified", view->extraStylesheet() == nullptr));
                view->setExtraStylesheet(mStylesheet);
            }
            mViews = std::move(views);
            return *this;
        }

        operator AVector<_<AView>>() noexcept {
            return std::move(mViews);
        }

    private:
        _<AStylesheet> mStylesheet;
        AVector<_<AView>> mViews;
    };
}
