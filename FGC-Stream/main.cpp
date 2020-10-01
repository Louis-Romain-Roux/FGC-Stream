#include "FGC-Stream.h"
#include "Transaction.h"
#include <iostream>
#include <fstream>
#include <chrono>

uint32_t NODE_ID = 0;
uint32_t minSupp = 3;
uint32_t totalGens = 0;
const uint32_t windowSize = 100;

std::set<uint32_t>* TListByID[windowSize];

int testedJp = 0;
float sumJp = 0;
float actgen = 0;

void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::multimap<uint32_t, ClosedIS*> fGenitors;

    std::vector<ClosedIS*>* newClosures = new std::vector<ClosedIS*>;

    // do we really need to keep newClosures here ?

    descend(root, emptySet, t_n, &fGenitors, ClosureList, newClosures);

    filterCandidates(&fGenitors, root, ClosureList);
    
    computeJumpers(root, t_n, newClosures, TList, root, ClosureList);

    for (std::vector<ClosedIS*>::iterator jClos = newClosures->begin(); jClos != newClosures->end(); ++jClos) {
        std::set<std::set<uint32_t>*> preds = compute_preds_exp(*jClos);
        //std::set<std::set<uint32_t>*> preds = computePreds(*jClos);

        uint32_t key = CISSum((*jClos)->itemset);

        for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {

            ClosedIS* predNode = findCI(**pred, ClosureList);
            predNode->succ->insert(std::make_pair(key, *jClos));
            (*jClos)->preds->insert(std::make_pair(CISSum(**pred), predNode));

        }
    
    }

    //std::cout << testedJp << " jumpers tested.\n";
    closureReset(ClosureList); // This is needed to set all visited flags back to false and clear the candidate list
}


void Deletion(std::set<uint32_t> t_0, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
    TList->remove(t_0, n);
    std::vector<ClosedIS*>* iJumpers = new std::vector<ClosedIS*>;
    std::multimap<uint32_t, ClosedIS*>* fObsoletes = new std::multimap<uint32_t, ClosedIS*>;

    descendM(root, t_0, ClosureList, iJumpers, fObsoletes);

    for (std::vector<ClosedIS*>::iterator jprIt = iJumpers->begin(); jprIt != iJumpers->end(); ++jprIt) {
        ClosedIS* jumper = *jprIt;
        dropJumper(jumper, ClosureList);
    }

    for (std::multimap<uint32_t, ClosedIS*>::reverse_iterator obsIt = fObsoletes->rbegin(); obsIt != fObsoletes->rend(); ++obsIt) {
        ClosedIS* obsolete = obsIt->second;
        dropObsolete(obsolete, ClosureList, root);
    }

    closureReset(ClosureList);
}



// Helper functions
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

void printClosureOrder(std::multimap<uint32_t, ClosedIS*> ClosureList) {
    for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList.begin(); clos != ClosureList.end(); ++clos) {
        ClosedIS currCI = *clos->second;
        std::cout << "Closed itemset { ";
        for (auto item : currCI.itemset) {
            std::cout << item << " ";
        }
        std::cout << "} (" << currCI.support << ") has children : ";
        for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.succ->begin(); child != currCI.succ->end(); ++child) {
            ClosedIS currChild = *child->second;
            std::cout << "{";
            for (auto item : currChild.itemset) {
                std::cout << item << " ";
            }
            std::cout << "}, ";
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
    std::ifstream input("Datasets/retail.txt");
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

        if (i == 1000) {
            i++; i--;
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
        

        if (i % 50 == 0) {
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
    printClosureOrder(ClosureList);


    return 0;
}