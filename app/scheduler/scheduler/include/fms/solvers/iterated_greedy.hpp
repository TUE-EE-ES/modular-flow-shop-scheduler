#ifndef FMS_SOLVERS_ITERATED_GREEDY_HPP
#define FMS_SOLVERS_ITERATED_GREEDY_HPP

#include "no_fixed_order_solution.hpp"

#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"

#include <cstdlib> /* srand, rand */
#include <iostream>
#include <utility>
#include <vector>

namespace fms::solvers {

class Mutator {
public:
    problem::Instance problemInstance;
    NoFixedOrderSolution input;
    NoFixedOrderSolution output;
    std::vector<void (Mutator::*)()> searchMutators = {
            &Mutator::SwapMutator, &Mutator::GapIncreaseMutator,
            //   &Mutator::GapDecreaseMutator,
    };

    /* syntax on pointers to member function
    Source: https://cplusplus.com/forum/general/26259/
    class T
    {
        public:
            double Multiply(void) {return 1.0;}
            typedef double (T::*fptr)(void); // typedef for better readability
            fptr GetFPointer(void)
            {
                return &T::Multiply;
            }
    };
    int main()
    {
        T t;
        T::fptr p;
        p = t.GetFPointer(); // get function pointer
        (t.*p)(); // call function pointer
    return 0;
    }*/

    using mutatorFunction = NoFixedOrderSolution (Mutator::*)(); // typedef for better readability

    NoFixedOrderSolution mutate() { return input; }

    Mutator(problem::Instance problemInstance, NoFixedOrderSolution input) :
        problemInstance(std::move(problemInstance)),
        input(std::move(input)) {
    } // why would it call the default constructor for input and not the copy constructor

    void SwapMutator() {
        std::cout << "Executing swap mutator \n";
        auto j1 = rand() % problemInstance.jobs().size();
        auto j2 = rand() % problemInstance.jobs().size();
        output = input;
    }

    void GapIncreaseMutator();

    void GapDecreaseMutator();

    void DestructionConstructionMutator() {
        std::cout << "Executing destruction construction mutator \n";
        output = input;
    }
};

class IteratedGreedy {
    static void createInitialSolution(problem::Instance &problemInstance,
                                      problem::MachineId reEntrantMachine,
                                      NoFixedOrderSolution &initialSolution);

public:
    static NoFixedOrderSolution solve(problem::Instance &problemInstance, const cli::CLIArgs &args);
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_ITERATED_GREEDY_HPP