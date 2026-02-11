#include "../header/Thread.h"

auto dummyFunction(int a, int b, bool sleep)
{
    std::cout << "Dummy function called with a: " << a << ", b: " << b << ", sleep: " << sleep << std::endl;
    if (sleep)
    {
        std::cout << "Sleeping for 2 seconds..." << std::endl;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
    }

    std::cout << "Dummy function finished execution." << std::endl;
}

int main(int, char **)
{
    using namespace Common;

    auto t1 = createAndStartThread(-1, "dummy function 1", dummyFunction, 12, 10, false);
    auto t2 = createAndStartThread(1, "dummy function 2", dummyFunction, 5, 15, true);
    std::cout << "Main thread is doing other work while dummy functions are running..." << std::endl;
    t1->join();
    t2->join();
    std::cout << "All threads have finished execution. Exiting main thread." << std::endl;
    delete t1;
    delete t2;
    return 0;
}