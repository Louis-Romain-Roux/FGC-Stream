#include "FGC-Stream.h"
#include "Transaction.h"
#include <iostream>
#include <fstream>


uint32_t NODE_ID = 0;
uint32_t minSupp = 3;
uint32_t totalGens = 0;

void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::multimap<uint32_t, ClosedIS*> fGenitors;

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
        std::cout << "} (" << currCI.support << ") has generators : ";
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
    const uint32_t window_size = 1000;
<<<<<<< HEAD
    if (argc < 2) return 1;
    minSupp = strtoul(argv[2], 0, 10);//1;
    std::ifstream input(/*"Datasets/in.txt"*/argv[1]);
    char s[10000];
    uint32_t i = 0;

    TIDList* TList = new TIDList();


    std::set<uint32_t> closSet;
    std::multimap<uint32_t, ClosedIS*> ClosureList;

    input.getline(s, 10000);
    char* pch = strtok(s, " ");
    Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
    i++;
    std::vector<uint32_t> closSetvec = *new_transaction.data();
    closSet.insert(closSetvec.begin(), closSetvec.end());
    TList->add(closSet, i);

    while (i < minSupp) {
        input.getline(s, 10000);
        char* pch = strtok(s, " ");
        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        i++;
        std::vector<uint32_t> closSetvec = *new_transaction.data();

        std::set<uint32_t> closSetPart(closSetvec.begin(), closSetvec.end());
        TList->add(closSetPart, i);

        std::set<uint32_t>::iterator it1 = closSet.begin();
        std::set<uint32_t>::iterator it2 = closSetPart.begin();

        while ((it1 != closSet.end()) && (it2 != closSetPart.end())) {
            if (*it1 < *it2) {
                closSet.erase(it1++);
            }
            else if (*it2 < *it1) {
                ++it2;
            }
            else {
                ++it1;
                ++it2;
            }
        }
        closSet.erase(it1, closSet.end());
        
    }


    ClosedIS EmptyClos(closSet, minSupp, &ClosureList); 
    GenNode* root = new GenNode(0, nullptr, &EmptyClos);

    


    while (input.getline(s, 10000)) {
        i++;

        char* pch = strtok(s, " ");

        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        std::vector<uint32_t> t_nVec = *new_transaction.data();

        std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());


        Addition(t_n, i, root, TList, &ClosureList);


        if (i % 10 == 0) {
            std::cout << i << " transaction(s) processed" << std::endl;
        }
    }
    std::cout << "Displaying all found generators as of transaction " << i << " :\n";
    //printAllClosuresWithGens(ClosureList);
    printAllGens(root);
    std::cout << "Total number of generators: " << totalGens << "\n";


    return 0;
}