#include <iostream>
#include "FGC-Stream.h"


// ADDITION ROUTINE
void descend(GenNode* n, std::set<uint32_t> X, std::set<uint32_t> t_n, std::multimap < uint32_t, ClosedIS* >* fGenitors, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
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
			closure = findCI(iset, ClosureList);
			if (closure == nullptr) {
				closure = new ClosedIS(iset, n->clos->support + 1, ClosureList);
			}
			n->clos->newCI = closure;
			n->clos->visited = true;
			fGenitors->insert(std::make_pair(n->clos->itemset.size(), n->clos));
		}
		else {
			closure = n->clos->newCI;
		}
		//breaks here
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
				descend(x.second, X, t_n, fGenitors, ClosureList);
			}
		}
	}
}

void filterCandidates(std::multimap < uint32_t, ClosedIS* >* fGenitors, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	for (auto genitor : *fGenitors) {
		for (auto iset : genitor.second->candidates) {
			bool sset = false;
			for (auto n : genitor.second->gens) {
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
				if (father != nullptr) {
					GenNode* newGI = new GenNode(lastItem, father, genitor.second);
					genitor.second->gens.insert(newGI);
				}

			}
		}
		std::set<std::set<uint32_t>*> preds = computePreds(genitor.second);
		uint32_t key = CISSum(genitor.second->itemset);
		for (std::multimap<uint32_t, ClosedIS*>::iterator currP = genitor.second->preds.begin(); currP != genitor.second->preds.end(); ++currP) {
			std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = currP->second->succ.equal_range(key);
			std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
			for (; it != iterpair.second; ++it) {
				if (it->second == genitor.second) {
					currP->second->succ.erase(it);
					break;
				}
			}
		}
		
		for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {

			ClosedIS* predNode = findCI(**pred, ClosureList);
			predNode->succ.insert(std::make_pair(key, genitor.second));
			genitor.second->preds.insert(std::make_pair(CISSum(**pred), predNode));

		}
		preds = computePreds(genitor.second->newCI);
		key = CISSum(genitor.second->newCI->itemset);

		for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {

			ClosedIS* predNode = findCI(**pred, ClosureList);
			predNode->succ.insert(std::make_pair(key, genitor.second->newCI));
			genitor.second->newCI->preds.insert(std::make_pair(CISSum(**pred), predNode));

		}
		
	}


}

void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

	for (std::map<uint32_t, GenNode*>::const_iterator child = n->succ->begin(); child != n->succ->end(); child++) {
		computeJumpers(child->second, t_n, newClosures, TList, root, ClosureList);
	}

	//The below is experimental, seems a bit faster.
	std::set<uint32_t> intersect;
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

	std::set_intersection
	(mybegin, myend,
		t_n.begin(), t_n.end(),
		std::inserter(intersect, intersect.begin()));

	//Different ways of iterating
	//The fastest way (limited testing done) seems to be the uncommented version below.

	//for(std::set<uint32_t>::const_iterator lefti = t_n.begin(); lefti!=t_n.end(); lefti++){
	//for (std::map<uint32_t, GenNode*>::const_iterator left = n->succ->begin(); left != n->succ->end(); left++) {
	for (std::set<uint32_t>::const_iterator lefti = intersect.begin(); lefti != intersect.end(); lefti++) {
		//std::map<uint32_t, GenNode*>::iterator left = n->succ->find(*lefti);
		GenNode* left = (*n->succ)[*lefti];
		//if (left != n->succ->end()) {
		//if (t_n.find(left->second->item) != t_n.end()) {
			std::set<uint32_t> ISTL;
			std::set<uint32_t> candIS;
			for (std::set<uint32_t>::const_iterator righti = std::next(lefti); righti != intersect.end(); righti++) {
			//for (std::set<uint32_t>::const_iterator righti = std::next(lefti); righti != t_n.end(); righti++) {
			//for (std::map<uint32_t, GenNode*>::const_iterator right = std::next(left); right != n->succ->end(); right++) {
				//std::map<uint32_t, GenNode*>::iterator right = n->succ->find(*righti);
				GenNode* right = (*n->succ)[*righti];

				//if (right != n->succ->end()) {
				//if (t_n.find(right->second->item) != t_n.end()) {
					//GenNode* leftN = left->second;
					//GenNode* rightN = right->second;
					GenNode* leftN = left;
					GenNode* rightN = right;
					if (leftN->succ->find(rightN->item) == leftN->succ->end()) {
						if (candIS.empty()) {
							candIS = leftN->items();
							ISTL = TList->getISTL(candIS);
						}
						
						candIS.insert(rightN->item);
						int support = TList->singleInterSupp(rightN->item, ISTL);
						if(support == minSupp){
							bool isGen = true;
							std::set<uint32_t> cpCandIS;
							cpCandIS.insert(candIS.begin(), candIS.end());
							for (auto item : candIS) {
								cpCandIS.erase(item);
								GenNode* subset = genLookUp(cpCandIS, root);
								if (subset == nullptr || subset->clos->support==minSupp) {
									isGen = false;
									break;
								}
								cpCandIS.insert(item);
							}
							if (isGen) {
								GenNode* newGen = new GenNode(rightN->item, leftN, nullptr);
								std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList);
								if (!result.first) {
									newClosures.push_back(result.second);
								}
								ClosedIS* clos = result.second;
								clos->gens.insert(newGen);
								newGen->clos = clos;
							}
						}
						candIS.erase(rightN->item);
					//}
				//}
			}
		}
	}
	
	if (n->item == 0) {
		for(std::set<uint32_t>::const_iterator item = t_n.begin(); item != t_n.end(); item++){
			if (n->succ->find(*item) == n->succ->end()) {
				int support = TList->supp_singleton(*item);
				if (support == minSupp) {
					if (root->clos->support > minSupp) {
						GenNode* newGen = new GenNode(*item, root, nullptr);
						std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList);
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

std::pair<bool,ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*> newClosures, GenNode* root, TIDList *TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	std::set<uint32_t> iset = gen->items();
	for (auto clos : newClosures) {
		if (std::includes(iset.begin(), iset.end(), clos->itemset.begin(), clos->itemset.end())) {
			return std::make_pair(true, clos);
		}
	}
	std::set<uint32_t> currClosure;
	currClosure.insert(iset.begin(), iset.end());

	// Commenting the below block seems to be a few % faster
	//TODO : more tests
	/*
	std::set<uint32_t> isetCp;
	isetCp.insert(iset.begin(), iset.end());
	for (auto item : iset) {
		isetCp.erase(item); //TODO : implement a genLookUp version that skips the i-th member of iset (->no need for erase+insert)
		GenNode* subset = genLookUp(isetCp, root);
		currClosure.insert(subset->clos->itemset.begin(), subset->clos->itemset.end());
		isetCp.insert(item);
	}
	*/
	std::set<uint32_t> outside;
	std::set_difference(t_n.begin(), t_n.end(), currClosure.begin(), currClosure.end(), std::inserter(outside, outside.end()));

	std::set<uint32_t> ISTL = TList->getISTL(iset);
	
	for (auto item : outside) {
		if (TList->closureIncludes(ISTL, item)) {
			currClosure.insert(item);
		}
	}


	ClosedIS* clos = findCI(currClosure, ClosureList);
	if (clos == nullptr) {
		clos = new ClosedIS(currClosure, minSupp, ClosureList);
	}
	return std::make_pair(false, clos);
}


std::set<std::set<uint32_t>*> computePreds(ClosedIS* clos) {
	std::set<std::set<uint32_t>*> faces;
	
	std::multimap<uint32_t, std::set<uint32_t>*> faceCandidates;

	GenNode* firstGenN = *clos->gens.begin();
	std::set<uint32_t> firstIS = firstGenN->items();

	for (std::set<uint32_t>::iterator item = firstIS.begin(); item != firstIS.end(); ++item) {
		std::set<uint32_t>* newFace = new std::set<uint32_t>;
		newFace->insert(*item);
		faces.insert(newFace);
	}

	for (std::set<GenNode*>::iterator gen = ++clos->gens.begin(); gen != clos->gens.end(); ++gen) {
		std::set<uint32_t> IS = (*gen)->items();
		for (std::set<uint32_t>::iterator item = IS.begin(); item != IS.end(); ++item) {
			for (std::set<std::set<uint32_t>*>::iterator pred = faces.begin(); pred != faces.end(); ++pred) {
				std::set<uint32_t>* newFace = new std::set<uint32_t>;
				newFace->insert((*pred)->begin(), (*pred)->end());
				newFace->insert(*item);
				faceCandidates.insert(std::make_pair(newFace->size(), newFace));
			}
		}
		faces.clear();
		for (std::multimap<uint32_t, std::set<uint32_t>*>::iterator cand = faceCandidates.begin(); cand != faceCandidates.end(); ++cand) {
			bool minimal = true;
			for (std::set<std::set<uint32_t>*>::iterator pred = faces.begin(); pred != faces.end(); ++pred) {
				if (std::includes(cand->second->begin(), cand->second->end(), (*pred)->begin(), (*pred)->end())) {
					minimal = false;
					break;
				}
				if (minimal) {
					faces.insert(cand->second);
				}
			}
		}
		faceCandidates.clear();
	}
	std::set<std::set<uint32_t>*> preds;
	for (std::set<std::set<uint32_t>*>::iterator face = faces.begin(); face != faces.end(); ++face) {
		std::set<uint32_t>* pred = new std::set<uint32_t>;
		std::set_difference(clos->itemset.begin(), clos->itemset.end(), (*face)->begin(), (*face)->end(),
			std::inserter(*pred, pred->end()));
		preds.insert(pred);
	}

	return preds;
}







void resetStatus(GenNode* n) {
	n->clos->visited = false;
	n->clos->candidates.clear();
	for (auto child : *(n->succ)) {
		resetStatus(child.second);
	}
}

void closureReset(std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	for (std::multimap<uint32_t, ClosedIS*>::iterator clos = ClosureList->begin(); clos != ClosureList->end(); clos++) {
		clos->second->visited = false;
		clos->second->candidates.clear();
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

ClosedIS* findCI(std::set<uint32_t> itemSet, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	typedef std::multimap<uint32_t, ClosedIS*>::iterator MMAPIterator;
	std::pair<MMAPIterator, MMAPIterator> result = ClosureList->equal_range(CISSum(itemSet));
	for (MMAPIterator it = result.first; it != result.second; it++) {
		if (it->second->itemset == itemSet) {
			return it->second;
		}
	}
	return nullptr;
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

int CISSum(std::set<uint32_t> Itemset) {
	int sum = 0;
	int mult = 1; // This will "slightly" reduce collisions
	for (auto item : Itemset) {
		sum += item*mult;
		mult *= 10;
	}
	return sum;
}

ClosedIS::ClosedIS(std::set<uint32_t> itemset, uint32_t support, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	this->support = support;
	this->itemset = itemset;
	this->visited = false;
	this->newCI = nullptr;
	ClosureList->insert(std::make_pair(CISSum(itemset), this));
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

void TIDList::remove(std::set<uint32_t> t_n, uint32_t n) {
	for (auto item : t_n) {
		this->TransactionList[item]->erase(n);
		this->singletonSupport[item]--;
		if (this->TransactionList[item]->empty()) {
			this->TransactionList.erase(item);
		}
	}
}



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

bool TIDList::closureIncludes(std::set<uint32_t> currList, uint32_t item) {
	return std::includes(this->TransactionList[item]->begin(), this->TransactionList[item]->end(), currList.begin(), currList.end());
}

int TIDList::singleInterSupp(uint32_t item, std::set<uint32_t> currTIDList) {
	std::set<uint32_t> intersect;
	set_intersection(currTIDList.begin(), currTIDList.end(), this->TransactionList[item]->begin(), this->TransactionList[item]->end(),
		std::inserter(intersect, intersect.begin()));
	return intersect.size();
}

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

int TIDList::supp_singleton(uint32_t item) {
	return this->singletonSupport[item];
}


std::ostream& operator<<(std::ostream& os, ClosedIS CI) {
	os << "CI {";
	for (auto x : CI.itemset) {
		os << x << " ";
	}
	os << "} has gens: ";
	for (auto x : CI.gens) {
		os << "{";
		for (auto y : x->items()) {
			os << y << " ";
		}
		os << "}, ";
	}
	return os;
}
