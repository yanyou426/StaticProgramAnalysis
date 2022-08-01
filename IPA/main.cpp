#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <fstream>
#include <vector>
#include <list>
#include <utility>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include "llvm/Support/SourceMgr.h"
using namespace llvm;
using namespace std;

class InstructionDataSet
{
public:
	Function *f;
	BasicBlock *bb;
	Instruction *inst;

	InstructionDataSet(Function *f, BasicBlock *bb, Instruction *inst):f(f), bb(bb), inst(inst){}
};
	

/*

	Ignore function List: 
	 - malloc/free
	 - llvm.dbg.declare
*/




//typedef std::vector<std::pair<const BasicBlock*, const Value*>> BBtoValue_vector;
typedef std::vector<std::pair<const BasicBlock*, const BasicBlock*>> BBtoBB_vector;

void graph_module_start(Module&);
void graph_module_end();
void graph_function_start(Function&);
void graph_function_end();
void graph_basicblock_start(BasicBlock&);
void graph_basicblock_end();

static std::string digraph("digraph");
static std::string curly_bracket_left("{");
static std::string curly_bracket_right("}");
static std::string label("label=");
static std::string shape("shape=");
static std::string color("color=lightgrey");
static std::string subgraph("subgraph");
static std::string cluster("cluster_");
static std::string symbol(":s");
static std::string point(" -> ");
static std::string semicolon(";");
static std::string NewLine("\\l");

static std::string _graph_name("\"Call Graph\"");
static std::string _bb_shape("[shape=record, label=");
static std::string Node("Node");
static std::string TF_symbol("|{<s0>T|<s1>F}");

static int BasicBlockNumber = 0;

//void graph_function_call(BBtoValue_vector& bb_function_call_list){
void graph_function_call(BBtoBB_vector& bb_function_call_list){

	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	for(auto iter = bb_function_call_list.begin(); \
			iter != bb_function_call_list.end(); iter++){

		fout << "\t";
		fout << Node;
		fout << (*iter).first << point << Node << (*iter).second << std::endl;

	}
	fout.close();
}

void graph_module_start(Module& m){

	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::trunc);
	fout << digraph << " " << curly_bracket_left << std::endl << std::endl;
	fout << "\t" << label << _graph_name << semicolon << std::endl;
	fout.close();
}

void graph_module_end(){

	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	fout << curly_bracket_right;
	fout.close();
}

void graph_function_start(Function& f){

	static unsigned int cluster_number = 0;
	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	fout << "\t" << subgraph;
	fout << " " << cluster + std::to_string(cluster_number);
	fout << " " << curly_bracket_left << std::endl;
	fout << "\t\t" << color << semicolon << std::endl;
	fout << "\t\t" << label << "\""<< f.getName().str() << "\"" << semicolon; 
	fout << std::endl;
	fout.close();
	
	// Reset BasicBlockNumber //
	BasicBlockNumber = 0;
	cluster_number++;
}

void graph_function_end()
{
	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	fout << "\t" << curly_bracket_right << std::endl;
	fout.close();
}

void graph_basicblock_start(BasicBlock& bb)
{
	Function *func = bb.getParent();	

	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	fout << "\t\t";
	fout << Node;
	fout << &bb << " " << _bb_shape << "\"" << curly_bracket_left << std::endl;
	fout << "\t\t\t\t[" << func->getName().str();
	fout << "%" <<  std::to_string(BasicBlockNumber++) << "]" << NewLine << std::endl;
	fout.close();
}

void graph_basicblock_print(BasicBlock& bb){


	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);

	Instruction *br_inst;

	for (auto iter3 = bb.begin(); iter3 != bb.end(); iter3++) {
		Instruction &inst = *iter3;
		br_inst = &inst;
					
		if(!strcmp(inst.getOpcodeName(), "call"))
		{
			Function* callee_func = reinterpret_cast<Function* >(inst.getOperand(inst.getNumOperands() - 1));
			if(callee_func->getName().str() == "llvm.dbg.declare")
			{
				continue;
			}	
		}		
		//fout << "\t\t\t\t[" << inst.getType()->getTypeID() << "] ";
		//fout << &inst << ": " << inst.getOpcodeName() << " ";
		
		fout << "\t\t\t\t";
		fout << inst.getOpcodeName() << " ";

		int opnt_cnt = inst.getNumOperands();

		for(int i = 0; i < opnt_cnt; i++)
		{
			Value *opnd = inst.getOperand(i);
			std::string o;
		
			if(opnd->hasName()) {
				o = opnd->getName();
				fout << "%" << o;
				if(i < opnt_cnt-1)
					fout <<", ";
			} else {
				fout << "%" << opnd;
				if(i < opnt_cnt-1)
					fout <<", ";
			}
		}
		fout << NewLine <<std::endl;

	}
	if(br_inst->getNumOperands() == 3){
		fout << "\t\t\t\t" << TF_symbol << std::endl;
	}

	fout.close();

}


void graph_basicblock_link(BasicBlock& bb, Instruction& inst){

	unsigned int opnd_cnt = inst.getNumOperands();

	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);

	unsigned int i;
	unsigned int symbol_num;

	if(opnd_cnt == 1){
		
		fout << "\t\t";
		fout << Node;
		fout << &bb;
		fout << point;
		fout << Node;
		fout << inst.getOperand(0)<< semicolon << std::endl;
	}

	else if(opnd_cnt == 3){

		for(symbol_num = 0, i = opnd_cnt-1; i > 0; symbol_num++, i--){

			fout << "\t\t";
			fout << Node;
			fout << &bb << symbol << std::to_string(symbol_num);
			fout << point;
			fout << Node;
			fout << inst.getOperand(i)<< semicolon <<std::endl;
		}

	} else{

		//std::error();
	}

	fout.close();
}

void graph_basicblock_end()
{
	std::ofstream fout;
	fout.open("graph.dot", std::ofstream::out | std::ofstream::app);
	fout << "\t\t\t" << "}\"]" << semicolon << std::endl;
	fout.close();
}



bool splitBBModuleOnce(std::unique_ptr<Module> &m){

	std::list<InstructionDataSet *> ToSplitBBList;
	InstructionDataSet *data;
  
	for (auto iter1 = m->getFunctionList().begin(); 
		iter1 != m->getFunctionList().end(); iter1++) 
	{

		Function &f = *iter1;
		for (auto iter2 = f.getBasicBlockList().begin(); 
			iter2 != f.getBasicBlockList().end(); iter2++) 
		{

			BasicBlock &bb = *iter2;
			Instruction *pastInst = reinterpret_cast<Instruction *>(0);
			for (auto iter3 = bb.begin(); iter3 != bb.end(); iter3++) 
			{

				Instruction &inst = *iter3;
				/* store split point */
				if(pastInst != reinterpret_cast<Instruction *>(0)){
					/* Ignore if the next instruction is "br" */
					if((!strcmp(pastInst->getOpcodeName(), "call")) && strcmp(inst.getOpcodeName(), "br")){
						data = new InstructionDataSet(&f, &bb, &inst);
						ToSplitBBList.push_back(data);
						break;
					}
				}
				pastInst = &inst;
      		}
    	}
  	}


	BasicBlock *tmp;
	std::list<InstructionDataSet *>::iterator iter;
	/* split by using splitBasicBlock function */
	for(iter = ToSplitBBList.begin(); iter != ToSplitBBList.end(); iter++){
		tmp = (*iter)->bb->splitBasicBlock((*iter)->inst);
	}

	return true;
}

int splitBBModuleChecker(std::unique_ptr<Module> &m){


	int CheckSum = 0;
	int InstCall_flag = 0;

	std::list<InstructionDataSet *> ToSplitBBList;
  
	for (auto iter1 = m->getFunctionList().begin(); 
			iter1 != m->getFunctionList().end(); iter1++) {

		Function &f = *iter1;
		for (auto iter2 = f.getBasicBlockList().begin(); 
				iter2 != f.getBasicBlockList().end(); iter2++) 
		{

			InstCall_flag = 0;
			
			BasicBlock &bb = *iter2;
			for (auto iter3 = bb.begin(); iter3 != bb.end(); iter3++) 
			{
				
				Instruction &inst = *iter3;
				/* Increase CheckSum per Call Inst */
				if(!strcmp(inst.getOpcodeName(), "call")){
					CheckSum++;
					InstCall_flag = 1;
				}
      			}
			/* Decrease CheckSum per BB if BB has a Call Inst */
			if(InstCall_flag == 1)
			{
				CheckSum--;
			}
    		}
		
		
  	}

	return CheckSum;
}

void modulePreprocess(std::unique_ptr<Module>& m)
{

	while(splitBBModuleChecker(m) != 0){
		splitBBModuleOnce(m);
	}
	splitBBModuleOnce(m);

}



int main(int argc, char *argv[]){

	if (argc != 2) {
   		std::cerr << "Usage: " << argv[0] << "bitcode_filename" << std::endl;
    	return 1;
	}

	StringRef filename = argv[1];
	LLVMContext context;
	SMDiagnostic error;
	std::unique_ptr<Module> m = parseIRFile(filename, error, context);

	modulePreprocess(m);

	#ifdef DEBUG_FLAG
	std::cout << " Successfully read Module:" << std::endl;
	std::cout << " Name: " << m->getName().str() << std::endl;
	std::cout << " Target triple: " << m->getTargetTriple() << std::endl;
	#endif  	

 	unsigned int i = 0;
 	unsigned int opnt_cnt = 0;

	graph_module_start(*m);


	//BBtoValue_vector bb_function_call_list;
	BBtoBB_vector bb_function_call_list;

	// Print all bitcode in Module
	// m->print(*os, NULL);

  	for (auto iter1 = m->getFunctionList().begin(); iter1 != m->getFunctionList().end(); iter1++) {
    	Function &f = *iter1;

		graph_function_start(f);
	#ifdef DEBUG_FLAG
		std::cout << " Function: " << f.getName().str() << std::endl;
		std::cout << " Function EntyBlackBlock: : " << &(f.getEntryBlock()) << std::endl;	
	#endif
		// Print all bitcode in Function
		// f.print(*os, NULL);

		for (auto iter2 = f.getBasicBlockList().begin(); iter2 != f.getBasicBlockList().end(); iter2++) {
      		BasicBlock &bb = *iter2;
			// Print all bitcode in BasicBlock

			graph_basicblock_start(bb);
			graph_basicblock_print(bb);

			//bb.print(*os, false);
	#ifdef DEBUG_FLAG
	  		std::cout << "  BasicBlock: " << &bb << std::endl;
	  		std::cout << "  BasicBlockName: " << bb.getName().str() << std::endl;
	  		std::cout << "  BasicBlock Parent: " << bb.getParent()->getName().str() << std::endl;
	  		std::cout << "  BasicBlock Module: " << bb.getModule()->getName().str() << std::endl;
	#endif
			Instruction* br_inst;
		  	for (auto iter3 = bb.begin(); iter3 != bb.end(); iter3++) {
  		      	Instruction &inst = *iter3;
				br_inst = &inst;
	#ifdef DEBUG_FLAG
   		     	std::cout << "   #Instruction " << &inst << std::endl;
   		     	std::cout << "   #Instruction Type.TypeID " << inst.getType()->getTypeID() << std::endl; 
				std::cout << "   #Instruction OpCode: " << inst.getOpcode() << std::endl;
				std::cout << "   #Instruction OpName: " << inst.getOpcodeName() << std::endl;
				std::cout << "   #Instruction Operand Num: " << inst.getNumOperands() << std::endl;
	#endif
       		 	opnt_cnt = inst.getNumOperands();

			for(i = 0; i < opnt_cnt; i++)
       	 		{
          			Value *opnd = inst.getOperand(i);
          			std::string o;
          		
					raw_string_ostream os(o);
					//std::cout << "      opnd Value print: " << opnd << std::endl;
					//std::cout << "      opnd Type print: " << opnd->getType() <<std::endl;
	#ifdef DEBUG_FLAG
					std::cout << "      opnd Type.ValueID print: " << opnd->getValueID() <<std::endl;
					std::cout << "      opnd Type.TypeID: " << opnd->getType()->getTypeID() << std::endl;
	#endif
					opnd->getType()->print(os, false, false);
	#ifdef DEBUG_FLAG
					std::cout << "      opnd type Print: " << std::endl;
	#endif						
					if(opnd->hasName()) {
            			o = opnd->getName();
	#ifdef DEBUG_FLAG
            			std::cout << "          [Name]" << o << std::endl;
        #endif
	  			} else {
	#ifdef DEBUG_FLAG
            			std::cout << "          [ptr ]" << opnd << std::endl;
        #endif
	 			}
        		}

				if(!strcmp(inst.getOpcodeName(), "call")){


					Function* callee_func = reinterpret_cast<Function* >(inst.getOperand(inst.getNumOperands() - 1));
					if(callee_func->getName().str() == "malloc"
							|| callee_func->getName().str() == "free"
							|| callee_func->getName().str() == "llvm.dbg.declare"
						)
					{
						continue;
					}


					bb_function_call_list.push_back(std::make_pair(&bb, \
								&(reinterpret_cast<Function* >(inst.getOperand(inst.getNumOperands() - 1)))->getEntryBlock()));
	#ifdef DEBUG_FLAG
					std::cout << "call!\n";
	#endif
				}
				
	#ifdef DEBUG_FLAG
        		std:: cout << std::endl;
	#endif
      		}
			
			graph_basicblock_end();

			if(!strcmp(br_inst->getOpcodeName(), "br")){
					
				graph_basicblock_link(bb, *br_inst);
	#ifdef DEBUG_FLAG
				std::cout << "branch!\n";
	#endif
			}

    	}
		graph_function_end();
  	}
	graph_function_call(bb_function_call_list);
	graph_module_end();
 
	system("dot -Tpng graph.dot > graph.png");

	return 0;
}

