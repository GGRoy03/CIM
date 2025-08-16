typedef enum CimWindow_Flags
{
    CimWindow_AllowDrag = 1 << 0,
} CimWindow_Flags;

#define _UI_CONCAT2(a,b) a##b
#define _UI_CONCAT(a,b) _UI_CONCAT2(a,b)
#define _UI_UNIQUE(name) _UI_CONCAT(name, __LINE__)

#define UIWindow(Id, Flags)                                                              \
    for (int _UI_UNIQUE(_ui_iter) = 0; _UI_UNIQUE(_ui_iter) < 2; ++_UI_UNIQUE(_ui_iter)) \
        if (Cim_Window(Id, Flags))                                                       \
            for (int _UI_UNIQUE(_ui_once) = 1; _UI_UNIQUE(_ui_once);                     \
                 (Cim_EndWindow(), _UI_UNIQUE(_ui_once) = 0))


#define UIButton(Id, ...)                                         \
    for (struct { bool _clicked; int _once; } _UI_UNIQUE(_ui) = { \
              ._clicked = Cim_Button((Id)), ._once = 1 };         \
         _UI_UNIQUE(_ui)._once;                                   \
         (_UI_UNIQUE(_ui)._once = 0)                              \
        )                                                         \
        if (_UI_UNIQUE(_ui)._clicked __VA_ARGS__)

bool
Cim_Window(const char *Id, cim_bit_field Flags)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        cim_component *Component = FindComponent(Id);
        cim_window *Window = &Component->For.Window;

        if (!Window->IsInitialized)
        {
            Window->LastFrameScreenX = 500;
            Window->LastFrameScreenY = 500;
            Window->IsInitialized = true;
        }

        cim_layout_node *Node = PushLayoutNode(true, &Component->LayoutNodeIndex);
        Node->PrefWidth = Window->Style.Size.x;
        Node->PrefHeight = Window->Style.Size.y;
        Node->Padding = Window->Style.Padding;
        Node->Spacing = Window->Style.Spacing;
        Node->Order = Window->Style.Order;
        Node->X = Window->LastFrameScreenX;
        Node->Y = Window->LastFrameScreenY;

        return true; // Need to find a way to cache some state. E.g. closed and not hovering
    }
    else if (State == CimContext_Interaction)
    {
        UI_COMMANDS.ClippingRectChanged = true; // Uhm.. Probably should not even be a direct set, but more of a check.

        cim_component *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_window *Window = &Component->For.Window;
        cim_layout_node *Node = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?

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

        // Duplicate code
        if (Window->Style.BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Window->Style.BorderWidth;
            cim_f32 y0 = Node->Y - Window->Style.BorderWidth;
            cim_f32 x1 = Node->X + Node->Width + Window->Style.BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Window->Style.BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Window->Style.BorderColor);
        }
        DrawQuadFromNode(Node, Window->Style.Color);

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

bool
Cim_Button(const char *Id)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        cim_component *Component = FindComponent(Id);
        cim_button_style Style = Component->For.Button.Style;

        cim_layout_node *Node = PushLayoutNode(false, &Component->LayoutNodeIndex);
        Node->ContentWidth = Style.Size.x;
        Node->ContentHeight = Style.Size.y;

        return IsMouseDown(CimMouse_Left, UIP_INPUT);
    }
    else if (State == CimContext_Interaction) // Maybe rename this state then?
    {
        cim_layout_tree *Tree = UIP_LAYOUT.Tree;                              // Another global access.
        cim_component *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_button_style Style = Component->For.Button.Style;
        cim_layout_node *Node = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?

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

        // NOW: Do we draw here? We can for now.

        if (Style.BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Style.BorderWidth;
            cim_f32 y0 = Node->Y - Style.BorderWidth;
            cim_f32 x1 = Node->X + Node->Width + Style.BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Style.BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Style.BorderColor);
        }
        DrawQuadFromNode(Node, Style.Color);

        return Node->Clicked;
    }
    else
    {
        CimLog_Error("Invalid state");
        return false;
    }
}

void
Cim_EndWindow()
{
    Cim_Assert(CimCurrent);
    cim_layout_tree *Tree = UIP_LAYOUT.Tree;
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        Tree->DragTransformX = 0;
        Tree->DragTransformY = 0;

        cim_u32 DownUpStack[1024] = {};
        cim_u32 StackAt = 0;
        cim_u32 NodeCount = 0;
        char    Visited[512] = {};

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
                    Child = Tree->Nodes[Child].NextSibling;
                }
            }
            else
            {
                StackAt--;
                NodeCount++;

                cim_layout_node *Node = Tree->Nodes + Current;
                if (Node->FirstChild == CimLayout_InvalidNode)
                {
                    Node->PrefWidth = Node->ContentWidth;
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

                Root->Held = MouseDown;
                Root->Clicked = MouseClicked;
            }
        }
    }
    else if (State == CimContext_Interaction)
    {
        // Unsure. Maybe dragging?
    }
    else
    {
        CimLog_Error("Invalid State");
    }
}