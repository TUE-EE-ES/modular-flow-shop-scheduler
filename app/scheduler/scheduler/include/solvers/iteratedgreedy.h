#ifndef ITERATEDGREEDY_H
#define ITERATEDGREEDY_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/module.hpp"
#include "longest_path.h"
#include "nofixedordersolution.h"

#include "pch/containers.hpp"

#include <cstdlib> /* srand, rand */
#include <iostream>
#include <memory>

namespace algorithm {

class Mutator{
    public:

    FORPFSSPSD::Instance problemInstance;
    NoFixedOrderSolution input;
    NoFixedOrderSolution output;
    std::vector<void(algorithm::Mutator::*)()> searchMutators = {&Mutator::SwapMutator, 
                                                                              &Mutator::GapIncreaseMutator,
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

    typedef NoFixedOrderSolution(Mutator::*mutatorFunction)(); // typedef for better readability 

    NoFixedOrderSolution mutate(){
        return input;
    }

    Mutator(const FORPFSSPSD::Instance &problemInstance,
            const NoFixedOrderSolution &input):
        problemInstance(problemInstance),
        input(input){
        } //why would it call the default constructor for input and not the copy constructor

    void SwapMutator(){
        std::cout << "Executing swap mutator \n";
        auto j1 = rand() % problemInstance.jobs().size();
        auto j2 = rand() % problemInstance.jobs().size();
        output = input;
    }

    void GapIncreaseMutator (){
        std::cout << "Executing gap increase mutator \n";
        //move all higher passes further down by one step
        DelayGraph::delayGraph dg = input.getDelayGraph();
        FORPFSSPSD::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();

        // TODO: getChosenEdges does a map search which is slow every time. Use reference wrapper?
        auto solution = input.solution;
        auto iter = solution.getChosenEdges(reentrant_machine).end();
        iter--;
        while (iter != solution.getChosenEdges(reentrant_machine).begin()) {
            
            if (dg.get_vertex(iter->dst).operation.operationId
                < dg.get_vertex(iter->src).operation.operationId) {
                // remove src from current location
               
                const auto &prevV = dg.get_vertex(std::prev(iter, 1)->src); // i-1
                                                                            // src
                const auto &currV = dg.get_vertex(iter->src);               // i src
                const auto &nextV = dg.get_vertex(iter->dst);               // i dst
                auto op = dg.get_vertex(iter->src).operation;
                auto prevnextW = problemInstance.query(prevV, nextV);

                // create the option and remove it
                const DelayGraph::edge curr{prevV.id, nextV.id, prevnextW};
                const DelayGraph::edge next =
                        curr; // removal should leave next edge unaffected, just initialised to
                              // current to build a proper option instance
                auto removalPosition =
                        std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter);
                
                option rem_opt(
                        curr,
                        next,
                        prevV.id,
                        currV.id,
                        nextV.id,
                        std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter));
                solution = solution.remove(reentrant_machine,
                                           rem_opt,
                                           solution.getASAPST(),
                                           true); // making last argument true by default means
                                                  // that last inserted edge is unchanged
                LOG(fmt::format("Removed {} before {}.\n", currV.operation, nextV.operation));
                for (auto edge : solution.getChosenEdges(reentrant_machine)) {
                        std::cout << fmt::to_string(edge) << " " << "\n";
                    }
                iter = solution.getChosenEdges(reentrant_machine).begin() + removalPosition;

                // insert src after dst
                auto iterE = iter;

                const auto &prevVE = dg.get_vertex(iterE->src);
                const auto &currVE = dg.get_vertex(op); // add second pass
                const auto &nextVE = dg.get_vertex(iterE->dst);

                const auto prevcurrW = problemInstance.query(prevVE, currVE);
                const auto currnextW = problemInstance.query(currVE, nextVE);

                // create the option and add it
                option loop_opt(
                        DelayGraph::edge{prevVE.id, currVE.id, prevcurrW},
                        DelayGraph::edge{currVE.id, nextVE.id, currnextW},
                        prevVE.id,
                        currVE.id,
                        nextVE.id,
                        std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter),
                        false);
                solution = solution.add(reentrant_machine, loop_opt, solution.getASAPST());
                LOG(fmt::format("Added {} between {} and {}.\n",
                                currVE.operation,
                                prevVE.operation,
                                nextVE.operation));
                iter = solution.getChosenEdges(reentrant_machine).begin() + removalPosition - 1;

                for (auto edge : solution.getChosenEdges(reentrant_machine)) {
                        std::cout << fmt::to_string(edge) << " " << "\n";
                    }
            }
            else{
                iter--;
            }     
        }
        output = NoFixedOrderSolution{input.jobOrder, std::move(solution)};
        output.updateDelayGraph(dg);
    }


    void GapDecreaseMutator (){
        std::cout << "Executing gap decrease mutator \n";
        //move all higher passes further down by one step
        DelayGraph::delayGraph dg = input.getDelayGraph();
        FORPFSSPSD::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();

        // TO DO: getChosenEdges does a map search which is slow every time. Use reference wrapper?
        auto solution = input.solution;
        auto iter = solution.getChosenEdges(reentrant_machine).end();
        iter--;
        iter--;
        
        while (iter != solution.getChosenEdges(reentrant_machine).begin()) {
            
            if (dg.get_vertex(iter->dst).operation.operationId
                > dg.get_vertex(iter->src).operation.operationId) {
                // remove src from current location
                const auto &prevV = dg.get_vertex(iter->src);
                const auto &currV = dg.get_vertex(iter->dst);
                const auto &nextV = dg.get_vertex(std::next(iter,1)->dst);
                auto op = dg.get_vertex(iter->dst).operation;
                auto prevnextW = problemInstance.query(prevV, nextV);

                // create the option and remove it
                const DelayGraph::edge curr{prevV.id, nextV.id, prevnextW};
                const DelayGraph::edge next = curr; // removal should leave next edge unaffected, just initialised to
                              // current to build a proper option instance
                auto removalPosition = std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter);
                
                option rem_opt(
                        curr,
                        next,
                        prevV.id,
                        currV.id,
                        nextV.id,
                        std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter)+1); //because we remove the destination
                solution = solution.remove(reentrant_machine,
                                           rem_opt,
                                           solution.getASAPST(),
                                           true); // making last argument true by default means
                                                  // that last inserted edge is unchanged
                LOG(fmt::format("Removed {} between {} and {}.\n", currV.operation, prevV.operation, nextV.operation));
                for (auto edge : solution.getChosenEdges(reentrant_machine)) {
                        std::cout << fmt::to_string(edge) << " " << "\n";
                    }
                iter = solution.getChosenEdges(reentrant_machine).begin() + removalPosition -1; //because we remove the destination

                // insert src after dst
                auto iterE = iter;

                const auto &prevVE = dg.get_vertex(iterE->src);
                const auto &currVE = dg.get_vertex(op); // add second pass
                const auto &nextVE = dg.get_vertex(iterE->dst);

                const auto prevcurrW = problemInstance.query(prevVE, currVE);
                const auto currnextW = problemInstance.query(currVE, nextVE);

                // create the option and add it
                option loop_opt(
                        DelayGraph::edge{prevVE.id, currVE.id, prevcurrW},
                        DelayGraph::edge{currVE.id, nextVE.id, currnextW},
                        prevVE.id,
                        currVE.id,
                        nextVE.id,
                        std::distance(solution.getChosenEdges(reentrant_machine).begin(), iter),
                        false);
                solution = solution.add(reentrant_machine, loop_opt, solution.getASAPST());
                LOG(fmt::format("Added {} between {} and {}.\n",
                                currVE.operation,
                                prevVE.operation,
                                nextVE.operation));
                iter = solution.getChosenEdges(reentrant_machine).begin() + removalPosition - 1;
                for (auto edge : solution.getChosenEdges(reentrant_machine)) {
                        std::cout << fmt::to_string(edge) << " " << "\n";
                    }
                std::cout << fmt::to_string(*iter) << " \n";
            }
            else{
               iter--; 
            }           
        }
        output = NoFixedOrderSolution{input.jobOrder, std::move(solution)};
        output.updateDelayGraph(dg);
    }

    void DestructionConstructionMutator (){
        std::cout << "Executing destruction construction mutator \n";
        output = input;
    }
};

class IteratedGreedy
{
    static void createInitialSolution(FORPFSSPSD::Instance &problemInstance,
                                                        FORPFSSPSD::MachineId reEntrantMachine, NoFixedOrderSolution &initialSolution);
public:
    static NoFixedOrderSolution solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args);

};
}// namespace algorithm

#endif // ITERATEDGREEDY_H