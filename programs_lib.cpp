#include "programs.h"

std::vector<Block> loadBlocksFromFile(const std::string& filename) {
    std::vector<Block> blocks;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << "\n";
        return blocks;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        Block blk;
        std::getline(stream, blk.hash, '|');
        std::getline(stream, blk.height, '|');
        std::getline(stream, blk.total, '|');
        std::getline(stream, blk.time, '|');
        std::getline(stream, blk.relayed_by, '|');
        std::getline(stream, blk.prev_block, '|');
        blk.received_time = blk.time;
        blocks.push_back(blk);
    }

    return blocks;
}


void refreshDataBase(const string& scriptPath,const string& numOfBlocks){

string command = scriptPath + " " + numOfBlocks;
int result = system(command.c_str());
if(result==0)
{
    cout<< "Data Base has been refreshed\n";
}
else{
    cerr<< " Failed, Data base hasnt been refreshed"<< command<<"\n";
}


}



void exportToCSV(const std::string& inFile, const std::string& outFile) {
    std::vector<Block> blocks = loadBlocksFromFile(inFile);
    
    std::ofstream out(outFile);
    if (!out.is_open()) {
        std::cerr << "Could not create " << outFile << "\n";
        return;
    }

    // CSV header
    out << "hash,height,total,time,received_time,relayed_by,prev_block\n";

    for (const auto& blk : blocks) {
        out << blk.hash << ","
            << blk.height << ","
            << blk.total << ","
            << blk.time << ","
            << blk.received_time << ","
            << blk.relayed_by << ","
            << blk.prev_block << "\n";
    }

    std::cout << "Exported " << blocks.size() << " blocks to CSV.\n";
}
void findBlockByField(const string& field, const string& filename, const string& value) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        Block blk;

        std::getline(ss, blk.hash, '|');
        std::getline(ss, blk.height, '|');
        std::getline(ss, blk.total, '|');
        std::getline(ss, blk.time, '|');
        std::getline(ss, blk.relayed_by, '|');
        std::getline(ss, blk.prev_block, '|');
        blk.received_time = blk.time;

        if ((field == "hash" && blk.hash == value) ||
            (field == "height" && blk.height == value)) {
            printBlock(blk);
            return;
        }
    }

    std::cout << "Block not found.\n";
}

void printBlock(const Block& block) {
    std::cout << "hash: " << block.hash << "\n";
    std::cout << "height: " << block.height << "\n";
    std::cout << "total: " << block.total << "\n";
    std::cout << "time: " << block.time << "\n";
    std::cout << "received_time: " << block.time << "\n";
    std::cout << "relayed_by: " << block.relayed_by << "\n";
    std::cout << "prev_block: " << block.prev_block << "\n";
}

void printChainFromFile(const string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << "\n";
        return;
    }

    std::unordered_map<std::string, Block> blocks;
    std::unordered_set<std::string> allPrevHashes;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        Block blk;

        std::getline(ss, blk.hash, '|');
        std::getline(ss, blk.height, '|');
        std::getline(ss, blk.total, '|');
        std::getline(ss, blk.time, '|');
        std::getline(ss, blk.relayed_by, '|');
        std::getline(ss, blk.prev_block, '|');

        blk.received_time = blk.time;
        blocks[blk.hash] = blk;

        if (!blk.prev_block.empty()) {
            allPrevHashes.insert(blk.prev_block);
        }
    }

    file.close();

    // Find latest block (not referenced by any prev_block)
    std::string latestHash;
    for (const auto& [hash, blk] : blocks) {
        if (allPrevHashes.find(hash) == allPrevHashes.end()) {
            latestHash = hash;
            break;
        }
    }

    if (latestHash.empty()) {
        std::cerr << "Could not find the latest block (no head found).\n";
        return;
    }

    // Print the chain in reverse
    while (!latestHash.empty() && blocks.find(latestHash) != blocks.end()) {
        const Block& blk = blocks[latestHash];
        printBlock(blk);

        if (!blk.prev_block.empty() && blocks.find(blk.prev_block) != blocks.end()) {
            std::cout << "    |\n    |\n    V\n";
        }

        latestHash = blk.prev_block;
    }
}
