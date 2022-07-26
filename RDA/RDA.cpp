#include <map>
#include <string>
#include <stdint.h>
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/BasicBlock.h"

using namespace llvm;
using namespace std;


namespace{

  struct ReachingDefinitionAnalysis: public FunctionPass{
    static char ID;
    ReachingDefinitionAnalysis(): FunctionPass(ID){}

    map<Instruction *, int> InstToNumber;
    int inst_idx = 0;
    map<BasicBlock *, BitVector> genB;
    map<BasicBlock *, BitVector> killB;//killB can be the result of genB xor variables involved
    map<Value *, BitVector> variables;//key is value, actually variable names
    bool change_to_output = true;
    map<BasicBlock*, BitVector> outB;
    map<BasicBlock*, BitVector> inB;

    //pick out all the "define" operation in the function
    void Get_definition_number(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                //x = y + 1
                // errs() << opcode << "\n";
                if(opcode == "store") {
                    InstToNumber.insert(make_pair(dyn_cast<Instruction>(inst), inst_idx));
                    inst_idx++;
                }
            }
        }

        //print <inst, idx> 
        //sum.bc: idx is 10
        errs() << "checking out every definition instrument and its index" << "\n";
        for(auto it = InstToNumber.begin(); it != InstToNumber.end(); it++){
            errs() << *(it->first) << ": " << it->second << "\n";
        }
        errs() << "\n";
    }

    void GenerateAndKillOfBlock(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){
            BitVector bv = BitVector(inst_idx, false);
            // BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            // for(auto it = pred_begin(basicblock); it != pred_end(basicblock); it++){
            //     BasicBlock* predecessor = *it;
            //     errs() << *predecessor << "\n";
            // }
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                if(opcode == "store") {
                    //get the index of instruction, the corresponding bit set to 1
                    int idx = InstToNumber[dyn_cast<Instruction>(inst)];
                    bv[idx] = true;//bv.set(idx, true) core dumped
                    Value* operand1 = inst->getOperand(1);
                    // errs() << *operand1 << "\n";
                    if(variables.find(operand1) == variables.end()){
                        BitVector tmp = BitVector(inst_idx, false);
                        variables.insert(make_pair(operand1, tmp));
                    }
                    variables[operand1][idx] = true;
                }
            }
            //bv is like 00110100001
            genB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv));
        }

        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){
            BitVector bv = BitVector(inst_idx, false);
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                if(opcode == "store") {
                    Value* operand1 = inst->getOperand(1);
                    bv |= variables[operand1];
                }
            }
            bv ^= genB[dyn_cast<BasicBlock>(bb)];
            //bv is like 00110100001
            killB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv));
        }

    }

    void Analysis(Function &F){
        //OUT[] = 0 IN[] = 0
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            if(outB.find(basicblock) == outB.end()){
                BitVector bv = BitVector(inst_idx, false);
                outB.insert(make_pair(basicblock, bv));
            }
            if(inB.find(basicblock) == inB.end()){
                BitVector bv = BitVector(inst_idx, false);
                inB.insert(make_pair(basicblock, bv));
            }
        }
        //if change exsits, for each bb, implement transfer function
        //can be improved: when a output of bb has not change, the transfer function can be skipped 
        while(change_to_output){
            change_to_output = false;
            for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
                BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
                BitVector tmp_out = outB[basicblock];
                // for(int i = 0; i < tmp_out.size(); i++) {
                //    errs() << tmp_out[i] << " ";
                // }
                // errs() << "\n";

                // get the predecessor and union
                // BitVector tmp_in = inB[basicblock];
                for(auto it = pred_begin(basicblock); it != pred_end(basicblock); it++){
                    BasicBlock* predecessor = *it;
                    inB[basicblock] |= outB[predecessor];                    
                }

                //there is no change in IN, we can skip the next part

                //outB = genB U (inB - killB)
                for(int i = 0; i < inst_idx; i++){
                    if(inB[basicblock][i] && !killB[basicblock][i])
                        outB[basicblock][i] = true;
                    else
                        outB[basicblock][i] = false;
                }
                outB[basicblock] |= genB[basicblock];

                // for(int i = 0; i < outB[basicblock].size(); i++) {
                //    errs() << outB[basicblock][i] << " ";
                // }
                // errs() << "\n";
                if(outB[basicblock] != tmp_out){
                    change_to_output = true;
                }
            }
        }

    }

    void printRes(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){
            BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            errs() << "Block:" << "\n";
            errs() << *basicblock << "\n";
            errs() << "Input:" << "\n";
            for(int i = 0; i < inB[basicblock].size(); i++) {
                errs() << inB[basicblock][i] << " ";
            }
            errs() << "\n" << "Output:" << "\n";
            for(int i = 0; i < outB[basicblock].size(); i++) {
                errs() << outB[basicblock][i] << " ";
            }
            errs() << "\n" << "\n";
        }
    }

    bool runOnFunction(Function &F) override {
        Get_definition_number(F);
        GenerateAndKillOfBlock(F);
        Analysis(F);
        printRes(F);
        return false;
    }
  };
}

char ReachingDefinitionAnalysis::ID = 0;
static RegisterPass<ReachingDefinitionAnalysis> X("RDA", "Reaching Definition Analysis");