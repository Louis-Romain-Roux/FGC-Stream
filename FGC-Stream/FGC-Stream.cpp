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
	for (std::multimap<uint32_t, ClosedIS*>::iterator genitorEntry = fGenitors->begin(); genitorEntry != fGenitors->end(); ++genitorEntry) {
		ClosedIS* genitor = genitorEntry->second;

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
				if (father != nullptr) {
					GenNode* newGI = new GenNode(lastItem, father, genitor);
					genitor->gens.insert(newGI);
				}

			}
		}

		//std::set<std::set<uint32_t>*> preds = computePreds(genitor->newCI);
		std::set<std::set<uint32_t>*> preds = compute_preds_exp(genitor->newCI);

		uint32_t key = CISSum(genitor->newCI->itemset);
		uint32_t oldKey = CISSum(genitor->itemset);

		for (std::set<std::set<uint32_t>*>::iterator pred = preds.begin(); pred != preds.end(); ++pred) {

			ClosedIS* predNode = findCI(**pred, ClosureList);
			predNode->succ->insert(std::make_pair(key, genitor->newCI));
			std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = predNode->succ->equal_range(oldKey);
			std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
			for (; it != iterpair.second; ++it) {
				if (it->second == genitor) {
					predNode->succ->erase(it);
					break;
				}
			}
			genitor->newCI->preds->insert(std::make_pair(CISSum(**pred), predNode));

		}

		std::multimap<uint32_t, ClosedIS*>* newPreds = new std::multimap<uint32_t, ClosedIS*>;
		std::set_difference(genitor->preds->begin(), genitor->preds->end(),
			genitor->newCI->preds->begin(), genitor->newCI->preds->end(),
			std::inserter(*newPreds, newPreds->begin())
		);

		genitor->preds = newPreds;

		//Something buggy below
		/*
		std::set_difference(std::make_move_iterator(genitor->preds.begin()), std::make_move_iterator(genitor->preds.end()),
			genitor->newCI->preds.begin(), genitor->newCI->preds.end(),
			std::inserter(*newPreds, newPreds->begin())
		);

		genitor->preds.swap(*newPreds);
		genitor->preds.insert(std::make_pair(key, genitor->newCI));
		genitor->newCI->succ.insert(std::make_pair(oldKey, genitor));
		*/

	}
}

void computeJumpers(GenNode* n, std::set<uint32_t> t_n, std::vector<ClosedIS*>* newClosures, TIDList* TList, GenNode* root, std::multimap<uint32_t, ClosedIS*>* ClosureList) {

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
									newClosures->push_back(result.second);
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
				if (support >= minSupp) {
					if (root->clos->support > support) {
						GenNode* newGen = new GenNode(*item, root, nullptr);
						std::pair<bool, ClosedIS*> result = computeClosure(newGen, t_n, newClosures, root, TList, ClosureList);
						if (!result.first) {
							newClosures->push_back(result.second);
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

std::pair<bool,ClosedIS*> computeClosure(GenNode* gen, std::set<uint32_t> t_n, std::vector<ClosedIS*>* newClosures, GenNode* root, TIDList *TList, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	std::set<uint32_t> iset = gen->items();
	for (std::vector<ClosedIS*>::iterator closIt = newClosures->begin(); closIt != newClosures->end(); ++closIt) {
		ClosedIS* clos = *closIt;
		if (std::includes(clos->itemset.begin(), clos->itemset.end(), iset.begin(), iset.end())) {
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

// UNUSED
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

using namespace std;

std::set<std::set<uint32_t>*> compute_preds_exp(ClosedIS* clos) {

	vector<vector<uint32_t>>* _generators = new vector<vector<uint32_t>>;
	//Fill with generators
	for (set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
		set<uint32_t> itemsS = (*genIt)->items();
		vector<uint32_t> itemsV(itemsS.begin(), itemsS.end());
		_generators->push_back(itemsV);
	}

	vector<set<uint32_t>>* _faces = new vector<set<uint32_t>>;


	std::map<uint32_t, vector<size_t>>* reversed_gens = new std::map<uint32_t, vector<size_t>>();
	for (size_t i = 0; i != _generators->size(); ++i) {
		vector<uint32_t>* const f = &_generators->at(i);
		for (size_t j = 0; j != f->size(); ++j) {
			//if never found item
			if (reversed_gens->find(f->at(j)) == reversed_gens->end()) {
				reversed_gens->emplace(std::make_pair(f->at(j), vector<size_t>()));
			}
			reversed_gens->at(f->at(j)).push_back(i);
		}
	}
	std::vector<uint32_t> item_candidates;
	{
		std::map<uint32_t, vector<size_t>>::iterator it = reversed_gens->begin();
		for (; it != reversed_gens->end(); ++it) {
			if (it->second.size() == _generators->size()) {
				//il est un generateur singleton
				std::set<uint32_t> new_gen = std::set<uint32_t>();
				new_gen.insert(it->first);
				_faces->push_back(new_gen);
			}
			else {
				//dans le cas contraire, il faut le mettre de cote
				item_candidates.push_back(it->first);
			}
		}
	}




	vector<MinNode*> all_nodes = vector<MinNode*>();
	MinNode* const ROOT = new MinNode();
	ROOT->children = new std::map<uint32_t, MinNode*>();
	all_nodes.push_back(ROOT);
	for (size_t j = item_candidates.size(); j != 0; --j) {
		const uint32_t item = item_candidates.at(j - 1);
		vector<size_t>* ref = &(reversed_gens->at(item));
		vector<size_t>* fid_left = new vector<size_t>(ref->begin(), ref->end());
		std::set<uint32_t>* new_gen = new std::set<uint32_t>();
		new_gen->insert(item);
		MinNode* atom = new MinNode();
		{
			ROOT->children->emplace(std::pair<uint32_t, MinNode*>(item, atom));
			atom->parent = ROOT;
			atom->fidset = fid_left;
			atom->item = item;
			atom->generator = new_gen;
		}
		all_nodes.push_back(atom);
		grow_generator(_faces, _generators, atom, ROOT, &all_nodes);
		//delete fid_left;
	}
	vector<MinNode*>::iterator it = all_nodes.begin();
	for (; it != all_nodes.end(); ++it) {
		MinNode* const node = *it;
		if (node->children) delete node->children;
		delete node->fidset;
		delete node->generator;
		delete node;
	}
	//perform_minimality_test(_faces);

	delete reversed_gens;
	//std::cout << _faces->size() << " from " << _generators->size() << std::endl;
	std::set<std::set<uint32_t>*> preds;
	for (vector<set<uint32_t>>::iterator face = _faces->begin(); face != _faces->end(); ++face) {
		std::set<uint32_t>* pred = new std::set<uint32_t>;
		std::set_difference(clos->itemset.begin(), clos->itemset.end(), face->begin(), face->end(),
			std::inserter(*pred, pred->end()));
		preds.insert(pred);
	}

	return preds;

}

void grow_generator(vector<set<uint32_t>>* _generators,
	vector<vector<uint32_t>>* _faces, MinNode* const _parent_node, MinNode* const _root, vector<MinNode*>* const _nodes) {
	std::map<uint32_t, MinNode*>* const _parent_siblings = _parent_node->parent->children;
	std::map<uint32_t, MinNode*>::reverse_iterator it_par_sib = _parent_siblings->rbegin();
	for (; it_par_sib != _parent_siblings->rend(); ++it_par_sib) {
		MinNode* const ref_sib = it_par_sib->second;
		const uint32_t item = ref_sib->item;

		if (item <= _parent_node->item) continue;
		/*std::cout << "Combining ";
		print_concept_as_set(_parent_node->generator);
		std::cout << "with ";
		print_concept_as_set(ref_sib->generator);
		std::cout << " (" << item << ") " << std::endl;*/
		//print_concept_as_set(_parent_node->generator);
		//std::cout << std::endl;
		vector<size_t>* const fid_in = _parent_node->fidset;
		vector<size_t>* const fid_right = ref_sib->fidset;
		vector<size_t> actual_set;
		vector<size_t>* fid_out = &actual_set;// vector<size_t>();
		fid_out->resize(_faces->size());
		vector<size_t>::iterator it = set_union(fid_in->begin(), fid_in->end(), fid_right->begin(), fid_right->end(), fid_out->begin());
		fid_out->resize(it - fid_out->begin());
		if (fid_out->size() != fid_in->size() && fid_out->size() != fid_right->size() && fid_out->size() == _faces->size()) {
			if (!is_valid_candidate(_parent_node, item, fid_out, _root)) {
				//delete fid_out;
				continue;
			}
			//ok fini, il est transveral minimal
			std::set<uint32_t>* new_gen = new std::set<uint32_t>(_parent_node->generator->begin(), _parent_node->generator->end());
			new_gen->insert(item);
			_generators->push_back(*new_gen);//should have pointer refs into generators instead of copies (but should also ensuite contiguous mem)
			MinNode* const newnode = new MinNode();
			newnode->fidset = new vector<size_t>(fid_out->begin(), fid_out->end());
			newnode->generator = new_gen;
			newnode->parent = _parent_node;
			newnode->item = item;
			_nodes->push_back(newnode);
			if (!_parent_node->children) {
				_parent_node->children = new std::map<uint32_t, MinNode*>();
			}
			_parent_node->children->emplace(std::make_pair(item, newnode));
		}
		else {
			if (fid_out->size() == fid_in->size() || fid_out->size() == fid_right->size()) {
				//on a rien gagne donc il est pas utile de rajouter cet item au candidat generateur actuel, on elague tous ses supersets
				//delete fid_out;
				continue;
			}
			//ici check les subsets du generateur candidat pour s'assurer qu'il y en a pas deja un avec le meme fid-set
			//enumerer les subsets
			if (is_valid_candidate(_parent_node, item, fid_out, _root)) {
				//ok on le garde, il est un candidat valide (qui va devoir encore grandir)
				MinNode* const newnode = new MinNode();
				newnode->fidset = new vector<size_t>(fid_out->begin(), fid_out->end());
				std::set<uint32_t>* new_gen = new std::set<uint32_t>(_parent_node->generator->begin(), _parent_node->generator->end());
				new_gen->insert(item);
				newnode->generator = new_gen;
				newnode->parent = _parent_node;
				newnode->item = item;
				if (!_parent_node->children) {
					_parent_node->children = new std::map<uint32_t, MinNode*>();
				}
				_nodes->push_back(newnode);
				_parent_node->children->emplace(std::make_pair(item, newnode));
				grow_generator(_generators, _faces, newnode, _root, _nodes);
			}
			else {
				//delete fid_out;
				//std::cout << "pruned candidated because not minimal" << std::endl;
			}
		}
	}
}

bool is_valid_candidate(MinNode* const _parent_node, const uint32_t _item, vector<size_t>* const _fid_out, MinNode* const _root) {
	bool is_valid_candidate = true;
	vector<vector<uint32_t>> all_direct_subsets = vector<vector<uint32_t>>();

	//comment enumerer les subsets ?
	vector<uint32_t> copy_as_vect = vector<uint32_t>(_parent_node->generator->begin(), _parent_node->generator->end());

	/*std::cout << "subset of ";
	print_concept_as_vector(&copy_as_vect);
	std::cout << " + " << _item;
	std::cout << endl;
	*/
	//recall that all shifted by +1 in order to keep cur1sor positive
	for (uint32_t i = _parent_node->generator->size() - 1; i != 0; --i) {
		vector<uint32_t> candidate_immediate_subset = vector<uint32_t>();
		for (uint32_t j = 0; j != _parent_node->generator->size() + 1; ++j) {
			if (j != (i - 1)) {
				if (j == _parent_node->generator->size()) {
					candidate_immediate_subset.push_back(_item);
				}
				else candidate_immediate_subset.push_back(copy_as_vect.at(j));
			}
		}
		// print_concept_as_vector(&candidate_immediate_subset);
		// std::cout << endl;
		all_direct_subsets.push_back(candidate_immediate_subset);
	}

	MinNode* subset;
	vector<vector<uint32_t>>::iterator it = all_direct_subsets.begin();
	for (; it != all_direct_subsets.end(); ++it) {

		/*std::cout << " is ";
		print_concept_as_vector(&*it);
		std::cout << std::endl;
		*/

		vector<uint32_t>* vv = &(*it);
		if (!vv) {
			//std::cout << "lol" << std::endl;
			exit(300);
		}
		subset = get_from_path(vv, _root);
		if (subset) {
			if (subset->fidset->size() == _fid_out->size()) {
				//non, il est pas minimal
				//is_valid_candidate = false;
				return false;
				//break;
			}
		}
		else {
			//le sous ensemble n'a pas ete trouve, donc il existe un subset de ce subset avec le meme fid, 
			//donc il n'est pas canonique, donc on skipe
			//is_valid_candidate = false;
			return false;
			//break;
		}
	}
	return true;
}

MinNode* get_from_path(vector<uint32_t>* const _path, MinNode* const _root) {
	MinNode* curr = _root;
	std::map<uint32_t, MinNode*>::iterator it;// = curr->children->begin();
	for (size_t i = 0; i != _path->size(); ++i) {
		uint32_t next = _path->at(i);
		//std::cout << "searching for " << next << std::endl;
		if (curr->children && (it = curr->children->find(next)) != curr->children->end() && it->second) {
			//ok found
			//std::cout << "ok was found !" << std::endl;
			curr = it->second;
		}
		else {
			//std::cout << "oh shit not found" << std::endl;
			return 0x00;
		}
	}
	//std::cout << "found w/ " << curr->fidset->size() << std::endl;
	return curr;
}



set<set<uint32_t>*> compute_preds_efficient(ClosedIS* clos) {
	vector<vector<uint32_t>> _generators;
	//Fill with generators
	for (set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
		set<uint32_t> itemsS = (*genIt)->items();
		vector<uint32_t> itemsV(itemsS.begin(), itemsS.end());
		_generators.push_back(itemsV);
	}

	vector<set<uint32_t>>* _faces = new vector<set<uint32_t>>;
	vector<set<uint32_t>> faceTemp;

	// 1 - Create mono-face face
	for (size_t i = 0; i != _generators[0].size(); ++i) {
		set<uint32_t> face = set<uint32_t>();
		face.insert(_generators[0][i]);
		_faces->push_back(face);
	}

	/*if (_faces[0].size() == 31 && _faces.size() == 1) {
	  std::cout << "lol" << std::endl;
	}*/

	std::vector<std::vector<set<uint32_t>*>> test_strat_face;
	test_strat_face.reserve(_generators.size() + 1);
	// 2 - loop all gens 
	for (size_t i = 1; i != _generators.size(); ++i) {
		if (_generators[i].size() == 0) {
			//std::cout << "" << std::endl;
			exit(1);
		}
		//std::map<uint32_t, std::vector<size_t>> test_index_gen;
		test_strat_face.clear();
		// loop on one face
		for (size_t j = 0; j != _generators[i].size(); ++j) {

			// grow mono-gens
			for (size_t k = 0; k != _faces->size(); ++k) {
				set<uint32_t> face = _faces->at(k);
				face.insert(_generators[i][j]);
				faceTemp.push_back(face);
				if (!(test_strat_face.size() > face.size())) {
					test_strat_face.resize(face.size() + 1);
				}
				test_strat_face[face.size()].push_back(&face);
			}
		}

		// Check minimality
		_faces->clear();

		for (size_t j = 0; j != faceTemp.size(); ++j) {
			// check min
			// if min -> _generators
			set<uint32_t>* g = &faceTemp.at(j);
			bool was_broken = false;
			//loop and include
			/*for (size_t ll = 0; ll != g->size(); ++ll) {
			  vector<set<uint32_t>*>* ttttt = &test_strat_face[ll];

			  for (size_t z = 0; z != ttttt->size(); ++z) {
				set<uint32_t>* gg = ttttt->at(z);// &faceTemp.at(ttttt->at(z));
				if (includes(g->begin(), g->end(), gg->begin(), gg->end())) {
				  was_broken = true;
				  break;
				}
			  }
			  if(was_broken) break;

			  //std::vector<size_t>::iterator it = ttttt->begin();
			  //for (; it != ttttt->end(); ++it) {
			  //  set<uint32_t>* gg = &faceTemp.at(*it);
			  //  if (includes(g->begin(), g->end(), gg->begin(), gg->end())) {
			  //    was_broken = true;
			  //    break;
			  //  }
			  //}
			}*/


			for (size_t k = 0; k != faceTemp.size(); ++k) {
				if (k == j) continue;
				//this is not enough, same candidate can be generated several times from several face combinations !
				set<uint32_t>* gg = &faceTemp.at(k);
				if (gg->size() >= g->size()) continue;
				//if (!(g->size() > gg->size())) continue;
				if (includes(g->begin(), g->end(), gg->begin(), gg->end())) {
					/*print_concept_as_set(g);
					std::cout << " includes ";
					print_concept_as_set(gg);
					std::cout << std::endl;*/
					was_broken = true;
					break;
				}
			}
			if (!was_broken) {
				_faces->push_back(*g);
			}
		}
		faceTemp.clear();
	}
	// here we have candidate generators
	vector<set<uint32_t>> realFaces;
	for (size_t k = 0; k != _faces->size(); ++k) {
		set<uint32_t>* g = &_faces->at(k);
		bool was_broken = false;
		for (size_t j = 0; j != realFaces.size(); ++j) {
			set<uint32_t>* gg = &realFaces.at(j);
			if (includes(g->begin(), g->end(), gg->begin(), gg->end())) {
				was_broken = true;
				break;
			}
		}
		if (!was_broken)
			realFaces.push_back(*g);
	}

	//copy
	_faces->clear();
	for (size_t j = 0; j != realFaces.size(); ++j) {
		set<uint32_t>* gg = &realFaces.at(j);
		_faces->push_back(*gg);
	}

	//final checks
	for (size_t k = 0; k != _faces->size(); ++k) {
		set<uint32_t>* g = &_faces->at(k);
		for (size_t j = 0; j != _faces->size(); ++j) {
			if (k == j) continue;
			set<uint32_t>* gg = &_faces->at(j);
			if (includes(g->begin(), g->end(), gg->begin(), gg->end())) {
				exit(1);
			}
		}
	}
	//std::cout << _generators->size() << " from " << _faces.size() << std::endl;

	std::set<std::set<uint32_t>*> preds;
	for (std::vector<std::set<uint32_t>>::iterator face = _faces->begin(); face != _faces->end(); ++face) {
		std::set<uint32_t>* pred = new std::set<uint32_t>;
		std::set_difference(clos->itemset.begin(), clos->itemset.end(), face->begin(), face->end(),
			std::inserter(*pred, pred->end()));
		preds.insert(pred);
	}


	return preds;

	/*std::cout << "faces : " << std::endl;
	for (size_t i = 0; i != _faces.size(); ++i) {
	  std::vector<uint32_t>* f = &_faces[i];
	  print_concept_as_vector(f);
	  std::cout << std::endl;
	}*/
}


// Deletion routine

void descendM(GenNode* n, std::set<uint32_t> t_0, std::multimap<uint32_t, ClosedIS*>* ClosureList, std::vector<ClosedIS*>* iJumpers, std::multimap<uint32_t, ClosedIS*>* fObsoletes) {
	if (n->clos->support == minSupp) {
		if (!n->clos->visited) {
			n->clos->visited = true;
			iJumpers->push_back(n->clos);
		}
	}
	else {
		if (!n->clos->visited) {
			n->clos->visited = true;
			ClosedIS* cg = findGenitor(n->clos, t_0);
			if (cg == nullptr) {
				n->clos->support--;
			}
			else {
				n->clos->gtr = cg;
				fObsoletes->insert(std::make_pair(n->clos->itemset.size(), n->clos));
			}
		}
	}
	for (std::map<uint32_t, GenNode*>::iterator succ = n->succ->begin(); succ != n->succ->end(); ++succ) {
		if (t_0.find(succ->second->item) != t_0.end()) {
			descendM(succ->second, t_0, ClosureList, iJumpers, fObsoletes);
		}
	}
}

ClosedIS* findGenitor(ClosedIS* clos, std::set<uint32_t> t_0) {
	for (std::multimap<uint32_t, ClosedIS*>::iterator succIt = clos->succ->begin(); succIt != clos->succ->end(); ++succIt) {
		ClosedIS* succ = succIt->second;
		if (clos->support == succ->support + 1 && !(std::includes(t_0.begin(), t_0.end(), succ->itemset.begin(), succ->itemset.end()))) {
			return succ;
		}
	}
	return nullptr;
}

void dropObsolete(ClosedIS* clos, std::multimap<uint32_t, ClosedIS*>* ClosureList, GenNode* root) {
	ClosedIS* cg = clos->gtr;
	uint32_t keyS = CISSum(cg->itemset);

	for (std::multimap<uint32_t, ClosedIS*>::iterator predIt = clos->preds->begin(); predIt != clos->preds->end(); ++predIt) {
		ClosedIS* pred = predIt->second;
		if (pred->gtr == nullptr) {
			uint32_t keyP = CISSum(pred->itemset);
			bool link = true;
			for (std::multimap<uint32_t, ClosedIS*>::iterator succIt = pred->succ->begin(); succIt != pred->succ->end(); ++succIt) {
				ClosedIS* succ = succIt->second;
				if (std::includes(cg->itemset.begin(), cg->itemset.end(), succ->itemset.begin(), succ->itemset.end())) {
					link = false;
					break;
				}
			}
			if (link) {
				pred->succ->insert(std::make_pair(keyS, cg));
				cg->preds->insert(std::make_pair(keyP, pred));
			}
			std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = pred->succ->equal_range(CISSum(clos->itemset));
			std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
			for (; it != iterpair.second; ++it) {
				if (it->second == clos) {
					pred->succ->erase(it);
					break;
				}
			}
		}
	}

	dropObsoleteGs(root, clos);

	std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = ClosureList->equal_range(CISSum(clos->itemset));
	std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
	for (; it != iterpair.second; ++it) {
		if (it->second == clos) {
			ClosureList->erase(it);
			break;
		}
	}

	for (std::set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
		GenNode* gen = *genIt;
		gen->clos = cg;
		cg->gens.insert(gen);
	}
}

void dropObsoleteGs(GenNode* root, ClosedIS* clos) {
	std::set<uint32_t> face;
	std::set_difference(clos->gtr->itemset.begin(), clos->gtr->itemset.end(), clos->itemset.begin(), clos->itemset.end(), std::inserter(face, face.end()));
	for (std::set<GenNode*>::iterator genIt = clos->gtr->gens.begin(); genIt != clos->gtr->gens.end();) {
		GenNode* gen = *genIt;
		std::set<uint32_t> items = gen->items();

		std::set<uint32_t> inter;
		std::set_intersection(face.begin(), face.end(), items.begin(), items.end(), std::inserter(inter, inter.begin()));

		if (inter.size() == 1) {
			items.erase(*(inter.begin()));
			GenNode* genO = genLookUp(items, root);
			bool del = false;
			if (genO != nullptr) {
				if (genO->clos == clos) {
					removeChildren(gen);
					gen->parent->succ->erase(gen->item);
					genIt = gen->clos->gens.erase(genIt);
					del = true;
				}
			}
			if (!del) {
				genIt++;
			}
		}
	}

}

void dropJumper(ClosedIS* clos, std::multimap<uint32_t, ClosedIS*>* ClosureList) {
	for (std::multimap<uint32_t, ClosedIS*>::iterator predIt = clos->preds->begin(); predIt != clos->preds->end(); ++predIt) {
		ClosedIS* pred = predIt->second;

		std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = pred->succ->equal_range(CISSum(clos->itemset));
		std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
		for (; it != iterpair.second; ++it) {
			if (it->second == clos) {
				pred->succ->erase(it);
				break;
			}
		}
	}

	std::pair<std::multimap<uint32_t, ClosedIS*>::iterator, std::multimap<uint32_t, ClosedIS*>::iterator> iterpair = ClosureList->equal_range(CISSum(clos->itemset));
	std::multimap<uint32_t, ClosedIS*>::iterator it = iterpair.first;
	for (; it != iterpair.second; ++it) {
		if (it->second == clos) {
			ClosureList->erase(it);
			break;
		}
	}

	for (std::set<GenNode*>::iterator genIt = clos->gens.begin(); genIt != clos->gens.end(); ++genIt) {
		GenNode* gen = *genIt;
		gen->parent->succ->erase(gen->item);
	}
}

void removeChildren(GenNode* gen) {
	for (std::map<uint32_t, GenNode*>::iterator childIt = gen->succ->begin(); childIt != gen->succ->end();) {
		innerDelete(childIt->second);
		childIt = gen->succ->erase(childIt);
	}
}

void innerDelete(GenNode* gen) {
	for (std::map<uint32_t, GenNode*>::iterator childIt = gen->succ->begin(); childIt != gen->succ->end();) {
		innerDelete(childIt->second);
		childIt = gen->succ->erase(childIt);
	}
	gen->parent->succ->erase(gen->item);
	gen->clos->gens.erase(gen);
	if(gen->clos->gtr != nullptr){
		gen->clos->gtr->gens.erase(gen);
	}
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
	this->gtr = nullptr;
	ClosureList->insert(std::make_pair(CISSum(itemset), this));
	this->preds = new std::multimap<uint32_t, ClosedIS*>;
	this->succ = new std::multimap<uint32_t, ClosedIS*>;

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
