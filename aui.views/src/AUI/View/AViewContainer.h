﻿//  AUI Framework - Declarative UI toolkit for modern C++20
//  Copyright (C) 2020-2023 Alex2772
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <AUI/Views.h>
#include <AUI/View/AView.h>
#include "AUI/Common/SharedPtr.h"
#include "AUI/Util/ABitField.h"
#include <glm/glm.hpp>
#include "AUI/Layout/ALayout.h"
#include "AUI/Common/AVector.h"
#include "AUI/Render/Render.h"
#include "AUI/Render/RenderHints.h"


AUI_ENUM_FLAG(AViewLookupFlags) {
    NONE = 0,

    /**
     * @brief On AViewContainer::getViewAt*, ignores visibility flags.
     */
    IGNORE_VISIBILITY = 0b1,

    /**
     * @brief On AViewContainer::getViewAt* with callbacks, only the first view in the container is passed to callback.
     * @details
     * Basically this flag allows to replicate mouse click handling behaviour, which is useful in creating custom events
     * (i.e. drag&drop).
     */
    ONLY_ONE_PER_CONTAINER = 0b10,
};

/**
 * @brief A view that represents a set of views.
 * @ingroup useful_views
 * @details
 * AViewContainer can store, render, resize, provide events to and handle the child views.
 *
 * AViewContainer does not control the position and size of the child views by itself; instead, it delegates that
 * responsibility to it's @ref layout_managers "layout manager".
 *
 * Since AViewContainer is an instance of AView, AViewContainer can handle AViewContainers recursively, thus, making
 * possible complex UI by nested AViewContainers with different layout managers.
 */
class API_AUI_VIEWS AViewContainer : public AView {
public:
    AViewContainer();

    virtual ~AViewContainer();

    void setViews(AVector<_<AView>> views) {
        views.removeIf([](const _<AView>& v) {
            return v == nullptr;
        });
        mViews = std::move(views);

        for (const auto& view: mViews) {
            view->mParent = this;
            if (mLayout)
                mLayout->addView(-1, view);
        }
    }

    void addViews(AVector<_<AView>> views);
    void addView(const _<AView>& view);
    void addView(size_t index, const _<AView>& view);

    void removeView(const _<AView>& view);
    void removeView(AView* view);
    void removeView(size_t index);
    void removeAllViews();

    void render() override;

    void onMouseEnter() override;

    void onPointerMove(glm::ivec2 pos) override;

    void onMouseLeave() override;

    void onDpiChanged() override;

    void onClickPrevented() override;

    int getContentMinimumWidth(ALayoutDirection layout) override;

    int getContentMinimumHeight(ALayoutDirection layout) override;

    void onPointerPressed(const APointerPressedEvent& event) override;

    void onPointerDoubleClicked(const APointerPressedEvent& event) override;

    void onPointerReleased(const APointerReleasedEvent& event) override;

    void onScroll(const AScrollEvent& event) override;

    bool onGesture(const glm::ivec2& origin, const AGestureEvent& event) override;

    bool consumesClick(const glm::ivec2& pos) override;

    void setSize(glm::ivec2 size) override;

    void setEnabled(bool enabled = true) override;

    void adjustContentSize();

    void adjustHorizontalSizeToContent();

    void adjustVerticalSizeToContent();

    auto begin() const {
        return mViews.cbegin();
    }

    auto end() const {
        return mViews.cend();
    }

    /**
     * @brief Set new layout manager for this AViewContainer. DESTROYS OLD LAYOUT MANAGER WITH ITS VIEWS!!!
     */
    void setLayout(_<ALayout> layout);

    _<ALayout> getLayout() const;

    /**
     * @brief Finds first direct child view under position.
     * @param pos position relative to this container
     * @param flags see AViewLookupFlags
     * @return found view or nullptr
     * @details
     * Some containers may implement getViewAt by it's own (i.e. AListView for performance reasons).
     */
    [[nodiscard]]
    virtual _<AView> getViewAt(glm::ivec2 pos, ABitField<AViewLookupFlags> flags = AViewLookupFlags::NONE) const noexcept;

    /**
     * @brief Acts as AViewContainer::getViewAt but recursively (may include non-direct child).
     * @param pos position relative to this container
     * @param flags see AViewLookupFlags
     * @return found view or nullptr
     */
    [[nodiscard]]
    _<AView> getViewAtRecursive(glm::ivec2 pos, ABitField<AViewLookupFlags> flags = AViewLookupFlags::NONE) const noexcept;

    /**
     * @brief Acts as AViewContainer::getViewAtRecursive but calls a callback instead of returning value.
     * @param pos position relative to this container
     * @param flags see AViewLookupFlags
     * @return true if callback returned true; false otherwise
     * @details
     * The passed callback is a predicate. If predicate returns true, the execution of lookup is stopped and getViewAt
     * returns true. If predicate returns false, the lookup continues.
     */
    template<aui::predicate<_<AView>> Callback>
    bool getViewAtRecursive(glm::ivec2 pos, const Callback& callback, ABitField<AViewLookupFlags> flags = AViewLookupFlags::NONE) {
        _<AView> possibleOutput; // for case if anyone does not consumesClick
        auto process = [&](const _<AView>& view) {
            if (callback(view))
                return true;
            if (auto container = _cast<AViewContainer>(view)) {
                if (container->getViewAtRecursive(pos - view->getPosition(), callback, flags)) {
                    return true;
                }
            }
            return false;
        };

        for (const auto& view : aui::reverse_iterator_wrap(mViews)) {
            auto targetPos = pos - view->getPosition();

            if (targetPos.x < 0 || targetPos.y < 0 || targetPos.x >= view->getSize().x || targetPos.y >= view->getSize().y) {
                continue;
            }
            if (!flags.test(AViewLookupFlags::IGNORE_VISIBILITY)) {
                if (view->getVisibility() == Visibility::GONE ||
                    view->getVisibility() == Visibility::UNREACHABLE) {
                    continue;
                }
            }

            if (view->consumesClick(targetPos)) {
                if (flags.test(AViewLookupFlags::IGNORE_VISIBILITY) || (view->getVisibility() != Visibility::GONE &&
                                                                        view->getVisibility() !=
                                                                        Visibility::UNREACHABLE)) {
                    if (process(view)) {
                        return true;
                    }

                    if (flags.test(AViewLookupFlags::ONLY_ONE_PER_CONTAINER)) {
                        return false;
                    }
                }
            } else {
                if (possibleOutput == nullptr) {
                    possibleOutput = view;
                }
            }
        }
        if (possibleOutput) {
            return process(possibleOutput);
        }

        return false;
    }

    /**
     * @brief
     * @param pos position relative to this container
     * @param flags see AViewLookupFlags
     * @return found view or nullptr
     */
    template<aui::predicate<_<AView>> Callback>
    bool visitsViewRecursive(Callback&& callback, ABitField<AViewLookupFlags> flags = AViewLookupFlags::NONE) {
        for (auto it = mViews.rbegin(); it != mViews.rend(); ++it) {
            auto view = *it;
            if (flags.test(AViewLookupFlags::IGNORE_VISIBILITY) || (view->getVisibility() != Visibility::GONE && view->getVisibility() != Visibility::UNREACHABLE)) {
                if (callback(view))
                    return true;
                if (auto container = _cast<AViewContainer>(view)) {
                    if (container->visitsViewRecursive(callback, flags)) {
                        return true;
                    }
                }
                if (flags.test(AViewLookupFlags::ONLY_ONE_PER_CONTAINER)) {
                    break;
                }
            }
        }
        return false;
    }


    /**
     * @brief Acts as AViewContainer::getViewAtRecursive but finds a view castable to specified template type.
     * @param pos position relative to this container
     * @param flags see AViewLookupFlags
     * @return found view or nullptr
     */
    template<typename T>
    _<T> getViewAtRecursiveOfType(glm::ivec2 pos, ABitField<AViewLookupFlags> flags = AViewLookupFlags::NONE) {
        _<T> result;
        getViewAtRecursive(pos, [&] (const _<AView>& v) { return bool(result = _cast<T>(v)); }, flags);
        return result;
    }


    /**
     * @brief Set focus chain target.
     * @param target a new focus chain view belonging to this container
     * @details
     * See mFocusChainTarget for further info.
     */
    void setFocusChainTarget(_weak<AView> target) {
        if (auto v = target.lock()) {
            assert(v->mParent == this);
        }
        mFocusChainTarget = std::move(target);
    }
    /**
     * @return Focus chain target.
     * @details
     * See mFocusChainTarget for further info.
     */
    _<AView> focusChainTarget() {
        if (auto v = mFocusChainTarget.lock()) {
            if (v->mParent != this) {
                mFocusChainTarget.reset();
                return nullptr;
            }
            return v;
        }
        return nullptr;
    }


    virtual void updateLayout();

    const AVector<_<AView>>& getViews() const {
        return mViews;
    }

    void addViewCustomLayout(const _<AView>& view);

    void onKeyDown(AInput::Key key) override;

    void onKeyRepeat(AInput::Key key) override;

    void onKeyUp(AInput::Key key) override;

    void onCharEntered(wchar_t c) override;

    bool capturesFocus() override;

    virtual void setScrollbarAppearance(ScrollbarAppearance scrollbarAppearance) {
        mScrollbarAppearance = scrollbarAppearance;
        emit scrollbarAppearanceSet(scrollbarAppearance);
    }

protected:
    AVector<_<AView>> mViews;
    ScrollbarAppearance mScrollbarAppearance;

    void drawView(const _<AView>& view);

    template<typename Iterator>
    void drawViews(Iterator begin, Iterator end) {
        for (auto i = begin; i != end; ++i) {
            drawView(*i);
        }
    }

    void invalidateAllStyles() override;

    void invalidateAssHelper() override;

    /**
     * @brief Updates layout of the parent AViewContainer if size of this AViewContainer was changed.
     */
    virtual void updateParentsLayoutIfNecessary();

    /**
     * @brief Moves (like via std::move) all children and layout of the specified container to this container.
     * @param container container. Must be pure AViewContainer (cannot be a derivative from AViewContainer).
     * @note If access to this function is restricted or you want to pass an object derived from AViewContainer, you
     * should use ALayoutInflater::inflate instead.
     */
    void setContents(const _<AViewContainer>& container);

signals:
    emits<ScrollbarAppearance> scrollbarAppearanceSet;

private:
    _<ALayout> mLayout;
    bool mSizeSet = false;
    glm::ivec2 mPreviousSize = mSize;

    void notifyParentEnabledStateChanged(bool enabled) override;

    /**
     * @brief Focus chain target.
     * @details
     * Focus chain target is a view belonging to this container which focus-aware (i.e. keyboard) events are passed to.
     * The focus chaining mechanism allows to catch such events and process them in the containers.
     */
    _weak<AView> mFocusChainTarget;
};
