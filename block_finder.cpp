#include "programs.h"

int main(int argc, char* argv[]){

    std::string flag=argv[1];
    std::string value=argv[2];
    
    if(flag== "--hash")
    {

        findBlockByField("hash","blocks.dat",value);
    }
    else if (flag == "--height")
    {
        findBlockByField("height","blocks.dat",value);

    }else{
        std::cerr <<"invalid flag.";
    
    }
    return 0;
   
}