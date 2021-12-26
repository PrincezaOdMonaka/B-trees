#include <string>
#include <iostream>
#include <stack>
#include <queue>
#include <fstream>
#define N 10

using namespace std;

class B_node {
public:
	B_node* parent;
	string* keys;
	B_node** children;
	//string keys[4 * N /3];
	//B_node* children[4 * N / 3 + 1];
	int filled_keys;
	bool leaf;


	B_node() : leaf(true), parent(nullptr), filled_keys(0) {

		keys = new string[4 * N / 3];
		children = new B_node * [4 * N / 3 + 1];

		for (int i = 0; i < 4 * N / 3 + 1; ++i) {
			children[i] = nullptr;
		}
	};

	~B_node() {
		delete[] keys;
		delete[] children;
	}


	void insert_node(string key);
	int index_in_parent();
	B_node* left_sibiling();
	B_node* right_sibiling();
	void overflow_right_inner();
	bool overflow_right_sibiling();
	void overflow_left_inner();
	bool overflow_left_sibiling();
	bool full();
	bool overflow();
	bool underflow();
	void split_node();
	bool try_two_right_sibiling();
	bool try_two_left_sibiling();
	bool try_right_sibiling();
	bool try_left_sibiling();
	void borrow_from_right();
	void borrow_from_left();
	void delete_in_node(string key);
	void merge();
	B_node* switch_with_succ(string key);
	void merge_two();
	int max_children();
	int min_children();

	friend class B_tree;
};

// svi osim korena: najvise m podstabala, a najmanje(2m - 1)/3
// koren najmanje 2, a najvise 2* floor((2m -2)/ 3) + 1 podstabala
// cvor moze da bude prazan
class B_tree {

	B_node* root;

public:
	// red stabla, podrazumevano 3
	// red stable = maksimalan broj potomaka
	static int m;

	B_tree() {
		root = new B_node();
	};

	B_tree(string file_name) {
		string line;
		ifstream read_file(file_name);
		root = new B_node();

		while (getline(read_file, line)) {
			if (!line.empty()) {
				insert(line);
			}
		}
		read_file.close();
	}

	~B_tree() {
		delete_tree();
	}

	bool search(string key);
	bool search(string key, B_node** place);
	friend ostream& operator<<(ostream& os, const B_tree& dt);
	void delete_tree();
	bool insert(string key);
	void split_root();
	void merge_root();
	void print_inorder();
	string find_kth_key(int k);
	bool delete_key(string key);
	bool empty();
	void validate_tree();
};

int B_node::max_children() {
	if (this == nullptr) return -1;
	if (parent == nullptr) return  2 * ((2 * B_tree::m - 2) / 3) + 1;
	return B_tree::m;
}

int B_node::min_children() {
	if (this == nullptr) return -1;
	if (parent == nullptr) return  2;
	return (2 * B_tree::m + 1) / 3;
}

bool B_tree::search(string key) {
	B_node* node;
	return search(key, &node);
}

//TODO: testiraj
bool B_tree::search(string key, B_node** place) {
	B_node* curr = root;
	*place = nullptr;

	while (curr != nullptr) {
		int i;
		for (i = 0; i < curr->filled_keys && curr->keys[i] <= key; ++i) {
			if (curr->keys[i] == key) {
				*place = curr;
				return true;
			}
		}
		*place = curr;
		curr = curr->children[i];
	}
	return false;
}

void B_node::insert_node(string key) {
	//TODO: proveri da li ima smisla da se na ovaj nacin ubacuje u nonleaf node
	int i = 0;
	children[filled_keys + 1] = children[filled_keys];
	for (i = filled_keys - 1; i >= 0 && keys[i] > key; --i) {
		keys[i + 1] = keys[i];
		children[i + 1] = children[i];
	}
	keys[i + 1] = key;
	filled_keys++;
}

int B_node::index_in_parent() {
	int index = 0;
	B_node* p = parent;
	if (p == nullptr) return -1;
	for (; index <= p->filled_keys && p->children[index] != this; ++index);
	return index;
}

B_node* B_node::right_sibiling() {
	int index = this->index_in_parent();
	B_node* p = parent;
	if (p == nullptr) return nullptr;

	if (index < B_tree::m - 1) {
		return p->children[index + 1];
	}
	return nullptr;
}

B_node* B_node::left_sibiling() {
	int index = this->index_in_parent();
	B_node* p = parent;
	if (p == nullptr) return nullptr;

	if (index > 0) {
		return p->children[index - 1];
	}
	return nullptr;
}

//poziva se samo kad sigurno ima desnog koji nije pun
void B_node::overflow_right_inner() {
	int index = this->index_in_parent();
	B_node* p = parent;
	B_node* right = p->children[index + 1];
	int key_cnt = filled_keys + right->filled_keys + 1;

	int j, i;

	j = key_cnt - key_cnt / 2 - 1;
	i = right->filled_keys;
	//TODO: moze li j da bude < i
	right->children[j] = right->children[i];
	right->children[i] = nullptr;
	if (right->children[j]) right->children[j]->parent = right;

	--j;
	--i;
	for (; j >= 0 && i >= 0; --j, --i) {
		right->keys[j] = right->keys[i];
		right->children[j] = right->children[i];
		right->children[i] = nullptr;
		if (right->children[j]) right->children[j]->parent = right;
	}
	right->keys[j] = p->keys[index];
	right->children[j] = children[filled_keys];
	children[filled_keys] = nullptr;

	//TODO: parent = right?
	if (right->children[j]) right->children[j]->parent = right;
	--j;
	for (i = filled_keys - 1; j >= 0 && i >= 0; --j, --i) {
		right->keys[j] = keys[i];
		right->children[j] = children[i];
		children[i] = nullptr;
		if (right->children[j])right->children[j]->parent = this;
	}
	p->keys[index] = keys[i];
	filled_keys = key_cnt / 2;
	right->filled_keys = key_cnt - key_cnt / 2 - 1;
}

bool B_node::overflow_right_sibiling() {
	int index = index_in_parent();
	B_node* p = parent;

	if (index < B_tree::m - 1 && index < p->filled_keys - 1) {
		B_node* right = p->children[index + 1];

		if (right && right->filled_keys < B_tree::m - 1) {
			overflow_right_inner();
			return true;
		}
	}
	return false;
}

//poziva se samo kad ima levog koji nije pun
void B_node::overflow_left_inner() {

	B_node* p = parent;
	int index = index_in_parent();
	B_node* left = p->children[index - 1];
	int key_cnt = filled_keys + left->filled_keys + 1;

	int j = left->filled_keys, i;

	left->keys[j++] = p->keys[index - 1];

	for (i = 0; j < key_cnt / 2 && i < filled_keys; ++j, ++i) {
		left->keys[j] = keys[i];
		left->children[j] = children[i];
		children[i] = nullptr;
		if (left->children[j]) left->children[j]->parent = left;
	}
	left->children[j] = children[i];
	children[i] = nullptr;
	if (left->children[j]) left->children[j]->parent = left;
	p->keys[index - 1] = keys[i++];

	for (j = 0; i < filled_keys; ++j, ++i) {
		keys[j] = keys[i];
		children[j] = children[i];
		children[i] = nullptr;
		if (children[j]) children[j]->parent = this;
	}
	children[j] = children[i];
	children[i] = nullptr;
	if (children[j]) children[j]->parent = this;
	filled_keys = key_cnt - key_cnt / 2 - 1;
	left->filled_keys = key_cnt / 2;
}

bool B_node::overflow_left_sibiling() {
	B_node* p = parent;
	int index = index_in_parent();
	if (index > 0) {
		B_node* left = p->children[index - 1];

		if (left && left->filled_keys < B_tree::m - 1) {
			overflow_left_inner();
			return true;
		}
	}
	return false;
}

bool B_node::full() {
	if (this == nullptr) return false;
	if (parent == nullptr) return filled_keys == 2 * ((2 * B_tree::m - 2) / 3);
	return filled_keys == B_tree::m - 1;
}

bool B_node::overflow() {
	if (this == nullptr) return false;
	if (parent == nullptr) return filled_keys > 2 * ((2 * B_tree::m - 2) / 3);
	return filled_keys > B_tree::m - 1;
}


void B_node::split_node() {
	int index = index_in_parent();
	B_node* p = parent;

	B_node* left = this, * right = nullptr;
	right = right_sibiling();

	if (right == nullptr || !right->full()) {
		left = left_sibiling();
		right = this;
		index--;
	}
	B_node* middle = new B_node();

	middle->leaf = this->leaf;
	middle->parent = this->parent;

	//in node
	int pos1 = (2 * B_tree::m - 2) / 3;
	string key1 = left->keys[pos1];

	//in right sib
	int pos2 = right->filled_keys - (2 * B_tree::m) / 3 - 1;
	string key2 = p->keys[index];
	if (pos2 >= 0) key2 = right->keys[pos2];

	int i, j, k;
	middle->children[0] = left->children[pos1 + 1];
	if (middle->children[0]) middle->children[0]->parent = middle;
	left->children[pos1 + 1] = nullptr;

	for (i = pos1 + 1, k = 0; k < (2 * B_tree::m - 1) / 3 && i < left->filled_keys; ++i, ++k) {
		middle->keys[k] = left->keys[i];
		middle->children[k + 1] = left->children[i + 1];
		if (middle->children[k + 1]) middle->children[k + 1]->parent = middle;
		left->children[i + 1] = nullptr;
	}

	if (k < (2 * B_tree::m - 1) / 3) {
		middle->keys[k] = p->keys[index];
		middle->children[k + 1] = right->children[0];
		if (middle->children[k + 1]) middle->children[k + 1]->parent = middle;
		right->children[0] = nullptr;
		++k;
	}

	j = 0;
	if (pos2 >= 0) {
		while (k < (2 * B_tree::m - 1) / 3 && j < pos2) {
			middle->keys[k] = right->keys[j];
			middle->children[k + 1] = right->children[j + 1];
			if (middle->children[k + 1]) middle->children[k + 1]->parent = middle;
			right->children[j + 1] = nullptr;
			++k;
			++j;
		}
		++j;
		for (k = 0; j < right->filled_keys; ++j, ++k) {
			right->keys[k] = right->keys[j];
			right->children[k] = right->children[j];
		}
		right->children[k] = right->children[j];
		++k;
		for (; k <= right->filled_keys; ++k) {
			right->children[k] = nullptr;
		}
	}

	middle->filled_keys = (2 * B_tree::m - 1) / 3;
	left->filled_keys = (2 * B_tree::m - 2) / 3;
	right->filled_keys = 2 * B_tree::m / 3;

	p->keys[index] = key1;
	p->insert_node(key2);
	p->children[index + 1] = middle;
}


void B_node::delete_in_node(string key) {
	int i;
	for (i = 0; i < filled_keys && keys[i] < key; ++i);
	for (; i < filled_keys - 1; ++i) {
		keys[i] = keys[i + 1];
		children[i] = children[i + 1];
	}
	//node->children[node->filled_keys] = nullptr;
	filled_keys--;
}

//poziva se samo ako ima dovoljno da se pozajmi
void B_node::borrow_from_right() {
	int index = index_in_parent();
	B_node* p = parent;
	B_node* right = p->children[index + 1];
	keys[filled_keys] = p->keys[index];
	p->keys[index] = right->keys[0];
	children[filled_keys + 1] = right->children[0];
	if (right->children[0]) right->children[0]->parent = this;
	filled_keys++;

	for (int i = 1; i < right->filled_keys; ++i) {
		right->keys[i - 1] = right->keys[i];
		right->children[i - 1] = right->children[i];
	}
	right->children[right->filled_keys - 1] = right->children[right->filled_keys];
	right->children[right->filled_keys] = nullptr;
	right->filled_keys--;
}

bool B_node::try_right_sibiling() {
	B_node* p = parent;
	if (p == nullptr) return false;
	int index = index_in_parent();

	if (index < B_tree::m - 1 && index < p->filled_keys) {
		B_node* right = p->children[index + 1];
		if (right && right->filled_keys + 1 > (2 * B_tree::m + 1) / 3) {
			this->borrow_from_right();
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

//poziva se samo ako ima dovoljno da se pozajmi
void B_node::borrow_from_left() {
	B_node* p = parent;
	int index = index_in_parent();
	B_node* left = p->children[index - 1];
	children[filled_keys + 1] = children[filled_keys];
	for (int i = filled_keys - 1; i >= 0; --i) {
		keys[i + 1] = keys[i];
		children[i + 1] = children[i];
	}
	keys[0] = p->keys[index - 1];
	children[0] = left->children[left->filled_keys];
	if (children[0]) children[0]->parent = left;
	filled_keys++;

	left->children[left->filled_keys] = nullptr;
	p->keys[index - 1] = left->keys[left->filled_keys - 1];
	left->filled_keys--;
}

bool B_node::try_left_sibiling() {
	B_node* p = parent;
	if (p == nullptr) return false;
	int index = index_in_parent();
	if (index > 0) {
		B_node* left = p->children[index - 1];

		if (left && left->filled_keys + 1 > (2 * B_tree::m + 1) / 3) {
			this->borrow_from_left();
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

bool B_node::try_two_right_sibiling() {
	B_node* p = parent;
	if (p == nullptr) return false;
	int index = index_in_parent();

	if (index < B_tree::m - 2 && index < p->filled_keys - 1) {
		B_node* two_right = p->children[index + 2];
		B_node* right = p->children[index + 1];
		if (two_right && two_right->filled_keys + 1 > (2 * B_tree::m + 1) / 3) {
			this->borrow_from_right();
			right->borrow_from_right();
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

bool B_node::try_two_left_sibiling() {

	B_node* p = parent;
	if (p == nullptr) return false;
	int index = index_in_parent();

	if (index > 1) {
		B_node* two_left = p->children[index - 2];
		B_node* left = p->children[index - 1];
		if (two_left && two_left->filled_keys + 1 > (2 * B_tree::m + 1) / 3) {
			this->borrow_from_left();
			left->borrow_from_left();
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

B_node* B_node::switch_with_succ(string key) {
	int i;
	for (i = 0; i < filled_keys; ++i) {
		if (keys[i] == key) break;
	}
	B_node* curr = children[i + 1], * ret = nullptr;
	while (curr) {
		ret = curr;
		curr = curr->children[0];
	}
	string s = ret->keys[0];
	ret->keys[0] = key;
	keys[i] = s;
	return ret;
}

bool B_node::underflow() {
	if (this == nullptr) return true;
	if (parent == nullptr) return filled_keys + 1 < 2;
	return filled_keys + 1 < (2 * B_tree::m + 1) / 3;
}

void B_node::merge_two() {
	B_node* left = nullptr, * right = nullptr, * p = parent;
	int index = index_in_parent();
	if (index < B_tree::m - 1 && index < p->filled_keys) {
		left = this;
		right = p->children[index + 1];
	}

	if (!right && index > 0) {
		left = p->children[index - 1];
		right = this;
		index--;
	}
	if (!left) return;


	int i = left->filled_keys, j;
	left->keys[i++] = p->keys[index];

	for (j = 0; j < right->filled_keys; ++j, ++i) {
		left->keys[i] = right->keys[j];
		left->children[i] = right->children[j];
		if (left->children[i]) left->children[i]->parent = left;
	}

	left->children[i] = right->children[j];
	if (left->children[i]) left->children[i]->parent = left;

	left->filled_keys = i;
	right->filled_keys = 0;
	p->delete_in_node(p->keys[index]);
	p->children[index] = left;
}


void B_node::merge() {
	int index = index_in_parent();
	B_node* left = nullptr, * middle = nullptr, * right = nullptr, * p = this->parent;
	if (index < B_tree::m - 1 && index < p->filled_keys && index > 0) {
		left = p->children[index - 1];
		middle = this;
		right = p->children[index + 1];
	}
	else if (index < B_tree::m - 2 && index < p->filled_keys - 1) {
		left = this;
		middle = p->children[index + 1];
		right = p->children[index + 2];
	}
	else
	if (index > 1) {
		left = p->children[index - 2];
		middle = p->children[index - 1];
		right = this;
	}
	else
	if (index <= 1 && (left == nullptr || middle == nullptr || right == nullptr)) {
		this->merge_two();
		return;
	}

	int key_cnt = left->filled_keys + middle->filled_keys + right->filled_keys + 2;

	//TODO: dinamicki 
	string s[3 * N + 2];
	B_node* c[3 * N + 3];
	int cnt1 = 0, cnt2 = 0, cnt3 = 0;
	int k = 0;

	for (int i = 0; i < key_cnt; ++i) {
		if (i < left->filled_keys) {
			s[i] = left->keys[cnt1];
			c[k++] = left->children[cnt1];
			cnt1++;
			continue;
		}
		if (i == left->filled_keys) {
			s[i] = p->keys[left->index_in_parent()];
			c[k++] = left->children[left->filled_keys];
			continue;
		}
		if (i > left->filled_keys && i < left->filled_keys + middle->filled_keys + 1) {
			s[i] = middle->keys[cnt2];
			c[k++] = middle->children[cnt2];
			cnt2++;
			continue;
		}
		if (i == left->filled_keys + middle->filled_keys + 1) {
			s[i] = p->keys[middle->index_in_parent()];
			c[k++] = middle->children[middle->filled_keys];
			continue;
		}
		s[i] = right->keys[cnt3];
		c[k++] = right->children[cnt3];
		cnt3++;
	}
	c[k] = right->children[right->filled_keys];


	int i, j;
	for (i = 0; i < key_cnt / 2; ++i) {
		left->keys[i] = s[i];
		left->children[i] = c[i];
		if (left->children[i]) left->children[i]->parent = left;
	}
	left->children[i] = c[i];
	if (left->children[i]) left->children[i]->parent = left;

	left->filled_keys = key_cnt / 2;
	p->keys[left->index_in_parent()] = s[i++];


	int idx = left->index_in_parent();
	for (k = idx + 1; k < p->filled_keys - 1; ++k) {
		p->keys[k] = p->keys[k + 1];
		p->children[k] = p->children[k + 1];
	}
	if (k < p->filled_keys) p->children[k] = p->children[k + 1];
	p->filled_keys--;
	p->children[p->filled_keys + 1] = nullptr;

	for (j = 0; i < key_cnt; ++i, ++j) {
		right->keys[j] = s[i];
		right->children[j] = c[i];
		if (right->children[j]) right->children[j]->parent = right;
	}
	right->children[j] = c[i];
	if (right->children[j]) right->children[j]->parent = right;

	right->filled_keys = key_cnt - key_cnt / 2 - 1;
	idx = left->index_in_parent();
	p->children[idx] = left;
	p->children[idx + 1] = right;
}

void B_tree::print_inorder() {

	for (int i = 1; i <= 100; ++i)
		cout << find_kth_key(i) << " ";
	cout << endl;
}

bool B_tree::insert(string key) {

	if (root == nullptr) {
		root = new B_node();
	}
	if (root->filled_keys == 0) {
		root->keys[0] = key;
		root->filled_keys = 1;
		return true;
	}
	B_node* node = nullptr;
	if (search(key, &node)) {
		return false;
	}

	if (node != nullptr && !node->full()) {
		node->insert_node(key);
		return true;
	}

	if (node->parent == nullptr && node->full()) {
		node->insert_node(key);
		split_root();
		return true;
	}

	if (node != nullptr && node->full()) {
		node->insert_node(key);
	}
	while (node->overflow()) {
		if (node->parent == nullptr) {
			split_root();
			return true;
		}
		if (!node->overflow_right_sibiling()) {
			if (!node->overflow_left_sibiling()) {
				node->split_node();
				node = node->parent;
			}
			else {
				return true;
			}
		}
		else {
			return true;
		}
	}
}

void B_tree::split_root() {
	int key_cnt = root->filled_keys;

	B_node* left = new B_node();
	B_node* right = new B_node();

	int i, j;
	for (i = 0; i < key_cnt / 2; ++i) {
		left->keys[i] = root->keys[i];
		left->children[i] = root->children[i];
		if (left->children[i]) left->children[i]->parent = left;
		root->children[i] = nullptr;
	}
	left->children[i] = root->children[i];
	if (left->children[i]) left->children[i]->parent = left;
	root->children[i] = nullptr;

	if (left->children[0] != nullptr) left->leaf = false;
	left->filled_keys = key_cnt / 2;

	for (i = 0, j = key_cnt / 2 + 1; j < key_cnt; ++j, ++i) {
		right->keys[i] = root->keys[j];
		right->children[i] = root->children[j];
		if (right->children[i]) right->children[i]->parent = right;
		root->children[j] = nullptr;
	}
	right->children[i] = root->children[j];
	if (right->children[i]) right->children[i]->parent = right;

	root->children[j] = nullptr;

	if (right->children[0] != nullptr) right->leaf = false;
	right->filled_keys = key_cnt - left->filled_keys - 1;

	left->parent = root;
	right->parent = root;
	root->keys[0] = root->keys[key_cnt / 2];
	root->filled_keys = 1;
	root->children[0] = left;
	root->children[1] = right;
	root->leaf = false;
}


ostream& operator<<(ostream& os, const B_tree& dt) {
	queue<B_node*> q;
	q.push(dt.root);

	while (!q.empty()) {
		int sz = q.size();

		for (int j = 0; j < sz; ++j) {
			B_node* curr = q.front();
			q.pop();
			if (curr == nullptr) os << "   ";
			else {
				if (j > 0) os << "   ";
				for (int i = 0; i < curr->filled_keys; ++i) {
					os << curr->keys[i] << " ";
					if (curr->children[i]) q.push(curr->children[i]);
				}
				if (curr->children[curr->filled_keys]) q.push(curr->children[curr->filled_keys]);
				q.push(nullptr);

			}
		}
		os << endl;
	}
	return os;
}

string B_tree::find_kth_key(int k) {
	B_node* curr = root;
	stack<pair<B_node*, int>> s;
	while (curr) {
		s.push({ curr, 0 });
		curr = curr->children[0];
	}
	while (!s.empty()) {
		auto elem = s.top();
		s.pop();
		curr = elem.first;
		int pos = elem.second;
		if (curr->leaf) {
			if (curr->filled_keys < k) {
				k -= curr->filled_keys;
			}
			else {
				return curr->keys[k - 1];
			}
		}
		else {
			if (pos < curr->filled_keys) {
				--k;
				if (k == 0) {
					return curr->keys[pos];
				}
				else {
					s.push({ curr, pos + 1 });
					curr = curr->children[pos + 1];
					while (curr) {
						s.push({ curr, 0 });
						curr = curr->children[0];
					}
				}
			}
		}
	}
	return "";
}

void B_tree::merge_root() {
	if (root->children[1] == nullptr || root->children[1]->filled_keys == 0) {
		root = root->children[0];
		if (root) root->parent = nullptr;
	}
	//TODO: mogu li oba cvora deteta da budu neprazna
}


// u korenu najmanji broj potomaka 2
// u ostalim cvorovim (2 * m + 1) / 3
bool B_tree::delete_key(string key) {
	B_node* node = nullptr;
	if (!search(key, &node)) {
		return false;
	}
	if (!node->leaf) {
		node = node->switch_with_succ(key);
	}
	if (node->filled_keys + 1 > (2 * m + 1) / 3) {
		node->delete_in_node(key);
		return true;
	}
	node->delete_in_node(key);

	while (node && node->underflow()) {
		if (!node->try_right_sibiling()) {
			if (!node->try_left_sibiling()) {
				if (!node->try_two_right_sibiling()) {
					if (!node->try_two_left_sibiling()) {
						//merging
						//cout << "Merge";
						if (node->parent == nullptr) {
							merge_root();
						}
						else {
							node->merge();
						}
						node = node->parent;
					}
				}
			}
		}
	}
	return true;
}

void B_tree::delete_tree() {
	stack<B_node*> s;
	B_node* curr = root;
	s.push(curr);

	while (!s.empty()) {
		curr = s.top();
		s.pop();
		for (int i = 0; i < curr->filled_keys; ++i) {
			if (curr->children[i] != nullptr) s.push(curr->children[i]);
		}
		delete curr;
	}
	root = nullptr;
}

bool B_tree::empty() {
	return root == nullptr || root->filled_keys == 0;
}

void B_tree::validate_tree() {
	if (!root) return;
	B_node* curr = root;
	queue<B_node*> q;
	q.push(curr);

	while (!q.empty()) {
		int sz = q.size();
		B_node* temp = q.front();
		if (temp->leaf) {
			for (int i = 0; i < sz; ++i) {
				curr = q.front();
				q.pop();
				if (!curr->leaf) {
					cout << "Tree not valid 5" << endl;
					return;
				}
			}
		}
		else {
			for (int i = 0; i < sz; ++i) {
				curr = q.front();
				q.pop();
				if (curr->filled_keys + 1 > curr->max_children() || curr->filled_keys + 1 < curr->min_children()) {
					cout << "Tree not valid 1" << endl;
					return;
				}
				for (int j = 0; j <= curr->filled_keys; ++j) {
					if (!curr->leaf && curr->children[j] == nullptr) {
						cout << "Tree not valid 2" << endl;
						return;
					}
					if (curr->leaf && curr->children[j] != nullptr) {
						cout << "Tree not valid 3" << endl;
						return;
					}
					if (!curr->leaf && curr->children[j] && curr->children[j]->parent != curr) {
						cout << "Tree not valid 4" << endl;
						return;
					}
					if (curr->children[j]) {
						q.push(curr->children[j]);
					}
				}
			}
		}
	}
	cout << "Tree is valid" << endl;
}


int B_tree::m = 3;

int main() {
	int m, option = 1, k = 1;
	//cin >> m;
	B_tree::m = 4;
	B_tree* tree = nullptr;
	string file_name, key;
	/*
	while (option != 0) {
		cout << endl;
		cout << "1. Postavi red stabla" << endl;
		cout << "2. Formiraj stabla iz fajla" << endl;
		cout << "3. Pronaladji kljuc" << endl;
		cout << "4. Ubaci kljuc" << endl;
		cout << "5. Obrisi kljuc" << endl;
		cout << "6. Ispis stabla" << endl;
		cout << "7. Pronadji k-ti kljuc u stablu" << endl;
		cout << "8. Brisi stablo" << endl;
		cout << "9. Ispis inorder poretka" << endl;
		cout << "0. Izlazak" << endl;
		cout << "Izaberite opciju: ";
		cin >> option;

		switch (option) {

		case 1:
			cout << "Unesite red stabla: ";
			cin >> m;
			B_tree::m = m;
			tree = new B_tree();
			break;

		case 2: cout << "Unesite naziv fajla: ";
			cin >> file_name;
			tree = new B_tree(file_name);
			break;

		case 3:
			cout << "Unesite kljuc: ";
			cin >> key;
			if (!tree->search(key)) {
				cout << "Kljuc ne postoji" << endl;
			}
			else {
				cout << "Kljuc postoji" << endl;
			}
			break;

		case 4:
			cout << "Unesite kljuc: ";
			cin >> key;
			if (!tree->insert(key)) {
				cout << "Kljuc vec postoji << endl";
			}
			break;

		case 5:
			cout << "Unesite kljuc: ";
			cin >> key;
			if (tree->delete_key(key)) cout << "Kljuc obrisan";
			else cout << "Nema trazenog kljuca";
			break;

		case 6:
			cout << *tree;
			break;

		case 7:
			cout << "Unesite k: ";
			cin >> k;
			cout << tree->find_kth_key(k);
			break;

		case 8:
			tree->delete_tree();
			break;
		case 9: 
			tree->print_inorder();
			break;

		default: break;
		}
	}
	*/
	
	
	B_tree* t = new B_tree();
	t->insert("01");
	t->insert("02");
	t->insert("03");
	t->insert("04");
	t->insert("05");
	t->insert("06");
	t->insert("07");
	t->insert("08");
	t->insert("09");
	t->insert("10");
	t->insert("11");
	t->insert("12");
	t->insert("13");
	t->insert("14");
	t->insert("15");
	t->insert("16");
	t->insert("17");
	t->print_inorder();
	cout << *t;

	t->delete_key("09"); 
	t->print_inorder();
	cout << *t;

	t->delete_key("04");
	t->print_inorder();
	cout << *t;

	t->delete_key("06");
	t->print_inorder();
	cout << *t;
	
	/*
	B_tree* t = new B_tree("input.txt");
	bool flag = false;
	while (!t->empty()) {
		string s = t->find_kth_key(1);
		if (s == "13") flag = true;
		t->delete_key(s);
		//t->validate_tree();
		t->print_inorder();
	}
	t->print_inorder();
	*/
	return 0;
}