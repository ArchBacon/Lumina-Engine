#pragma once

class Engine
{
public:
    void Initialize();
    void Run();
    void Shutdown();
};

extern Engine gEngine;
