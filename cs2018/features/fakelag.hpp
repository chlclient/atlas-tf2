#pragma once

#include "link.hpp"

class fakelag
{
public:
    void think( );

private:
    bool should_run( );

    i32 m_roll{ };
};

extern fakelag g_fakelag;
