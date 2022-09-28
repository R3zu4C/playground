#include <bits/stdc++.h>

using namespace std;

#define ll long long
#define f(i, a, b) for(ll i = a; i < b; i++)

int totalPageSize;

class Page{
    Page* prevPage;
    Page* nextPage;
    int countOfRecords = 0;
    int startAddressOfFreeSpace = 0;

    int freeSpace;
    map<ll, pair<ll, ll>> directory;
    friend class Heap;
    
public:
    Page(){
        freeSpace = totalPageSize - 4*4;
    }
};

class Heap{
    int numOfPages;
    Page* firstPage;

public:
    Heap(){
        numOfPages = 0;
        firstPage = NULL;
    }

    void showStatus(){
        if(numOfPages > 0){
            cout<<numOfPages<<" ";

            Page* currentPage = firstPage;

            while(currentPage != NULL){
                cout<<currentPage->countOfRecords<<" ";

                currentPage = currentPage->nextPage;
            }cout<<endl;
        }else{
            cout<<numOfPages<<endl;
        }
    }

    void insert(int recordSize, int keyToInsert){
        if(firstPage == NULL){
            firstPage = new Page();

            firstPage -> prevPage = NULL;
            firstPage -> nextPage = NULL;
            firstPage -> countOfRecords = 1;

            firstPage -> directory[firstPage -> startAddressOfFreeSpace] = {keyToInsert, recordSize};
            firstPage -> startAddressOfFreeSpace = firstPage -> startAddressOfFreeSpace + recordSize; // here, a +1 may be required to produce the correct output.
            firstPage -> freeSpace = firstPage -> freeSpace - (recordSize + 4);
            
            numOfPages++;
        }else{
            Page* currentPage = firstPage;
            Page* upcomingPage = NULL;
            bool inserted = false;
            
            while(currentPage != NULL && inserted == false){
                if(currentPage -> freeSpace >= recordSize + 4){
                    //insert the record in currentPage

                    currentPage -> countOfRecords++;
                    
                    currentPage -> directory[currentPage -> startAddressOfFreeSpace] = {keyToInsert, recordSize};
                    currentPage -> startAddressOfFreeSpace = firstPage -> startAddressOfFreeSpace + recordSize;
                    currentPage -> freeSpace = firstPage -> freeSpace - (recordSize + 4);

                    inserted = true;
                }else{
                    if(currentPage->nextPage == NULL){
                        //create a new Page.
                        //Point the nextPage pointer of currentPage to the new Page.
                        //Also point the prevPage pointer of new Page to currentPage.
                        //insert the new Record.
                        //increment the value of numOfPages.

                        upcomingPage = new Page();

                        upcomingPage -> prevPage = currentPage;
                        upcomingPage -> nextPage = NULL;
                        upcomingPage -> countOfRecords = 1;

                        upcomingPage -> directory[upcomingPage -> startAddressOfFreeSpace] = {keyToInsert, recordSize};
                        upcomingPage -> startAddressOfFreeSpace = upcomingPage -> startAddressOfFreeSpace + recordSize;
                        upcomingPage -> freeSpace = upcomingPage -> freeSpace - (recordSize + 4);


                        currentPage -> nextPage = upcomingPage;
                        numOfPages++;

                        inserted = true;
                    }else{
                        //change the value of "currentPage" to "nextPage".

                        currentPage = currentPage -> nextPage;
                    }
                }
                    
            }
            
        }
    }

    void search(int keyToSearch){
        Page* currentPage = firstPage;
        int currentPageID = 0;
        bool found = false;

        while(currentPage != NULL && found == false){
            int slotID = 0;

            for(auto i: currentPage->directory){
                if(i.second.first == keyToSearch){
                    cout<<currentPageID<<" "<<slotID<<endl;
                    found = true;
                    break;
                }
                slotID++;
            }

            currentPage = currentPage -> nextPage;
            currentPageID++;
        }

        if(!found){
            cout<<-1<<" "<<-1<<endl;
        }
        
    }
};

int main(){

    cin>>totalPageSize;

    Heap heap;

    while(1){
        int x;
        cin>>x;

        if(x == 1){
            int recordSize;
            int keyToInsert;

            cin>>recordSize>>keyToInsert;

            heap.insert(recordSize, keyToInsert);
        }else if(x == 2){
            heap.showStatus();
        }else if(x == 3){
            int keyToSearch;

            cin>>keyToSearch;

            heap.search(keyToSearch);
        }else{
            cout<<"Quitting the program..."<<endl;
            break;
        }
    }
}
