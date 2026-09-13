# toolbar.imgui
# ximgui Toolbar

`ximgui_toolbar.h` is a small, header-only toolbar host for Dear ImGui. It provides
Unity-style toolbars that belong to a host window instead of becoming peers in the
application's global ImGui dockspace.

The library is intentionally independent of xGPU and E29. It only depends on
`imgui.h`, the standard library, and a caller-provided renderer for each toolbar.
Copy the dependency into an existing Dear ImGui project, add its directory to the
include path, and include the one public header.

## Why a host-owned toolbar?

Regular ImGui docking treats every dockable window as an equal participant in the
global dock tree. That is useful for editor panels, but it is the wrong model for a
toolbar that belongs to one editor window:

- toolbars stay inside their host;
- floating toolbars remain constrained to the host;
- edge toolbars have fixed, content-driven sizes;
- multiple toolbars share an edge without equal-size dock splits;
- the host body is laid out around the edge strips;
- toolbar orientation follows its edge and is remembered while floating.

## Features

- Top, right, bottom, left, and host-relative floating placement.
- Fixed horizontal and vertical sizes supplied per toolbar.
- A visible drag grip that works even when a toolbar has no title bar.
- Exact-edge docking. The current implementation uses a zero snap distance, so a
  toolbar docks only when its dragged rectangle reaches an edge.
- A translucent blue drop preview showing the final rectangle and insertion slot.
- Corner ownership: horizontal toolbars claim top/bottom corners, and vertical
  toolbars claim left/right corners.
- Two stacks per edge:
  - top/bottom edges split at the horizontal midpoint into left and right stacks;
  - left/right edges split at the vertical midpoint into top and bottom stacks.
- Floating toolbars preserve their distance from the nearest horizontal and vertical
  host edges, so resizing the host preserves their nearest-corner relationship.
- Toolbar ordering is maintained independently inside each edge stack.
- The body is rendered before toolbar children so floating toolbars remain clickable.

## Minimal integration

```cpp
#include "ximgui_toolbar.h"

static ximgui::toolbar::toolbar_host_state ToolbarHost;

if (ToolbarHost.m_Items.empty())
{
    ToolbarHost.m_Items.push_back
    ({ "Main"
     , ximgui::toolbar::toolbar_host_edge::Top
     , ximgui::toolbar::axis::Horizontal
     , ImVec2(500.0f, 28.0f)  // horizontal size
     , ImVec2(32.0f, 220.0f)  // vertical size
     , ImVec2(24.0f, 24.0f)   // initial floating position
    });
}

auto RenderToolbar = [](const char* Name, ximgui::toolbar::axis Axis)
{
    if (Axis == ximgui::toolbar::axis::Horizontal)
        ImGui::Button("Save");
    else
        ImGui::Button("S", ImVec2(32.0f, 28.0f));
};

ImGui::Begin("Editor");
ximgui::toolbar::RenderToolbarHost
( ToolbarHost
, ImGui::GetContentRegionAvail()
, RenderToolbar
, []()
  {
      ImGui::TextUnformatted("Host content");
  }
);
ImGui::End();
```

`RenderToolbarHost()` must be called while the host window is active. The host
renderer receives the registered toolbar name and its current orientation. The body
renderer is called exactly where the remaining host content belongs.

## Public API

### `ximgui::toolbar::toolbar_host_item`

Each item describes one toolbar:

- `m_Name`: stable label used to identify the child window.
- `m_Edge`: `Top`, `Right`, `Bottom`, `Left`, or `Floating`.
- `m_LastFloatingAxis`: remembered orientation while floating.
- `m_HorizontalSize`: fixed size on top/bottom edges.
- `m_VerticalSize`: fixed size on left/right edges.
- `m_FloatingPos`: host-relative position while floating.

Register items before the first call to `RenderToolbarHost()`. The host assigns
initial ordering from registration order.

### `ximgui::toolbar::toolbar_host_state`

Owns the registered items and persistent layout state. Keep one state object per
host window. Do not share one state object between unrelated host windows.

### `RenderToolbarHost()`

```cpp
template<typename ToolbarRenderer, typename BodyRenderer>
void RenderToolbarHost
(
    toolbar_host_state& host,
    const ImVec2& host_size,
    ToolbarRenderer&& render_toolbar,
    BodyRenderer&& render_body
) noexcept;
```

The host size should normally be `ImGui::GetContentRegionAvail()`. If a host
intentionally removes its window padding, pass the resulting content size directly.
The library uses screen-space cursor positions internally, so the caller should not
move the cursor between the size query and the host call.

## Layout rules

An edge has a midpoint. A release on the first half selects the start stack; a
release on the second half selects the end stack:

| Edge | Start stack | End stack |
| --- | --- | --- |
| Top | left | right |
| Bottom | left | right |
| Left | top | bottom |
| Right | top | bottom |

Each stack is anchored to its corresponding host corner. Items are laid out in
their stack order and never resize the host window. The host reserves only the
largest toolbar height on horizontal edges and the largest toolbar width on
vertical edges.

While dragging, the preview uses the same edge, stack, and ordering rules as the
release path. This makes the preview an accurate representation of the resulting
layout, including right and bottom stacks.

## Sizing guidance

The library does not resize a toolbar to fit its renderer. Choose sizes that can
contain every button in both orientations. A renderer can use the `axis` argument
to switch from full labels to compact labels:

```cpp
const bool horizontal = Axis == ximgui::toolbar::axis::Horizontal;
ImGui::Button(horizontal ? "Save" : "S", ImVec2(52.0f, 24.0f));
```

The toolbar child is rendered with compact internal padding. A vertical toolbar's
content should normally use a button width equal to its vertical toolbar width.

## Build integration

This is a header-only library. No source file or link library is required. For a
CMake project, add the header to the target's source list for IDE visibility and
add the dependency directory to the target include paths:

```cmake
target_sources(MyEditor PRIVATE
    "${CMAKE_SOURCE_DIR}/dependencies/toolbar.imgui/ximgui_toolbar.h"
)
target_include_directories(MyEditor PRIVATE
    "${CMAKE_SOURCE_DIR}/dependencies/toolbar.imgui"
)
```

## Current design boundaries

The host deliberately does not depend on `imgui_internal.h`, docking internals, or
application-specific state. It is a host-owned layout and interaction layer built
from public Dear ImGui APIs. A caller that needs persistence should serialize the
public item state it cares about (`m_Edge`, sizes, floating position, and ordering)
using its own settings format.

The repository was vendored from
`https://github.com/LIONant-depot/toolbar.imgui`.