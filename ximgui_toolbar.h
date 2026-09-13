#ifndef XIMGUI_TOOLBAR_HOST_H
#define XIMGUI_TOOLBAR_HOST_H
#pragma once

#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

namespace ximgui::toolbar
{
    enum class axis
    {
        Horizontal,
        Vertical
    };

    enum class toolbar_host_edge
    {
        Top,
        Right,
        Bottom,
        Left,
        Floating
    };

    struct toolbar_host_item
    {
        const char*         m_Name = nullptr;
        toolbar_host_edge   m_Edge = toolbar_host_edge::Top;
        axis                 m_LastFloatingAxis = axis::Horizontal;
        ImVec2               m_HorizontalSize = ImVec2(480.0f, 34.0f);
        ImVec2               m_VerticalSize = ImVec2(42.0f, 240.0f);
        ImVec2               m_FloatingPos = ImVec2(24.0f, 24.0f);
        ImVec2               m_FloatingAnchorOffset = ImVec2(24.0f, 24.0f);
        ImVec2               m_DragOffset = ImVec2(0.0f, 0.0f);
        bool                 m_bFloatingAnchorRight = false;
        bool                 m_bFloatingAnchorBottom = false;
        bool                 m_bEndStack = false;
        bool                 m_bDragging = false;
        int                  m_Order = 0;
        bool                 m_bDropOrderPending = false;
        float                m_DropCoordinate = 0.0f;
        ImVec2               m_LastScreenPos = ImVec2(0.0f, 0.0f);
        ImVec2               m_LastSize = ImVec2(0.0f, 0.0f);
    };

    struct toolbar_host_state
    {
        std::vector<toolbar_host_item> m_Items;
        bool m_bInitialized = false;
    };

    inline bool IsHorizontalToolbarEdge(toolbar_host_edge Edge) noexcept
    {
        return Edge == toolbar_host_edge::Top || Edge == toolbar_host_edge::Bottom;
    }

    inline axis ToolbarAxisForEdge(const toolbar_host_item& Item) noexcept
    {
        if (Item.m_Edge == toolbar_host_edge::Floating)
            return Item.m_LastFloatingAxis;
        return IsHorizontalToolbarEdge(Item.m_Edge) ? axis::Horizontal : axis::Vertical;
    }

    inline ImVec2 ToolbarSizeForEdge(const toolbar_host_item& Item) noexcept
    {
        return ToolbarAxisForEdge(Item) == axis::Horizontal ? Item.m_HorizontalSize : Item.m_VerticalSize;
    }

    inline ImVec2 ToolbarSizeForPlacement
    ( const toolbar_host_item& Item
    , toolbar_host_edge Edge
    ) noexcept
    {
        return IsHorizontalToolbarEdge(Edge) ? Item.m_HorizontalSize : Item.m_VerticalSize;
    }

    inline toolbar_host_edge ChooseDropEdge
    ( const toolbar_host_item& Item
    , float LeftDistance
    , float RightDistance
    , float TopDistance
    , float BottomDistance
    , float SnapDistance
    ) noexcept
    {
        const float Closest = ImMin(ImMin(LeftDistance, RightDistance), ImMin(TopDistance, BottomDistance));
        if (Closest > SnapDistance)
            return toolbar_host_edge::Floating;

        // At a corner two edges have the same distance. Keep the toolbar's current orientation
        // in that tie: horizontal toolbars claim top/bottom corners, vertical toolbars claim
        // left/right corners.
        if (Item.m_LastFloatingAxis == axis::Horizontal)
        {
            if (Closest == TopDistance)    return toolbar_host_edge::Top;
            if (Closest == BottomDistance) return toolbar_host_edge::Bottom;
            if (Closest == LeftDistance)   return toolbar_host_edge::Left;
            return toolbar_host_edge::Right;
        }

        if (Closest == LeftDistance)   return toolbar_host_edge::Left;
        if (Closest == RightDistance)  return toolbar_host_edge::Right;
        if (Closest == TopDistance)    return toolbar_host_edge::Top;
        return toolbar_host_edge::Bottom;
    }

    inline void ClampFloatingToolbar(toolbar_host_item& Item, const ImVec2& HostSize) noexcept
    {
        const ImVec2 Size = ToolbarSizeForEdge(Item);
        const float MaxX = ImMax(0.0f, HostSize.x - Size.x);
        const float MaxY = ImMax(0.0f, HostSize.y - Size.y);
        Item.m_FloatingPos.x = ImClamp(Item.m_FloatingPos.x, 0.0f, MaxX);
        Item.m_FloatingPos.y = ImClamp(Item.m_FloatingPos.y, 0.0f, MaxY);
    }

    inline void CaptureFloatingAnchor(toolbar_host_item& Item, const ImVec2& HostSize) noexcept
    {
        const ImVec2 Size = ToolbarSizeForEdge(Item);
        const float LeftOffset = ImMax(0.0f, Item.m_FloatingPos.x);
        const float RightOffset = ImMax(0.0f, HostSize.x - (Item.m_FloatingPos.x + Size.x));
        const float TopOffset = ImMax(0.0f, Item.m_FloatingPos.y);
        const float BottomOffset = ImMax(0.0f, HostSize.y - (Item.m_FloatingPos.y + Size.y));

        Item.m_bFloatingAnchorRight = RightOffset < LeftOffset;
        Item.m_bFloatingAnchorBottom = BottomOffset < TopOffset;
        Item.m_FloatingAnchorOffset.x = Item.m_bFloatingAnchorRight ? RightOffset : LeftOffset;
        Item.m_FloatingAnchorOffset.y = Item.m_bFloatingAnchorBottom ? BottomOffset : TopOffset;
    }

    inline void ApplyFloatingAnchor(toolbar_host_item& Item, const ImVec2& HostSize) noexcept
    {
        const ImVec2 Size = ToolbarSizeForEdge(Item);
        Item.m_FloatingPos.x = Item.m_bFloatingAnchorRight
            ? HostSize.x - Size.x - Item.m_FloatingAnchorOffset.x
            : Item.m_FloatingAnchorOffset.x;
        Item.m_FloatingPos.y = Item.m_bFloatingAnchorBottom
            ? HostSize.y - Size.y - Item.m_FloatingAnchorOffset.y
            : Item.m_FloatingAnchorOffset.y;
        ClampFloatingToolbar(Item, HostSize);
    }

    inline void UpdateToolbarDrag
    ( toolbar_host_item& Item
    , const ImVec2& HostOrigin
    , const ImVec2& HostSize
    , const ImVec2& ToolbarPos
    ) noexcept
    {
        ImGuiIO& IO = ImGui::GetIO();
        if (ImGui::IsItemActivated())
        {
            Item.m_bDragging = true;
            Item.m_Edge = toolbar_host_edge::Floating;
            Item.m_DragOffset = ImVec2
            ( IO.MousePos.x - ToolbarPos.x
            , IO.MousePos.y - ToolbarPos.y
            );
        }

        if (!Item.m_bDragging)
            return;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            Item.m_FloatingPos = ImVec2
            ( IO.MousePos.x - HostOrigin.x - Item.m_DragOffset.x
            , IO.MousePos.y - HostOrigin.y - Item.m_DragOffset.y
            );
            ClampFloatingToolbar(Item, HostSize);
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const ImVec2 FloatingPos
            ( HostOrigin.x + Item.m_FloatingPos.x
            , HostOrigin.y + Item.m_FloatingPos.y
            );
            const ImVec2 Size = ToolbarSizeForEdge(Item);
            const ImVec2 HostMax
            ( HostOrigin.x + HostSize.x
            , HostOrigin.y + HostSize.y
            );
            const float LeftDistance   = ImAbs(FloatingPos.x - HostOrigin.x);
            const float RightDistance  = ImAbs(HostMax.x - (FloatingPos.x + Size.x));
            const float TopDistance    = ImAbs(FloatingPos.y - HostOrigin.y);
            const float BottomDistance = ImAbs(HostMax.y - (FloatingPos.y + Size.y));
            const float SnapDistance = 0.0f;
            const toolbar_host_edge DropEdge = ChooseDropEdge
            ( Item
            , LeftDistance, RightDistance, TopDistance, BottomDistance
            , SnapDistance
            );

            if (DropEdge != toolbar_host_edge::Floating)
            {
                Item.m_Edge = DropEdge;
                Item.m_LastFloatingAxis = ToolbarAxisForEdge(Item);
                Item.m_bDropOrderPending = true;
                Item.m_DropCoordinate = IsHorizontalToolbarEdge(Item.m_Edge)
                    ? IO.MousePos.x : IO.MousePos.y;
                const float EdgeMidpoint = IsHorizontalToolbarEdge(Item.m_Edge)
                    ? HostOrigin.x + HostSize.x * 0.5f
                    : HostOrigin.y + HostSize.y * 0.5f;
                Item.m_bEndStack = Item.m_DropCoordinate > EdgeMidpoint;
            }
            else
            {
                Item.m_Edge = toolbar_host_edge::Floating;
                ClampFloatingToolbar(Item, HostSize);
                CaptureFloatingAnchor(Item, HostSize);
            }
            Item.m_bDragging = false;
        }
    }

    template<typename ToolbarRenderer, typename BodyRenderer>
    inline void RenderToolbarHost
    ( toolbar_host_state& Host
    , const ImVec2& HostSize
    , ToolbarRenderer&& RenderToolbar
    , BodyRenderer&& RenderBody
    ) noexcept
    {
        if (!Host.m_bInitialized)
        {
            Host.m_bInitialized = true;
            for (std::size_t Index = 0; Index < Host.m_Items.size(); ++Index)
            {
                auto& Item = Host.m_Items[Index];
                Item.m_FloatingPos = ImVec2(24.0f, 24.0f);
                Item.m_Order = static_cast<int>(Index);
            }
        }

        std::vector<int> RenderOrder;
        RenderOrder.reserve(Host.m_Items.size());
        for (std::size_t Index = 0; Index < Host.m_Items.size(); ++Index)
            RenderOrder.push_back(static_cast<int>(Index));
        std::sort(RenderOrder.begin(), RenderOrder.end(), [&](int Left, int Right)
        {
            return Host.m_Items[Left].m_Order < Host.m_Items[Right].m_Order;
        });

        const ImVec2 HostOrigin = ImGui::GetCursorScreenPos();
        const float HostWidth = ImMax(1.0f, HostSize.x);
        const float HostHeight = ImMax(1.0f, HostSize.y);

        float TopHeight = 0.0f;
        float BottomHeight = 0.0f;
        float LeftWidth = 0.0f;
        float RightWidth = 0.0f;
        float TopStartWidth = 0.0f;
        float TopEndWidth = 0.0f;
        float BottomStartWidth = 0.0f;
        float BottomEndWidth = 0.0f;
        float LeftStartHeight = 0.0f;
        float LeftEndHeight = 0.0f;
        float RightStartHeight = 0.0f;
        float RightEndHeight = 0.0f;
        for (const auto& Item : Host.m_Items)
        {
            const ImVec2 Size = ToolbarSizeForEdge(Item);
            switch (Item.m_Edge)
            {
            case toolbar_host_edge::Top:
                TopHeight = ImMax(TopHeight, Size.y);
                (Item.m_bEndStack ? TopEndWidth : TopStartWidth) += Size.x;
                break;
            case toolbar_host_edge::Bottom:
                BottomHeight = ImMax(BottomHeight, Size.y);
                (Item.m_bEndStack ? BottomEndWidth : BottomStartWidth) += Size.x;
                break;
            case toolbar_host_edge::Left:
                LeftWidth = ImMax(LeftWidth, Size.x);
                (Item.m_bEndStack ? LeftEndHeight : LeftStartHeight) += Size.y;
                break;
            case toolbar_host_edge::Right:
                RightWidth = ImMax(RightWidth, Size.x);
                (Item.m_bEndStack ? RightEndHeight : RightStartHeight) += Size.y;
                break;
            case toolbar_host_edge::Floating: break;
            }
        }

        auto RenderOne = [&](toolbar_host_item& Item, const ImVec2& Position, const ImVec2& Size)
        {
            const axis Axis = ToolbarAxisForEdge(Item);
            Item.m_LastScreenPos = Position;
            Item.m_LastSize = Size;
            ImGui::SetCursorScreenPos(Position);
            ImGui::PushStyleVar
            ( ImGuiStyleVar_WindowPadding
            , Axis == axis::Horizontal ? ImVec2(4.0f, 2.0f) : ImVec2(0.0f, 2.0f)
            );
            const std::string ChildId = std::string("##Toolbar_") + Item.m_Name;
            if (ImGui::BeginChild(ChildId.c_str(), Size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
            {
                const ImVec2 ToolbarPos = ImGui::GetWindowPos();
                const float GripThickness = 11.0f;
                const ImVec2 GripSize = Axis == axis::Horizontal
                    ? ImVec2(GripThickness, ImMax(1.0f, Size.y - 4.0f))
                    : ImVec2(ImMin(32.0f, ImMax(1.0f, Size.x - 2.0f)), GripThickness);
                const ImVec2 GripPos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##Grip", GripSize);
                const bool bGripHovered = ImGui::IsItemHovered();
                UpdateToolbarDrag(Item, HostOrigin, HostSize, ToolbarPos);
                if (bGripHovered)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.70f);
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted("Drag toolbar");
                    ImGui::EndTooltip();
                    ImGui::PopFont();
                }

                ImDrawList* pDrawList = ImGui::GetWindowDrawList();
                const ImU32 GripBackground = ImGui::GetColorU32
                ( Item.m_bDragging ? ImGuiCol_ButtonActive
                : bGripHovered ? ImGuiCol_ButtonHovered
                : ImGuiCol_FrameBg
                );
                pDrawList->AddRectFilled
                ( GripPos
                , ImVec2(GripPos.x + GripSize.x, GripPos.y + GripSize.y)
                , GripBackground
                , ImGui::GetStyle().FrameRounding
                );
                const ImU32 GripDots = ImGui::GetColorU32(ImGuiCol_Text);
                if (Axis == axis::Horizontal)
                {
                    for (int Row = 0; Row < 3; ++Row)
                        for (int Column = 0; Column < 2; ++Column)
                        {
                            const float X = GripPos.x + GripSize.x * 0.5f - 3.0f + Column * 6.0f;
                            const float Y = GripPos.y + GripSize.y * 0.5f - 4.0f + Row * 4.0f;
                            pDrawList->AddCircleFilled(ImVec2(X, Y), 0.9f, GripDots);
                        }
                }
                else
                {
                    for (int Row = 0; Row < 2; ++Row)
                        for (int Column = 0; Column < 3; ++Column)
                        {
                            const float X = GripPos.x + GripSize.x * 0.5f - 4.0f + Column * 4.0f;
                            const float Y = GripPos.y + GripSize.y * 0.5f - 3.0f + Row * 6.0f;
                            pDrawList->AddCircleFilled(ImVec2(X, Y), 0.9f, GripDots);
                        }
                }
                if (Axis == axis::Horizontal)
                    ImGui::SameLine();
                RenderToolbar(Item.m_Name, Axis);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        };

        const ImVec2 BodyPos
        ( HostOrigin.x + LeftWidth
        , HostOrigin.y + TopHeight
        );
        const ImVec2 BodySize
        ( ImMax(1.0f, HostWidth - LeftWidth - RightWidth)
        , ImMax(1.0f, HostHeight - TopHeight - BottomHeight)
        );
        // Submit the host body first. Floating toolbar children are rendered afterward so they
        // remain the topmost mouse target instead of being covered by this transparent-looking
        // content child.
        ImGui::SetCursorScreenPos(BodyPos);
        if (ImGui::BeginChild("##E29ToolbarHostBody", BodySize, false, ImGuiWindowFlags_NoScrollbar))
        {
            RenderBody();
        }
        ImGui::EndChild();

        float TopX = HostOrigin.x;
        float TopEndX = HostOrigin.x + HostWidth - TopEndWidth;
        float BottomX = HostOrigin.x;
        float BottomEndX = HostOrigin.x + HostWidth - BottomEndWidth;
        float LeftY = HostOrigin.y + TopHeight;
        float LeftEndY = HostOrigin.y + HostHeight - BottomHeight - LeftEndHeight;
        float RightY = HostOrigin.y + TopHeight;
        float RightEndY = HostOrigin.y + HostHeight - BottomHeight - RightEndHeight;
        for (int Index : RenderOrder)
        {
            auto& Item = Host.m_Items[Index];
            const ImVec2 Size = ToolbarSizeForEdge(Item);
            switch (Item.m_Edge)
            {
            case toolbar_host_edge::Top:
                if (Item.m_bEndStack)
                {
                    RenderOne(Item, ImVec2(TopEndX, HostOrigin.y), Size);
                    TopEndX += Size.x;
                }
                else
                {
                    RenderOne(Item, ImVec2(TopX, HostOrigin.y), Size);
                    TopX += Size.x;
                }
                break;
            case toolbar_host_edge::Bottom:
                if (Item.m_bEndStack)
                {
                    RenderOne(Item, ImVec2(BottomEndX, HostOrigin.y + HostHeight - Size.y), Size);
                    BottomEndX += Size.x;
                }
                else
                {
                    RenderOne(Item, ImVec2(BottomX, HostOrigin.y + HostHeight - Size.y), Size);
                    BottomX += Size.x;
                }
                break;
            case toolbar_host_edge::Left:
                if (Item.m_bEndStack)
                {
                    RenderOne(Item, ImVec2(HostOrigin.x, LeftEndY), Size);
                    LeftEndY += Size.y;
                }
                else
                {
                    RenderOne(Item, ImVec2(HostOrigin.x, LeftY), Size);
                    LeftY += Size.y;
                }
                break;
            case toolbar_host_edge::Right:
                if (Item.m_bEndStack)
                {
                    RenderOne(Item, ImVec2(HostOrigin.x + HostWidth - Size.x, RightEndY), Size);
                    RightEndY += Size.y;
                }
                else
                {
                    RenderOne(Item, ImVec2(HostOrigin.x + HostWidth - Size.x, RightY), Size);
                    RightY += Size.y;
                }
                break;
            case toolbar_host_edge::Floating:
                if (!Item.m_bDragging)
                    ApplyFloatingAnchor(Item, ImVec2(HostWidth, HostHeight));
                RenderOne
                ( Item
                , ImVec2(HostOrigin.x + Item.m_FloatingPos.x, HostOrigin.y + Item.m_FloatingPos.y)
                , Size
                );
                break;
            }
        }

        // Show where the toolbar will land if the mouse is released now. Near an edge, the
        // preview uses the same snap and insertion rules as the release path; otherwise it follows
        // the constrained floating position.
        for (const auto& Item : Host.m_Items)
        {
            if (!Item.m_bDragging)
                continue;

            const ImVec2 FloatingPos
            ( HostOrigin.x + Item.m_FloatingPos.x
            , HostOrigin.y + Item.m_FloatingPos.y
            );
            const ImVec2 FloatingSize = ToolbarSizeForEdge(Item);
            const ImVec2 HostMax
            ( HostOrigin.x + HostWidth
            , HostOrigin.y + HostHeight
            );
            const float LeftDistance   = ImAbs(FloatingPos.x - HostOrigin.x);
            const float RightDistance  = ImAbs(HostMax.x - (FloatingPos.x + FloatingSize.x));
            const float TopDistance    = ImAbs(FloatingPos.y - HostOrigin.y);
            const float BottomDistance = ImAbs(HostMax.y - (FloatingPos.y + FloatingSize.y));
            const float SnapDistance = 0.0f;
            const toolbar_host_edge PreviewEdge = ChooseDropEdge
            ( Item
            , LeftDistance, RightDistance, TopDistance, BottomDistance
            , SnapDistance
            );

            ImVec2 PreviewPos = FloatingPos;
            ImVec2 PreviewSize = FloatingSize;
            if (PreviewEdge != toolbar_host_edge::Floating)
            {
                PreviewSize = ToolbarSizeForPlacement(Item, PreviewEdge);

                std::vector<int> EdgeOrder;
                const float DropCoordinate = IsHorizontalToolbarEdge(PreviewEdge)
                    ? ImGui::GetIO().MousePos.x : ImGui::GetIO().MousePos.y;
                const float EdgeMidpoint = IsHorizontalToolbarEdge(PreviewEdge)
                    ? HostOrigin.x + HostWidth * 0.5f
                    : HostOrigin.y + HostHeight * 0.5f;
                const bool PreviewEndStack = DropCoordinate > EdgeMidpoint;
                for (std::size_t Index = 0; Index < Host.m_Items.size(); ++Index)
                    if (&Host.m_Items[Index] != &Item
                        && Host.m_Items[Index].m_Edge == PreviewEdge
                        && Host.m_Items[Index].m_bEndStack == PreviewEndStack)
                        EdgeOrder.push_back(static_cast<int>(Index));
                std::sort(EdgeOrder.begin(), EdgeOrder.end(), [&](int Left, int Right)
                {
                    return Host.m_Items[Left].m_Order < Host.m_Items[Right].m_Order;
                });

                int InsertOrder = 0;
                for (int Index : EdgeOrder)
                {
                    const auto& Other = Host.m_Items[Index];
                    const float Midpoint = IsHorizontalToolbarEdge(PreviewEdge)
                        ? Other.m_LastScreenPos.x + Other.m_LastSize.x * 0.5f
                        : Other.m_LastScreenPos.y + Other.m_LastSize.y * 0.5f;
                    if (DropCoordinate > Midpoint)
                        ++InsertOrder;
                }

                if (PreviewEdge == toolbar_host_edge::Top || PreviewEdge == toolbar_host_edge::Bottom)
                {
                    float StackWidth = 0.0f;
                    for (int Index : EdgeOrder)
                        StackWidth += ToolbarSizeForPlacement
                        ( Host.m_Items[Index], PreviewEdge ).x;
                    StackWidth += PreviewSize.x;
                    PreviewPos.x = PreviewEndStack
                        ? HostOrigin.x + HostWidth - StackWidth
                        : HostOrigin.x;
                    for (int Order = 0; Order < InsertOrder; ++Order)
                        PreviewPos.x += ToolbarSizeForPlacement
                        ( Host.m_Items[EdgeOrder[Order]], PreviewEdge ).x;
                    PreviewPos.y = PreviewEdge == toolbar_host_edge::Top
                        ? HostOrigin.y : HostOrigin.y + HostHeight - PreviewSize.y;
                }
                else
                {
                    float StackHeight = 0.0f;
                    for (int Index : EdgeOrder)
                        StackHeight += ToolbarSizeForPlacement
                        ( Host.m_Items[Index], PreviewEdge ).y;
                    StackHeight += PreviewSize.y;
                    PreviewPos.y = PreviewEndStack
                        ? HostOrigin.y + HostHeight - BottomHeight - StackHeight
                        : HostOrigin.y + TopHeight;
                    for (int Order = 0; Order < InsertOrder; ++Order)
                        PreviewPos.y += ToolbarSizeForPlacement
                        ( Host.m_Items[EdgeOrder[Order]], PreviewEdge ).y;
                    PreviewPos.x = PreviewEdge == toolbar_host_edge::Left
                        ? HostOrigin.x : HostOrigin.x + HostWidth - PreviewSize.x;
                }
            }

            ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList();
            const ImVec2 PreviewMax
            ( PreviewPos.x + PreviewSize.x
            , PreviewPos.y + PreviewSize.y
            );
            pForegroundDrawList->AddRectFilled
            ( PreviewPos
            , PreviewMax
            , IM_COL32(50, 140, 255, 45)
            , 2.0f
            );
            pForegroundDrawList->AddRect
            ( PreviewPos
            , PreviewMax
            , IM_COL32(80, 170, 255, 230)
            , 2.0f
            , 0
            , 2.0f
            );
        }

        // A drop onto an occupied edge and stack inserts at the release coordinate rather than
        // always appending in the collection's registration order. Normalize only that stack so
        // toolbars on other edges and the opposite stack retain their independent ordering.
        for (auto& Dropped : Host.m_Items)
        {
            if (!Dropped.m_bDropOrderPending)
                continue;

            int InsertOrder = 0;
            for (auto& Other : Host.m_Items)
            {
                if (&Other == &Dropped || Other.m_Edge != Dropped.m_Edge || Other.m_bEndStack != Dropped.m_bEndStack)
                    continue;
                const float Midpoint = IsHorizontalToolbarEdge(Dropped.m_Edge)
                    ? Other.m_LastScreenPos.x + Other.m_LastSize.x * 0.5f
                    : Other.m_LastScreenPos.y + Other.m_LastSize.y * 0.5f;
                if (Dropped.m_DropCoordinate > Midpoint)
                    ++InsertOrder;
            }
            Dropped.m_bDropOrderPending = false;

            std::vector<int> EdgeOrder;
            for (std::size_t Index = 0; Index < Host.m_Items.size(); ++Index)
                if (&Host.m_Items[Index] != &Dropped
                    && Host.m_Items[Index].m_Edge == Dropped.m_Edge
                    && Host.m_Items[Index].m_bEndStack == Dropped.m_bEndStack)
                    EdgeOrder.push_back(static_cast<int>(Index));
            std::sort(EdgeOrder.begin(), EdgeOrder.end(), [&](int Left, int Right)
            {
                return Host.m_Items[Left].m_Order < Host.m_Items[Right].m_Order;
            });

            InsertOrder = ImClamp(InsertOrder, 0, static_cast<int>(EdgeOrder.size()));
            const int DroppedIndex = static_cast<int>(&Dropped - Host.m_Items.data());
            EdgeOrder.insert(EdgeOrder.begin() + InsertOrder, DroppedIndex);
            for (std::size_t Order = 0; Order < EdgeOrder.size(); ++Order)
                Host.m_Items[EdgeOrder[Order]].m_Order = static_cast<int>(Order);
        }
    }
}

#endif
