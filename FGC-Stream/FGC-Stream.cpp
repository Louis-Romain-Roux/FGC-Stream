#include <iostream>
#include "FGC-Stream.h"

#ifndef _TM_STYLE_
void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::multimap < uint32_t, ClosedIS* >* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
#else
void descend(GenNode* n, std::set<uint32_t>* t_n, std::multimap<uint32_t, ClosedIS*>* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
#endif
	if (n->item != 0) { // 0 is reserved for the root, so n->item = 0 actually means n is the empty set
#ifndef _TM_STYLE_
		X.insert(n->item);
#else
		//X->insert(n->item);
#endif
	}

	std::set<uint32_t> iset;
#ifndef _TM_STYLE_
	std::set_intersection(n->clos->itemset.begin(), n->clos->itemset.end(), t_n.begin(), t_n.end(), std::inserter(iset, iset.begin()));
#else
	std::set_intersection(n->clos->itemset->begin(), n->clos->itemset->end(), t_n->begin(), t_n->end(), std::inserter(iset, iset.begin()));
#endif

	/*if (iset.size() != 0) {
		std::cout << iset.size() << std::endl;
	}*/

#ifndef _TM_STYLE_
	if (iset.size() == n->clos->itemset.size()) {
#else
	
	if (iset.size() == n->clos->itemset->size()) {
#endif
		if (!n->clos->visited) {
			n->clos->support++;
			n->clos->visited = true;
		}
	}
	else {
		ClosedIS* closure;
		if (!n->clos->visited) {
#ifndef _TM_STYLE_
			closure = findCI(iset, ClosureList);
			if (closure == nullptr) {
				//std::cout << "created new closure" << std::endl;
				closure = new ClosedIS(iset, n->clos->support + 1, ClosureList);
			}
#else
			closure = findCI(&iset, ClosureList);
			if (closure == nullptr) {
				std::set<uint32_t>* copy = new std::set<uint32_t>(iset.begin(), iset.end());
				//std::cout << "created new closure" << std::endl;
				// make a isset copy here
				closure = new ClosedIS(copy, n->clos->support + 1, ClosureList);
			}
#endif
			n->clos->newCI = closure;
			n->clos->visited = true;
#ifndef _TM_STYLE_
			fGenitors->insert(std::make_pair(n->clos->itemset.size(), n->clos));
#else
			fGenitors->insert(std::make_pair(n->clos->itemset->size(), n->clos));
#endif
		}
		else {
			closure = n->clos->newCI;
		}
		std::set<uint32_t> face;
#ifndef _TM_STYLE_
		closure->gens.insert(n);
		n->clos->gens.erase(n);
		std::set_difference(n->clos->itemset.begin(), n->clos->itemset.end(), iset.begin(), iset.end(), std::inserter(face, face.end()));
#else
		closure->gens->insert(n);
		n->clos->gens->erase(n);
		std::set_difference(n->clos->itemset->begin(), n->clos->itemset->end(), iset.begin(), iset.end(), std::inserter(face, face.end()));
#endif
		for (auto item : face) {
			std::set<uint32_t>* newIS = new std::set<uint32_t>;
#ifndef _TM_STYLE_
			newIS->insert(X.begin(), X.end());
#else
			std::set<uint32_t> X;
			n->items(&X);
			newIS->insert(X.begin(), X.end());
#endif
			newIS->insert(item);
#ifndef _TM_STYLE_
			n->clos->candidates.insert(newIS);
			//std::cout << "created new candidate" << std::endl;
#else
			if (!n->clos->candidates) {
				n->clos->candidates = new std::set<std::set<uint32_t>*>();
			}
			//std::cout << "created new candidate" << std::endl;
			n->clos->candidates->insert(newIS);
#endif
		}
		n->clos = closure;
	}
	if (n->succ != nullptr) {
#ifndef _TM_STYLE_
		for (auto x : *(n->succ)) {
			if (t_n.find(x.first) != t_n.end()) {
				descend(x.second, X, t_n, fGenitors, ClosureList);
			}
		}
#else
		// rewrote full loop to avoid copies
		// could be further improved with trx as vector and n->succ as dict ? (loop on trx, and keep cursors on the trie/trx)
		std::map<uint32_t, GenNode*>::iterator it = n->succ->begin();
		for(; it != n->succ->end(); ++it){
			if (t_n->find(it->first) != t_n->end()) {
				descend(it->second, t_n, fGenitors, ClosureList);
			}
		}
#endif
	}
}

void filterCandidates(std::multimap<uint32_t, ClosedIS*>* fGenitors, GenNode* root) {
	for (auto genitor : *fGenitors) {
#ifndef _TM_STYLE_
		for (auto iset : genitor.second->candidates) {
#else
		std::set<std::set<uint32_t>*>::iterator it = genitor.second->candidates->begin();
		for (; it != genitor.second->candidates->end(); ++it) {
			std::set<uint32_t>* iset = *it;
#endif
			bool sset = false;
#ifndef _TM_STYLE_
			for (auto n : genitor.second->gens) {
				std::set<uint32_t> itemset = n->items();
#else
			std::set<GenNode*>::iterator it2 = genitor.second->gens->begin();
			for (; it2 != genitor.second->gens->end(); ++it2) {
				GenNode* n = *it2;
				std::set<uint32_t> itemset;
				n->items(&itemset);
#endif

				if (std::includes(iset->begin(), iset->end(), itemset.begin(), itemset.end())) {
					sset = true;
					break;
				}
			}
			if (!sset) {
				uint32_t lastItem = *(iset->rbegin());
				iset->erase(std::prev(iset->end()));
#ifndef _TM_STYLE_
				std::set<uint32_t> copy = *iset;
				GenNode* father = genLookUp(copy, root);
#else
				GenNode* father = genLookUp(iset, root);
#endif
				if (father != nullptr) {
					//std::cout << "created new generator" << std::endl;
					GenNode* newGI = new GenNode(lastItem, father, genitor.second);
#ifndef _TM_STYLE_
					genitor.second->gens.insert(newGI);
#else
					genitor.second->gens->insert(newGI);
#endif
				}

			}
		}
	}
}


#ifndef _TM_STYLE_
void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, 
	TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
#else
uint32_t computeJumpers(std::vector<uint32_t>* const _bufferIntersect, GenNode* const n, 
	std::set<uint32_t>* const t_n, std::vector<ClosedIS*>* const newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* const ClosureList, uint32_t* const _bufferMetrics, 
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2) {
#endif

#ifndef _TM_STYLE_
	for (std::map<uint32_t, GenNode*>::const_iterator child = n->succ->begin(); child != n->succ->end(); child++) {
		computeJumpers(child->second, t_n, newClosures, TList, root, ClosureList);
	}
#else
	uint32_t candidates_tested = 0;

	if (n->succ) {
		//std::cout << "computeJumpers ?" << std::endl;
		std::map<uint32_t, GenNode*>::iterator child = n->succ->begin();
		for (; child != n->succ->end();) {
			GenNode* const tmp = (child++)->second;
			if (tmp->succ && tmp->succ->size() != 0 && t_n->find(tmp->item) != t_n->end()) {
				candidates_tested += computeJumpers(_bufferIntersect, tmp, t_n, newClosures, TList, root, ClosureList, _bufferMetrics, _buffer_1, _buffer_2);
			}
		}
	}
#endif

	/*for (std::map<uint32_t, GenNode*>::iterator child = n->succ->begin(); child != n->succ->end(); child++) {
		computeJumpers(child->second, t_n, newClosures, TList, root, ClosureList);
	}*/

#ifndef _TM_STYLE_
	std::set<uint32_t> intersect = std::set<uint32_t>();
	const uint32_t size = compute_succ_intersection(intersect, n, &t_n);
#else
	bool skip_loop = false;
	//std::vector<uint32_t>* intersect = new std::vector<uint32_t>();//&std::vector<uint32_t>();
	//_bufferIntersect->resize(t_n->size());
	uint32_t size;
	if (!n->succ) {
		size = 0;
	}
	else {
		size = compute_succ_intersection(_bufferIntersect, n, t_n);
	}

	//above 1 ?
	if (size > 1) {
		//_bufferIntersect->resize(size);
	}
	else {
		// skip below loop ?
		skip_loop = true;
		//delete intersect;
	}
	_bufferMetrics[1] += n->succ ? n->succ->size() : 0;
	_bufferMetrics[0] += size;
	_bufferMetrics[2] += 1;
#endif

#ifndef _TM_STYLE_
	for (std::set<uint32_t>::const_iterator lefti = intersect.begin(); lefti != intersect.end(); lefti++) {
		//GenNode* left = n->succ->at(*lefti);
#else
	//std::cout << "skip loop ? " << (uint32_t)skip_loop << std::endl;
	if (!skip_loop) {
		std::vector<uint32_t>::iterator lefti = _bufferIntersect->begin();
		std::vector<uint32_t>::iterator end = size + _bufferIntersect->begin();
		for (; lefti != /*_bufferIntersect->end()*/end; ++lefti) {
			//for (std::set<uint32_t>::const_iterator lefti = intersect.begin(); lefti != intersect.end(); lefti++) {
				//GenNode* left = n->succ->at(*lefti);
#endif
	//for(std::set<uint32_t>::const_iterator lefti = t_n.begin(); lefti!=t_n.end(); lefti++){
	//for (std::map<uint32_t, GenNode*>::const_iterator left = n->succ->begin(); left != n->succ->end(); left++) {
	//for (std::set<uint32_t>::const_iterator lefti = intersect.begin(); lefti != intersect.end(); lefti++) {
		//std::map<uint32_t, GenNode*>::iterator left = n->succ->find(*lefti);
#ifndef _TM_STYLE_
			GenNode* left = (*n->succ)[*lefti];
#else
			GenNode* left = n->succ->at(*lefti);//(*n->succ)[*lefti];
#endif

			//if (left != n->succ->end()) {
			//if (t_n.find(left->second->item) != t_n.end()) {
			std::set<uint32_t> candIS;
#ifndef _TM_STYLE_
			std::set<uint32_t> ISTL;
			for (std::set<uint32_t>::const_iterator righti = std::next(lefti); righti != intersect.end(); righti++) {
#else
			std::vector<uint32_t>* ISTL = nullptr;
			std::vector<uint32_t>::iterator righti = std::next(lefti);
			for (; righti != /*_bufferIntersect->end()*/end; ++righti) {
#endif

				//for (std::set<uint32_t>::const_iterator righti = std::next(lefti); righti != intersect.end(); righti++) {
				//for (std::set<uint32_t>::const_iterator righti = std::next(lefti); righti != t_n.end(); righti++) {
				//for (std::map<uint32_t, GenNode*>::const_iterator right = std::next(left); right != n->succ->end(); right++) {
					//std::map<uint32_t, GenNode*>::iterator right = n->succ->find(*righti);
					//GenNode* right = (*n->succ)[*righti];


#ifndef _TM_STYLE_
				GenNode* right = (*n->succ)[*righti];
#else
				GenNode* right = n->succ->at(*righti);
#endif

				//if (right != n->succ->end()) {
				//if (t_n.find(right->second->item) != t_n.end()) {
					//GenNode* leftN = left->second;
					//GenNode* rightN = right->second;
				GenNode* leftN = left;
				GenNode* rightN = right;

#ifdef _TM_STYLE_
				candidates_tested += 1;
#endif

				if (!leftN->succ || leftN->succ->find(rightN->item) == leftN->succ->end()) {
					if (candIS.empty()) {

#ifndef _TM_STYLE_
						candIS = leftN->items();
						ISTL = TList->getISTL(candIS);
#else
						leftN->items(&candIS);
						ISTL = TList->getISTL(&candIS, _buffer_1, _buffer_2);
#endif
					}

					candIS.insert(rightN->item);
					int support = TList->singleInterSupp(rightN->item, ISTL);
					if (support == minSupp) {
						bool isGen = true;
						std::set<uint32_t> cpCandIS;
						cpCandIS.insert(candIS.begin(), candIS.end());
						for (auto item : candIS) {
							cpCandIS.erase(item);
#ifndef _TM_STYLE_
							GenNode* subset = genLookUp(cpCandIS, root);
#else
							GenNode* subset = genLookUp(&cpCandIS, root);
#endif
							if (subset == nullptr || subset->clos->support == minSupp) {
								isGen = false;
								break;
							}
							cpCandIS.insert(item);
						}
						if (isGen) {
							GenNode* newGen = new GenNode(rightN->item, leftN, nullptr);
#ifndef _TM_STYLE_
							std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList);
							if (!result.first) {
								newClosures.push_back(result.second);
							}
#else
							//std::cout << "computing new closure" << std::endl;
							std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList, _buffer_1, _buffer_2);
							if (!result.first) {
								newClosures->push_back(result.second);
							}
#endif
							//std::cout << "found a new closure" << std::endl;
							ClosedIS* clos = result.second;
#ifndef _TM_STYLE_
							clos->gens.insert(newGen);
#else
							clos->gens->insert(newGen);
#endif
							newGen->clos = clos;
						}
					}
					candIS.erase(rightN->item);
					//}
				//}
				}
			}
		}
#ifdef _TM_STYLE_
		}
#endif
	
	if (n->item == 0) {
#ifndef _TM_STYLE_
		update_singleton_itemsets(n, t_n, newClosures, TList, root, ClosureList);
#else
		update_singleton_itemsets(n, t_n, newClosures, TList, root, ClosureList, _buffer_1, _buffer_2);
#endif
	}
#ifdef _TM_STYLE_
	return candidates_tested;
#endif
}

#ifndef _TM_STYLE_
uint32_t compute_succ_intersection(std::set<uint32_t>& buffer, GenNode* n, std::set<uint32_t>* t_n) {
#else
uint32_t compute_succ_intersection(std::vector<uint32_t>* buffer, GenNode* n, std::set<uint32_t>* t_n) {
#endif
	//The below is experimental, seems a bit faster.
	//std::vector<uint32_t>* intersect = new std::vector<uint32_t>();
	class key_iterator : public std::map<uint32_t, GenNode*>::iterator
	{
	public:
		key_iterator() : std::map<uint32_t, GenNode*>::iterator() {};
		key_iterator(std::map<uint32_t, GenNode*>::iterator s) : std::map<uint32_t, GenNode*>::iterator(s) {};
		uint32_t* operator->() { return (uint32_t* const)&(std::map<uint32_t, GenNode*>::iterator::operator->()->first); }
		uint32_t operator*() { return std::map<uint32_t, GenNode*>::iterator::operator*().first; }
	};

	key_iterator mybegin(n->succ->begin());
	key_iterator myend(n->succ->end());

#ifndef _TM_STYLE_
	std::set_intersection(mybegin, myend, t_n->begin(), t_n->end(), std::inserter(buffer, buffer.begin()));
	return buffer.size();
#else
	std::vector<uint32_t>::iterator pos = std::set_intersection(mybegin, myend, t_n->begin(), t_n->end(), buffer->begin());
	return pos - buffer->begin();
#endif
}


#ifndef _TM_STYLE_
int update_singleton_itemsets(GenNode* const n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* const ClosureList) {
#else
int update_singleton_itemsets(GenNode* const n, std::set<uint32_t>* const t_n, std::vector<ClosedIS*>* const newClosures,
	TIDList* const TList, GenNode* const root, std::multimap<uint32_t, ClosedIS*>* const ClosureList,
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2) {
#endif
	//if (n->item == 0) {
#ifndef _TM_STYLE_
		for (std::set<uint32_t>::const_iterator item = t_n.begin(); item != t_n.end(); item++) {
#else
		for (std::set<uint32_t>::const_iterator item = t_n->begin(); item != t_n->end(); item++) {
#endif
			//candidates_tested += 1;
			if (!n->succ || n->succ->find(*item) == n->succ->end()) {
				int support = TList->supp_singleton(*item);
				if (support == minSupp) {
					if (root->clos->support > minSupp) {
						GenNode* newGen = new GenNode(*item, root, nullptr);
#ifndef _TM_STYLE_
						std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList);
#else
						std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList, _buffer_1, _buffer_2);
#endif
						if (!result.first) {
#ifndef _TM_STYLE_
							newClosures.push_back(result.second);
#else
							newClosures->push_back(result.second);
#endif
						}
						ClosedIS* clos = result.second;
#ifndef _TM_STYLE_
						clos->gens.insert(newGen);
#else
						clos->gens->insert(newGen);
#endif
						newGen->clos = clos;
					}
				}
			}
		}
	//}
		return 0;
}

#ifndef _TM_STYLE_
std::pair<bool, ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, GenNode* root, TIDList* TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	std::set<uint32_t> iset = gen->items();
	for (auto clos : newClosures) {
		if (std::includes(iset.begin(), iset.end(), clos->itemset.begin(), clos->itemset.end())) {
			return std::make_pair(true, clos);
		}
	}
#else
std::pair<bool, ClosedIS*> computeClosure(GenNode* const gen, std::set<uint32_t>* const t_n, std::vector<ClosedIS*>* const newClosures, 
	GenNode* const root, TIDList* const TList, std::multimap<uint32_t, ClosedIS*>* const ClosureList, 
	// avoids allocating memory for istl
	std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2) {
	std::set<uint32_t> iset;
#ifndef _TM_STYLE_
	std::set<uint32_t> iset = gen->items();
#else
	gen->items(&iset);
#endif
	std::vector<ClosedIS*>::iterator it = newClosures->begin();
	for (; it != newClosures->end(); ++it) {
		ClosedIS* const clos = *it;
		if (std::includes(iset.begin(), iset.end(), clos->itemset->begin(), clos->itemset->end())) {
			return std::make_pair(true, clos);
		}
	}
#endif
	std::set<uint32_t> currClosure;
	currClosure.insert(iset.begin(), iset.end());

	// Commenting the below block seems to be a few % faster
	//TODO : more tests
	// tm: after cleaning copies it seems to be slightly faster (e.g. total run of 25s, gain between .5-1s)
	
	std::set<uint32_t> isetCp;
	isetCp.insert(iset.begin(), iset.end());
	for (auto item : iset) {
		isetCp.erase(item); //TODO : implement a genLookUp version that skips the i-th member of iset (->no need for erase+insert)
#ifndef _TM_STYLE_
		GenNode* subset = genLookUp(isetCp, root);
		currClosure.insert(subset->clos->itemset.begin(), subset->clos->itemset.end());
#else
		GenNode* subset = genLookUp(&isetCp, root);
		currClosure.insert(subset->clos->itemset->begin(), subset->clos->itemset->end());
#endif
		isetCp.insert(item);
	}
	
	std::set<uint32_t> outside;
	
#ifndef _TM_STYLE_
	std::set_difference(t_n.begin(), t_n.end(), currClosure.begin(), currClosure.end(), std::inserter(outside, outside.end()));
	std::set<uint32_t> ISTL = TList->getISTL(iset);
#else
	std::set_difference(t_n->begin(), t_n->end(), currClosure.begin(), currClosure.end(), std::inserter(outside, outside.end()));
	std::vector<uint32_t>* ISTL = TList->getISTL(&iset, _buffer_1, _buffer_2);
#endif
	for (auto item : outside) {
		if (TList->closureIncludes(ISTL, item)) {
			currClosure.insert(item);
		}
	}
#ifndef _TM_STYLE_
	ClosedIS* clos = findCI(currClosure, ClosureList);
#else
	ClosedIS* clos = findCI(&currClosure, ClosureList);
#endif
	if (clos == nullptr) {
		
#ifndef _TM_STYLE_
		clos = new ClosedIS(currClosure, minSupp, ClosureList);
#else
		std::set<uint32_t>* copyClo = new std::set<uint32_t>(currClosure.begin(), currClosure.end());
		clos = new ClosedIS(copyClo, minSupp, ClosureList);
#endif
		
	}
	return std::make_pair(false, clos);
}

void resetStatus(GenNode* n) {
	n->clos->visited = false;
#ifndef _TM_STYLE_
	n->clos->candidates.clear();
	for (auto child : *(n->succ)) {
		resetStatus(child.second);
}
#else
	n->clos->candidates->clear();
	std::map<uint32_t, GenNode*>::iterator it = n->succ->begin();
	for (; it != n->succ->end(); ++it) {
		resetStatus(it->second);
	}
#endif
}

void closureReset(std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList->begin(); clos != ClosureList->end(); clos++) {
		clos->second->visited = false;
#ifndef _TM_STYLE_
		clos->second->candidates.clear();
#else
		if (clos->second->candidates) {
			clos->second->candidates->clear();
		}
#endif
	}
}

#ifndef _TM_STYLE_
GenNode* genLookUp(std::set<uint32_t> iset, GenNode* root) { // Will return nullptr if itemset is not in trie
	GenNode* Node = root;
	for (const uint32_t item : iset) {
		if (Node->succ->find(item) != Node->succ->end()) {
			Node = Node->succ->at(item);
		}
		else {
			return nullptr;
		}
	}
	return Node;
}
#else
GenNode* genLookUp(std::set<uint32_t>* const iset, GenNode* const root) { // Will return nullptr if itemset is not in trie
	GenNode* Node = root;
	std::set<uint32_t>::iterator it = iset->begin();
	for (; it != iset->end(); ++it) {
		if (Node->succ) {
			const uint32_t item = *it;
			std::map<uint32_t, GenNode*>* succ = Node->succ;
			if (succ->find(item) != succ->end()) {
				Node = succ->at(item);
			}
			else {
				return nullptr;
			}
		}
		else {
			return nullptr;
		}
	}
	return Node;
}
#endif

#ifndef _TM_STYLE_
ClosedIS* findCI(std::set<uint32_t> itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	typedef std::multimap<uint32_t, ClosedIS*>::iterator MMAPIterator;
	std::pair<MMAPIterator, MMAPIterator> result = ClosureList->equal_range(CISSum(itemSet));
	std::set<uint32_t>& ref = itemSet;
#else
ClosedIS* findCI(std::set<uint32_t>* itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	typedef std::multimap<uint32_t, ClosedIS*>::iterator MMAPIterator;
	std::set<uint32_t>& ref = *itemSet;
	std::pair<MMAPIterator, MMAPIterator> result = ClosureList->equal_range(CISSum(ref));
#endif
	for (MMAPIterator it = result.first; it != result.second; it++) {
#ifndef _TM_STYLE_
		if (it->second->itemset == itemSet) {
			return it->second;
		}
#else
		if ((it->second->itemset->size() == itemSet->size()) 
			&& std::equal(it->second->itemset->begin(), it->second->itemset->end(), itemSet->begin())) {
			return it->second;
		}
#endif
	}
	return nullptr;
}

//ceci cree des copies recursives... a changer
#ifndef _TM_STYLE_
std::set<uint32_t> GenNode::items(){
	if (this->parent == NULL) {
		std::set<uint32_t> s;
		return s;
	}
	std::set<uint32_t> s = this->parent->items();
	s.insert(this->item);
	return s;
}
#else
uint32_t GenNode::items(std::set<uint32_t>* const _buffer) {
	if (this->parent == NULL) {
		return 0;
	}
	this->parent->items(_buffer);
	_buffer->insert(this->item);
	return 0;
}
#endif

GenNode::GenNode(uint32_t item, GenNode* parent, ClosedIS* closure)
{
	this->item = item;
	this->parent = parent;
	this->clos = closure;
	if (closure != nullptr) {
#ifndef _TM_STYLE_
		closure->gens.insert(this);
#else
		closure->gens->insert(this);
#endif
		
	}
	this->id = ++NODE_ID;
	// could avoid allocating this now, since it will be done on line below when needed

#ifndef _TM_STYLE_
	this->succ = new std::map<uint32_t, GenNode*>();
#else
	this -> succ = 0;
#endif

	if (parent != nullptr) {
		if (!parent->succ) {
			parent->succ = new std::map<uint32_t, GenNode*>();
		}
		parent->succ->emplace(item, this);
	}
}

#ifndef _TM_STYLE_
int CISSum(std::set<uint32_t> Itemset) {
#else
int CISSum(std::set<uint32_t>& Itemset) {
#endif
	int sum = 0;
	int mult = 1; // This will "slightly" reduce collisions
	for (auto item : Itemset) {
		sum += item*mult;
		mult *= 10;
	}
	return sum;
}


#ifndef _TM_STYLE_
ClosedIS::ClosedIS(std::set<uint32_t> itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
#else
ClosedIS::ClosedIS(std::set<uint32_t>* itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*> * ClosureList) {
#endif
	this->support = support;
	this->itemset = itemset;
	this->visited = false;
	this->newCI = nullptr;
#ifndef _TM_STYLE_
	ClosureList->insert(std::make_pair(CISSum(itemset), this));
#else
	this->gens = new std::set<GenNode*>();//can we delay creation ?
	std::set<uint32_t>& ref = *itemset;
	ClosureList->insert(std::make_pair(CISSum(ref), this));
#endif
};


#ifndef _TM_STYLE_
void TIDList::add(std::set<uint32_t> t_n, uint32_t n) {
	for (auto item : t_n) {
		if (this->TransactionList.find(item) == this->TransactionList.end()) {
			this->TransactionList.insert(std::make_pair(item, new std::set<uint32_t>));
		}
		this->TransactionList[item]->insert(n);
		this->singletonSupport[item]++;
	}
}
#else
void TIDList::add(std::set<uint32_t>* const t_n, const uint32_t n) {
	std::set<uint32_t>::iterator it = t_n->begin();
	for (; it != t_n->end(); ++it) {
		const uint32_t item = *it;
		if (this->TransactionList->find(item) == this->TransactionList->end()) {
			this->TransactionList->insert(std::make_pair(item, new std::set<uint32_t>));
		}
		this->TransactionList->at(item)->insert(n);
		if (this->singletonSupport->find(item) == this->singletonSupport->end()) {
			this->singletonSupport->insert(std::make_pair(item, 0));
		}
		this->singletonSupport->at(item)++;
	}
}
#endif

#ifndef _TM_STYLE_
int TIDList::supp_from_tidlist(std::set<uint32_t> itemset) { //TODO : start from a known tidlist and agregate other items one at a time
	std::set<uint32_t> iset = *this->TransactionList[*itemset.begin()];
	for (auto item : itemset) {
		std::set<uint32_t> itemList = *this->TransactionList[item];
		std::set<uint32_t>::iterator it1 = iset.begin();
		std::set<uint32_t>::iterator it2 = itemList.begin();

		while ((it1 != iset.end()) && (it2 != itemList.end())) {
			if (*it1 < *it2) {
				iset.erase(it1++);
			}
			else if (*it2 < *it1) {
				++it2;
			}
			else {
				++it1;
				++it2;
			}
		}
		iset.erase(it1, iset.end());

	}
	return iset.size();
}
#else
int TIDList::supp_from_tidlist(std::set<uint32_t>* _itemset) {
	std::set<uint32_t>::iterator it = _itemset->begin();
	std::set<uint32_t>* const tidlist_first_item = this->TransactionList->at(*it);
	
	if (_itemset->size() > 1) {
		std::set<uint32_t>* const tidlist_second_item = this->TransactionList->at(*++it);
		std::vector<uint32_t>* output = &std::vector<uint32_t>();
		// intersects first and second items' tidlist
		std::vector<uint32_t>::iterator pos = std::set_intersection(tidlist_first_item->begin(), tidlist_first_item->end(), tidlist_second_item->begin(), tidlist_second_item->end(), output->begin());
		uint32_t inter_size = pos - output->begin();
		output->resize(inter_size);

		if (_itemset->size() > 2) {
			// for all other items
			std::vector<uint32_t>* output_tmp = &std::vector<uint32_t>();
			for (++it; it != _itemset->end(); ++it) {
				std::set<uint32_t>* const itemList = this->TransactionList->at(*it);
				pos = std::set_intersection(output->begin(), output->end(), itemList->begin(), itemList->end(), output_tmp->begin());
				inter_size = pos - output_tmp->begin();
				output_tmp->resize(inter_size);

				// switch between input/output for next iterations
				output = output_tmp;
				output_tmp = &std::vector<uint32_t>();
			}
		}
		return output->size();
	}
	return tidlist_first_item->size();
}
#endif


#ifndef _TM_STYLE_
bool TIDList::closureIncludes(std::set<uint32_t> currList, uint32_t item) {
	return std::includes(this->TransactionList[item]->begin(), this->TransactionList[item]->end(), currList.begin(), currList.end());
}
#else
bool TIDList::closureIncludes(std::vector<uint32_t>* const currList, const uint32_t item) {
	std::set<uint32_t>* ref = this->TransactionList->at(item);
	/*std::vector<uint32_t>::iterator it = currList->begin();
	std::set<uint32_t>::iterator last = ref->begin();
	for (; it != currList->end(); ++it) {
		last = std::find(last, ref->end(), *it);
		if (last == ref->end()) {
			return false;
		}
	}
	return true;*/
	return std::includes(ref->begin(), ref->end(), currList->begin(), currList->end());
}
#endif


#ifndef _TM_STYLE_
int TIDList::singleInterSupp(uint32_t item, std::set<uint32_t> currTIDList) {
	std::set<uint32_t> intersect;
	set_intersection(currTIDList.begin(), currTIDList.end(), this->TransactionList[item]->begin(), this->TransactionList[item]->end(),
		std::inserter(intersect, intersect.begin()));
	return intersect.size();
}
#else
int TIDList::singleInterSupp(const uint32_t item, std::vector<uint32_t>* const currTIDList) {
	std::set<uint32_t> intersect;
	std::set<uint32_t>* tidlist = this->TransactionList->at(item);
	set_intersection(currTIDList->begin(), currTIDList->end(), tidlist->begin(), tidlist->end(), std::inserter(intersect, intersect.begin()));
	return intersect.size();
}
#endif


#ifndef _TM_STYLE_
std::set<uint32_t> TIDList::getISTL(std::set<uint32_t> itemset) {
	std::set<uint32_t> iset = *this->TransactionList[*itemset.begin()];
	for (auto item : itemset) {

		std::set<uint32_t> itemList = *this->TransactionList[item];
		std::set<uint32_t>::iterator it1 = iset.begin();
		std::set<uint32_t>::iterator it2 = itemList.begin();

		while ((it1 != iset.end()) && (it2 != itemList.end())) {
			if (*it1 < *it2) {
				iset.erase(it1++);
			}
			else if (*it2 < *it1) {
				++it2;
			}
			else {
				++it1;
				++it2;
			}
		}
		iset.erase(it1, iset.end());

	}
	return iset;
}
#else
std::vector<uint32_t>* TIDList::getISTL(std::set<uint32_t>* const itemset, std::vector<uint32_t>* const _buffer_1, std::vector<uint32_t>* const _buffer_2) {
	std::set<uint32_t>::iterator it = itemset->begin();
	std::set<uint32_t>* const tidlist_first_item = this->TransactionList->at(*it);

	_buffer_1->insert(_buffer_1->begin(), tidlist_first_item->begin(), tidlist_first_item->end());
	_buffer_1->resize(tidlist_first_item->size());//for now only

	//std::vector<uint32_t>* iset = new std::vector<uint32_t>(tidlist_first_item->begin(), tidlist_first_item->end());//copy

	if (itemset->size() > 1) {
		std::set<uint32_t>* const tidlist_second_item = this->TransactionList->at(*++it);
		//std::vector<uint32_t>* output = new std::vector<uint32_t>();
		_buffer_2->resize(tidlist_second_item->size());
		// intersects first and second items' tidlist

		std::vector<uint32_t>::iterator pos = std::set_intersection(_buffer_1->begin(), _buffer_1->end(), tidlist_second_item->begin(), tidlist_second_item->end(), _buffer_2->begin());
		uint32_t inter_size = pos - _buffer_2->begin();
		_buffer_2->resize(inter_size);//for now only

		if (itemset->size() > 2) {
			// for all other items
			int cursor = 0;
			for (++it; it != itemset->end(); ++it) {
				std::set<uint32_t>* itemList = this->TransactionList->at(*it);
				if (cursor % 2 == 0) {
					pos = std::set_intersection(_buffer_2->begin(), _buffer_2->end(), itemList->begin(), itemList->end(), _buffer_1->begin());
					inter_size = pos - _buffer_1->begin();
					_buffer_1->resize(inter_size);//for now only
				}
				else {
					pos = std::set_intersection(_buffer_1->begin(), _buffer_1->end(), itemList->begin(), itemList->end(), _buffer_2->begin());
					inter_size = pos - _buffer_2->begin();
					_buffer_2->resize(inter_size);//for now only
				}
				cursor += 1;
			}

			if (cursor % 2 == 0) {
				//delete iset;
				return _buffer_2;
			}
			else {
				//delete output;
				return _buffer_1;
			}
		}
		return _buffer_2;
	}
	return _buffer_1;// iset;
}
#endif


#ifndef _TM_STYLE_
int TIDList::supp_singleton(uint32_t item) {
	return this->singletonSupport[item];
}
#else
int TIDList::supp_singleton(const uint32_t item) {
	return this->singletonSupport->at(item);
}
#endif
