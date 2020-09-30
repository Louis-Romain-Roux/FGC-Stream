#pragma once

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <numeric>


using namespace std;

struct ClosedIS;
struct GenNode;
struct TIDList;
struct MinNode;

extern uint32_t NODE_ID;
extern uint32_t minSupp;
extern int testedJp;
extern float actgen;

void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::multimap < uint32_t, ClosedIS* >* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList, std::vector<ClosedIS*>* newClosures);

void filterCandidates(std::multimap < uint32_t, ClosedIS* >* fGenitors, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList);
GenNode* genLookUp(std::set<uint32_t> iset, GenNode* root);

void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*>* newClosures, TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList);
std::pair<bool, ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*>* newClosures, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList);

std::set<std::set<uint32_t>*> computePreds(ClosedIS* clos);
std::set<std::set<uint32_t>*> compute_preds_efficient(ClosedIS* clos);

std::set<std::set<uint32_t>*> compute_preds_exp(ClosedIS* clos);
void grow_generator(vector<set<uint32_t>>* _generators,
	vector<vector<uint32_t>>* _faces, MinNode* const _parent_node, MinNode* const _root, vector<MinNode*>* const _nodes);
bool is_valid_candidate(MinNode* const _parent_node, const uint32_t _item, vector<size_t>* const _fid_out, MinNode* const _root);
MinNode* get_from_path(vector<uint32_t>* const _path, MinNode* const _root);


void descendM(GenNode* n, std::set<uint32_t> t_0, std::multimap<uint32_t, ClosedIS*>* ClosureList, std::vector<ClosedIS*>* iJumpers, std::multimap<uint32_t, ClosedIS*>* fObsoletes);
ClosedIS* findGenitor(ClosedIS* clos, std::set<uint32_t> t_0);
void dropObsolete(ClosedIS* clos, std::multimap<uint32_t, ClosedIS*>* ClosureList, GenNode* root);
void dropObsoleteGs(GenNode* root, ClosedIS* clos);
void removeChildren(GenNode* gen);
void innerDelete(GenNode* gen);
void dropJumper(ClosedIS* clos, std::multimap<uint32_t, ClosedIS*>* ClosureList);



void resetStatus(GenNode*);
void closureReset(std::multimap<uint32_t, ClosedIS*>* ClosureList);
ClosedIS* findCI(std::set<uint32_t> itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList);
int CISSum(std::set<uint32_t> Itemset);

std::ostream& operator<<(std::ostream& os, ClosedIS CI);

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
	ClosedIS* gtr;
	std::set < std::set<uint32_t>* > candidates;
	std::multimap<uint32_t, ClosedIS*>* succ;
	std::multimap<uint32_t, ClosedIS*>* preds;



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

struct MinNode {
	MinNode* parent;
	vector<size_t>* fidset;
	uint32_t item;
	set<uint32_t>* generator;
	map<uint32_t, MinNode*>* children;
};

