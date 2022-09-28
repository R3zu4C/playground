#include <bits/stdc++.h>

using namespace std;
#define pb push_back

class Page{
    int maxNumOfRecords;
    int availableRecords;
    int* array;
    friend class File;
public:
    Page(int recordsPerPage){
        array = new int[recordsPerPage];
        maxNumOfRecords = recordsPerPage;
        availableRecords = 0;
    }

    int availableRecordsInPage(){
        return availableRecords;
    }
};

class File{
    deque<Page*> pagesPtr;
public:
    void insertPagePtr(Page* page){
        pagesPtr.pb(page);
    }

    Page* pointToPage(int pageIndex){
        return pagesPtr[pageIndex];
    }

    void insertInPage(Page* currentPage, int posToInsert, int numToInsert){
        currentPage->array[posToInsert] = numToInsert;
        currentPage->availableRecords++;
    }

    void extSort(int B){
        int pass = 0;
        while(pagesPtr.size() > 1){
            internalSort(pass, B);

            pass++;
        }

    }

    void internalSort( int pass, int B){
        if(pass == 0){
            int pageCount = 0;
            int initialPagesPtrSize = pagesPtr.size();
            int maxRecordsInNewPage = pagesPtr[0] -> maxNumOfRecords;

            vector<int> temp;
            while(pageCount <= initialPagesPtrSize){
                //If pageCount % B == 0, then insert elements of sorted temp array in newPages.
                if(pageCount % B == 0 || pageCount == initialPagesPtrSize){
                    //sort temp array
                    sort(temp.begin(), temp.end());

                    //create new pages whenever previous page is full
                        //insert the newPage in pagesPtr
                        //insert the element in new page
                    int i = 0;
                    Page* currentPagePtr;
                    while(i < temp.size()){

                        if(i % maxRecordsInNewPage == 0){
                            Page* newPage = new Page(maxRecordsInNewPage);
                            currentPagePtr = newPage;
                            pagesPtr.pb(newPage);
                        }

                        insertInPage(currentPagePtr, i % maxRecordsInNewPage, temp[i]);
                        i++;

                    }

                    temp.clear();
                }

                if(pageCount != initialPagesPtrSize){
                    //Otherwise, add the elemets of pagesPtr[0] to temp array.
                    for(int i = 0; i < pagesPtr[0]->availableRecordsInPage(); i++){
                        temp.pb(pagesPtr[0]->array[i]);
                    }

                    //delete the array which is pointed by pagesPtr[0] -> array;
                    //remove its pointer from pagesPtr deque.
                    delete pagesPtr[0] -> array;
                    pagesPtr.pop_front();
                }

                //Go to the next page
                pageCount++;
            }
        }else{
            //For pass = 1,2,3...... do this :-
            int pageCount = 0;
            int initialPagesPtrSize = pagesPtr.size();

            int numOfPagesToBeMerged = (B - 1);
            int maxRecordsInNewPage = numOfPagesToBeMerged * (pagesPtr[0] -> maxNumOfRecords);

            vector<int> temp;
            while(pageCount <= initialPagesPtrSize){
                //If pageCount % B == 0, then insert elements of sorted temp array in newPages.
                if(pageCount % numOfPagesToBeMerged == 0 || pageCount == initialPagesPtrSize){
                    //sort temp array
                    sort(temp.begin(), temp.end());

                    //create new pages whenever previous page is full
                        //insert the newPage in pagesPtr
                        //insert the element in new page
                    int i = 0;
                    Page* currentPagePtr;
                    while(i < temp.size()){

                        if(i % maxRecordsInNewPage == 0){
                            Page* newPage = new Page(maxRecordsInNewPage);
                            currentPagePtr = newPage;
                            pagesPtr.pb(newPage);
                        }

                        insertInPage(currentPagePtr, i % maxRecordsInNewPage, temp[i]);
                        i++;

                    }
                    temp.clear();
                }
                
                if(pageCount != initialPagesPtrSize){
                    //Otherwise, add the elemets of pagesPtr[0] to temp array.
                    for(int i = 0; i < pagesPtr[0]->availableRecordsInPage(); i++){
                        temp.pb(pagesPtr[0]->array[i]);
                    }

                    //delete the array which is pointed by pagesPtr[0] -> array;
                    //remove its pointer from pagesPtr deque.
                    delete pagesPtr[0] -> array;
                    pagesPtr.pop_front();
                }
                //Go to the next page
                pageCount++;
            }
        }
        return;
    }

    void showStatus(){
        int i = 0;
        while(i < pagesPtr[0]->availableRecordsInPage()){
            cout<<pagesPtr[0]->array[i]<<endl;
            i++;
        }
    }
};

int main(){
    int B;  //Available Buffer Pages
    cin>>B;

    int numOfRecordsPerPage;
    cin>>numOfRecordsPerPage;

    int totalNumOfRecords;
    cin>>totalNumOfRecords;

    int numOfPages = (totalNumOfRecords / numOfRecordsPerPage);
    //Number of pages required is equal to (totalNumOfRecords/numOfRecordsPerPage)
    (totalNumOfRecords % numOfRecordsPerPage) > 0 ? numOfPages += 1 : numOfPages = numOfPages;

    File file;

    for(int i = 0; i < numOfPages; i++){
        Page* newPage = new Page(numOfRecordsPerPage);
        file.insertPagePtr(newPage);
    }

    int cnt = 0;
    Page* currentPagePtr;
    for(int i = 0; i < totalNumOfRecords; i++){
        if(cnt % numOfRecordsPerPage == 0){
            int pageIndex = cnt / numOfRecordsPerPage;
            currentPagePtr = file.pointToPage(pageIndex);
        }

        int numToInsert;
        cin>>numToInsert;

        file.insertInPage(currentPagePtr, cnt % numOfRecordsPerPage, numToInsert);
        cnt++;
    }

    file.extSort(B);
    file.showStatus();

    return 0;
}
