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
