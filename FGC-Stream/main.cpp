#include "FGC-Stream.h"
#include "Transaction.h"
#include <iostream>
#include <fstream>
#include <chrono>

uint32_t NODE_ID = 0;
uint32_t minSupp = 1;
uint32_t totalGens = 0;
const uint32_t windowSize = 6;

std::set<uint32_t>* TListByID[windowSize];

int testedJp = 0;
float sumJp = 0;
float actgen = 0;

void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::multimap<uint32_t, ClosedIS*> fGenitors;

    descend(root, emptySet, t_n, &fGenitors, ClosureList);

    filterCandidates(&fGenitors, root);
    std::vector<ClosedIS*> newClosures;
    
    testedJp = 0;
    computeJumpers(root, t_n, newClosures, TList, root, ClosureList);
    //std::cout << testedJp << " jumpers tested.\n";
    sumJp += testedJp;
    if (n % 100 == 0) {
        std::cout << sumJp/(n+1) << " average jumpers.\n";
    }
    closureReset(ClosureList); // This is needed to set all visited flags back to false and clear the candidate list
}

void Deletion(std::set<uint32_t> t_0, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
    
    std::set<uint32_t> X;
    std::set<GenNode*>* iJumpers = new std::set<GenNode*>;
    std::set<ClosedIS*>* obsolClos = new std::set<ClosedIS*>;

    TList->remove(t_0, n);
    descendM(root, X, t_0, false, iJumpers, ClosureList, root, obsolClos);
    jumperReset(iJumpers);
    cleanJumpers(iJumpers, obsolClos, ClosureList);
    closureReset(ClosureList);

}

// Helper function
void printAllGens(GenNode* node) {
    totalGens = 0;
    for (auto child : *node->succ) {
        printAllGens(child.second);
    }
    std::set<uint32_t> itemset = node->items();
    for (auto item : itemset) {
        std::cout << item << " ";
    }
    if (itemset.empty()) {
        std::cout << "empty set";
    }
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


int main()
{

    for (int k = 0; k < windowSize; k++) {
        TListByID[k] = new std::set<uint32_t>;
    }

    auto start = std::chrono::high_resolution_clock::now();
    const uint32_t window_size = 1000;
    std::ifstream input("Datasets/in.txt");
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
    TListByID[i % windowSize]->insert(closSetvec.begin(), closSetvec.end());
    
    closSet.insert(closSetvec.begin(), closSetvec.end());
    TList->add(closSet, i);

    while (i < minSupp) {
        input.getline(s, 10000);
        char* pch = strtok(s, " ");
        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        i++;
        std::vector<uint32_t> closSetvec = *new_transaction.data();
        TListByID[i % windowSize]->insert(closSetvec.begin(), closSetvec.end());

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
        if (root->succ->find(39) != root->succ->end()) {
            std::cout << (*root->succ)[39]->clos->visited;

        }
        std::cout << "Processing xact " << i << std::endl;
        if (i==8) {
            std::cout << "break";
        }

        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        std::vector<uint32_t> t_nVec = *new_transaction.data();

        std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());


        Addition(t_n, i, root, TList, &ClosureList);

        if (i > windowSize) {
            Deletion(*TListByID[i%windowSize], i-windowSize, root, TList, &ClosureList);
        }
        TListByID[i % windowSize]->clear();
        TListByID[i % windowSize]->insert(t_n.begin(), t_n.end());

        if (i % 100 == 0) {
            std::cout << i << " transactions processed" << std::endl;
        }
        if (i % 500 == 0) {
            auto stop = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " milliseconds elapsed between start and current transaction" << std::endl;
        }
        
    }
    std::cout << "Displaying all found generators as of transaction " << i << " :\n";
    printAllClosuresWithGens(ClosureList);
    std::cout << "Total number of generators: " << totalGens << "\n";


    return 0;
}