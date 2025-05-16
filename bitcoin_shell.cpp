#include "programs.h"

int main(){
    int choice;
    while (true)
    {
    cout<<"\n === Bitcoin Shell Menu===\n";
    cout<<"1. Print db\n";
    cout<<"2. print block by hash\n";
    cout<<"3. print block by height\n";
    cout<<"4. Export data to csv\n";
    cout<<"5. Refreash data\n";
   
    cout<<"Enter your choice:";
    cin >> choice;
    string hash;
    string height;
    string num;

    switch (choice)
    {
    case 1:
    printChainFromFile("blocks.dat");
        break;
    case 2:
    cout<<"Enter block hash:";
    cin>>hash;
    findBlockByField("hash", "blocks.dat",hash);
        break;
    case 3: 
    cout<<"Enter block height:";
    cin>>height;
    findBlockByField("height", "blocks.dat",height);

    case 4:
        exportToCSV("blocks.dat","data.csv");
        break;
    case 5:
    cout << "Plase enter the number of blocks that you would like to fetch";
    cin >> num;
    refreshDataBase("./fetch_blocks.sh",num);
    break;

    case 0 :
    return 0;

        break;
    }
}

    return 0;
    
}