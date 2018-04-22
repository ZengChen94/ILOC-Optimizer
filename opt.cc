#include <stdio.h>
#include <string>
#include <cstring>
#include <vector>
// #include "opt.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <map>
#include <algorithm>

using namespace std;

extern "C" {
    #include "instruction.h"
}

class ilocInstruction {
public:
    Opcode_Name opCode;
    vector<string> sourceOperands;
    vector<string> targetOperands;
    string label = "";
};

void readLabels(string filename);
void multi2shifti();
void parseILOC();
void buildCFG();
void optVN();
void optLoopUnrolling();
void print2file(string outputFileName);
void print_instruction(ilocInstruction instruction);
bool compare(string label1, string label2);
int helper(int num);
int find_max_register();
void optCodeMotion();
void print2screen();
int find_max_label();

string enumList[50] = {"nop", "add", "sub", "mult", "div", "addI", "subI", "multI",
              "divI", "lshift", "lshiftI", "rshift", "rshiftI",
              "and", "andI", "or", "orI", "not",
              "loadI", "load", "loadAI", "loadAO", "cload", "cloadAI",
              "cloadAO", "store", "storeAI", "storeAO", "cstore", 
              "cstoreAI", "cstoreAO", "br", "cbr", "cmp_LT", "cmp_LE",
              "cmp_EQ", "cmp_NE", "cmp_GE", "cmp_GT", "i2i", "c2c", "i2c", "c2i",
              "output", "coutput", "read", "cread", "write", "cwrite", "halt"};

unordered_map<Instruction*, Label*> labelMap;

vector<string> labelList;

vector<ilocInstruction> instructions;

vector<int> block_begins;
vector<int> block_ends;

static FILE *ilocFile = NULL;
string outputFileName;

int main(int argc, char** argv) {
    // argc = 3;
    // argv[1] = (char*) "-v";
    // argv[2] = (char*) "./TestCodes/algred/algred.i";

    if (argc != 3 && argc != 4) {
        cout << "Error: please fill in 2 or 3 parameters" << endl;
        cout << "----------Choice 1----------" << endl;
        cout << "args: flags file_to_optimize" << endl;
        cout << "argv[1]: flags that specify the optimizations to run" << endl;
        cout << "argv[2]: the name of the ILOC input file" << endl;
        cout << "----------Choice 2----------" << endl;
        cout << "args: flags file_to_optimize" << endl;
        cout << "argv[1]: flags1 that specify the optimizations to run" << endl;
        cout << "argv[2]: flags2 that specify the optimizations to run" << endl;
        cout << "argv[3]: the name of the ILOC input file" << endl;
        return 0;
    }

    // cout << argc << endl;

    if (argc == 3) {
        ilocFile = fopen(argv[2],"r");
        if (ilocFile == NULL) {
            fprintf(stderr,"Error: failed to open the ILOC file '%s'.\n", argv[2]);
            return 0;
        }
        stdin = ilocFile;

        parseILOC();

        buildCFG();

        multi2shifti();

        outputFileName = "./output.i";

        if (strcmp(argv[1], "-v") == 0) { 
            optVN();
        } 
        else if (strcmp(argv[1], "-u") == 0) {
            optLoopUnrolling();
        }
        else if (strcmp(argv[1], "-i") == 0) {
            // optCodeMotion();
            cout << "-i: code motion not implemented" << endl;
            return 0;
        }
        else {
            cout << "Error: only option -v, -u are provided." << endl;
            return 0;
        }
    }
    else if (argc == 4) {
        ilocFile = fopen(argv[3],"r");
        if (ilocFile == NULL) {
            fprintf(stderr,"Error: failed to open the ILOC file '%s'.\n", argv[3]);
            return 0;
        }
        stdin = ilocFile;

        parseILOC();

        buildCFG();

        multi2shifti();

        outputFileName = "./output.i";

        if (strcmp(argv[1], "-i") == 0 || strcmp(argv[2], "-i") == 0) {
            cout << "-i: code motion not implemented" << endl; 
        } 
        else if (strcmp(argv[1], "-v") == 0 && strcmp(argv[2], "-u") == 0) {
            optVN();
            buildCFG();
            optLoopUnrolling();
        }
        else if (strcmp(argv[1], "-u") == 0 && strcmp(argv[2], "-v") == 0) {
            optLoopUnrolling();
            buildCFG();
            optVN();
        }
        else {
            cout << "Error: only option -v, -u are provided." << endl;
            return 0;
        }
    }
    

    // print2screen();

    print2file(outputFileName);

    return 0;
}

void parseILOC() {
    instructions.clear();
    Instruction* instructionEntry = parse();
    int labelCnt = 0;
    Instruction* cur = instructionEntry;
    while (cur != nullptr) {
        string res = "";
        Operation* operation = cur->operations;
        ilocInstruction instruction;
        instruction.opCode = operation->opcode;

        if (operation->srcs != nullptr) {
            instruction.sourceOperands.push_back("r" + to_string(operation->srcs->value));
            res = res + to_string(operation->srcs->value);
            if (operation->srcs->next != nullptr) {
                instruction.sourceOperands.push_back("r" + to_string(operation->srcs->next->value));
            }
        }

        if (operation->consts != nullptr) {
            instruction.sourceOperands.push_back(to_string(operation->consts->value));
            if (operation->consts->next != nullptr) {
                instruction.sourceOperands.push_back(to_string(operation->consts->next->value));
            }
        }

        if (operation->defs != nullptr) {
            instruction.targetOperands.push_back("r" + to_string(operation->defs->value));
            if (operation->defs->next != nullptr) {
                instruction.targetOperands.push_back("r" + to_string(operation->defs->next->value));
            }
        }

        // if (operation->opcode == opcode_name::NOP)
        //     instruction.label = labelList[labelCnt++];

        if (operation->labels != nullptr) {
            // string label1 = "L" + to_string(operation->labels->value);
            string label1 = string(get_label(operation->labels->value)->string);
            instruction.targetOperands.push_back(label1);
            string label2 = "";
            if (operation->labels->next != nullptr) {
                // label2 = "L" + to_string(operation->labels->next->value);
                label2 = string(get_label(operation->labels->next->value)->string);
                instruction.targetOperands.push_back(label2);
            }

            Label* label3 = get_label(operation->labels->value);
            labelMap[label3->target] = label3;
            if (operation->labels->next != nullptr) {
                Label* label4 = get_label(operation->labels->next->value);
                labelMap[label4->target] = label4;
            }
        }

        if (labelMap.find(cur) != labelMap.end()) {
            Label *curLabel = labelMap[cur];
            instruction.label = curLabel->string;
        }

        instructions.push_back(instruction);

        cur = cur->next;
    }
}

void print2file(string outputFileName) {
    ofstream in;
    in.open(outputFileName,ios::trunc);
    for (auto instruction : instructions) {
        string res = "";
        if (instruction.label != "") {
            res = (instruction.label + ":\t");
        }
        else {
            res = "\t";
        }
        res += enumList[instruction.opCode] + " ";
        for (auto sourceOperand : instruction.sourceOperands) {
            res += sourceOperand + ",";
        }
        if (instruction.sourceOperands.size() != 0) {
            res.pop_back();
        }
        if (instruction.targetOperands.size() != 0) {
            if (enumList[instruction.opCode] == "cbr" || enumList[instruction.opCode] == "br") {
                res += "->";
            }
            else {
                res += "=>";
            }
        }
        for (auto targetOperand : instruction.targetOperands) {
            res += targetOperand + ",";
        }
        if (instruction.targetOperands.size() != 0) {
            res.pop_back();
        }
        // cout << res << endl;
        in << res << endl;
    }
    in.close();
}

void print2screen() {
    for (auto instruction : instructions) {
        string res = "";
        if (instruction.label != "") {
            res = (instruction.label + ":\t");
        }
        else {
            res = "\t";
        }
        res += enumList[instruction.opCode] + " ";
        for (auto sourceOperand : instruction.sourceOperands) {
            res += sourceOperand + ",";
        }
        if (instruction.sourceOperands.size() != 0) {
            res.pop_back();
        }
        if (instruction.targetOperands.size() != 0) {
            if (enumList[instruction.opCode] == "cbr" || enumList[instruction.opCode] == "br") {
                res += "->";
            }
            else {
                res += "=>";
            }
        }
        for (auto targetOperand : instruction.targetOperands) {
            res += targetOperand + ",";
        }
        if (instruction.targetOperands.size() != 0) {
            res.pop_back();
        }
        cout << res << endl;
    }
}

void buildCFG() {
    int cnt = 0;
    block_begins.clear();
    block_ends.clear();

    block_begins.push_back(cnt);
    for (auto instruction : instructions) {
        if (instruction.label != "") {
            block_ends.push_back(cnt-1);
            block_begins.push_back(cnt);
        }
        cnt += 1;
    }
    block_ends.push_back(cnt-1);
}

void optVN() {
    for (int i = 0; i < block_begins.size(); i++) {
        // cout << i << endl;
        int begin = block_begins[i];
        int end = block_ends[i];
        unordered_map<string, unordered_set<string>> VN_pair1;// left_expression, rignt_set
        unordered_map<string, string> VN_pair2;// right, left
        for (int j = begin; j <= end; j++) {
            ilocInstruction instruction = instructions[j];
            string left = "";
            string right = "";

            string sign = enumList[instruction.opCode];

            if (sign.compare("add") != 0 && sign.compare("sub") != 0 && sign.compare("mult") != 0 && sign.compare("div") != 0 
                && sign.compare("addI") != 0 && sign.compare("subI") != 0 && sign.compare("multI") != 0 && sign.compare("divI") != 0) {
                continue;
            }

            // // DON'T SUBSTITE
            // if (instruction.sourceOperands[0].compare(instruction.targetOperands[0]) == 0 || instruction.sourceOperands[1].compare(instruction.targetOperands[0]) == 0) {
            //     continue;
            // }

            if (sign.compare("add") == 0 || sign.compare("mult") == 0) {
                if (compare(instruction.sourceOperands[0], instruction.sourceOperands[1])) {
                    string tmp = instruction.sourceOperands[0];
                    instruction.sourceOperands[0] = instruction.sourceOperands[1];
                    instruction.sourceOperands[1] = tmp;
                }
                // print_instruction(instruction);
            }

            left = sign + ":";
            for (auto sourceOperand : instruction.sourceOperands) {
                left += sourceOperand + ",";
            }
            if (instruction.sourceOperands.size() != 0) {
                left.pop_back();
            }

            for (auto targetOperand : instruction.targetOperands) {
                right += targetOperand + ",";
            }
            if (instruction.targetOperands.size() != 0) {
                right.pop_back();
            }

             // cout << instruction.sourceOperands[0] << ", " << instruction.sourceOperands[1] << endl;

            if (VN_pair1.find(left) != VN_pair1.end() && VN_pair1[left].size() != 0) {
                // instruction.opCode = opcode_name::I2I;
                // vector<string> newSourceOperands;
                // newSourceOperands.push_back(*VN_pair1[left].begin());
                // instruction.sourceOperands = newSourceOperands;
                // instructions[j] = instruction;
                string right_tmp = *VN_pair1[left].begin();
                if (instruction.sourceOperands[0].compare(right_tmp) != 0 && instruction.sourceOperands[1].compare(right_tmp) != 0) {
                    instruction.opCode = opcode_name::I2I;
                    vector<string> newSourceOperands;
                    newSourceOperands.push_back(*VN_pair1[left].begin());
                    instruction.sourceOperands = newSourceOperands;
                    instructions[j] = instruction;
                }
            }

            if (VN_pair2.find(right) != VN_pair2.end()) {
                string left_ori = VN_pair2[right];
                VN_pair1[left_ori].erase(right);
            }
            VN_pair1[left].insert(right);
            VN_pair2[right] = left;

        }
        // cout << "-----------" << endl;
    }
}

void print_instruction(ilocInstruction instruction) {
    string res = "";
    if (instruction.label != "") {
        res = (instruction.label + ":\t");
    }
    else {
        res = "\t";
    }
    res += enumList[instruction.opCode] + " ";
    for (auto sourceOperand : instruction.sourceOperands) {
        res += sourceOperand + ",";
    }
    if (instruction.sourceOperands.size() != 0) {
        res.pop_back();
    }
    if (instruction.targetOperands.size() != 0) {
        if (enumList[instruction.opCode] == "cbr" || enumList[instruction.opCode] == "br") {
            res += "->";
        }
        else {
            res += "=>";
        }
    }
    for (auto targetOperand : instruction.targetOperands) {
        res += targetOperand + ",";
    }
    if (instruction.targetOperands.size() != 0) {
        res.pop_back();
    }
    cout << res << endl;
}

void readLabels(string filename) {
    vector<string> labelList_tmp;
    ifstream fin(filename);  
    string res;  
    while (fin >> res) {
        if (res.find("L") == 0 && res.find(":") == res.length()-1) {
            res.pop_back();
            labelList.push_back(res);
            labelList_tmp.push_back(res);
        }
    }
    for (int i = labelList_tmp.size()-1; i > 0; i--) {
        for (int j = 0; j < i; j++) {
            if (!compare(labelList_tmp[j], labelList_tmp[j+1])) {
                string tmp = labelList_tmp[j];
                labelList_tmp[j] = labelList_tmp[j+1];
                labelList_tmp[j+1] = tmp;
            }
        }
    }
    int cnt = 0;
    for (auto label_tmp : labelList_tmp) {
        for (int i = 0; i < labelList.size(); i++) {
            if (labelList[i].compare(label_tmp) == 0) {
                labelList[i] = "L" + to_string(cnt);
                cnt += 1;
            }
        }
    }
    // for (auto label : labelList) {
    //     cout << label << endl;
    // }
}

bool compare(string label1, string label2) {
    return stoi(label1.substr(1)) < stoi(label2.substr(1));
}

void multi2shifti() {
    for (int i = 0; i < block_begins.size(); i++) {
        // cout << i << endl;
        int begin = block_begins[i];
        int end = block_ends[i];
        for (int j = begin; j <= end; j++) {
            ilocInstruction instruction = instructions[j];
            // print_instruction(instruction);
            // cout << "instruction" << endl;
            string sign = enumList[instruction.opCode];
            if (sign.compare("multI") == 0) {
                // print_instruction(instruction);
                // cout << stoi(instruction.sourceOperands[1]) << ", " << helper(stoi(instruction.sourceOperands[1])) << endl;
                int cons = helper(stoi(instruction.sourceOperands[1]));
                if (cons == -1) {
                    continue;
                }
                instruction.opCode = opcode_name::LSHIFTI;
                instruction.sourceOperands[1] = to_string(cons);
                instructions[j] = instruction;
            }
        }
    }
}

int helper(int num) {
    int cnt = 0;
    while (num % 2 == 0) {
        num = num / 2;
        cnt += 1;
    }
    if (num == 1)
        return cnt;
    else 
        return -1;
}

void optLoopUnrolling() {
    int cur_register = find_max_register() + 1;
    // int cur_label = labelList.size();
    int cur_label = find_max_label() + 1;
    for (int i = 0; i < block_begins.size(); i++) {
        int begin = block_begins[i];
        int end = block_ends[i];
        ilocInstruction instruction_begin = instructions[begin];
        ilocInstruction instruction_end = instructions[end];
        if (enumList[instruction_end.opCode].compare("cbr") == 0) {
            if ((instruction_end.targetOperands[0].compare(instruction_begin.label) == 0 || instruction_end.targetOperands[1].compare(instruction_begin.label) == 0)
                && enumList[instructions[end-2].opCode].compare("addI") == 0 && instructions[end-2].sourceOperands[1].compare("1") == 0) {
                bool flag = true;
                for (int j = begin; j <= end; j++) {
                    ilocInstruction instruction = instructions[j];
                    if (enumList[instruction.opCode].compare("store") == 0 || enumList[instruction.opCode].compare("load") == 0) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    // for (int j = begin; j <= end; j++) {
                    //     ilocInstruction instruction = instructions[j];
                    //     print_instruction(instruction);
                    // }
                    string label1 = "L" + to_string(cur_label);
                    string label2 = "L" + to_string(cur_label+1);
                    string register1 = "r" + to_string(cur_register);
                    string register2 = "r" + to_string(cur_register+1);
                    cur_label = cur_label + 2;
                    cur_register = cur_register + 2;

                    // substitute jump label
                    for (int j = 0; j < instructions.size(); j++) {
                        if (begin <= j && j <= end) {
                            continue;
                        }
                        ilocInstruction instruction = instructions[j];
                        if (enumList[instruction.opCode].compare("cbr") == 0) {
                            if (instruction.targetOperands[0].compare(instruction_begin.label) == 0)
                                instruction.targetOperands[0] = label1;
                            if (instruction.targetOperands[1].compare(instruction_begin.label) == 0)
                                instruction.targetOperands[1] = label1;
                        }
                        instructions[j] = instruction;
                    }

                    ilocInstruction instruction1;
                    instruction1.label = label1;
                    instruction1.opCode = opcode_name::NOP;
                    instructions.push_back(instruction1);

                    ilocInstruction instruction2;
                    instruction2.opCode = opcode_name::ADDI;
                    instruction2.sourceOperands.push_back(instructions[end-2].sourceOperands[0]);
                    instruction2.sourceOperands.push_back("4");
                    instruction2.targetOperands.push_back(register1);
                    instructions.push_back(instruction2);

                    ilocInstruction instruction3;
                    instruction3.opCode = instructions[end-1].opCode;
                    instruction3.sourceOperands.push_back(register1);
                    instruction3.sourceOperands.push_back(instructions[end-1].sourceOperands[1]);
                    instruction3.targetOperands.push_back(register2);
                    instructions.push_back(instruction3);

                    ilocInstruction instruction4;
                    instruction4.opCode = opcode_name::CBR;
                    instruction4.sourceOperands.push_back(register2);
                    instruction4.targetOperands.push_back(label2);
                    instruction4.targetOperands.push_back(instruction_begin.label);
                    instructions.push_back(instruction4);

                    ilocInstruction instruction5;
                    instruction5.label = label2;
                    instruction5.opCode = opcode_name::NOP;
                    instructions.push_back(instruction5);

                    for (int k = 0; k < 4; k++) {
                        for (int h = begin+1; h < end-2; h++) {
                            ilocInstruction instruction = instructions[h];
                            instructions.push_back(instruction);
                        }
                    }

                    ilocInstruction instruction6;
                    instruction6.opCode = opcode_name::ADDI;
                    instruction6.sourceOperands.push_back(instructions[end-2].sourceOperands[0]);
                    instruction6.sourceOperands.push_back("4");
                    instruction6.targetOperands.push_back(instructions[end-2].sourceOperands[0]);
                    instructions.push_back(instruction6);

                    ilocInstruction instruction7;
                    instruction7.opCode = opcode_name::BR;
                    instruction7.targetOperands.push_back(label1);
                    instructions.push_back(instruction7);
                }
            }
        }
    }
}

int find_max_register() {
    int max = 0;
    for (auto instruction : instructions) {
        for (auto sourceOperand : instruction.sourceOperands) {
            if (sourceOperand.find("r") == 0) {
                // cout << sourceOperand.substr(1) << endl;
                max = max > stoi(sourceOperand.substr(1))? max : stoi(sourceOperand.substr(1));
            }
        }
        for (auto targetOperand : instruction.targetOperands) {
            if (targetOperand.find("r") == 0) {
                // cout << targetOperand.substr(1) << endl;
                max = max > stoi(targetOperand.substr(1))? max : stoi(targetOperand.substr(1));
            }
        }
    }
    return max;
}

int find_max_label() {
    int max = 0;
    for (auto it = labelMap.begin(); it != labelMap.end(); ++it) {
        int num = stoi(string(it->second->string).substr(1));
        max = max > num? max : num;
        // max = max > stoi(it->second->string.substr(1))? max : stoi(it->second->string.substr(1));
    }
    // cout << max << endl;
    return max;
}

void optCodeMotion() {

}