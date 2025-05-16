#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;


struct Block {
    string hash;
    string height;
    string total;
    string time;
    string received_time;
    string relayed_by;
    string prev_block;
};

vector<Block> loadBlocksFromFile(const std::string& filename);
void refreshDataBase(const string& scriptPath,const string& numOfBlocks);
void exportToCSV(const string& inFile,const string& outFile);
void printBlock(const Block& block);
void printChainFromFile(const string& file_name);
void findBlockByField(const string& field, const string& filename, const string& value);