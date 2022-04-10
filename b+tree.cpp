#include <bits/stdc++.h>

using namespace std;

int D = 1, T = 1;

class Node {
    friend class BPlusTree;
    private:
        bool isLeafNode;
        int size;
        Node * rightSibling, * leftSibling, * parent;
        vector < int > keys;
        vector < Node * > children;
        
        Node * split(bool isLeaf, Node * node) { 
            if(isLeaf) {
                Node * tmp = new Node(true, node->parent);
                for(int i = D; i < 2*D + 1; i++) {
                    tmp->keys.push_back(node->keys[i]);
                }
                node->keys.resize(D);

                return tmp;
            }
            else {
                Node * tmp = new Node(false, node->parent);
                for(int i = T + 1; i < 2*T + 2; i++) {
                    tmp->keys.push_back(node->keys[i]);
                }
                node->keys.resize(T);

                return tmp;
            }

        }

        Node * insertIndex(int value, Node * node, Node * root) {
            if(node->parent == NULL) {
                Node * tmp = new Node(false, NULL);
                node->parent = tmp;
                node->rightSibling->parent = tmp;
                tmp->keys.push_back(value);
                tmp->children[0] = node;
                tmp->children[1] = node->rightSibling;
                return tmp;
            }
            else {
                Node * par = node->parent;
                par->keys.push_back(value);
                sort(par->keys.begin(), par->keys.end());

                if(par->keys.size() > par->size) {
                    int partition = par->keys[T];
                    Node * tmp = split(false, par);

                    tmp->rightSibling = par->rightSibling;
                    tmp->leftSibling = par;
                    if (par->rightSibling != NULL) par->rightSibling->leftSibling = tmp;
                    par->rightSibling = tmp;

                    for(int i = 0; i < par->keys.size(); i++) {
                        par->children[i + 1] = par->children[i]->rightSibling;
                    }

                    tmp->children[0] = par->children[par->keys.size()]->rightSibling;
                    for(int i = 0; i < tmp->keys.size(); i++) {
                        tmp->children[i + 1] = tmp->children[i]->rightSibling;
                    }

                    return insertIndex(partition, par, root);
                } 
                else {
                    for(int i = 0; i < par->keys.size(); i++) {
                        par->children[i + 1] = par->children[i]->rightSibling;
                    }
                }
                return root;
            }
        }

    public:
        Node(bool isLeaf, Node * parent) {
            this->isLeafNode = isLeaf;
            this->leftSibling = this->rightSibling = NULL;
            this->parent = parent;

            if(isLeaf) {
                this->size = 2*D;
            }
            else {
                this->size = 2*T + 1;
                this->children.resize(2 * (T + 1), NULL);
            }
        }


        Node * insertData(int value, Node * root) {
            keys.push_back(value);
            sort(keys.begin(), keys.end());

            if(keys.size() > size) {
                int partition = keys[D];

                Node * tmp = split(true, this);

                tmp->rightSibling = this->rightSibling;
                tmp->leftSibling = this;
                if (this->rightSibling != NULL) this->rightSibling->leftSibling = tmp;
                this->rightSibling = tmp;

                return insertIndex(partition, this, root);
            }
            return root;
        }

        bool isLeaf() {
            return isLeafNode;
        }
};

class BPlusTree {
    private:
        Node * root;

        int countIndexNodes(int count, Node * node) {
            if(node->isLeaf()) return 0;
            else {
                count += countIndexNodes(count, node->children[0]);
                count++;
                while(node->rightSibling != NULL) {
                    node = node->rightSibling;
                    count++;
                }
                return count;
            }
        }

        int countDataNode() {
            Node * node = root;
            while(node->isLeaf() == false) {
                node = node->children[0];
            }

            int cnt = 1;
            while(node->rightSibling != NULL) {
                node = node->rightSibling; 
                cnt++;
            }

            return cnt;
        }

    public:
        BPlusTree() {
            this->root = new Node(true, NULL);
        }

        void insert(int value, Node * node) {
            if(node->isLeaf()) {
                root = node->insertData(value, root);
            }
            else {
                if(value <= node->keys[0]) insert(value, node->children[0]);
                else if(value > node->keys[node->keys.size() - 1]) insert(value, node->children[node->keys.size()]);
                else {
                    for(int i = 1; i < node->keys.size(); i++) {
                        if(node->keys[i-1] < value && value <= node->keys[i]) 
                        {
                            insert(value, node->children[i]);
                            break;
                        }
                    }
                }
            }
        }

        void display() {
            Node * node = root;
            int nIndex = 0, nData = 0;
            nIndex = countIndexNodes(0, root);
            nData = countDataNode();

            cout << nIndex << " " << nData << " ";
            for(int i = 0; i < root->keys.size(); i++) {
                cout << root->keys[i] << " ";
            }
            cout << endl;
        }

        Node * getRoot() {
            return root;
        }

};

int main() {

    cin >> D >> T;

    BPlusTree tree;

    int q;
    while(cin >> q) {
        if(q == 1) {
            int val; cin >> val;
            tree.insert(val, tree.getRoot());
        }
        else if(q == 2) {
            tree.display();
        }
        else if(q == 3) {
            break;
        }
    }


    return 0;
}
