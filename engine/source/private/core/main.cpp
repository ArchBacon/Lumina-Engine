#include "core/engine.hpp"

int main(int argc, char* argv[]) 
{
    gEngine.Initialize();
    gEngine.Run();
    gEngine.Shutdown();
}
