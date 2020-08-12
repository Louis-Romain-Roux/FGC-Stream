#include "FGC-Stream.h"
#include "Transaction.h"
#include <iostream>
#include <fstream>

uint32_t NODE_ID = 0;
uint32_t minSupp = 1;
uint32_t totalGens = 0;

void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::vector<ClosedIS*> fGenitors;

    descend(root, emptySet, t_n, &fGenitors, ClosureList);

    filterCandidates(&fGenitors, root);
    std::vector<ClosedIS*> newClosures;

    computeJumpers(root, t_n, newClosures, TList, root, ClosureList);

    resetStatus(root); // This is needed to set all visited flags back to false and clear the candidate list
}

// Helper function
void printAllGens(GenNode* node) {
    totalGens = 0;
    for (auto child : *node->succ) {
        printAllGens(child.second);
    }
    std::cout << node->clos->support << " ";
    std::set<uint32_t> itemset = node->items();
    for (auto item : itemset) {
        std::cout << item << " ";
    }
    //if (itemset.empty()) {
    //    std::cout << "";//empty set
    //}
    totalGens++;
    std::cout << "\n";
}

void printAllClosuresWithGens(std::multimap<uint32_t, ClosedIS*> ClosureList) {
    totalGens = 0;
    for (auto x : ClosureList) {
        ClosedIS currCI = *x.second;
        std::cout << "Closed itemset { ";
        for (auto item : currCI.itemset) {
            std::cout << item << " ";
        }
        std::cout << "} has generators : ";
        for (auto gen : currCI.gens) {
            totalGens++;
            std::cout << "{ ";
            for (auto item : gen->items()) {
                std::cout << item << " ";
            }
            std::cout << "} ";
        }
        std::cout << "\n";
    }
}


int main(int argc, char** argv)
{
    // We get the first transaction manually, which is necessary as no itemset is present --> the algorithm will not work correctly
    // This means that it only works for minSupp = 1
    // TODO : get the intersection of the first minSupp transaction, then set root's closure as that intersection
    const uint32_t window_size = 1000;
    if (argc < 2) return 1;
    minSupp = strtoul(argv[2], 0, 10);//1;
    std::ifstream input(/*"Datasets/in.txt"*/argv[1]);
    char s[10000];
    uint32_t i = 0;
    input.getline(s, 10000);
    char* pch = strtok(s, " ");

    std::multimap<uint32_t, ClosedIS*> ClosureList;
    Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
    i++;
    std::vector<uint32_t> closSetvec = *new_transaction.data();

    std::set<uint32_t> closSet(closSetvec.begin(), closSetvec.end());

    ClosedIS EmptyClos(closSet, 1, &ClosureList); 
    GenNode* root = new GenNode(0, nullptr, &EmptyClos);
    TIDList* TList = new TIDList();

    TList->add(closSet, i);


    while (input.getline(s, 10000)) {
        i++;

        char* pch = strtok(s, " ");

        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        std::vector<uint32_t> t_nVec = *new_transaction.data();

        std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());

        Addition(t_n, i, root, TList, &ClosureList);
        

        if (i % 500 == 0) {
            std::cout << i << " transaction(s) processed" << std::endl;
        }
    }
    std::cout << "Displaying all found generators as of transaction " << i << " :\n";
    //printAllClosuresWithGens(ClosureList);
    printAllGens(root);
    std::cout << "Total number of generators: " << totalGens << "\n";


    return 0;
}