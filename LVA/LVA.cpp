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

  struct LiveVariableAnalysis: public FunctionPass{
    static char ID;
    LiveVariableAnalysis(): FunctionPass(ID){}

    map<Instruction *, int> VarToNumber;
    int var_idx = 0;
    map<BasicBlock *, BitVector> useB;
    map<BasicBlock *, BitVector> defB;//defB can be the result of useB xor variables involved
    // map<Value *, BitVector> variables;//key is value, actually variable names
    bool change_to_input = true;
    map<BasicBlock*, BitVector> outB;
    map<BasicBlock*, BitVector> inB;

    //pick out all the "define" operation in the function
    void Get_variable_number(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                // collect all the variable
                if(opcode == "alloca") {
                    VarToNumber.insert(make_pair(dyn_cast<Instruction>(inst), var_idx));
                    var_idx++;
                }
            }
        }

        //print <var, idx> 
        //sum.bc: idx is 7 (x y z p q m k)
        errs() << "Checking out every variable and its index" << "\n";
        for(auto it = VarToNumber.begin(); it != VarToNumber.end(); it++){
            errs() << *(it->first) << ": " << it->second << "\n";
        }
        errs() << "\n";
    }

    void DefAndUseOfBlock(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){
            BitVector bv_use = BitVector(var_idx, false);
            BitVector bv_def = BitVector(var_idx, false);
            // BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            // for(auto it = pred_begin(basicblock); it != pred_end(basicblock); it++){
            //     BasicBlock* predecessor = *it;
            //     errs() << *predecessor << "\n";
            // }
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                //store %0, %4, operand[1], add in def
                if(opcode == "store"){
                    // errs() << "left value: " << *(inst->getOperand(1)) << "\n";
                    // errs() << *inst << "\n";
                    Instruction* var = dyn_cast<Instruction>(inst->getOperand(1));
                    bv_def[VarToNumber[var]] = true;
                }
                //%30 = load i32, i32* %4, operand[0]:%4 = alloca i32, add in use
                else if(opcode == "load"){
                    // errs() << "left value: " << *(inst->getOperand(0)) << "\n";
                    // errs() << *inst << "\n";
                    Instruction* var = dyn_cast<Instruction>(inst->getOperand(0));
                    //the use of var should exist before def
                    if(bv_def[VarToNumber[var]] == 0)
                        bv_use[VarToNumber[var]] = true;
                }
            }
            useB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv_use));
            defB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv_def));
        }
    }

    //backward analysis?
    void Analysis(Function &F){
        //OUT[] = 0 IN[] = 0
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            if(outB.find(basicblock) == outB.end()){
                BitVector bv = BitVector(var_idx, false);
                outB.insert(make_pair(basicblock, bv));
            }
            if(inB.find(basicblock) == inB.end()){
                BitVector bv = BitVector(var_idx, false);
                inB.insert(make_pair(basicblock, bv));
            }
        }
        //if change exsits, for each bb, implement transfer function
        while(change_to_input){
            change_to_input = false;
            for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){        
                BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
                BitVector tmp_in = inB[basicblock];
                // for(int i = 0; i < tmp_out.size(); i++) {
                //    errs() << tmp_out[i] << " ";
                // }
                // errs() << "\n";

                //get the successor and union
                // BitVector tmp_out = outB[basicblock];
                for(BasicBlock* succ: successors(basicblock)){
                    outB[basicblock] |= inB[succ];
                }
                //there is no change in OUT, we can skip the next part

                //inB = useB U (outB - defB)
                for(int i = 0; i < var_idx; i++){
                    if(outB[basicblock][i] && !defB[basicblock][i])
                        inB[basicblock][i] = true;
                    else
                        inB[basicblock][i] = false;
                }
                inB[basicblock] |= useB[basicblock];

                // for(int i = 0; i < outB[basicblock].size(); i++) {
                //    errs() << outB[basicblock][i] << " ";
                // }
                // errs() << "\n";
                if(inB[basicblock] != tmp_in){
                    change_to_input = true;
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

    bool runOnFunction(Function &F) override{
        Get_variable_number(F);
        DefAndUseOfBlock(F);
        Analysis(F);
        printRes(F);
        return false;
    }
  };
}

char LiveVariableAnalysis::ID = 0;
static RegisterPass<LiveVariableAnalysis> X("LVA", "Live Variable Analysis");