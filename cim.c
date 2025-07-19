#include "cim.h"
#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

bool Window(const char *Id)
{
    cim_u32 Hashed = Cim_HashString(Id);

    return true;
}

bool Button(const char *Id)
{
    return true;
}

void Text(const char *Id)
{
}


#ifdef __cplusplus
}
#endif