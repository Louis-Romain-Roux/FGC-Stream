#pragma once

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <numeric>


struct ClosedIS;
struct GenNode;
struct TIDList;

extern uint32_t NODE_ID;
extern uint32_t minSupp;
extern int testedJp;
extern float actgen;

void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::multimap < uint32_t, ClosedIS* >* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList);

void filterCandidates(std::multimap < uint32_t, ClosedIS* >* fGenitors, GenNode* root);
GenNode* genLookUp(std::set<uint32_t> iset, GenNode* root);

void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList);
std::pair<bool, ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList);

void resetStatus(GenNode*);
void closureReset(std::multimap<uint32_t, ClosedIS*>* ClosureList);
ClosedIS* findCI(std::set<uint32_t> itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList);
int CISSum(std::set<uint32_t> Itemset);

bool descendM(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, bool met, std::set<GenNode*>* iJumpers, std::multimap<uint32_t, ClosedIS*>* ClosureList, GenNode* root, std::set<ClosedIS*>* obsolClos);
bool processObsolete(GenNode* n, std::set<uint32_t> X, bool met, GenNode* root, std::set<ClosedIS*>* obsolClos);
void cleanJumpers(std::set<GenNode*>* iJumpers, std::set<ClosedIS*>* obsolClos, std::multimap<uint32_t, ClosedIS*>* ClosureList);
void jumperReset(std::set<GenNode*>* iJumpers);

struct GenNode {
	uint32_t item;
	uint32_t id;

	std::map<uint32_t, GenNode*>* succ;
	GenNode* parent;
	ClosedIS* clos;

	std::set<uint32_t> items();
	GenNode(uint32_t, GenNode*, ClosedIS*);
	// TODO : find out why this destructor causes issues, and fix them
	/* 
	~GenNode() {
		if (succ) {
			for (const std::pair<uint32_t, GenNode*> child : *succ) {
				delete child.second;
			}
		}
		delete succ;
	}
	*/
};

struct ClosedIS {
	std::set<uint32_t> itemset;
	std::set<GenNode*> gens;
	uint32_t support;
	bool visited;
	ClosedIS* newCI;
	std::set < std::set<uint32_t>* > candidates;

	ClosedIS(std::set<uint32_t> itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*>* ClosureList);
};

struct TIDList {
	std::map<uint32_t, std::set<uint32_t>*> TransactionList;
	std::map<uint32_t, uint32_t> singletonSupport;

	void add(std::set<uint32_t> t_n, uint32_t n);
	void remove(std::set<uint32_t> t_0, uint32_t n);
	int supp_from_tidlist(std::set<uint32_t> itemset);
	int supp_singleton(uint32_t item);
	bool closureIncludes(std::set<uint32_t> currList, uint32_t item);
	std::set<uint32_t> getISTL(std::set<uint32_t> itemset);
	int singleInterSupp(uint32_t item, std::set<uint32_t> currTIDList);
};