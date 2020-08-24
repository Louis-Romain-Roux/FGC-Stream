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

#ifndef _TM_STYLE_
void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::multimap < uint32_t, ClosedIS* >* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#else
void descend(GenNode* n, std::set<uint32_t>* t_n, std::multimap<uint32_t, ClosedIS*>* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#endif

void filterCandidates(std::multimap<uint32_t, ClosedIS*>* fGenitors, GenNode* root);

#ifndef _TM_STYLE_
GenNode* genLookUp(std::set<uint32_t> iset, GenNode* root);
#else
GenNode* genLookUp(std::set<uint32_t>* const iset, GenNode* const root);
#endif

#ifndef _TM_STYLE_
void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList);
std::pair<bool, ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#else
uint32_t computeJumpers(std::vector<uint32_t>* intersect, GenNode* const n, std::set<uint32_t>* const t_n, 
	std::vector<ClosedIS*>* const newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* 
	const ClosureList, uint32_t* const _bufferMetrics, 	
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2);
std::pair<bool, ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t>* const t_n, std::vector<ClosedIS*>* const newClosures, 
	GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList, 
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2);
#endif

#ifndef _TM_STYLE_
int update_singleton_itemsets(GenNode* const n, std::set<uint32_t> const t_n, std::vector<ClosedIS*> const newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* const ClosureList);
#else
int update_singleton_itemsets(GenNode* const n, std::set<uint32_t>* const t_n, std::vector<ClosedIS*>* const newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* const ClosureList,
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2);
#endif


#ifndef _TM_STYLE_
uint32_t compute_succ_intersection(std::set<uint32_t>& buffer, GenNode* n, std::set<uint32_t>* t_n);
#else
uint32_t compute_succ_intersection(std::vector<uint32_t>* buffer, GenNode* n, std::set<uint32_t>* t_n);
#endif

void resetStatus(GenNode*);
void closureReset(std::multimap<uint32_t, ClosedIS*>* ClosureList);

#ifndef _TM_STYLE_
ClosedIS* findCI(std::set<uint32_t> itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#else
ClosedIS* findCI(std::set<uint32_t>* itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#endif


#ifndef _TM_STYLE_
int CISSum(std::set<uint32_t> Itemset);
#else
int CISSum(std::set<uint32_t>& Itemset);
#endif


struct GenNode {
	uint32_t item;
	uint32_t id;

	std::map<uint32_t, GenNode*>* succ;
	GenNode* parent;
	ClosedIS* clos;

#ifndef _TM_STYLE_
	std::set<uint32_t> items();
#else
	uint32_t items(std::set<uint32_t>* const _buffer);
#endif

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
#ifndef _TM_STYLE_
	std::set<uint32_t> itemset;
	std::set<GenNode*> gens;
	std::set<std::set<uint32_t>*> candidates;
#else
	std::set<uint32_t>* itemset = 0;
	std::set<GenNode*>* gens = 0;
	std::set<std::set<uint32_t>*>* candidates = 0;
#endif
	uint32_t support;
	bool visited;
	ClosedIS* newCI;
#ifndef _TM_STYLE_
	ClosedIS(std::set<uint32_t> itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#else
	ClosedIS(std::set<uint32_t>* itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*>* ClosureList);
#endif
};


struct TIDList {

#ifndef _TM_STYLE_ 
	std::map<uint32_t, std::set<uint32_t>*> TransactionList;
	std::map<uint32_t, uint32_t> singletonSupport;
#else
	std::map<uint32_t, std::set<uint32_t>*>* TransactionList;
	std::map<uint32_t, uint32_t>* singletonSupport;
#endif

#ifndef _TM_STYLE_
	int supp_from_tidlist(std::set<uint32_t> itemset);
	bool closureIncludes(std::set<uint32_t> currList, uint32_t item);
	int singleInterSupp(uint32_t item, std::set<uint32_t> currTIDList);
	std::set<uint32_t> getISTL(std::set<uint32_t> itemset);
	void add(std::set<uint32_t> t_n, uint32_t n);
	int supp_singleton(uint32_t item);
#else
	int supp_from_tidlist(std::set<uint32_t>* const itemset);
	bool closureIncludes(std::vector<uint32_t>* const currList, const uint32_t item);
	int singleInterSupp(const uint32_t item, std::vector<uint32_t>* const currTIDList);
	std::vector<uint32_t>* getISTL(std::set<uint32_t>* const itemset, std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2);
	void add(std::set<uint32_t>* const t_n, const uint32_t n);
	int supp_singleton(const uint32_t item);
#endif
};