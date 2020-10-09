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
bool extratext = false;


void Addition(std::set<uint32_t> t_n, int n, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

    TList->add(t_n, n);
    std::set<uint32_t> emptySet;
    std::multimap<uint32_t, ClosedIS*> fGenitors;


    descend(root, emptySet, t_n, &fGenitors, ClosureList);
    if (extratext) {
        std::cout << "filterCandidates" << std::endl;
    }
    filterCandidates(&fGenitors, root, ClosureList);
    std::vector<ClosedIS*>* newClosures = new std::vector<ClosedIS*>;
    if (extratext) {
        std::cout << "computeJumpers" << std::endl;
    }
    computeJumpers(root, t_n, newClosures, TList, root, ClosureList);
    if (extratext) {
        std::cout << "endloop " << newClosures->size() << std::endl;
    }
    for (std::vector<ClosedIS*>::iterator jClos = newClosures->begin(); jClos != newClosures->end(); ++jClos) {
        if (extratext) {
            std::cout << " -------- " << (*jClos)->itemset.size() << " ------ " << t_n.size() << std::endl;
        }

        std::set<std::set<uint32_t>*> preds = compute_preds_exp(*jClos);
        //std::set<std::set<uint32_t>*> preds = std::set<std::set<uint32_t>*>();
        //compute_generators_v2(&preds, *jClos);

        /*if (preds.size() != 0) {
          std::cout << "" << std::endl;
        }*/

        uint32_t key = CISSum((*jClos)->itemset);
        if (extratext) {
            std::cout << "|preds|=" << preds.size() << std::endl;
        }
        //for (std::vector<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {
        for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {
            // pour chaque predecesseur, on le retrouve via son intent
            ClosedIS* predNode = findCI(**pred, ClosureList);
            if (predNode) {
              // puis on link dans les deux sens
              predNode->succ->insert(std::make_pair(key, *jClos));
              (*jClos)->preds->insert(std::make_pair(CISSum(**pred), predNode));
            }
            else {
              std::cout << "oh pred is null..." << std::endl;
            }
            delete *pred;
        }
    }
    if (extratext) {
        std::cout << "reseting" << std::endl;
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
void printAllGens(GenNode* node, std::ostream& _output) {
  //totalGens = 0;
  if (node->succ) {
    for (auto child : *node->succ) {
      printAllGens(child.second, _output);
    }
  }
  _output << node->clos->support << " ";
  std::set<uint32_t> itemset = node->items();
  for (auto item : itemset) {
    _output << item << " ";
  }
  //totalGens++;
  _output << "\n";
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


void printAllClosuresWithGensTM(std::multimap<uint32_t, ClosedIS*> ClosureList, std::ostream& f) {
  //totalGens = 0;
  for (auto x : ClosureList) {
    ClosedIS currCI = *x.second;
    f << "s=" << currCI.support << " fermeture : ";
    for (auto item : currCI.itemset) {
      f << item << " ";
    }
    f << "generateurs : ";
    uint32_t cursor = 0;
    for (auto gen : currCI.gens) {
      if (cursor != 0) {
        f << "  ";
      }
      for (auto item : gen->items()) {
        f << item << " ";
      }
      cursor += 1;
    }
    f << "\n";
  }
}

void printClosureOrderTM(std::multimap<uint32_t, ClosedIS*> ClosureList, std::ostream& f) {
  for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList.begin(); clos != ClosureList.end(); ++clos) {
    ClosedIS currCI = *clos->second;
    uint32_t cursor = 0;
    f << "#" << currCI.id << "[";
    for (auto item : currCI.itemset) {
      if (cursor != 0) {
        f << ", ";
      }
      f << item;
      cursor += 1;
    }
    f << "]|s=" << currCI.support << " => {";
    cursor = 0;
    for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.preds->begin(); child != currCI.preds->end(); ++child) {
      ClosedIS currChild = *child->second;
      if (cursor != 0) {
        f << ", ";
      }
      f << "#" << currChild.id;
      cursor += 1;
    }
    f << "}\n";
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
        std::cout << " has predecessors : ";
        for (std::multimap<uint32_t, ClosedIS*>::iterator child = currCI.preds->begin(); child != currCI.preds->end(); ++child) {
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

void sanityCheck(GenNode* n) {
    if (n->clos == nullptr) {
        std::cout << "Sanity check failed for \"generator\" ";
        for (auto item : n->items()) {
            std::cout << item << " ";
        }
        std::cout << "\n";
    }
    for (std::map<uint32_t, GenNode*>::iterator child = n->succ->begin(); child != n->succ->end(); ++child) {
        sanityCheck(child->second);
    }
}

//only use this for (very) small datasets
void sanityCheck_full(ClosedIS* clos, TIDList* TList) {
    std::set<uint32_t> cIS = clos->itemset;
    std::set<uint32_t> closTL;

    if(!cIS.empty())closTL = TList->getISTL(clos->itemset);

    for (std::set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
        std::set<uint32_t> items = (*genIt)->items();
        if (!items.empty()){
            std::set<uint32_t> genTL = TList->getISTL(items);
            if (closTL != genTL) {
                std::cout << "pseudo-full SC failed !\n";
            }
        }
    }
}



// template override due to what seems to be a VS bug ?
// comment this function, and compare readings of trx with debug/release configs
// if differences between D and R, then nasty bug still present...
void Transaction<uint32_t>::load(char* _s, const char* _delims, const short _withcrc) {
  uint32_t v;
  uint32_t oldV = 1 << 31;
  clean();
  char* pch = _s;
  while (pch != 0) {
    std::from_chars(pch, pch + strlen(pch), v);
    //std::cout << "item is " << v << std::endl;
    //Stupid hack to avoid blank spaces at end of lines
    if (v != oldV) {
      __data.push_back(v);
    }
    pch = strtok(0, _delims);
    oldV = v;
  }
};

int main(int argc, char** argv)
{
  for (int k = 0; k < windowSize; k++) {
    TListByID[k] = new std::set<uint32_t>;
  }

  auto start = std::chrono::high_resolution_clock::now();


  uint32_t exitAt = 0;
  if (argc < 3) return 1;
  minSupp = strtoul(argv[2], 0, 10);//1;
  if (argc >= 4) {
    exitAt = strtoul(argv[3], 0, 10);//1;
  }
  std::ifstream input(/*"Datasets/in.txt"*/argv[1]);
  char s[10000];
  uint32_t i = 0;

  char* output_cis_gen = 0;
  char* output_order = 0;
  if (argc >=5) {
    output_cis_gen = argv[4];
  }
  if (argc >= 6) {
    output_order = argv[5];
  }

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
  GenNode* root = new GenNode(1 << 31, nullptr, &EmptyClos);
  while (input.getline(s, 10000)) {
    i++;

    if (ClosureList.find(160152528) != ClosureList.end()) {
        std::cout << "OK\n";
    }

    if (i == 121) {
        i++; i--;
    }

    char* pch = strtok(s, " ");
    /*
    sanityCheck(root);
    for (std::multimap<uint32_t, ClosedIS*>::iterator closIT = ClosureList.begin(); closIT != ClosureList.end(); ++closIT) {
        ClosedIS* clos = closIT->second;
        sanityCheck_full(clos, TList);
    }
    */
    Transaction<uint32_t> new_transaction = Transaction<uint32_t>(pch, " ", 0);
    std::vector<uint32_t> t_nVec = *new_transaction.data();

    std::set<uint32_t> t_n(t_nVec.begin(), t_nVec.end());

    Addition(t_n, i, root, TList, &ClosureList);


    if (i > windowSize) {
        Deletion(*TListByID[i%windowSize], i-windowSize, root, TList, &ClosureList);
    }

    TListByID[i % windowSize]->clear();
    TListByID[i % windowSize]->insert(t_n.begin(), t_n.end());
    

    if (i % 1 == 0) {
      std::cout << i << " transactions processed" << std::endl;
    }
    if (i % 500 == 0) {
      auto stop = std::chrono::high_resolution_clock::now();
      std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " milliseconds elapsed between start and current transaction" << std::endl;
    }
    if (i == exitAt) {
      break;
    }
  }
  std::cout << "Displaying all found generators as of transaction " << i << " :\n";

  //printAllClosuresWithGensTM(ClosureList, output_cis_gen);
    if (output_cis_gen) {
      std::ofstream f;
      f.open(output_cis_gen);
      //printAllGens(root, f);
      printAllClosuresWithGensTM(ClosureList, f);
      f.close();
    }
    //"./output-cis-gens.txt"
    //printAllClosuresWithGens(ClosureList);
    std::cout << "Total number of generators: " << totalGens << "\n";
    //printClosureOrder(ClosureList);

    if (output_order) {
      std::ofstream f2;
      f2.open(output_order);
      printClosureOrderTM(ClosureList, f2);
      f2.close();
    }
    //printClosureOrder(ClosureList);
    return 0;
}