#include "programs.h"

int main(
    int argc, char *argv[]){

    if(argc !=2)
    {  
    cerr<< "Failed. The right format is './refresh_db.out <numberOfBlocks>' \n";
    return 1;
    }

    string numOfBlocks=argv[1];
    refreshDataBase("./fetch_blocks.sh",numOfBlocks);

    return 0;
}