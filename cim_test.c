#include "cim.h"
#include "cim_private.h"

int main()
{
    InitializeContext();

    while(true)
    {
        if(Window("Test Window"))
        {
            if(Button("Button"))
            {
                Text("Text");
            }
        }
    }
}
