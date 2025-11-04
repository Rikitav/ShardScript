#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <iostream>

#ifdef _DEBUG
#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define DBG_NEW new
#endif

namespace shard::utilities
{
    class MemoryLeakDetector
    {
    public:
        MemoryLeakDetector()
        {
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
            _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        }

        ~MemoryLeakDetector()
        {
            _CrtDumpMemoryLeaks();
        }

        static void Checkpoint()
        {
            _CrtMemCheckpoint(&s_memState);
        }

        static void DumpSinceCheckpoint()
        {
            _CrtMemState newState, diffState;
            _CrtMemCheckpoint(&newState);

            if (_CrtMemDifference(&diffState, &s_memState, &newState))
            {
                std::cout << "=== Memory leaks since checkpoint ===" << std::endl;
                _CrtMemDumpStatistics(&diffState);
                _CrtMemDumpAllObjectsSince(&s_memState);
            }
        }

    private:
        static _CrtMemState s_memState;
    };
}
