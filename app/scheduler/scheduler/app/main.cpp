#include "fmsscheduler.h"
#include "utils/commandLine.h"

#include <iostream>

int main(int argc, char *argv[]) {

    /* extract the command line arguments */
    auto args = commandLine::getArgs(argc, argv);

    try {
        FmsScheduler::compute(args);
        std::cout << "FMS Scheduler has finished." << std::endl;
    } catch (const ParseException &e) {
        std::cerr << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    } catch (char const *e) {
        std::cerr << e << std::endl;
    }

    return 0;
}
