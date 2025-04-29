#include <fms/scheduler.hpp>
#include <fms/cli/command_line.hpp>

#include <iostream>

int main(int argc, char *argv[]) {

    /* extract the command line arguments */
    auto args = fms::cli::getArgs(argc, argv);

    try {
        fms::Scheduler::compute(args);
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
