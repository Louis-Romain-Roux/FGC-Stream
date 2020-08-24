#include "FGC-Stream.h"
#include "Transaction.h"
#include <iostream>
#include <fstream>
#include <chrono>

uint32_t NODE_ID = 0;
uint32_t minSupp = 3;
uint32_t totalGens = 0;

#ifndef _TM_STYLE_
void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
   std::set<uint32_t> emptySet;
#else
void Addition(std::set<uint32_t>* const t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList, uint32_t* const _bufferMetrics, std::vector<uint32_t>* const intersect,
  std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2) {
#endif
    TList->add(t_n, n); 
    std::multimap<uint32_t, ClosedIS*> fGenitors;
#ifndef _TM_STYLE_
    descend(root, emptySet, t_n, &fGenitors, ClosureList);
#else
    descend(root, t_n, &fGenitors, ClosureList);
#endif

    filterCandidates(&fGenitors, root);
#ifndef _TM_STYLE_
    std::vector<ClosedIS*> newClosures;
    computeJumpers(root, t_n, newClosures, TList, root, ClosureList);
#else
    //This one could be kept as a buffer across all shifts
    std::vector<ClosedIS*> newClosures;// = new std::vector<ClosedIS*>();
    // = new std::vector<uint32_t>();//&std::vector<uint32_t>();
    intersect->resize(t_n->size());
    uint32_t candidates_tested = computeJumpers(intersect, root, t_n, &newClosures, TList, root, ClosureList, _bufferMetrics, _buffer_1, _buffer_2);
    //delete intersect;
    //std::cout << "candidates jumpers " << candidates_tested << " w/ |trx|=" << t_n->size() << " |inter|=" << _bufferMetrics[0] << " |n|=" << _bufferMetrics[2] <<  " |n.succ|=" << (_bufferMetrics[1]/(float)_bufferMetrics[2]) << std::endl;
#endif
    
    closureReset(ClosureList); // This is needed to set all visited flags back to false and clear the candidate list
#ifdef _TM_STYLE_
    //delete newClosures;
#endif
}

// Helper function
void printAllGens(GenNode* node) {
    totalGens = 0;
    for (auto child : *node->succ) {
        printAllGens(child.second);
    }
    std::cout << node->clos->support << " ";
#ifndef _TM_STYLE_
    std::set<uint32_t> itemset = node->items();
#else
    std::set<uint32_t> itemset;
    node->items(&itemset);
#endif
    for (auto item : itemset) {
        std::cout << item << " ";
    }
    //if (itemset.empty()) {
    //    std::cout << "";//empty set
    //}
    totalGens++;
    std::cout << "\n";
}


#ifndef _TM_STYLE_
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
#else
void printAllClosuresWithGens(std::multimap<uint32_t, ClosedIS*>* ClosureList) {
  totalGens = 0;
  std::multimap<uint32_t, ClosedIS*>::iterator it = ClosureList->begin();
  for(; it != ClosureList->end(); ++it){
    ClosedIS* const currCI = it->second;
    std::cout << "Closed itemset { ";
    std::set<uint32_t>::iterator it2 = currCI->itemset->begin();
    for(; it2 != currCI->itemset->end(); ++it2){
    //for (auto item : currCI->itemset) {
      const uint32_t item = *it2;
      std::cout << item << " ";
    }
    std::cout << "} (" << currCI->support << ") has generators : ";
    std::set<GenNode*>::iterator it3 = currCI->gens->begin();
    for (; it3 != currCI->gens->end(); ++it3) {
      GenNode* const gen = *it3;
      totalGens++;
      std::cout << "{ ";
#ifndef _TM_STYLE_
      std::set<uint32_t> items = gen->items();
#else
      std::set<uint32_t> items;
      gen->items(&items);
#endif
      for (const uint32_t item : items) {
        std::cout << item << " ";
      }
      std::cout << "} ";
    }
    std::cout << "\n";

  }
}
#endif




int main(int argc, char** argv)
{
    auto start = std::chrono::high_resolution_clock::now();
    const uint32_t window_size = 1000;
    if (argc < 2) return 1;
    minSupp = strtoul(argv[2], 0, 10);//1;
    std::ifstream input(/*"Datasets/in.txt"*/argv[1]);
    char s[10000];
    uint32_t i = 0;

    TIDList* TList = new TIDList();

    std::set<uint32_t> closSet;
#ifndef _TM_STYLE_
    std::multimap<uint32_t, ClosedIS*> ClosureList;
#else
    TList->TransactionList = new std::map<uint32_t, std::set<uint32_t>*>();
    TList->singletonSupport = new std::map<uint32_t, uint32_t>();
    //std::set<uint32_t>* closSet = new std::set<uint32_t>();
    std::multimap<uint32_t, ClosedIS*>* ClosureList = new std::multimap<uint32_t, ClosedIS*>();
#endif
    input.getline(s, 10000);
    char* pch = strtok(s, " ");
    Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
    i++;
    std::vector<uint32_t> closSetvec = *new_transaction.data();
    closSet.insert(closSetvec.begin(), closSetvec.end());
#ifndef _TM_STYLE_
    TList->add(closSet, i);
#else
    TList->add(&closSet, i);
#endif

    while (i < minSupp) {
        input.getline(s, 10000);
        char* pch = strtok(s, " ");
        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        i++;
        std::vector<uint32_t> closSetvec = *new_transaction.data();

        std::set<uint32_t> closSetPart(closSetvec.begin(), closSetvec.end());
#ifndef _TM_STYLE_
        TList->add(closSetPart, i);
#else
        //std::set<uint32_t> closSetPartCopy(closSetvec.begin(), closSetvec.end());
        TList->add(&closSetPart, i);
#endif

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

#ifndef _TM_STYLE_
    ClosedIS EmptyClos(closSet, minSupp, &ClosureList); 
#else
    std::set<uint32_t>* emptyClosSet = new std::set<uint32_t>(closSet.begin(), closSet.end());
    ClosedIS EmptyClos(emptyClosSet, minSupp, ClosureList);
    uint32_t* bufferMetrics = new uint32_t[3];

    std::vector<uint32_t>* intersectionBuffer = new std::vector<uint32_t>();//&std::vector<uint32_t>();
    std::vector<uint32_t>* supportBuffer1 = new std::vector<uint32_t>();
    std::vector<uint32_t>* supportBuffer2 = new std::vector<uint32_t>();
    supportBuffer1->resize(5000);
    supportBuffer2->resize(5000);
#endif
    GenNode* root = new GenNode(0, nullptr, &EmptyClos);

    while (input.getline(s, 10000)) {
        i++;

        char* pch = strtok(s, " ");

        Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
        std::vector<uint32_t> t_nVec = *new_transaction.data();

#ifndef _TM_STYLE_
        // why copy the transaction and cast as set ?
        std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());   
        Addition(t_n, i, root, TList, &ClosureList);
#else
        bufferMetrics[0] = 0;
        bufferMetrics[1] = 0;
        bufferMetrics[2] = 0;
        //stack allocation instead ?
        std::set<uint32_t> t_n = std::set<uint32_t>(t_nVec.begin(), t_nVec.end());
        Addition(&t_n, i, root, TList, ClosureList, bufferMetrics, intersectionBuffer, supportBuffer1, supportBuffer2);
        //delete t_n;
#endif
        if (i % 100 == 0) {
          std::cout << i << " transaction(s) processed" << std::endl;
        }

        if (i % 2500 == 0) {
          break;
        }

        if (i % 500 == 0) {
            auto stop = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " milliseconds elapsed between start and current transaction" << std::endl;
        }
    }
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " milliseconds elapsed between start and current transaction" << std::endl;

    std::cout << "Displaying all found generators as of transaction " << i << " :\n";
    //printAllClosuresWithGens(ClosureList);
    //printAllGens(root);
    std::cout << "Total number of generators: " << totalGens << "\n";

#ifdef _TM_STYLE_
    delete intersectionBuffer;
    delete supportBuffer1;
    delete supportBuffer2;
    // memory should be freed here (gen tree and closed
#endif
    return 0;
}