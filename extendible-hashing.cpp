#include <bits/stdc++.h>

using namespace std;

class Bucket {
    private:
        int localDepth;
        int size;
        int creationTime;
        vector < int > values;
    public:
        Bucket(int depth, int size, int creationTime) {
            this->localDepth = depth;
            this->size = size;
            this->creationTime = creationTime;
        }

        void insert(int value) {
            values.push_back(value);
        }

        void remove(int value) {
            for(auto it = values.begin(); it != values.end(); it++) {
                if(*it == value) {
                    values.erase(it);
                    break;
                }
            }
        }

        bool search(int value) {
            for(auto it = values.begin(); it != values.end(); it++) {
                if(*it == value) {
                    return true;
                }
            }
            return false;
        }

        bool isFull() {
            if(values.size() == size) return true;
            else return false;
        }

        void increaseDepth() {
            localDepth++;
        }

        int getDepth() {
            return localDepth;
        }

        int getOccupancy() {
            return values.size();
        }

        int getCreationTime() {
            return creationTime;
        }

        vector < int > copy() {
            return values;
        }

        void display() {
            for(int i = 0; i < values.size(); i++) {
                cout << values[i] << " ";
            }

            cout << endl;
        }
};


class Directory {
    private:
        int globalDepth;
        int bucket_size;
        int time;
        int nBuckets;
        map < int, Bucket* > directory;
        
        int hash(int n, int depth) {
            return n & ((1 << depth) - 1);
        }

        void grow() {
            globalDepth = globalDepth + 1;
            if(globalDepth > 20) throw overflow_error("Memory limit exceeded :(");
            else {
                for(int i = (1 << (globalDepth - 1)); i < (1 << globalDepth); i++) {
                    directory[i] = directory[hash(i, globalDepth - 1)];
                }
            }
        }

        void split(int h) {
            directory[h]->increaseDepth();
            int depth = directory[h]->getDepth();
            Bucket* newB = new Bucket(depth, bucket_size, ++time);
            nBuckets++;
            vector < int > values = directory[h]->copy();
            for(int i = 0; i < values.size(); i++) {
                int val = values[i];
                int newH = hash(val, depth);

                if(h != newH) {
                    directory[h]->remove(val);
                    newB->insert(val);
                }
            }

            for(int i = 0; i < (1 << globalDepth); i++) {
                if(hash(i, depth - 1) == h) {
                    if(hash(i, depth) != h) directory[i] = newB;
                } 
            }
        }

    public:
        Directory(int depth, int bucket_size) {
            this->globalDepth = depth;
            this->bucket_size = bucket_size;
            this->time = 0;
            this->nBuckets = 0;
            for(int i = 0; i < (1 << globalDepth); i++) {
                directory[i] = new Bucket(depth, bucket_size, ++time);
                nBuckets++;
            }
        }

        void insert(int value) {
            try {
                int h = hash(value, globalDepth);
                if(directory[h]->isFull()) {
                    if(directory[h]->getDepth() == globalDepth) grow();
                    split(h);
                    insert(value);
                }
                else directory[h]->insert(value);
            } catch (overflow_error& e) {
                cerr << e.what() << endl;
                return;
            }
        }

        void remove(int value) {
            int h = hash(value, globalDepth);
            directory[h]->remove(value);
        }

        bool search(int value) {
            int h = hash(value, globalDepth);
            return directory[h]->search(value);
        }

        void display() {
            set < vector < int > > seen;
            set < pair < int, pair < int, int > > > show;
            for(int i = 0; i < (1 << globalDepth); i++) {
                int h = hash(i, globalDepth);
                if(seen.find(directory[h]->copy()) == seen.end())
                {
                    seen.insert(directory[h]->copy());
                    show.insert(make_pair(directory[h]->getCreationTime(), make_pair(directory[h]->getOccupancy(), directory[h]->getDepth())));
                    // cout << i << " : "; 
                    // cout << directory[h]->getOccupancy() << ", " << directory[h]->getDepth() << endl;
                    // cout << "Elements: ";
                    // directory[hash(i, globalDepth)]->display();

                }

            }

            for(auto it = show.begin(); it != show.end(); it++) {
                cout << (it->second).first << " " << (it->second).second << endl;
            }
        }

        int getDepth() {
            return globalDepth;
        }

        int getBuckets() {
            return nBuckets;
        }
};

int main() {

    int globalDepth, bucket_size;
    cin >> globalDepth >> bucket_size;

    Directory d(globalDepth, bucket_size);

    int q;
    while(cin >> q) {
        if(q == 2) {
            int v; cin >> v;
            d.insert(v);
        }
        if(q == 3) {
            int v; cin >> v;
            d.search(v);
        }
        if(q == 4) {
            int v; cin >> v;
            d.remove(v);
        }
        if(q == 5) {
            cout << d.getDepth() << endl;
            cout << d.getBuckets() << endl;
            d.display();
        }
        if(q == 6) break;
    }

    return 0;
}