#pragma once

typedef struct cim_arena
{
    void *Memory;
    size_t At;
    size_t Capacity;
} cim_arena;