#include "cim.h"

#include <stdio.h> // For printf

int main()
{
    while(true)
    {
        if(Window("Test Window", CimWindow_Draggable))
        {
            printf("Window is opened.\n");
        }
        else
        {
            printf("Window is closed.\n");
        }
    }
}
