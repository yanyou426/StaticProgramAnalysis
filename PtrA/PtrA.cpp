#include <map>
#include <string>
#include <stdint.h>
#include <bits/stdc++.h>
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
#include "llvm/IR/Value.h"

using namespace llvm;
using namespace std;

// still some situations not considered

namespace{
	struct PtrA: public ModulePass{
        static char ID;
        PtrA(): ModulePass(ID){}

        vector<Function*> RM;
        int newCount = 0;
        queue<pair<Value*, vector<int>>> WL;
        map<Value*, vector<Value*>> PFG;
        map<Value*, vector<int>> PT;
        map<Value*, int> newObj;
        map<string, vector<pair<size_t, string>>> CG;

        void AddEdge(Value *s, Value *t){
            if(PFG.find(s) == PFG.end()){
                vector<Value*> tmp;
                PFG.insert(make_pair(s, tmp));

            }
            vector<Value*> v = PFG[s];
            if(find(v.begin(), v.end(), t) == v.end()){
                PFG[s].push_back(t);
                //if PT[s] is not empty
                if(PT.find(s) != PT.end()){
                    vector<int> pts = PT[s];
                    WL.push(make_pair(t, pts));
                }
            }
        }

        void AddReachable(Function* F){
            if(find(RM.begin(), RM.end(), F) == RM.end()){
                RM.push_back(F);
                for(Function::iterator bb = F->begin(); bb != F->end(); ++bb){
                    for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst){
                        string opcode = inst->getOpcodeName();
                        // errs() << *inst <<"\n";
                        if(opcode == "store"){
                            // errs() << *inst <<"\n";
                            Value *leftV = inst->getOperand(1);
                            Value *rightV = inst->getOperand(0);
                            // errs() << *leftV << "\n";
                            // errs() << *rightV << "\n";
                            if(dyn_cast<AllocaInst>(leftV) && dyn_cast<LoadInst>(rightV)){
                                LoadInst *ldinst = dyn_cast<LoadInst>(rightV);
                                if(dyn_cast<AllocaInst>(ldinst->getOperand(0))){
                                    // errs() << *(ldinst->getOperand(0)) << "\n";
                                    AddEdge(ldinst->getOperand(0), leftV);
                                }
                            }
                            
                            else if(dyn_cast<AllocaInst>(leftV) && dyn_cast<BitCastInst>(rightV)){
                                BitCastInst *bcinst = dyn_cast<BitCastInst>(rightV);
                                // errs() << bcinst->getNumOperands() << "\n";
                                
                                Value* bcv = bcinst->getOperand(0);
                                // errs() << "wow!" << *bcv << "\n";

                                vector<int> newinsert;
                                newinsert.push_back(newCount);//<leftv, {o_new}>
                                WL.push(make_pair(leftV, newinsert));

                                newObj.insert(make_pair(bcv, newCount));           
                                //map<Value*, vector<int>> PT;
                                // if(PT.find(leftV) == PT.end()){
                                //     vector<int> tmp;
                                //     PT.insert(make_pair(leftV, tmp));
                                // }
                                // PT[leftV].push_back(newCount);
                                newCount++;
                            }
                            
                        }
                    }
                }
            }
        }

        void Propagate(Value *n, vector<int> pts){
            if(pts.size() != 0){
                if(PT.find(n) == PT.end()){
                    vector<int> tmp;
                    PT.insert(make_pair(n, tmp));
                }
                for(auto it = pts.begin(); it != pts.end(); it++){
                    if(find(PT[n].begin(), PT[n].end(), *it) == PT[n].end()){
                        PT[n].push_back(*it);
                    }
                }
                if(PFG.find(n) != PFG.end()){
                    for(Value* v: PFG[n]){
                        WL.push(make_pair(v, pts));
                    }
                }
            }            
        }
        string getStrOfV(Value *v) {
            string buffer;
            raw_string_ostream ostream(buffer);
            ostream << *v;
            return ostream.str();
        }

        

        //need to search all the functions in the RM
        //unbelievable, there are too many loops...
        //lack of more situations to consider...
        void ProcessCall(Module &M, Value* x, Value* o_i){
            vector<Function*> tmp_RM = RM;
            int l = 0;
            //go through all the statement in RM
            for(Function* F: tmp_RM){
                for (Function::iterator bb = F->begin(); bb != F->end(); ++bb){
                    for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst){
                        // errs() << *inst <<"\n";
                        Function* f = NULL;
                        l++;
                        if(CallInst *callinst = dyn_cast<CallInst>(inst)){
                            // errs() << *callinst <<"\n";
                            if(!callinst->isIndirectCall()){
                                
                                // errs() << *callinst <<"\n";
                                f = callinst->getCalledFunction();
                                // errs() << *f <<"\n";
                               
                               //exists some functions without content
                                if(f->begin() != f->end()){
                                    // errs() << *f <<"\n";
                                    // ; Function Attrs: noinline nounwind optnone uwtable
                                        // define linkonce_odr dso_local void @_ZN1A3fooES_(%class.A*) #2 comdat align 2 {
                                        // %2 = alloca %class.A, align 1
                                        // %3 = alloca %class.A*, align 8
                                        // store %class.A* %0, %class.A** %3, align 8
                                        // %4 = load %class.A*, %class.A** %3, align 8
                                        // ret void
                                        // }
                                    Instruction* start = dyn_cast<Instruction>(f->begin()->begin());
                                    // errs() << *start <<"\n";// %2 = alloca %class.A, align 1
                                    string startClass = getClassOfV(start);
                                    // errs() << startClass << "\n";
                                    string oiclass = getClassOfV(o_i);
                                    // errs() << oiclass << "\n";
                                    if(oiclass == startClass){
                                        string calledFunc = f->getName().data();
                                        // errs() << "wow!" << "\n";
                                        // errs() << calledFunc <<"\n";
                                    }
                                    else
                                        continue;
                                        
                                    
                                }
                            }

                            if(f){
                                // errs() << "dispatch func: " << f->getName() << "\n";
                                int idx = newObj[o_i];
                                vector<int> tmp;
                                tmp.push_back(idx);
                                Value* v = dyn_cast<Value>(f->begin()->begin());
                                WL.push(make_pair(v, tmp));
                                string callFunc = F->getName().data();
                                bool needadding = true;
                                if(CG.find(callFunc) == CG.end()){
                                    vector<pair<size_t, string>> tmpp;
                                    tmpp.push_back(make_pair(l, f->getName().data()));
                                    CG.insert(make_pair(callFunc, tmpp));
                                }
                                else{
                                    vector<pair<size_t, string>> tmpp = CG[callFunc];
                                    
                                    for(auto it = tmpp.begin(); it != tmpp.end(); it++){
                                        if(it->first == l && it->second == f->getName().data()){
                                            needadding = false;
                                            break;
                                        }
                                    }
                                    if(needadding){
                                        CG[callFunc].push_back(make_pair(l, f->getName().data()));
                                    }
                                }
                                if(needadding){
                                    AddReachable(f);
                                    int idx = 0;
                                    //for each parameter
                                    for(Argument &arg: f->args()){
                                        Value* param = &arg;
                                        Value* res = callinst->getOperand(idx);
                                        while(!dyn_cast<AllocaInst>(res)){
                                            Instruction *inst = dyn_cast<Instruction>(res);
                                            if(dyn_cast<LoadInst>(res)){
                                                res = inst->getOperand(0);
                                            }
                                        }
                                        // if (!dyn_cast<Constant>(res)){
                                            errs() << f->getName() << " argument: " << *res << "\n";
                                            AddEdge(res, param);
                                        // } 
                                        // else{
                                        //     errs() << f->getName() << " constant argument: " << *res << "\n";
                                        //     continue;
                                        // }
                                        idx++;
                                    }


                                    //for ret
                                    Value* r;
                                    Value *callValue = dyn_cast<Value>(inst);
                                    for(User *U : callValue->users()) {
                                        if(StoreInst *store = dyn_cast<StoreInst>(U)){
                                            r = store->getOperand(1);
                                            break;
                                        } 
                                        else if(dyn_cast<BitCastInst>(U)){
                                            User *tmp = U->user_back();
                                            if(StoreInst *store = dyn_cast<StoreInst>(U)) {
                                                r = store->getOperand(1);
                                                break;
                                            }
                                        }
                                    }
                                    for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {        
                                        for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
                                            if(ReturnInst *mret = dyn_cast<ReturnInst>(inst)) {
                                                Value *mretValue = mret->getReturnValue();
                                                if(LoadInst *load = dyn_cast<LoadInst>(mretValue)){
                                                    mretValue = load->getOperand(0);
                                                    // errs() << "return value in callee: " << *mretValue << "\n";
                                                    AddEdge(mretValue, r);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        string getClassOfV(Value *v) {
            string objClass = "";
            string objValue = getStrOfV(v);
            // size_t start = objValue.find("\%class");
            size_t start = objClass.find("%");
            if(start != string::npos){
                if(dyn_cast<AllocaInst>(v)) { 
                    size_t len = objValue.find(", align") - (start + 1);
                    objClass = objValue.substr(start + 1, len);
                }
                else if(dyn_cast<BitCastInst>(v)) { 
                    objClass = objValue.substr(start + 1); 
                }
                else if(dyn_cast<GetElementPtrInst>(v)) {
                    size_t len = objValue.find(", ") - (start + 1);
                    objClass = objValue.substr(start + 1, len);
                }
            }
            else{
                exit(1);
            }
            return objClass;
        }
        

        void PrintRes(){

            errs() << "The pointer flow graph is: \n\n";
            for(auto it =  PFG.begin(); it != PFG.end(); it++){
                Value* s = it->first;
            
                errs() << "The source value: " << *s << "\n";
                errs() << "The target value: ";
                for(auto itt = it->second.begin(); itt != it->second.end(); itt++){
                    Value* t = *itt;
                    errs() << *t << "; ";
                }
                errs() << "\n";
            }
            errs() << "\n\n\n";

            errs() << "The object of every value is: \n\n";
            for(auto it =  PT.begin(); it != PT.end(); it++){
                Value* s = it->first;
            
                errs() << "The value: " << *s << "\n";
                errs() << "The object: ";
                for(auto itt = it->second.begin(); itt != it->second.end(); itt++){
                    int o = *itt;
                    errs() << "o_" << o << "; ";
                }
                errs() << "\n";
            }
            errs() << "\n\n\n";


        }

        bool runOnModule(Module &M) override{
            
            
            Function *F = M.getFunction("main");
            if (!F) {
                return false;
            }
            AddReachable(F);
            while(!WL.empty()){
                pair<Value*, vector<int>> n_pts = WL.front();
                Value* n = n_pts.first;
                vector<int> pts = n_pts.second;
                WL.pop();

                vector<int> delta;

                //delta = pts - pt[n]
                if(PT.find(n) == PT.end()){
                    vector<int> tmp;
                    PT.insert(make_pair(n, tmp));
                }

                //pick out the pts element, if it is not the member of pt[n], add it to delta
                for(auto it = pts.begin(); it != pts.end(); it++){
                    if(find(PT[n].begin(), PT[n].end(), *it) == PT[n].end()){
                        delta.push_back(*it);
                    }
                }
                Propagate(n, delta);

                if(dyn_cast<AllocaInst>(n)){
                    for(auto it = delta.begin(); it != delta.end(); it++){
                        int o_index = *it;
                        Value* o_i;
                        for(auto itt = newObj.begin(); itt != newObj.end(); itt++){
                            if(itt->second == o_index){
                                o_i = itt->first;
                                break;
                            }
                        }
                        
                        // errs() << "value oi is " << *o_i << "\n";
                        // errs() << getClassOfV(o_i) << "\n";

                        for(Function* F: RM){
                            for (Function::iterator bb = F->begin(); bb != F->end(); ++bb){
                                for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst){
                                    if(dyn_cast<StoreInst>(inst)){
                                        StoreInst *ins = dyn_cast<StoreInst>(inst);
                                        Value *leftV = ins->getOperand(1);
                                        Value *rightV = ins->getOperand(0);
                                        Value *field;

                                        if(dyn_cast<AllocaInst>(rightV)){
                                            field = leftV;
                                        }
                                        else if(dyn_cast<AllocaInst>(leftV)){
                                            field = rightV;
                                        }

                                        Instruction *fieldIns = dyn_cast<Instruction>(field);
                                        string fieldStr = getStrOfV(field);
                                        if(fieldIns->getNumOperands() == 3) {
                                            string fieldObjClass = getClassOfV(field);
                                            string oiClass = getClassOfV(o_i);

                                            if(oiClass == fieldObjClass) {
                                                AddEdge(rightV, leftV); 
                                            }
                                        }

                                    }
                                }
                            }
                        }
                        ProcessCall(M, n_pts.first, o_i);
                    }
                }
            }
            PrintRes();
            return false;
        }
    };
}


char PtrA::ID = 0;
static RegisterPass<PtrA> X("PtrA","Pointer Analysis", false, false); 
