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

  struct AvailablExpressionAnalysis: public FunctionPass{
    static char ID;
    AvailablExpressionAnalysis(): FunctionPass(ID){}

    map<string, int> ExpreToNumber;// expression map to idx
    int expre_idx = 0;

    map<Instruction*, int> operaToVar;// variable map to idx
    int var_idx = 0;

    map<Constant*, int> constToIdx;// constant map to idx e.g. double 1.000e+07  ->  const_idx = 0
    int const_idx = 0;

    map<BasicBlock *, BitVector> genB;
    map<BasicBlock *, BitVector> killB;
    bool change_to_output = true;
    map<BasicBlock*, BitVector> outB;
    map<BasicBlock*, BitVector> inB;

    
    //output: 
    //if(v is a constant) constant0/1/2... depending on the map<Constant*, int> constToIdx
    //if(v is a variable) var0/1/2... depending on the map<Instruction*, int> operaToVar
    string getValueName(Value* v){
        Instruction *inst = dyn_cast<Instruction>(v);
        // errs() << *v << "\n";
        string res = ""; 
        if(inst){
            // errs() << *inst << "\n";//%11 = load i32, i32* %4, align 4
            string opcode = inst->getOpcodeName();
            if(opcode == "alloca"){ 
                // Instruction* var = dyn_cast<Instruction>(inst->getOperand(0));
                if(operaToVar.find(inst) == operaToVar.end()){
                    operaToVar.insert(make_pair(inst, var_idx));
                    var_idx++;
                }
                res += "var";
                res += to_string(operaToVar[inst]);
            }
            // if(opcode == "sitofp" || opcode == "load")
            else{
                // errs() << "sitofp" << "\n";
                // errs() << inst->getNumOperands() << "\n";
                res = getValueName(inst->getOperand(0));
            }
        }
        else if(Constant* cons = dyn_cast<Constant>(v)){
            // std::string s = "";
            // raw_string_ostream * strm = new raw_string_ostream(s);
            // cint->getValue().print(*strm,true);
            // errs() << strm->str() << "\n";
            // res += "const";
            // res += strm->str();
            
            // else if(ConstantInt * cint = dyn_cast<ConstantInt>(v)){
            
            // }
            // errs() << *cons << "\n";
            if(constToIdx.find(cons) == constToIdx.end()){
                constToIdx.insert(make_pair(cons, const_idx));
                const_idx++;
            }
            res += "const";
            res += to_string(constToIdx[cons]);
            // errs() << constToIdx[constIntValue] << "\n";
        }
        // errs() << res << "\n";
        return res;
    }

    //pick out all the expression in the function
    void Get_expression_number(Function &F){
        // for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
        //     for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
        //         Instruction* ins = dyn_cast<Instruction>(inst);
        //         errs() << *ins << "\n";
        //     }
        //     errs() << "\n";
        // }
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                //3AC: only care about available expressions for BinaryOperators
                if(inst->isBinaryOp()){
                    string tmp = "";
                    string opcode = inst->getOpcodeName();
                    // Instruction *ins = dyn_cast<Instruction>(inst);
                    // errs() << *ins << "\n"; //%12 = sub nsw i32 %11, 1
                    for(int i = 0; i < inst->getNumOperands(); i++){
                        // errs() << i << "\n";
                        tmp += getValueName(inst->getOperand(i));
                        if(i != inst->getNumOperands()-1){
                            tmp += " ";
                            tmp += opcode;
                            tmp += " ";
                        }        
                    }
                    if(ExpreToNumber.find(tmp) == ExpreToNumber.end()){
                        ExpreToNumber.insert(make_pair(tmp, expre_idx));
                        expre_idx++;
                    }
                } 
            }
        }
        //print <expre, idx> 
        //easy.bc: idx is 0-4, 5 in all
        errs() << "Checking out every expression and its index:" << "\n";
        for(auto it = ExpreToNumber.begin(); it != ExpreToNumber.end(); it++){
            errs() << it->first << ": " << it->second << "\n";
        }
        errs() << "\n";

    }


    void GenerateAndKillOfBlock(Function &F){
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb){
            BitVector bv_genB = BitVector(expre_idx, false);
            BitVector bv_killB = BitVector(expre_idx, false);
            // BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            // for(auto it = pred_begin(basicblock); it != pred_end(basicblock); it++){
            //     BasicBlock* predecessor = *it;
            //     errs() << *predecessor << "\n";
            // }
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                string opcode = inst->getOpcodeName();
                if(inst->isBinaryOp()){
                    string tmp = "";
                    for(int i = 0; i< inst->getNumOperands();i++) {
                        // errs() << i << "\n";
                        tmp += getValueName(inst->getOperand(i));
                        if(i != inst->getNumOperands()-1){
                            tmp += " ";
                            tmp += opcode;
                            tmp += " ";
                        }
                    }
                    if(ExpreToNumber.find(tmp) != ExpreToNumber.end())
                        bv_genB[ExpreToNumber[tmp]] = true;
                    else
                        errs() << "Wrong!" << "\n";
                }
                if(opcode == "store"){
                    Instruction *ins = dyn_cast<Instruction>(inst->getOperand(1));
                    string tmp = "";
                    if(operaToVar.find(ins) != operaToVar.end()){
                        tmp += "var";
                        tmp += operaToVar[ins];
                        // errs() << operaToVar[ins] << "\n";
                    }
                    for(map<string, int>::iterator it = ExpreToNumber.begin();it != ExpreToNumber.end(); it++) {
                        if(it->first.find(tmp) != string::npos) {
                            bv_killB[ExpreToNumber[it->first]] = true; 
                        }
                    }
                }

            }
            genB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv_genB));
            killB.insert(make_pair(dyn_cast<BasicBlock>(bb), bv_killB));
        }
    }

    void Analysis(Function &F){
        //OUT[] = 1111 IN[] = 0000
        for(Function::iterator bb = F.begin(); bb != F.end(); ++bb) {        
            BasicBlock* basicblock = dyn_cast<BasicBlock>(bb);
            if(outB.find(basicblock) == outB.end()){
                BitVector bv = BitVector(expre_idx, true);
                outB.insert(make_pair(basicblock, bv));
            }
            if(inB.find(basicblock) == inB.end()){
                BitVector bv = BitVector(expre_idx, true);
                inB.insert(make_pair(basicblock, bv));
            }
        }
        //if change exsits, for each bb, implement transfer function
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
                    inB[basicblock] &= outB[predecessor];                    
                }

                //there is no change in IN, we can skip the next part

                //outB = genB U (inB - killB)
                for(int i = 0; i < expre_idx; i++){
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
        Get_expression_number(F);
        GenerateAndKillOfBlock(F);
        Analysis(F);
        printRes(F);
        return false;
    }
  };
}

char AvailablExpressionAnalysis::ID = 0;
static RegisterPass<AvailablExpressionAnalysis> X("AEA", "Availabl Expression Analysis");