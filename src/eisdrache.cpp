/**
 * @file eisdrache.cpp
 * @author fuechs
 * @brief Eisdrache class implementation
 * @version 0.1
 * @date 2023-01-30
 * 
 * @copyright Copyright (c) 2023, Fuechs.
 * 
 */

#include "eisdrache.hpp"

namespace llvm {

WrappedVal::WrappedVal(Kind kind, Type *type, Value *value, BasicBlock *parent) 
: kind(kind), type(type), value(value), future(nullptr), parent(parent) {}

WrappedVal &WrappedVal::operator=(const WrappedVal &copy) {
    kind = copy.kind;
    type = copy.type;
    value = copy.value;
    future = copy.future;
    parent = copy.parent;
    return *this;
}

WrappedType::WrappedType(StructType *structType, std::vector<Type *> elementTypes) 
: type(structType), elementTypes(elementTypes) {}

WrappedType &WrappedType::operator=(const WrappedType &copy) {
    type = copy.type;
    elementTypes = copy.elementTypes;
    return *this;
}

Type *WrappedType::operator[](int64_t index) {
    if (index < 0) 
        return type;
    return elementTypes[index];
}

/// CREATE ///

Eisdrache::Eisdrache(LLVMContext *context, Module *module, IRBuilder<> *builder, std::string targetTriple) {
    this->context = context;
    this->module = module;
    this->builder = builder;

    TargetOptions targetOptions = TargetOptions();
    targetOptions.FloatABIType = FloatABI::Hard;
    TargetMachine *targetMachine = nullptr;

    if (targetTriple.empty()) {
        EngineBuilder engineBuilder = EngineBuilder();
        engineBuilder.setTargetOptions(targetOptions);
        targetMachine = engineBuilder.selectTarget();
    } else {
        std::string error;
        const Target *target = TargetRegistry::lookupTarget(targetTriple, error);
        if (!target) {
            std::cerr << error << "\n";
            assert(false && "TargetRegistry::lookupTarget() failed");
        }
        targetMachine = target->createTargetMachine(targetTriple, "generic", "", targetOptions, None);
    }

    module->setTargetTriple(targetMachine->getTargetTriple().str());
    module->setDataLayout(targetMachine->createDataLayout());
}

Eisdrache::~Eisdrache() {
    delete builder;
    memoryFunctions.clear();
}

void Eisdrache::init() {
    PassRegistry *registry = PassRegistry::getPassRegistry();
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeNativeTarget();
    initializeTarget(*registry);
}

Eisdrache *Eisdrache::create(std::string moduleID, std::string targetTriple) {
    LLVMContext *context = new LLVMContext();
    return new Eisdrache(context, new Module(moduleID, *context), new IRBuilder<>(*context), targetTriple);
}

Eisdrache *Eisdrache::create(LLVMContext *context, std::string moduleID, std::string targetTriple) {
    return new Eisdrache(context, new Module(moduleID, *context), new IRBuilder<>(*context), targetTriple);
}

Eisdrache *Eisdrache::create(Module *module, std::string targetTriple) {
    return new Eisdrache(&module->getContext(), module, new IRBuilder<>(module->getContext()), targetTriple);
}

Eisdrache *Eisdrache::create(Module *module, IRBuilder<> *builder, std::string targetTriple) {
    return new Eisdrache(&module->getContext(), module, builder, targetTriple);
};

/// DEBUG ///

void Eisdrache::dump(raw_fd_ostream &os) { module->print(os, nullptr); }

/// GETTER ///

LLVMContext *Eisdrache::getContext() { return context; }
Module *Eisdrache::getModule() { return module; }
IRBuilder<> *Eisdrache::getBuilder() { return builder; }

Type *Eisdrache::getVoidTy() { return builder->getVoidTy(); }
Type *Eisdrache::getBoolTy() { return builder->getInt1Ty(); }
IntegerType *Eisdrache::getSizeTy() { return getIntTy(64); }
IntegerType *Eisdrache::getIntTy(size_t bit) { return Type::getIntNTy(*context, bit); }
PointerType *Eisdrache::getIntPtrTy(size_t bit) { return Type::getIntNPtrTy(*context, bit); }
PointerType *Eisdrache::getIntPtrPtrTy(size_t bit) { return getIntPtrTy(bit)->getPointerTo(); }
Type *Eisdrache::getFloatTy(size_t bit) {
    switch (bit) {
        case 16:    return builder->getHalfTy();
        case 32:    return builder->getFloatTy();
        case 64:    return builder->getDoubleTy();
        default:    assert(false && "invalid amount of bits");
    }
}

ConstantInt *Eisdrache::getBool(bool value) { return builder->getInt1(value); }
ConstantInt *Eisdrache::getInt(IntegerType *type, size_t value) { return ConstantInt::get(type, value); }
ConstantInt *Eisdrache::getInt(size_t bit, size_t value) { return ConstantInt::get(getIntTy(bit), value); }
ConstantFP *Eisdrache::getFloat(double value) { return ConstantFP::get(*context, APFloat(value)); }
ConstantPointerNull *Eisdrache::getNullPtr(PointerType *type) { return ConstantPointerNull::get(type); }

/// BUILDER ///

Value *Eisdrache::allocate(Type *type, std::string name) { 
    Value *allocated = builder->CreateAlloca(type, nullptr, name);
    values[allocated->getName().str()] = WrappedVal(WrappedVal::LOCAL, type, allocated);
    return allocated;
}

Value *Eisdrache::call(Function *function, std::vector<Value *> args, std::string name) { return builder->CreateCall(function, args, name); };

Function *Eisdrache::declare(Type *type, std::vector<Type *> parameters, std::string name, bool entry) {
    FunctionType *FT = FunctionType::get(type, parameters, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, name, *module);
    if (entry) {
        BasicBlock *BB = BasicBlock::Create(*context, "entry", F);
        builder->SetInsertPoint(BB);
        for (Argument &arg : F->args()) // FIXME: parameters can not found in values
            values[arg.getName().str()] = WrappedVal(WrappedVal::PARAMETER, arg.getType(), &arg, BB);
    } else
        llvm::verifyFunction(*F);
    return F;
}

ReturnInst *Eisdrache::createRet(BasicBlock *next) {
    ReturnInst *inst = builder->CreateRetVoid();

    for (WrappedVal::Map::value_type &x : values) 
        if (x.second.parent 
        && x.second.parent->getParent() == inst->getParent()->getParent())
            values.erase(x.first);
    
    if (next)
        builder->SetInsertPoint(next);
    
    return inst;
} 

ReturnInst *Eisdrache::createRet(Value *value, BasicBlock *next) {
    ReturnInst *inst = builder->CreateRet(value);
    
    for (WrappedVal::Map::value_type &x : values) 
        if (x.second.parent 
        && x.second.parent->getParent() == inst->getParent()->getParent())
            values.erase(x.first);
    
    if (next)
        builder->SetInsertPoint(next);
    
    return inst;
}

StructType *Eisdrache::createType(std::vector<Type *> elements, std::string name) { 
    StructType *that = StructType::create(*context, elements, name);
    types[that->getName().str()] = WrappedType(that, elements);
    return that;
}

Value *Eisdrache::getElementPtr(Value *ptr, size_t index, std::string name) {
    Type *structType = getWrap(ptr).type; 
    WrappedType &wrap = getWrap(structType);
    Value *elementPtr = builder->CreateGEP(structType, ptr, {getInt(64, 0), getInt(32, index)}, name);
    values[elementPtr->getName().str()] = WrappedVal(WrappedVal::LOADED, wrap[index], elementPtr);
    return elementPtr;
} 

Value *Eisdrache::getElementVal(Value *ptr, size_t index, std::string name) {
    Value *elementPtr = getElementPtr(ptr, index, name+"_ptr_");
    Value *elementVal = loadValue(elementPtr, name, true);
    values[elementVal->getName().str()] = WrappedVal(WrappedVal::LOADED, elementVal->getType(), elementVal);
    return elementVal;
}

void Eisdrache::store(Value *value, Value *structPtr, size_t index) {
    Value *elementPtr = getElementPtr(structPtr, index, "unnamed_gep_");
    store(value, elementPtr);
}

void Eisdrache::store(Value *value, Value *ptr) { builder->CreateStore(value, ptr); }

BranchInst *Eisdrache::jump(BasicBlock *block) { return builder->CreateBr(block); }

BranchInst *Eisdrache::condJump(Value *condition, BasicBlock *then, BasicBlock *Else) { return builder->CreateCondBr(condition, then, Else); }

BasicBlock *Eisdrache::block(bool insert, std::string name) {
    BasicBlock *BB = BasicBlock::Create(*context, name, builder->GetInsertBlock()->getParent());
    if (insert)
        builder->SetInsertPoint(BB);
    return BB;
}

Value *Eisdrache::binaryOp(BinaryOp op, Value *LHS, Value *RHS, std::string name) {
    Value *lLoad = loadValue(LHS, name+"_lhs_load_");
    Value *rLoad = loadValue(RHS, name+"_rhs_load_");

    if (lLoad->getType() != rLoad->getType())
        assert(false && "operands have different types");

    size_t llvmOp;
    Type *retType = lLoad->getType();
    bool comparison = false;
    CmpInst::Predicate pred;

    switch (op) {
        case ADD: 
            if (retType->isFloatingPointTy()) llvmOp = Instruction::FAdd;
            else llvmOp = Instruction::Add;
            break;
        case SUB: 
            if (retType->isFloatingPointTy()) llvmOp = Instruction::FSub;
            else llvmOp = Instruction::Sub; 
            break;
        case MUL: 
            if (retType->isFloatingPointTy()) llvmOp = Instruction::FMul;
            else llvmOp = Instruction::Mul;
            break;
        case DIV: 
            if (retType->isIntegerTy()) retType = getFloatTy(64);
            llvmOp = Instruction::FDiv; 
            break;        
        case XOR:
            if (!retType->isIntegerTy()) assert(false && "invalid operands for xor");
            llvmOp = Instruction::Xor;
            break;
        case OR:
            if (!retType->isIntegerTy()) assert(false && "invalid operands for or");
            llvmOp = Instruction::Or;
            break;
        case AND:
            if (!retType->isIntegerTy()) assert(false && "invalid operands for and");
            llvmOp = Instruction::And;
            break;
        case LSH:
            if (!retType->isIntegerTy()) assert(false && "invalid operands for left shift");
            llvmOp = Instruction::Shl;
            break;
        case RSH:
            if (!retType->isIntegerTy()) assert(false && "invalid operands for right shift");
            llvmOp = Instruction::LShr;
            break;
        case EQU:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_EQ;
            else pred = CmpInst::FCMP_OEQ;
            break;
        case NEQ:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_NE;
            else pred = CmpInst::FCMP_ONE;
            break;
        case GT:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_UGT;
            else pred = CmpInst::FCMP_OGT; 
            break;
        case GTE:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_UGE;
            else pred = CmpInst::FCMP_OGE; 
            break;
        case LT:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_ULT;
            else pred = CmpInst::FCMP_OLT;
            break;
        case LTE:
            comparison = true;
            if (retType->isIntegerTy()) pred = CmpInst::ICMP_ULE;
            else pred = CmpInst::FCMP_OLE;
            break;
        default:    assert(false && "binary operation not implemented");
    }
    
    Value *binOp;
    if (comparison) 
        binOp = builder->CreateCmp(pred, LHS, RHS, name);
    else 
        binOp = builder->CreateBinOp((Instruction::BinaryOps) llvmOp, LHS, RHS, name);
    values[binOp->getName().str()] = WrappedVal(WrappedVal::LOADED, retType, binOp, builder->GetInsertBlock());
    return binOp;
}

Value *Eisdrache::convert(Type *type, Value *value, std::string name) {
    value = loadValue(value, name+"_cast_value_load_");
    Type *from = value->getType();
    Instruction::CastOps op;

    if (from == type) return value;

    if (from->isIntegerTy()) {      
        if (type->isIntegerTy()) {                                              // INTEGER TO INTEGER
            if (from->getIntegerBitWidth() > type->getIntegerBitWidth()) 
                op = Instruction::Trunc;
            else 
                op = Instruction::ZExt;
        } else if (type->isPointerTy())                                         // INTEGER TO POINTER
            op = Instruction::IntToPtr;
        else if (type->isFloatingPointTy())                                     // INTEGER TO FLOAT
            op = Instruction::UIToFP;
        else
            assert(false && "cast not implemented");
    } else if (from->isPointerTy()) {                     
        if (type->isPointerTy())                                                // POINTER TO POINTER
            op = Instruction::BitCast;
        else if (type->isIntegerTy())                                           // POINTER TO INTEGER
            op = Instruction::PtrToInt;
        else
            assert(false && "cast not implemented");
    } else if (from->isFloatingPointTy()) { 
        if (type->isFloatingPointTy()) {                                        // FLOAT TO FLOAT
            if (from->getFPMantissaWidth() > type->getFPMantissaWidth())
                op = Instruction::FPTrunc;
            else
                op = Instruction::FPExt;
        } else if (type->isIntegerTy())                                         // FLOAT TO INTEGER
            op = Instruction::FPToUI;
        else
            assert(false && "cast not implemented");
    } else 
        assert(false && "cast not implemented");

    Value *cast = builder->CreateCast(op, value, type, name);
    values[cast->getName().str()] = WrappedVal(WrappedVal::LOADED, type, cast, builder->GetInsertBlock());
    return cast;
}

WrappedVal &Eisdrache::getWrap(Value *pointer) {
    for (WrappedVal::Map::value_type &x : values) 
        if (x.second.value == pointer)
            return x.second;
    assert(false && "value not found in WrappedVal::Map Eisdrache::values");
}

WrappedType &Eisdrache::getWrap(Type *type) {
    if (!type->isStructTy())
        assert(false && "type is not a StructType *");

    for (WrappedType::Map::value_type &x : types) 
        if ((Type *) x.second.type == type)
            return x.second;
    
    assert(false && "struct type not found in WrappedType::Map Eisdrache::types");
}

Value *Eisdrache::loadValue(Value *pointer, std::string name, bool force) {
    if (isConstant(pointer) | isArgument(pointer)) 
        return pointer;

    WrappedVal &that = getWrap(pointer);
    
    if (that.future) {
        builder->CreateStore(that.future, that.value);
        that.future = nullptr;
    }
    
    if (force || that.kind == WrappedVal::LOCAL) {
        Value *loaded = builder->CreateLoad(that.type, pointer, name.empty() ? pointer->getName()+"_load_" : name);
        values[loaded->getName().str()] = WrappedVal(WrappedVal::LOADED, that.type, loaded, builder->GetInsertBlock());
        return loaded;
    }
    
    return pointer;
}

void Eisdrache::setFuture(Value *local, Value *value) { getWrap(local).future = value; }

bool Eisdrache::isConstant(Value *value) { return isa<Constant>(value); }

bool Eisdrache::isArgument(Value *value) { return isa<Argument>(value); }

Value *Eisdrache::malloc(Type *type, Value *size, std::string name) { 
    return call(memoryFunctions.at(type->getPointerTo())["malloc"], {size}, name); 
}

void Eisdrache::free(Type *type, Value *value) { 
    call(memoryFunctions.at(type->getPointerTo())["free"], {value}); 
}

Value *Eisdrache::memcpy(Type *type, Value *dest, Value *source, Value *size, std::string name) { 
    return call(memoryFunctions.at(type->getPointerTo())["memcpy"], {dest, source, size}, name); 
}

void Eisdrache::createMemoryFunctions(Type *type) {
    PointerType *ptrType = type->getPointerTo();
    if (memoryFunctions.contains(ptrType))
        return;
    memoryFunctions[type]["malloc"] = declare(ptrType, {getSizeTy()}, "malloc");
    memoryFunctions[type]["free"] = declare(getVoidTy(), {ptrType}, "free");
    memoryFunctions[type]["memcpy"] = declare(ptrType, {ptrType, ptrType, getSizeTy()}, "memcpy");
}

}