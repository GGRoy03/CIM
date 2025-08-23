typedef enum CimWindow_Flags
{
    CimWindow_AllowDrag = 1 << 0,
} CimWindow_Flags;

// TODO: Simplify these macros.

#define _UI_CONCAT2(a,b) a##b
#define _UI_CONCAT(a,b) _UI_CONCAT2(a,b)
#define _UI_UNIQUE(name) _UI_CONCAT(name, __LINE__)

#define UIWindow(Id, ThemeId, Flags)                                                     \
    for (int _UI_UNIQUE(_ui_iter) = 0; _UI_UNIQUE(_ui_iter) < 2; ++_UI_UNIQUE(_ui_iter)) \
        if (Cim_Window(Id, ThemeId, Flags))                                              \
            for (int _UI_UNIQUE(_ui_once) = 1; _UI_UNIQUE(_ui_once);                     \
                 (Cim_EndWindow(), _UI_UNIQUE(_ui_once) = 0))


#define UIButton(Id, ThemeId, ...)                                \
    for (struct { bool _clicked; int _once; } _UI_UNIQUE(_ui) = { \
              ._clicked = Cim_Button(Id, ThemeId), ._once = 1 };  \
         _UI_UNIQUE(_ui)._once;                                   \
         (_UI_UNIQUE(_ui)._once = 0)                              \
        )                                                         \
        if (_UI_UNIQUE(_ui)._clicked __VA_ARGS__)

#define UIText(Text) Cim_Text(Text)

static bool
Cim_Window(const char *Id, const char *ThemeId, cim_bit_field Flags)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        cim_component *Component = FindComponent(Id);
        cim_window    *Window    = &Component->For.Window;

        if (!Window->IsInitialized)
        {
            Window->LastFrameScreenX = 500;
            Window->LastFrameScreenY = 500;

            Window->IsInitialized = true;
        }

        cim_layout_node *Node  = PushLayoutNode(true, &Component->LayoutNodeIndex);
        theme           *Theme = GetTheme(ThemeId, &Component->ThemeId);

        if (!Theme || !Node)
        {
            return false;
        }

        Node->PrefWidth  = Theme->Size.x;
        Node->PrefHeight = Theme->Size.x;
        Node->Padding    = Theme->Padding;
        Node->Spacing    = Theme->Spacing;

        // NOTE: This is still wrong/weird/bad.
        Node->X = Window->LastFrameScreenX;
        Node->Y = Window->LastFrameScreenY;

        // I guess we still use this for now, so hardcode it.
        Node->Order = Layout_Horizontal;

        return true; // Need to find a way to cache some state. E.g. closed and not hovering
    }
    else if (State == CimContext_Interaction)
    {
        cim_component   *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_layout_node *Node      = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?
        theme           *Theme     = GetTheme(ThemeId, &Component->ThemeId);       // And then another access to the theme for the same frame...

        cim_window *Window = &Component->For.Window;

        if (Flags & CimWindow_AllowDrag)
        {
            if (Node->Held)
            {
                cim_layout_tree *Tree = UIP_LAYOUT.Tree;          // NOTE: Should somehow access our own tree.
                Tree->DragTransformX = GetMouseDeltaX(UIP_INPUT);
                Tree->DragTransformY = GetMouseDeltaY(UIP_INPUT);

                Node->X += Tree->DragTransformX;
                Node->Y += Tree->DragTransformY;
            }
        }

        // Duplicate code.
        if (Theme->BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Theme->BorderWidth;
            cim_f32 y0 = Node->Y - Theme->BorderWidth;
            cim_f32 x1 = Node->X + Node->Width + Theme->BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Theme->BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Theme->BorderColor);
        }
        DrawQuadFromNode(Node, Theme->Color);

        Window->LastFrameScreenX = Node->X;
        Window->LastFrameScreenY = Node->Y;

        return true;
    }
    else
    {
        CimLog_Error("Invalid state");
        return false;
    }
}

static bool
Cim_Button(const char *Id, const char *ThemeId)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        cim_component   *Component = FindComponent(Id);
        cim_layout_node *Node      = PushLayoutNode(false, &Component->LayoutNodeIndex);
        theme           *Theme     = GetTheme(ThemeId, &Component->ThemeId);

        Node->ContentWidth  = Theme->Size.x;
        Node->ContentHeight = Theme->Size.y;

        return IsMouseDown(CimMouse_Left, UIP_INPUT); // NOTE: Can we check for hovers?
    }
    else if (State == CimContext_Interaction) // Maybe rename this state then?
    {
        cim_layout_tree *Tree      = UIP_LAYOUT.Tree;                              // Another global access.
        cim_component   *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_layout_node *Node      = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?
        theme           *Theme     = GetTheme(ThemeId, &Component->ThemeId);       // And then another access?

        // BUG: This is faulty logic if button is deeper. Because the things in between the tree
        // won't know that the button has been clicked.

        if (Node->Clicked)
        {
            Tree->DragTransformX = 0;
            Tree->DragTransformY = 0;
        }
        else
        {
            Node->X += Tree->DragTransformX;
            Node->Y += Tree->DragTransformY;
        }

        // Duplicate code.
        if (Theme->BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Theme->BorderWidth;
            cim_f32 y0 = Node->Y - Theme->BorderWidth;
            cim_f32 x1 = Node->X + Node->Width  + Theme->BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Theme->BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Theme->BorderColor);
        }
        DrawQuadFromNode(Node, Theme->Color);

        return Node->Clicked;
    }
    else
    {
        CimLog_Error("Invalid state");
        return false;
    }
}

static void
Cim_Text(char *TextToRender)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    // Faking it.
    static cim_component Component;

    if (State == CimContext_Layout)
    {
        cim_text *Text = &Component.For.Text;

        // Unsure.
        cim_layout_node *Node = PushLayoutNode(false, &Component.LayoutNodeIndex);
        Node->ContentWidth  = 100;
        Node->ContentHeight = 50;

        if (!Component.IsInitialized)
        {
            Text->TextLayoutInfo = OSCreateTextLayout(TextToRender,Node->ContentWidth, Node->ContentHeight);

            Component.IsInitialized = true;
        }
    }
    else if (State == CimContext_Interaction) // NOTE: This name is really misleading, but idk what to call it.
    {
        cim_text        *Text = &Component.For.Text;
        cim_layout_node *Node = GetNodeFromIndex(Component.LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?

        // WARN: Shouldn't be done here.
        cim_cmd_buffer   *Buffer  = UIP_COMMANDS;
        cim_draw_command *Command = GetDrawCommand(Buffer, {}, UIPipeline_Text);

        cim_f32 PenX       = Node->X;
        cim_f32 PenY       = Node->Y;
        cim_f32 RowAdvance = 0.0f;

        // NOTE: Most of this loop is garbage.
        for (cim_u32 Idx = 0; Idx < Text->TextLayoutInfo.GlyphCount; Idx++)
        {
            glyph_layout_info *Layout = &Text->TextLayoutInfo.GlyphLayoutInfo[Idx];
            glyph_info        *Glyph  = GetGlyphInfo(TextToRender[Idx]);

            if (!Glyph || !Layout)
            {
                continue;
            }

            typedef struct local_vertex
            {
                cim_f32 PosX, PosY;
                cim_f32 U, V;
                cim_f32 R, G, B, A;
            } local_vertex;

            cim_f32 MinX = PenX + Layout->OffsetX;
            cim_f32 MinY = PenY + Layout->OffsetY;
            cim_f32 MaxX = PenX + Layout->OffsetX + Glyph->Size.Width;
            cim_f32 MaxY = PenY + Layout->OffsetY + Glyph->Size.Height;

            local_vertex Vtx[4];
            Vtx[0] = (local_vertex){MinX, MinY, Glyph->U0, Glyph->V0, 0.0f, 0.0f, 0.0f, 1.0f}; // Top left
            Vtx[1] = (local_vertex){MinX, MaxY, Glyph->U0, Glyph->V0, 0.0f, 0.0f, 0.0f, 1.0f}; // Bot left
            Vtx[2] = (local_vertex){MaxX, MinY, Glyph->U0, Glyph->V0, 0.0f, 0.0f, 0.0f, 1.0f}; // Top right
            Vtx[3] = (local_vertex){MaxX, MaxY, Glyph->U0, Glyph->V0, 0.0f, 0.0f, 0.0f, 1.0f}; // Bot right

            cim_u32 Indices[6];
            Indices[0] = Command->VtxCount + 0;
            Indices[1] = Command->VtxCount + 2;
            Indices[2] = Command->VtxCount + 1;
            Indices[3] = Command->VtxCount + 2;
            Indices[4] = Command->VtxCount + 3;
            Indices[5] = Command->VtxCount + 1;

            WriteToArena(Vtx    , sizeof(Vtx)    , &Buffer->FrameVtx);
            WriteToArena(Indices, sizeof(Indices), &Buffer->FrameIdx);;

            Command->VtxCount += 4;
            Command->IdxCount += 6;

            RowAdvance += Layout->AdvanceX;
            if (RowAdvance >= Node->Width)
            {
                RowAdvance = 0;
                PenX       = Node->X;
                PenY      += Text->TextLayoutInfo.LineHeight;
            }
        }

    }
}

static void
Cim_EndWindow()
{
    Cim_Assert(CimCurrent);
    cim_layout_tree *Tree = UIP_LAYOUT.Tree;
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        // Why?
        Tree->DragTransformX = 0;
        Tree->DragTransformY = 0;

        cim_u32 DownUpStack[1024] = {};
        cim_u32 StackAt           = 0;
        cim_u32 NodeCount         = 0;
        char    Visited[512]      = {};

        DownUpStack[StackAt++] = 0;

        while (StackAt > 0)
        {
            cim_u32 Current = DownUpStack[StackAt - 1];

            if (!Visited[Current])
            {
                Visited[Current] = 1;

                cim_u32 Child = Tree->Nodes[Current].FirstChild;
                while (Child != CimLayout_InvalidNode)
                {
                    DownUpStack[StackAt++] = Child;
                    Child                  = Tree->Nodes[Child].NextSibling;
                }
            }
            else
            {
                StackAt--;
                NodeCount++;

                cim_layout_node *Node = Tree->Nodes + Current;
                if (Node->FirstChild == CimLayout_InvalidNode)
                {
                    // Can we set a special flag such that one's width depend on it's 
                    // container size? Okay, but then the container's size depends on
                    // the content itself.
                    Node->PrefWidth  = Node->ContentWidth;
                    Node->PrefHeight = Node->ContentHeight;
                }
                else
                {
                    cim_u32 ChildCount = 0;

                    if (Node->Order == Layout_Horizontal)
                    {
                        cim_f32 MaximumHeight = 0;
                        cim_f32 TotalWidth = 0;

                        cim_u32 Child = Tree->Nodes[Current].FirstChild;
                        while (Child != CimLayout_InvalidNode)
                        {
                            if (Tree->Nodes[Child].PrefHeight > MaximumHeight)
                            {
                                MaximumHeight = Tree->Nodes[Child].PrefHeight;
                            }

                            TotalWidth += Tree->Nodes[Child].PrefWidth;
                            ChildCount += 1;

                            Child = Tree->Nodes[Child].NextSibling;
                        }

                        cim_f32 Spacing = (ChildCount > 1) ? Node->Spacing.x * (ChildCount - 1) : 0.0f;

                        Node->PrefWidth = TotalWidth + (Node->Padding.x + Node->Padding.z) + Spacing;
                        Node->PrefHeight = MaximumHeight + (Node->Padding.y + Node->Padding.w);
                    }
                    else if (Node->Order == Layout_Vertical)
                    {
                        cim_f32 MaximumWidth = 0;
                        cim_f32 TotalHeight = 0;

                        cim_u32 Child = Tree->Nodes[Current].FirstChild;
                        while (Child != CimLayout_InvalidNode)
                        {
                            if (Tree->Nodes[Child].PrefWidth > MaximumWidth)
                            {
                                MaximumWidth = Tree->Nodes[Child].PrefWidth;
                            }

                            TotalHeight += Tree->Nodes[Child].PrefHeight;
                            ChildCount += 1;

                            Child = Tree->Nodes[Child].NextSibling;
                        }

                        cim_f32 Spacing = (ChildCount > 1) ? Node->Spacing.y * (ChildCount - 1) : 0.0f;

                        Node->PrefWidth = MaximumWidth + (Node->Padding.x + Node->Padding.z);
                        Node->PrefHeight = TotalHeight + (Node->Padding.y + Node->Padding.w) + Spacing;
                    }
                }

            }
        }

        cim_u32 UpDownStack[1024] = {};
        StackAt = 0;
        UpDownStack[StackAt++] = 0;

        // How to place the window is still an issue.
        Tree->Nodes[0].Width = Tree->Nodes[0].PrefWidth;
        Tree->Nodes[0].Height = Tree->Nodes[0].PrefHeight;

        cim_f32 ClientX;
        cim_f32 ClientY;

        while (StackAt > 0)
        {
            cim_u32          Current = UpDownStack[--StackAt];
            cim_layout_node *Node = Tree->Nodes + Current;

            ClientX = Node->X + Node->Padding.x;
            ClientY = Node->Y + Node->Padding.y;

            // TODO: Change how we iterate this.
            cim_u32 Child = Node->FirstChild;
            cim_u32 Temp[256] = {};
            cim_u32 Idx = 0;
            while (Child != CimLayout_InvalidNode)
            {
                cim_layout_node *CNode = Tree->Nodes + Child;

                CNode->X = ClientX;
                CNode->Y = ClientY;
                CNode->Width = CNode->PrefWidth;  // Weird.
                CNode->Height = CNode->PrefHeight; // Weird.

                if (Node->Order == Layout_Horizontal)
                {
                    ClientX += CNode->Width + Node->Spacing.x;
                }
                else if (Node->Order == Layout_Vertical)
                {
                    ClientY += CNode->Height + Node->Spacing.y;
                }

                Temp[Idx++] = Child;
                Child = CNode->NextSibling;
            }

            for (cim_i32 TempIdx = (cim_i32)Idx - 1; TempIdx >= 0; --TempIdx)
            {
                UpDownStack[StackAt++] = Temp[TempIdx];
            }
        }

        UI_STATE = CimContext_Interaction;
        PopParent();

        bool MouseClicked = IsMouseClicked(CimMouse_Left, UIP_INPUT);
        bool MouseDown = IsMouseDown(CimMouse_Left, UIP_INPUT);

        if (MouseClicked || MouseDown)
        {
            cim_layout_node *Root = Tree->Nodes;
            cim_rect         WindowHitBox = MakeRectFromNode(Root);

            if (IsInsideRect(WindowHitBox))
            {
                for (cim_u32 StackIdx = NodeCount - 1; StackIdx > 0; StackIdx--)
                {
                    cim_layout_node *Node = Tree->Nodes + DownUpStack[StackIdx];
                    cim_rect         HitBox = MakeRectFromNode(Node);

                    if (IsInsideRect(HitBox))
                    {
                        Node->Held = MouseDown;
                        Node->Clicked = MouseClicked;
                        return;
                    }
                }

                Root->Held    = MouseDown;
                Root->Clicked = MouseClicked;
            }
        }
    }
    else if (State == CimContext_Interaction)
    {
        // NOTE: What can we even do here?
    }
    else
    {
        CimLog_Error("Invalid State");
    }
}