#include <iostream>
#include "FGC-Stream.h"



void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::vector<ClosedIS*>* fGenitors) {
	if (n->item != 0) { // 0 is reserved for the root, so n->item = 0 actually means n is the empty set
		X.insert(n->item);
	}

	std::set<uint32_t> iset;
	std::set_intersection(n->clos->itemset.begin(), n->clos->itemset.end(), t_n.begin(), t_n.end(), std::inserter(iset, iset.begin()));
	if (iset.size() == n->clos->itemset.size()) {
		if (!n->clos->visited) {
			n->clos->support++;
			n->clos->visited = true;
		}
	}
	else {
		ClosedIS* closure;
		if (!n->clos->visited) {
			closure = new ClosedIS(iset, n->clos->support + 1);
			n->clos->newCI = closure;
			n->clos->visited = true;
		}
		else {
			closure = n->clos->newCI;
		}
		fGenitors->push_back(n->clos);
		closure->gens.insert(n);
		n->clos->gens.erase(n);
		std::set<uint32_t> face;
		std::set_difference(n->clos->itemset.begin(), n->clos->itemset.end(), iset.begin(), iset.end(), std::inserter(face, face.end()));
		for (auto item : face) {
			std::set<uint32_t>* newIS = new std::set<uint32_t>;
			newIS->insert(X.begin(), X.end());
			newIS->insert(item);
			n->clos->candidates.insert(newIS);
		}
		n->clos = closure;
	}
	if (n->succ != nullptr) {
		for (auto x : *(n->succ)) {
			if (t_n.find(x.first) != t_n.end()) {
				descend(x.second, X, t_n, fGenitors);
			}
		}
	}
}

void filterCandidates(std::vector<ClosedIS*>* fGenitors, GenNode* root) {
	for (auto genitor : *fGenitors) {
		for (auto iset : genitor->candidates) {
			bool sset = false;
			for (auto n : genitor->gens) {
				std::set<uint32_t> itemset = n->items();
				if (std::includes(iset->begin(), iset->end(), itemset.begin(), itemset.end())) {
					sset = true;
					break;
				}
			}
			if (!sset) {
				uint32_t lastItem = *iset->rbegin();
				iset->erase(std::prev(iset->end()));
				std::set<uint32_t> copy = *iset;
				GenNode* father = genLookUp(copy, root);

				GenNode* newGI = new GenNode(lastItem, father, genitor);
				genitor->gens.insert(newGI);
			}
		}
	}
}

void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, TIDList* TList, GenNode* root) {
	if (n->succ != nullptr) {
		for (auto child : *(n->succ)) {
			computeJumpers(child.second, t_n, newClosures, TList, root);
		}
	}
	if (n->succ != nullptr) {
		for (auto left : *(n->succ)) {
			if (t_n.find(left.second->item) != t_n.end()) {
				for (auto right : *(n->succ)) {
					if (t_n.find(right.second->item) != t_n.end() and right.second->item > left.second->item) {
						GenNode* leftN = left.second;
						GenNode* rightN = right.second;
						if (leftN->succ->find(rightN->item) == leftN->succ->end()) {
							std::set<uint32_t> candIS = leftN->items();
							candIS.insert(rightN->item);
							int support = TList->supp_from_tidlist(candIS);
							if (support == minSupp) {
								bool isGen = true;
								std::set<uint32_t> cpCandIS;
								cpCandIS.insert(candIS.begin(), candIS.end());
								for (auto item : candIS) {
									cpCandIS.erase(item);
									if (genLookUp(cpCandIS, root) == nullptr) {
										isGen = false;
										break;
									}
									cpCandIS.insert(item);
								}
								if (isGen) {
									GenNode* newGen = new GenNode(rightN->item, leftN, nullptr);
									std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList);
									if (!result.first) {
										newClosures.push_back(result.second);
									}
									ClosedIS* clos = result.second;
									clos->gens.insert(newGen);
									newGen->clos = clos;
								}
							}
						}
					}
				}
			}
		}
	}
	if (n->item == 0) {
		for (auto item : t_n) {
			if (n->succ->find(item) == n->succ->end()) {
				int support = TList->supp_singleton(item);
				if (support == minSupp) {
					if (root->clos->support > minSupp) {
						GenNode* newGen = new GenNode(item, root, nullptr);
						std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList);
						if (!result.first) {
							newClosures.push_back(result.second);
						}
						ClosedIS* clos = result.second;
						clos->gens.insert(newGen);
						newGen->clos = clos;
					}
				}
			}
		}
	}
}

std::pair<bool,ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, GenNode* root, TIDList *TList) {
	std::set<uint32_t> iset = gen->items();
	for (auto clos : newClosures) {
		if (std::includes(iset.begin(), iset.end(), clos->itemset.begin(), clos->itemset.end())) {
			return std::make_pair(true, clos);
		}
	}
	std::set<uint32_t> currClosure;
	std::set<uint32_t> isetCp;
	isetCp.insert(iset.begin(), iset.end());
	for (auto item : iset) {
		isetCp.erase(item); //TODO : implement a genLookUp version that skips the i-th member of iset (->no need for erase+insert)
		GenNode* subset = genLookUp(isetCp, root);
		currClosure.insert(subset->clos->itemset.begin(), subset->clos->itemset.end());
		isetCp.insert(item);
	}
	std::set<uint32_t> outside;
	std::set_difference(t_n.begin(), t_n.end(), currClosure.begin(), currClosure.end(), std::inserter(outside, outside.end()));
	
	for (auto item : outside) {
		if (iset.find(item) != iset.end()) {
			iset.insert(item);
			int support = TList->supp_from_tidlist(iset);
			if (support == minSupp) {
				currClosure.insert(item);
			}
			iset.erase(item);
		}
		else {
			iset.insert(item);
			int support = TList->supp_from_tidlist(iset);
			if (support == minSupp) {
				currClosure.insert(item);
			}
		}
	}
	ClosedIS* clos = new ClosedIS(currClosure, minSupp);
	return std::make_pair(false, clos);
}

void resetStatus(GenNode* n) {
	n->clos->visited = false;
	n->clos->candidates.clear();
	for (auto child : *(n->succ)) {
		resetStatus(child.second);
	}
}

GenNode* genLookUp(std::set<uint32_t> iset, GenNode* root) { // Will return nullptr if itemset is not in trie
	GenNode* Node = root;
	for (auto item : iset) {
		if ((*Node->succ).find(item) != (*Node->succ).end()) {
			Node = (*Node->succ)[item];
		}
		else {
			return nullptr;
		}
	}
	return Node;
}

std::set<uint32_t> GenNode::items()
{
	if (this->parent == NULL) {
		std::set<uint32_t> s;
		return s;
	}
	std::set<uint32_t> s = this->parent->items();
	s.insert(this->item);
	return s;
}

GenNode::GenNode(uint32_t item, GenNode* parent, ClosedIS* closure)
{
	this->item = item;
	this->parent = parent;
	this->clos = closure;
	if (closure != nullptr) {
		closure->gens.insert(this);
	}

	this->id = ++NODE_ID;
	this->succ = new std::map<uint32_t, GenNode*>;
	if (parent != nullptr) {
		if (!parent->succ) {
			parent->succ = new std::map<uint32_t, GenNode*>();
		}
		parent->succ->emplace(item, this);
	}
}

ClosedIS::ClosedIS(std::set<uint32_t> itemset, uint32_t support) {
	this->support = support;
	this->itemset = itemset;
	this->visited = false;
	this->newCI = nullptr;
};

void TIDList::add(std::set<uint32_t> t_n, uint32_t n) {
	for (auto item : t_n) {
		if (this->TransactionList.find(item) == this->TransactionList.end()) {
			this->TransactionList.insert(std::make_pair(item, new std::set<uint32_t>));
		}
		this->TransactionList[item]->insert(n);
		this->singletonSupport[item]++;
	}
}

int TIDList::supp_from_tidlist(std::set<uint32_t> itemset) { //TODO : start from a known tidlist and agregate other items one at a time
	std::set<uint32_t> iset = *this->TransactionList[*itemset.begin()];
	for (auto item : itemset) {
		// Snippet below does not work
		/*
		std::set<uint32_t> placeholderSet;

		std::set<uint32_t> itemList = *this->TransactionList[item];

		std::set_intersection(iset.begin(), iset.end(), itemList.begin(), itemList.end(), std::inserter(placeholderSet, placeholderSet.begin()));
		//iset = placeholderSet;
		std::copy(
			placeholderSet.begin(), placeholderSet.end(),
			std::inserter(iset, iset.begin()));
		std::cout << 0;
		*/
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
		//tant que �a crash/leak pas on verra plus tard
		//�a vaut p.e. la peine de garder les r�sultats interm�diaires
		//combinatoire pour la classe d'�quivalence	
	return iset.size();
}

int TIDList::supp_singleton(uint32_t item) {
	return this->singletonSupport[item];
}