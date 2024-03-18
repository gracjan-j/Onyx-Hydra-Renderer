#pragma once

#include <iostream>


namespace GUI
{


class View
{
public:

    virtual ~View() {};

    virtual bool Initialise() = 0;

    virtual bool PrepareBeforeDraw() = 0;

    virtual void Draw() = 0;

    virtual bool SetNewSize() = 0;
};


}



