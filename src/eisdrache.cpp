/**
 * @file eisdrache.cpp
 * @author fuechs
 * @brief Eisdrache class implementation
 * @version 0.3.2
 * @date 2023-10-01
 * 
 * @copyright Copyright (c) 2023-2024, Fuechs.
 * 
 */

#include "eisdrache.hpp"


namespace llvm {

/// ENTITY ///

Eisdrache::Entity::Entity(Eisdrache::Ptr eisdrache) : eisdrache(eisdrache) {}

Eisdrache::Entity::~Entity() {}

/// EISDRACHE TY ///

Eisdrache::Ty::Ty(Eisdrache::Ptr eisdrache) { this->eisdrache = eisdrache; }

Eisdrache::Ty::Ptr Eisdrache::Ty::create(Eisdrache::Ptr eisdrache, Type *llvmTy) {
    Ty::Ptr that = nullptr;
    switch (llvmTy->getTypeID()) {
        case Type::IntegerTyID:
            that = std::make_shared<IntTy>(eisdrache, llvmTy->getIntegerBitWidth());
            break;
        case Type::HalfTyID:    
            that = std::make_shared<FloatTy>(eisdrache, 16); 
            break;
        case Type::FloatTyID:   
            that = std::make_shared<FloatTy>(eisdrache, 32); 
            break;
        case Type::DoubleTyID:  
            that = std::make_shared<FloatTy>(eisdrache, 64); 
            break;
        case Type::PointerTyID: 
            that = std::make_shared<PtrTy>(eisdrache, eisdrache->getVoidTy()); 
            break;
        case Type::StructTyID:
            Eisdrache::complain("Eisdrache::Ty::Ty(): Can not construct Eisdrache::Ty from a llvm::Type with ID: Type::StructTyID.");
        case Type::FunctionTyID:
            Eisdrache::complain("Eisdrache::Ty::Ty(): Can not construct Eisdrache::Ty from a llvm::Type with ID: Type::FunctionTyID.");
        case Type::ArrayTyID:
            Eisdrache::complain("Eisdrache::Ty::Ty(): Can not construct Eisdrache::Ty from a llvm::Type with ID: Type::ArrayTyID.");
        default:
            break;
    }

    return eisdrache->addTy(that);
}

Eisdrache::Ty::Ptr Eisdrache::Ty::getPtrTo() { 
    return eisdrache->addTy(std::make_shared<PtrTy>(eisdrache,  shared_from_this())); 
}

size_t Eisdrache::Ty::getBit() const { return 0; }

constexpr bool Eisdrache::Ty::isPtrTy() const { return kind() == PTR; }

constexpr bool Eisdrache::Ty::isIntTy() const { return kind() == INT; }

constexpr bool Eisdrache::Ty::isFloatTy() const { return kind() == FLOAT; }

constexpr bool Eisdrache::Ty::isSignedTy() { 
    return kind() == FLOAT || (kind() == INT && dynamic_cast<IntTy *>(this)->getSigned()); 
}

/// ALIAS TY ///

Eisdrache::AliasTy::AliasTy(Eisdrache::Ptr eisdrache, std::string alias, Ty::Ptr type) 
: alias(alias), type(type) { this->eisdrache = eisdrache; }

Eisdrache::AliasTy::~AliasTy() { alias.clear(); }

size_t Eisdrache::AliasTy::getBit() const { return type->getBit(); }

Type *Eisdrache::AliasTy::getTy() const { return type->getTy(); }
 
bool Eisdrache::AliasTy::isValidRHS(Ty::Ptr comp) const { return type->isValidRHS(comp); }

bool Eisdrache::AliasTy::isEqual(Ty::Ptr comp) const { return type->isEqual(comp); }

Eisdrache::AliasTy::Kind Eisdrache::AliasTy::kind() const { return ALIAS; }

/// VOID TY ///

Eisdrache::VoidTy::VoidTy(Eisdrache::Ptr eisdrache) { this->eisdrache = eisdrache; } 

Type *Eisdrache::VoidTy::getTy() const { return Type::getVoidTy(*eisdrache->getContext()); }

bool Eisdrache::VoidTy::isValidRHS(const Ty::Ptr comp) const { return dynamic_cast<VoidTy *>(comp.get()); }

bool Eisdrache::VoidTy::isEqual(const Ty::Ptr comp) const { return dynamic_cast<VoidTy *>(comp.get()); }

Eisdrache::VoidTy::Kind Eisdrache::VoidTy::kind() const { return VOID; }

/// POINTER TY /// 

Eisdrache::PtrTy::PtrTy(Eisdrache::Ptr eisdrache, Ty::Ptr pointee) {
    this->eisdrache = eisdrache;
    this->pointee = pointee;
}

Eisdrache::Ty::Ptr &Eisdrache::PtrTy::getPointeeTy() { return pointee; }

size_t Eisdrache::PtrTy::getBit() const { return pointee->getBit(); }

Type *Eisdrache::PtrTy::getTy() const { return PointerType::get(*eisdrache->getContext(), 0); }

bool Eisdrache::PtrTy::isValidRHS(const Ty::Ptr comp) const {
    // there are no valid binary operations for pointers
    return false;
}

bool Eisdrache::PtrTy::isEqual(const Ty::Ptr comp) const {
    return comp->kind() == PTR                                                  // check wether comp is a pointer
        && pointee->isEqual(dynamic_cast<PtrTy *>(comp.get())->getPointeeTy()); // check wether pointee type is the same
}

Eisdrache::PtrTy::Kind Eisdrache::PtrTy::kind() const { return PTR; }

/// INTEGER TY ///

Eisdrache::IntTy::IntTy(Eisdrache::Ptr eisdrache, size_t bit, bool _signed) {
    this->eisdrache = eisdrache;
    this->bit = bit;
    this->_signed = _signed;
}

size_t Eisdrache::IntTy::getBit() const { return bit; }

const bool &Eisdrache::IntTy::getSigned() const { return _signed; }

Eisdrache::Ty::Ptr Eisdrache::IntTy::getSignedTy() const {
    return eisdrache->addTy(std::make_shared<IntTy>(eisdrache, bit, true));
}

Type *Eisdrache::IntTy::getTy() const { return Type::getIntNTy(*eisdrache->getContext(), bit); }

bool Eisdrache::IntTy::isValidRHS(const Ty::Ptr comp) const { return isEqual(comp); }

bool Eisdrache::IntTy::isEqual(const Ty::Ptr comp) const {
    if (comp->kind() != INT)
        return false;

    IntTy *conv = dynamic_cast<IntTy *>(comp.get());
    return bit == conv->bit && _signed == conv->_signed;
}

Eisdrache::IntTy::Kind Eisdrache::IntTy::kind() const { return INT; }

/// FLOAT TY ///

Eisdrache::FloatTy::FloatTy(Eisdrache::Ptr eisdrache, size_t bit) {
    this->eisdrache = eisdrache;
    this->bit = bit;
}

size_t Eisdrache::FloatTy::getBit() const { return bit; }

Type *Eisdrache::FloatTy::getTy() const {
    switch (bit) {
        case 16:    return Type::getHalfTy(*eisdrache->getContext());
        case 32:    return Type::getFloatTy(*eisdrache->getContext());
        case 64:    return Type::getDoubleTy(*eisdrache->getContext());
        case 128:   return Type::getFP128Ty(*eisdrache->getContext());
        default:    return Eisdrache::complain("Eisdrache::FloatTy::getTy(): Invalid amount of bits ("+std::to_string(bit)+").");
    }
}

bool Eisdrache::FloatTy::isValidRHS(const Ty::Ptr comp) const { return isEqual(comp); }

bool Eisdrache::FloatTy::isEqual(const Ty::Ptr comp) const {
    return comp->kind() == FLOAT 
        && bit == dynamic_cast<FloatTy *>(comp.get())->bit;
}

Eisdrache::FloatTy::Kind Eisdrache::FloatTy::kind() const { return FLOAT; }

/// EISDRACHE REFERENCE ///

Eisdrache::Reference::Reference(Eisdrache::Ptr eisdrache, std::string symbol) 
: eisdrache(eisdrache), symbol(symbol) {}

Eisdrache::Reference::~Reference() { symbol.clear(); }

Eisdrache::Reference &Eisdrache::Reference::operator=(const Reference &copy) {
    symbol = copy.symbol;
    eisdrache = copy.eisdrache;
    return *this;
}

const std::string &Eisdrache::Reference::getSymbol() const { return symbol; }

Eisdrache::Entity &Eisdrache::Reference::getEntity() const {
    Entity *ret = eisdrache->getFunc(symbol);

    if (!ret)
        ret = &eisdrache->getCurrentParent()[symbol];

    return *ret;
}

Eisdrache::Entity::Kind Eisdrache::Reference::kind() const { return REFERENCE; }

/// EISDRACHE LOCAL ///

Eisdrache::Local::Local(Eisdrache::Ptr eisdrache, Constant *constant) 
: eisdrache(eisdrache), v_ptr(constant), future(nullptr) {
    type = eisdrache->addTy(Ty::create(eisdrache, constant->getType()));
}

Eisdrache::Local::Local(Eisdrache::Ptr eisdrache, Ty::Ptr type, Value *ptr, Value *future, ValueVec future_args)
: eisdrache(eisdrache), type(type), v_ptr(ptr), future(future), future_args(future_args) {}

Eisdrache::Local& Eisdrache::Local::operator=(const Local &copy) {
    v_ptr = copy.v_ptr;
    type = copy.type;
    future = copy.future;
    eisdrache = copy.eisdrache;
    return *this;
}

bool Eisdrache::Local::operator==(const Local &comp) const { return v_ptr == comp.v_ptr; }

bool Eisdrache::Local::operator==(const Value *comp) const { return v_ptr == comp; }

AllocaInst *Eisdrache::Local::operator*() {
    invokeFuture();
    if (!isAlloca())
        return complain("Eisdrache::Local::operator*(): Tried to get AllocaInst * of Value * (%"+v_ptr->getName().str()+").");
    return a_ptr;
}

void Eisdrache::Local::setPtr(Value *ptr) { v_ptr = ptr; }

void Eisdrache::Local::setFuture(Value *future) { this->future = future; }

void Eisdrache::Local::setFutureArgs(ValueVec args) { future_args = args; }

void Eisdrache::Local::setTy(Ty::Ptr ty) { type = ty; }

AllocaInst *Eisdrache::Local::getAllocaPtr() { return operator*(); }

Value *Eisdrache::Local::getValuePtr() { 
    invokeFuture();
    return v_ptr; 
}

Eisdrache::Ty::Ptr Eisdrache::Local::getTy() { return type; }

std::string Eisdrache::Local::getName() const { 
    if (!v_ptr || !v_ptr->hasName())
        return "unnamed";
    return v_ptr->getName().str();
}

bool Eisdrache::Local::isAlloca() { return dyn_cast<AllocaInst>(v_ptr); }

Eisdrache::Local &Eisdrache::Local::loadValue(bool force, std::string name) {
    if ((!force && !isAlloca()) || !type->isPtrTy())
        return *this;

    if (isAlloca())
        invokeFuture();

    Ty::Ptr loadTy = dynamic_cast<PtrTy *>(type.get())->getPointeeTy();
    LoadInst *load = eisdrache->getBuilder()->CreateLoad(loadTy->getTy(), 
        v_ptr, name.empty() ? v_ptr->getName().str()+"_load" : name);
    return eisdrache->getCurrentParent().addLocal(Local(eisdrache, loadTy, load));
}

void Eisdrache::Local::invokeFuture() {
    if (!future)
        return;
        
    if (Function *func = dyn_cast<Function>(future)) {
        if (func->getReturnType()->isVoidTy()) {
            eisdrache->getBuilder()->CreateCall(func, future_args);
            future = nullptr;
            future_args.clear();
            return;
        } else 
            future = eisdrache->getBuilder()->CreateCall(func, future_args, getName()+"_future");
    } 

    eisdrache->getBuilder()->CreateStore(future, v_ptr);
    future = nullptr;
    future_args.clear();
}

Eisdrache::Entity::Kind Eisdrache::Local::kind() const { return LOCAL; }

/// EISDRACHE FUNC ///

Eisdrache::Func::Func() {
    func = nullptr;
    type = nullptr;
    parameters = Local::Vec();
    locals = Local::Map();
    eisdrache = nullptr;
}

Eisdrache::Func::Func(Eisdrache::Ptr eisdrache, Ty::Ptr type, std::string name, Ty::Map parameters, bool entry) {
    this->eisdrache = eisdrache;
    this->type = type;
    this->locals = Local::Map();
    this->parameters = Local::Vec();

    std::vector<std::string> paramNames;
    std::vector<Type *> paramTypes;
    for (Ty::Map::value_type &param : parameters) {
        paramNames.push_back(param.first);
        paramTypes.push_back(param.second->getTy());
        this->parameters.push_back(Local(eisdrache, param.second));
    }

    FunctionType *FT = FunctionType::get(type->getTy(), paramTypes, false);
    func = Function::Create(FT, Function::ExternalLinkage, name, *eisdrache->getModule());

    for (size_t i = 0; i < func->arg_size(); i++) {  
        func->getArg(i)->setName(paramNames[i]);
        this->parameters[i].setPtr(func->getArg(i));
    }

    if (entry) {
        BasicBlock *entry = BasicBlock::Create(*eisdrache->getContext(), "entry", func);
        eisdrache->setBlock(entry);
    } else
        llvm::verifyFunction(*func);
}

Eisdrache::Func::~Func() { locals.clear(); }

Eisdrache::Func &Eisdrache::Func::operator=(const Func &copy) {
    func = copy.func;
    type = copy.type;
    parameters = copy.parameters;
    locals = copy.locals;
    eisdrache = copy.eisdrache;
    return *this;
}

bool Eisdrache::Func::operator==(const Func &comp) const { return func == comp.func; }

bool Eisdrache::Func::operator==(const Function *comp) const { return func == comp; }

Eisdrache::Local &Eisdrache::Func::operator[](std::string symbol) { 
    if (locals.contains(symbol))
        return locals[symbol];

    for (Local &param : parameters)
        if (param.getName() == symbol)
            return param;
    
    Eisdrache::complain("Eisdrache::Func::operator[]: Symbol not found: %"+symbol+".");
    return locals.begin()->second; // silence warning
}

Function *Eisdrache::Func::operator*() { return func; }

Eisdrache::Local &Eisdrache::Func::arg(size_t index) { return parameters[index]; }

Eisdrache::Local &Eisdrache::Func::call(ValueVec args, std::string name) { 
    Value *ret = eisdrache->getBuilder()->CreateCall(func, args, name); 
    return eisdrache->getCurrentParent().addLocal(Local(eisdrache, type, ret));
}

Eisdrache::Local &Eisdrache::Func::call(Local::Vec args, std::string name) { 
    ValueVec raw_args = {};
    for (Local &local : args)
        raw_args.push_back(local.getValuePtr());
    return this->call(raw_args, name);
}
 
Eisdrache::Local &Eisdrache::Func::addLocal(Local local) { 
    std::string symbol = "";
    if (local.getName() == "unnamed" || locals.contains(local.getName()))
        symbol = local.getName()+std::to_string(locals.size());
    else
        symbol = local.getName();
    locals[symbol] = local;
    return locals[symbol];
}

void Eisdrache::Func::addAttr(Attribute attr, int64_t index) {
    if (index < 0)
        func->addFnAttr(attr);
    else
        func->getArg(index)->addAttr(attr);
}

void Eisdrache::Func::addAttr(Attribute::AttrKind attr, int64_t index) {
    if (index < 0)
        func->addFnAttr(attr);
    else
        func->getArg(index)->addAttr(attr);
}

void Eisdrache::Func::setCallingConv(CallingConv::ID conv) { func->setCallingConv(conv); }

void Eisdrache::Func::setDoesNotThrow() { func->setDoesNotThrow(); }

Eisdrache::Ty::Ptr Eisdrache::Func::getTy() { return type; }

Eisdrache::Entity::Kind Eisdrache::Func::kind() const { return FUNC; }

/// EISDRACHE STRUCT ///

Eisdrache::Struct::Struct() {
    name = "";
    type = nullptr;
    elements = Ty::Vec();
    eisdrache = nullptr;
}

Eisdrache::Struct::Struct(Eisdrache::Ptr eisdrache, std::string name, Ty::Vec elements) {
    TypeVec elementTypes = TypeVec();
    for (Ty::Ptr &e : elements)
        elementTypes.push_back(e->getTy());
    this->name = name;
    this->type = StructType::create(elementTypes, name);
    this->elements = elements;
    this->eisdrache = eisdrache;
}

Eisdrache::Struct::~Struct() { name.clear(); }

Eisdrache::Struct &Eisdrache::Struct::operator=(const Struct &copy) {
    name = copy.name;
    type = copy.type;
    elements = copy.elements;
    eisdrache = copy.eisdrache;
    return *this;
}

bool Eisdrache::Struct::operator==(const Struct &comp) const { return type == comp.type; }

bool Eisdrache::Struct::operator==(const Type *comp) const { return type == comp; }

Eisdrache::Ty::Ptr Eisdrache::Struct::operator[](size_t index) { return elements.at(index); }

StructType *Eisdrache::Struct::operator*() { return type; }

Eisdrache::Local &Eisdrache::Struct::allocate(std::string name) {
    AllocaInst *alloca = eisdrache->getBuilder()->CreateAlloca(**this, nullptr, name);
    return eisdrache->getCurrentParent().addLocal(Local(eisdrache, shared_from_this(), alloca));
} 

Eisdrache::Func *Eisdrache::Struct::createMemberFunc(Ty::Ptr type, std::string name, Ty::Map args) {
    Ty::Map processed = {{"this", getPtrTo()}};
    for (Ty::Map::value_type &x : args)
        processed.push_back(x);
    return &eisdrache->declareFunction(type, this->name+"_"+name, processed, true);
}

Type *Eisdrache::Struct::getTy() const { return type; }

// can't do arithmetic operations with a struct
bool Eisdrache::Struct::isValidRHS(const Ty::Ptr comp) const { return false; }

bool Eisdrache::Struct::isEqual(const Ty::Ptr comp) const {
    return dynamic_cast<Struct *>(comp.get()) && dynamic_cast<Struct *>(comp.get())->type == type;
}

Eisdrache::Entity::Kind Eisdrache::Struct::kind() const { return STRUCT; }

/// EISDRACHE ARRAY ///

Eisdrache::Array::Array(Eisdrache::Ptr eisdrache, Ty::Ptr elementTy, std::string name) {
    this->eisdrache = eisdrache;
    this->name = name;
    this->elementTy = elementTy;
    this->bufferTy = elementTy->getPtrTo();
    this->self = eisdrache->declareStruct(name, {
        bufferTy,                   // TYPE* buffer
        eisdrache->getSizeTy(),     // i64 size
        eisdrache->getSizeTy(),     // i64 max
        eisdrache->getSizeTy(),     // i64 factor
    });

    Func *malloc = nullptr;
    if (!(malloc = eisdrache->getFunc("malloc")))
        malloc = &eisdrache->declareFunction(eisdrache->getUnsignedPtrTy(8), "malloc", 
            {eisdrache->getSizeTy()});

    Func *free = nullptr;
    if (!(free = eisdrache->getFunc("free")))
        free = &eisdrache->declareFunction(eisdrache->getVoidTy(), "free",
            {eisdrache->getUnsignedPtrTy(8)});

    Func *memcpy = nullptr;
    if (!(memcpy = eisdrache->getFunc("memcpy")))
        memcpy = &eisdrache->declareFunction(eisdrache->getUnsignedPtrTy(8), "memcpy",
            {eisdrache->getUnsignedPtrTy(8), 
                eisdrache->getUnsignedPtrTy(8), 
                eisdrache->getSizeTy()});

    { // get_buffer
    get_buffer = self->createMemberFunc(bufferTy, "get_buffer");
    Local &buffer = eisdrache->getElementVal(get_buffer->arg(0), 0, "buffer");
    eisdrache->createRet(buffer);
    }

    { // set_buffer
    set_buffer = self->createMemberFunc(eisdrache->getVoidTy(), "set_buffer",
        {{"buffer", bufferTy}});
    Local &buffer_ptr = eisdrache->getElementPtr(set_buffer->arg(0), 0, "buffer_ptr");
    eisdrache->storeValue(buffer_ptr, set_buffer->arg(1));
    eisdrache->createRet();
    }

    { // get_size
    get_size = self->createMemberFunc(eisdrache->getSizeTy(), "get_size");
    Local &size = eisdrache->getElementVal(get_size->arg(0), 1, "size");
    eisdrache->createRet(size);
    }

    { // set_size
    set_size = self->createMemberFunc(eisdrache->getVoidTy(), "set_size",
        {{"size", eisdrache->getSizeTy()}});
    Local &size_ptr = eisdrache->getElementPtr(set_size->arg(0), 1, "size_ptr");
    eisdrache->storeValue(size_ptr, set_size->arg(1));
    eisdrache->createRet();
    }

    { // get_max
    get_max = self->createMemberFunc(eisdrache->getSizeTy(), "get_max");
    Local &max = eisdrache->getElementVal(get_max->arg(0), 2, "max");
    eisdrache->createRet(max);
    }

    { // set_max
    set_max = self->createMemberFunc(eisdrache->getVoidTy(), "set_max",
        {{"max", eisdrache->getSizeTy()}});
    Local &max_ptr = eisdrache->getElementPtr(set_max->arg(0), 1, "max_ptr");
    eisdrache->storeValue(max_ptr, set_max->arg(1));
    eisdrache->createRet();
    }
    
    { // get_factor
    get_factor = self->createMemberFunc(eisdrache->getSizeTy(), "get_factor");
    Local &factor = eisdrache->getElementVal(get_factor->arg(0), 3, "factor");
    eisdrache->createRet(factor);
    }

    { // set_factor
    set_factor = self->createMemberFunc(eisdrache->getVoidTy(), "set_factor",
        {{"factor", eisdrache->getSizeTy()}});
    Local &factor_ptr = eisdrache->getElementPtr(set_factor->arg(0), 1, "factor_ptr");
    eisdrache->storeValue(factor_ptr, set_factor->arg(1));
    eisdrache->createRet();
    }

    { // constructor
    constructor = self->createMemberFunc(eisdrache->getVoidTy(), "constructor");
    (**constructor)->setCallingConv(CallingConv::Fast);
    (**constructor)->setDoesNotThrow();
    set_buffer->call({constructor->arg(0).getValuePtr(), eisdrache->getNullPtr(bufferTy)});
    set_size->call({constructor->arg(0).getValuePtr(), eisdrache->getInt(64, 0)});
    set_max->call({constructor->arg(0).getValuePtr(), eisdrache->getInt(64, 0)});
    set_factor->call({constructor->arg(0).getValuePtr(), eisdrache->getInt(64, 16)});
    eisdrache->createRet();
    }

    { // constructor_size
    constructor_size = self->createMemberFunc(eisdrache->getVoidTy(), "constructor_size", 
        {{"size", eisdrache->getSizeTy()}});
    Local byteSize = Local(eisdrache, eisdrache->getInt(64, elementTy->getBit() / 8));
    Local &bytes = eisdrache->binaryOp(MUL, constructor_size->arg(1), byteSize, "bytes");
    set_buffer->call({constructor_size->arg(0), malloc->call({bytes}, "buffer")});
    set_size->call({constructor_size->arg(0), constructor_size->arg(1)});
    set_max->call({constructor_size->arg(0).getValuePtr(), eisdrache->getInt(64, 0)});
    set_factor->call({constructor_size->arg(0).getValuePtr(), eisdrache->getInt(64, 16)});
    eisdrache->createRet();
    }

    { // constructor_copy
    constructor_copy = self->createMemberFunc(eisdrache->getVoidTy(), "constructor_copy", 
        {{"original", self->getPtrTo()}});
    // TODO: implement copy constructor
    eisdrache->createRet();
    }

    { // destructor
    destructor = self->createMemberFunc(eisdrache->getVoidTy(), "destructor");
    destructor->setCallingConv(CallingConv::Fast);
    destructor->setDoesNotThrow();
    BasicBlock *free_begin = eisdrache->createBlock("free_begin");
    BasicBlock *free_close = eisdrache->createBlock("free_close");
    Local &buffer = get_buffer->call({destructor->arg(0)}, "buffer");
    eisdrache->jump(eisdrache->compareToNull(buffer, "cond"), free_close, free_begin);
    eisdrache->setBlock(free_begin);
    Local &buffer_cast = eisdrache->bitCast(buffer, eisdrache->getUnsignedPtrTy(8), "buffer_cast");
    free->call({buffer});
    eisdrache->jump(free_close);
    eisdrache->setBlock(free_close);
    eisdrache->createRet();
    }

    { // resize
    resize = self->createMemberFunc(eisdrache->getVoidTy(), "resize",
        {{"new_size", eisdrache->getSizeTy()}});
    BasicBlock *copy = eisdrache->createBlock("copy");
    BasicBlock *empty = eisdrache->createBlock("empty");
    BasicBlock *end = eisdrache->createBlock("end"); 
    
    Local byteSize = Local(eisdrache, eisdrache->getInt(64, elementTy->getBit() / 8));
    Local &bytes = eisdrache->binaryOp(MUL, resize->arg(1), byteSize, "bytes");
    Local &new_buffer = malloc->call({bytes}, "new_buffer");
    Local &buffer = get_buffer->call({resize->arg(0)}, "buffer");
    Local &size = get_size->call({resize->arg(0)}, "size");
    eisdrache->jump(eisdrache->compareToNull(buffer, "cond"), empty, copy);
    
    eisdrache->setBlock(copy);
    memcpy->call({new_buffer, buffer, size});
    free->call({buffer});
    eisdrache->jump(end);

    eisdrache->setBlock(empty);
    eisdrache->storeValue(new_buffer, eisdrache->getNullPtr(bufferTy));
    eisdrache->jump(end);
    
    eisdrache->setBlock(end);
    set_buffer->call({resize->arg(0), new_buffer});
    Local &max_ptr = eisdrache->getElementPtr(resize->arg(0), 3, "max_ptr");
    eisdrache->storeValue(max_ptr, resize->arg(1));
    eisdrache->createRet();
    }

    { // is_valid_index
    is_valid_index = self->createMemberFunc(eisdrache->getBoolTy(), "is_valid_index",
        {{"index", eisdrache->getSizeTy()}});
    Local &max = get_max->call({is_valid_index->arg(0)}, "max");
    eisdrache->createRet(eisdrache->binaryOp(LES, is_valid_index->arg(1), max, "equals"));
    }

    { // get_at_index
    get_at_index = self->createMemberFunc(elementTy, "get_at_index", 
        {{"index", eisdrache->getUnsignedTy(32)}});
    Local &buffer = get_buffer->call({get_at_index->arg(0)}, "buffer");
    Local &element_ptr = eisdrache->getArrayElement(buffer, get_at_index->arg(1), "element_ptr");
    eisdrache->createRet(element_ptr.loadValue(true, "element"));
    }

    { // set_at_index
    set_at_index = self->createMemberFunc(eisdrache->getVoidTy(), "set_at_index",
        {{"index", eisdrache->getUnsignedTy(32)}, {"value", elementTy}});
    Local &buffer = get_buffer->call({set_at_index->arg(0)}, "buffer");
    Value *raw_ptr = eisdrache->getBuilder()->CreateGEP(bufferTy->getTy(), buffer.getValuePtr(), 
        {set_at_index->arg(1).getValuePtr()}, "element_ptr");
    Local element_ptr = Local(eisdrache, bufferTy, raw_ptr);
    eisdrache->storeValue(element_ptr, set_at_index->arg(2));
    eisdrache->createRet();
    }
}

Eisdrache::Array::~Array() { name.clear(); }

Eisdrache::Local &Eisdrache::Array::allocate(std::string name) {
    return eisdrache->allocateStruct(self, name);
}

Eisdrache::Local &Eisdrache::Array::call(Member callee, ValueVec args, std::string name) {
    switch (callee) {
        case GET_BUFFER:        return get_buffer->call(args, name);
        case SET_BUFFER:        return set_buffer->call(args, name);
        case GET_SIZE:          return get_size->call(args, name);
        case SET_SIZE:          return set_size->call(args, name);
        case GET_MAX:           return get_max->call(args, name);
        case SET_MAX:           return set_max->call(args, name);
        case GET_FACTOR:        return get_factor->call(args, name);
        case SET_FACTOR:        return set_factor->call(args, name);
        case CONSTRUCTOR:       return constructor->call(args, name);
        case CONSTRUCTOR_SIZE:  return constructor_size->call(args, name);
        case CONSTRUCTOR_COPY:  return constructor_copy->call(args, name);
        case DESTRUCTOR:        return destructor->call(args, name);
        case RESIZE:            return resize->call(args, name);
        case IS_VALID_INDEX:    return is_valid_index->call(args, name);
        case GET_AT_INDEX:      return get_at_index->call(args, name);
        case SET_AT_INDEX:      return set_at_index->call(args, name);
        default:            
            Eisdrache::complain("Eisdrache::Array::call(): Callee not implemented.");
            return eisdrache->getCurrentParent().arg(0); // silence warning
    }
}

Eisdrache::Local &Eisdrache::Array::call(Member callee, Local::Vec args, std::string name) {
    ValueVec raw_args = {};
    for (Local &local : args)
        raw_args.push_back(local.getValuePtr());

    return call(callee, raw_args, name);
}

/// EISDRACHE WRAPPER ///

Eisdrache::~Eisdrache() {
    delete builder;
    functions.clear();
    structs.clear();
    types.clear();
}

void Eisdrache::initialize() {
    PassRegistry *registry = PassRegistry::getPassRegistry();
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeNativeTarget();
    initializeTarget(*registry);
}

Eisdrache::Ptr Eisdrache::create(std::string moduleID, std::string targetTriple) {
    LLVMContext *context = new LLVMContext();
    // has to be this way because std::shared_ptr / make_shared cannot access the private constructor, 
    // neither should the user be able to
    return Ptr(new Eisdrache(context, new Module(moduleID, *context), new IRBuilder<>(*context), targetTriple));
}

void Eisdrache::dump(raw_fd_ostream &os) { module->print(os, nullptr); }

void Eisdrache::dump(const std::string &filePath) {
    std::error_code EC;
    raw_fd_ostream dumpFile(filePath, EC);
    module->print(dumpFile, nullptr);
}

/// TYPES ///

Eisdrache::Ty::Ptr Eisdrache::getVoidTy() { return addTy(std::make_shared<VoidTy>(shared_from_this())); }

Eisdrache::Ty::Ptr Eisdrache::getBoolTy() { return addTy(std::make_shared<IntTy>(shared_from_this(), 1)); }

Eisdrache::Ty::Ptr Eisdrache::getSizeTy() { return addTy(std::make_shared<IntTy>(shared_from_this(), 64)); }

Eisdrache::Ty::Ptr Eisdrache::getSignedTy(size_t bit) { return addTy(std::make_shared<IntTy>(shared_from_this(), bit, true)); }

Eisdrache::Ty::Ptr Eisdrache::getSignedPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getSignedTy(bit)));}

Eisdrache::Ty::Ptr Eisdrache::getSignedPtrPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getSignedPtrTy(bit))); }

Eisdrache::Ty::Ptr Eisdrache::getUnsignedTy(size_t bit) { return addTy(std::make_shared<IntTy>(shared_from_this(), bit)); }

Eisdrache::Ty::Ptr Eisdrache::getUnsignedPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getUnsignedTy(bit))); }

Eisdrache::Ty::Ptr Eisdrache::getUnsignedPtrPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getUnsignedPtrTy(bit))); }

Eisdrache::Ty::Ptr Eisdrache::getFloatTy(size_t bit) { return addTy(std::make_shared<FloatTy>(shared_from_this(), bit)); }

Eisdrache::Ty::Ptr Eisdrache::getFloatPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getFloatTy(bit))); }

Eisdrache::Ty::Ptr Eisdrache::getFloatPtrPtrTy(size_t bit) { return addTy(std::make_shared<PtrTy>(shared_from_this(), getFloatPtrTy(bit))); };

/// VALUES ///

ConstantInt *Eisdrache::getBool(bool value) { return builder->getInt1(value); }

ConstantInt *Eisdrache::getInt(size_t bit, uint64_t value) { return builder->getIntN(bit, value); }

Value *Eisdrache::getNegative(ConstantInt *value) { 
    return builder->CreateSub(builder->getIntN(value->getBitWidth(), 0), value, "eisdrache_negate_");
}

ConstantFP *Eisdrache::getFloat(double value) { return ConstantFP::get(*context, APFloat(value)); }

Constant *Eisdrache::getLiteral(std::string value, std::string name) { return builder->CreateGlobalStringPtr(value, name); }

ConstantPointerNull *Eisdrache::getNullPtr(Ty::Ptr ptrTy) { return ConstantPointerNull::get(dyn_cast<PointerType>(ptrTy->getTy())); }

/// FUNCTIONS ///

Eisdrache::Func &Eisdrache::declareFunction(Ty::Ptr type, std::string name, Ty::Vec parameters) {   
    Ty::Map parsedParams = Ty::Map();
    for (Ty::Ptr &param : parameters)
        parsedParams.push_back({std::to_string(parsedParams.size()), param});
    functions[name] = Func(shared_from_this(), type, name, parsedParams);
    parent = &functions.at(name);
    return *parent;
}

Eisdrache::Func &Eisdrache::declareFunction(Ty::Ptr type, std::string name, Ty::Map parameters, bool entry) {
    functions[name] = Func(shared_from_this(), type, name, parameters, entry);
    parent = &functions.at(name);
    return *parent;
}

Eisdrache::Func &Eisdrache::getWrap(Function *function) {
    for (Func::Map::value_type &wrap : functions)
        if (wrap.second == function)
            return wrap.second;
    complain("Could not find Eisdrache::Func of @" + function->getName().str() + "().");
    return functions.end()->second;
}

bool Eisdrache::verifyFunc(Func &wrap) { 
    return llvm::verifyFunction(**wrap); 
}

Eisdrache::Local &Eisdrache::callFunction(Func &wrap, ValueVec args, std::string name) { 
    return wrap.call(args, name);
}

Eisdrache::Local &Eisdrache::callFunction(Func &wrap, Local::Vec args, std::string name) { 
    return wrap.call(args, name);
}

Eisdrache::Local &Eisdrache::callFunction(std::string callee, ValueVec args, std::string name) { 
    return functions.at(callee).call(args, name);
}

Eisdrache::Local &Eisdrache::callFunction(std::string callee, Local::Vec args, std::string name) { 
    return functions.at(callee).call(args, name);
}
 
/// LOCALS ///

Eisdrache::Local &Eisdrache::declareLocal(Ty::Ptr type, std::string name, Value *value, ValueVec future_args) {
    AllocaInst *alloca = builder->CreateAlloca(type->getTy(), nullptr, name);
    return parent->addLocal(Local(shared_from_this(), type->getPtrTo(), alloca, value, future_args));
}

Eisdrache::Local &Eisdrache::loadLocal(Local &local, std::string name) { return local.loadValue(); }

StoreInst *Eisdrache::storeValue(Local &local, Local &value) {
    if (!local.getTy()->isPtrTy())
        return Eisdrache::complain("Eisdrache::storeValue(): Local is not a pointer (%"+local.getName()+").");
    
    return builder->CreateStore(value.getValuePtr(), local.getValuePtr());
} 

StoreInst *Eisdrache::storeValue(Local &local, Constant *value) {
    if (!local.getTy()->isPtrTy())
        return Eisdrache::complain("Eisdrache::storeValue(): Local is not a pointer.");
    
    return builder->CreateStore(value, local.getValuePtr());
}

void Eisdrache::createFuture(Local &local, Value *value) { local.setFuture(value); }

void Eisdrache::createFuture(Local &local, Func &func, ValueVec args) {
    local.setFuture(*func);
    local.setFutureArgs(args);
}

/// STRUCT TYPES ///

Eisdrache::Struct::Ptr &Eisdrache::declareStruct(std::string name, Ty::Vec elements) {
    structs[name] = make_shared<Struct>(shared_from_this(), name, elements);
    return structs.at(name);
}

Eisdrache::Local &Eisdrache::allocateStruct(Struct::Ptr wrap, std::string name) {
    AllocaInst *alloca = builder->CreateAlloca(**wrap, nullptr, name);
    return parent->addLocal(Local(shared_from_this(), wrap, alloca));
}

Eisdrache::Local &Eisdrache::allocateStruct(std::string typeName, std::string name) {
    Struct::Ptr &ref = structs.at(typeName);
    AllocaInst *alloca = builder->CreateAlloca(**ref, nullptr, name);
    return parent->addLocal(Local(shared_from_this(), ref->getPtrTo(), alloca));
}

Eisdrache::Local &Eisdrache::getElementPtr(Local &parent, size_t index, std::string name) {
    if (parent.getTy()->kind() != Entity::PTR)
        complain("Eisdrache::getElementPtr(): Type of parent is not a pointer.");

    PtrTy *ptr = dynamic_cast<PtrTy *>(parent.getTy().get());
    
    if (ptr->getPointeeTy()->kind() != Entity::STRUCT)
        complain("Eisdrache::getElementPtr(): Type of parent is not a pointer to a struct.");
    
    Struct &ref = *dynamic_cast<Struct *>(ptr->getPointeeTy().get());
    Value *gep = builder->CreateGEP(*ref, parent.getValuePtr(), 
        {getInt(32, 0), getInt(32, index)}, name);
    return this->parent->addLocal(Local(shared_from_this(), ref[index]->getPtrTo(), gep));
}

Eisdrache::Local &Eisdrache::getElementVal(Local &parent, size_t index, std::string name) {
    Local &ptr = getElementPtr(parent, index, name+"_ptr");
    return ptr.loadValue(true, name);
}

/// BUILDER ///

ReturnInst *Eisdrache::createRet(BasicBlock *next) {
    ReturnInst *inst = builder->CreateRetVoid();
    if (next)
        builder->SetInsertPoint(next);
    return inst;
}

ReturnInst *Eisdrache::createRet(Local &value, BasicBlock *next) {
    ReturnInst *inst = builder->CreateRet(value.loadValue().getValuePtr());
    if (next)
        builder->SetInsertPoint(next);
    return inst;
}

ReturnInst *Eisdrache::createRet(Constant *value, BasicBlock *next) {
    ReturnInst *inst = builder->CreateRet(value);
    if (next)
        builder->SetInsertPoint(next);
    return inst;
}

BasicBlock *Eisdrache::createBlock(std::string name, bool insert) {
    BasicBlock *BB = BasicBlock::Create(*context, name, **parent);
    if (insert)
        setBlock(BB);
    return BB;
}

void Eisdrache::setBlock(BasicBlock *block) {
    builder->SetInsertPoint(block);
}

Eisdrache::Local &Eisdrache::binaryOp(Op op, Local &LHS, Local &RHS, std::string name) {
    Local &l = LHS.loadValue(false, LHS.getName()+"_lhs_load");
    Local &r = RHS.loadValue(false, RHS.getName()+"_rhs_load");
    Ty::Ptr ty = l.getTy();
    Local bop = Local(shared_from_this(), ty); 
    if (!ty->isValidRHS(r.getTy()))
        Eisdrache::complain("Eisdrache::binaryOp(): LHS and RHS types differ too much.");

    switch (op) {
        case ADD:
            if (name.empty()) name = "addtmp";
            if (ty->isFloatTy()) 
                bop.setPtr(builder->CreateFAdd(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateAdd(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case SUB:
            if (name.empty()) name = "subtmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFSub(l.getValuePtr(), r.getValuePtr(), name));
            else 
                bop.setPtr(builder->CreateSub(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case MUL:
            if (name.empty()) name = "multmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFMul(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateMul(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case DIV:
            if (name.empty()) name = "divtmp";
            bop.setPtr(builder->CreateFDiv(l.getValuePtr(), r.getValuePtr(), name));
            bop.setTy(getFloatTy(64));
            break;
        case MOD:
            if (name.empty()) name = "modtmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFRem(l.getValuePtr(), r.getValuePtr(), name));
            else if (ty->isSignedTy())
                bop.setPtr(builder->CreateSRem(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateURem(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case OR:    
            bop.setPtr(builder->CreateOr(l.getValuePtr(), r.getValuePtr(), name.empty() ? "ortmp" : name)); 
            break;
        case XOR:   
            bop.setPtr(builder->CreateXor(l.getValuePtr(), r.getValuePtr(), name.empty() ? "xortmp" : name)); 
            break;
        case AND:   
            bop.setPtr(builder->CreateAnd(l.getValuePtr(), r.getValuePtr(), name.empty() ? "andtmp" : name)); 
            break;
        case LSH:   
            bop.setPtr(builder->CreateShl(l.getValuePtr(), r.getValuePtr(), name.empty() ? "lshtmp" : name)); 
            break;
        case RSH:   
            bop.setPtr(builder->CreateLShr(l.getValuePtr(), r.getValuePtr(), name.empty() ? "rshtmp" : name)); 
            break;
        case EQU:
            if (name.empty()) name = "equtmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpOEQ(l.getValuePtr(), r.getValuePtr(), name)); 
            else
                bop.setPtr(builder->CreateICmpEQ(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case NEQ:
            if (name.empty()) name = "neqtmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpONE(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateICmpNE(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case LES:
            if (name.empty()) name = "lestmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpOLT(l.getValuePtr(), r.getValuePtr(), name));
            else if (ty->isSignedTy())
                bop.setPtr(builder->CreateICmpSLT(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateICmpULT(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case LTE:
            if (name.empty()) name = "ltetmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpOLE(l.getValuePtr(), r.getValuePtr(), name));
            else if (ty->isSignedTy())
                bop.setPtr(builder->CreateICmpSLE(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateICmpULE(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case GRE:
            if (name.empty()) name = "gretmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpOGT(l.getValuePtr(), r.getValuePtr(), name));
            else if (ty->isSignedTy())
                bop.setPtr(builder->CreateICmpSGT(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateICmpUGT(l.getValuePtr(), r.getValuePtr(), name));
            break;
        case GTE:
            if (name.empty()) name = "gtetmp";
            if (ty->isFloatTy())
                bop.setPtr(builder->CreateFCmpOGE(l.getValuePtr(), r.getValuePtr(), name));
            else if (ty->isSignedTy())
                bop.setPtr(builder->CreateICmpSGE(l.getValuePtr(), r.getValuePtr(), name));
            else
                bop.setPtr(builder->CreateICmpUGE(l.getValuePtr(), r.getValuePtr(), name));
            break;
        default:
            Eisdrache::complain("Eisdrache::binaryOp(): Operation (ID "+std::to_string(op)+") not implemented.");
    }

    return parent->addLocal(bop);
}

Eisdrache::Local &Eisdrache::bitCast(Local &ptr, Ty::Ptr to, std::string name) {
    Value *cast = builder->CreateBitCast(ptr.getValuePtr(), to->getTy(), name);
    return parent->addLocal(Local(shared_from_this(), to, cast));
}

BranchInst *Eisdrache::jump(BasicBlock *next) {
    return builder->CreateBr(next);
}

BranchInst *Eisdrache::jump(Local &condition, BasicBlock *then, BasicBlock *else_) {
    return builder->CreateCondBr(condition.loadValue().getValuePtr(), then, else_);
}

Eisdrache::Local &Eisdrache::typeCast(Local &value, Ty::Ptr to, std::string name) {
    if (value.getTy()->isEqual(to))
        return value.loadValue();

    Local &load = value.loadValue();
    Value *v = load.getValuePtr();
    Ty::Ptr from = load.getTy();
    Local &cast = parent->addLocal(Local(shared_from_this(), to));
    
    if (from->isFloatTy()) {
        if (to->isFloatTy()) {                                                      // FLOAT -> FLOAT
            if (from->getBit() < to->getBit())
                cast.setPtr(builder->CreateFPExt(v, to->getTy(), name));
            else
                cast.setPtr(builder->CreateFPTrunc(v, to->getTy(), name));
        } else if (to->isSignedTy())                                                // FLOAT -> SIGNED
            cast.setPtr(builder->CreateFPToSI(v, to->getTy(), name));
        else if (to->isPtrTy())                                                     // FLOAT -> POINTER
            complain("Eisdrache::typeCast(): Invalid type cast (Float -> Pointer).");
        else                                                                        // FLOAT -> UNSIGNED
            cast.setPtr(builder->CreateFPToUI(v, to->getTy(), name));
    } else if (from->isSignedTy()) {
        if (to->isFloatTy())                                                        // SIGNED -> FLOAT
            cast.setPtr(builder->CreateSIToFP(v, to->getTy(), name));
        else if (to->isPtrTy())                                                     // SIGNED -> POINTER
            cast.setPtr(builder->CreateIntToPtr(v, to->getTy(), name));
        else if (to->isSignedTy()) {                                                // SIGNED -> SIGNED
            if (from->getBit() < to->getBit())
                cast.setPtr(builder->CreateSExt(v, to->getTy(), name));
            else
                cast.setPtr(builder->CreateTrunc(v, to->getTy(), name));
        } else {                                                                    // SIGNED -> UNSIGNED
            if (from->getBit() < to->getBit())
                cast.setPtr(builder->CreateZExt(v, to->getTy(), name));
            else
                cast.setPtr(builder->CreateTrunc(v, to->getTy(), name));
        }
    } else if (from->isPtrTy()) {
        if (to->isFloatTy())                                                        // POINTER -> FLOAT
            complain("Eisdrache::typeCast(): Invalid type cast (Pointer -> Float).");
        else if (to->isPtrTy())                                                     // POINTER -> POINTER
            return bitCast(value, to, name);
        else 
            cast.setPtr(builder->CreatePtrToInt(v, to->getTy(), name));             // POINTER -> INTEGER
    } else {
        if (to->isFloatTy())                                                        // UNSIGNED -> FLOAT
            cast.setPtr(builder->CreateUIToFP(v, to->getTy(), name));
        else if (to->isPtrTy()) {                                                   // UNSIGNED -> POINTER
            cast.setPtr(builder->CreateIntToPtr(v, to->getTy(), name));
        } else {                                                                    // UNSIGNED -> INTEGER
            if (from->getBit() < to->getBit()) 
                cast.setPtr(builder->CreateZExt(v, to->getTy(), name));
            else
                cast.setPtr(builder->CreateTrunc(v, to->getTy(), name));
        }
    }

    return cast;
}

Eisdrache::Local &Eisdrache::getArrayElement(Local &array, size_t index, std::string name) {
    Value *ptr = builder->CreateGEP(array.getTy()->getTy(), array.getValuePtr(),
        {getInt(32, index)}, name);
    return parent->addLocal(Local(shared_from_this(), array.getTy(), ptr));
}

Eisdrache::Local &Eisdrache::getArrayElement(Local &array, Eisdrache::Local &index, std::string name) {
    Value *ptr = builder->CreateGEP(array.getTy()->getTy(), array.getValuePtr(), {index.getValuePtr()}, name);
    return parent->addLocal(Local(shared_from_this(), array.getTy(), ptr));
}

Eisdrache::Local &Eisdrache::compareToNull(Local &pointer, std::string name) {
    if (!pointer.getTy()->isPtrTy())
        complain("Eisdrache::compareToNull(): Local is not a pointer.");
    Value *cond = builder->CreateICmpEQ(pointer.getValuePtr(), getNullPtr(pointer.getTy()), name);
    return parent->addLocal(Local(shared_from_this(), getBoolTy(), cond));
}

Eisdrache::Local &Eisdrache::unaryOp(Op op, Local &expr, std::string name) {
    Local &load = expr.loadValue();
    Ty::Ptr loadTy = load.getTy();
    Local ret = Local(shared_from_this());
    
    switch (op) {
        case NEG: {
            if (loadTy->isFloatTy())
                ret.setPtr(builder->CreateFNeg(load.getValuePtr(), "negtmp"));
            else
                ret.setPtr(builder->CreateNeg(load.getValuePtr(), "negtmp"));
            ret.setTy(dynamic_cast<IntTy *>(loadTy.get()) ? dynamic_cast<IntTy *>(loadTy.get())->getSignedTy() : loadTy);
            break;
        }
        case NOT: 
            ret.setPtr(builder->CreateNot(load.getValuePtr(), "nottmp"));
            ret.setTy(loadTy);
            break;
        default: 
            complain("Eisdrache::unaryOp(): Operation not implemented.");
    }

    return parent->addLocal(ret);
} 

/// GETTER ///

LLVMContext *Eisdrache::getContext() { return context; }

Module *Eisdrache::getModule() { return module; }

IRBuilder<> *Eisdrache::getBuilder() { return builder; }

Eisdrache::Func &Eisdrache::getCurrentParent() { return *parent; }

Eisdrache::Ty::Vec &Eisdrache::getTypes() { return types; }

Eisdrache::Ty::Ptr Eisdrache::addTy(Ty::Ptr ty) {
    for (Ty::Ptr &cty : types)
        if (cty->isEqual(ty))
            return cty;
    types.push_back(ty);
    return types.back();
}

Eisdrache::Func *Eisdrache::getFunc(std::string name) {
    return (functions.contains(name) ? &functions[name] : nullptr);
}

void Eisdrache::setParent(Func *func) { parent = func; }

/// PRIVATE ///

Eisdrache::Eisdrache(LLVMContext *context, Module *module, IRBuilder<> *builder, std::string targetTriple) {
    this->context = context;
    this->module = module;
    this->builder = builder;
    parent = nullptr;
    functions = Func::Map();
    structs = Struct::Map();

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
        if (!target) 
            complain("TargetRegistry::lookupTargt() failed: " + error);
        targetMachine = target->createTargetMachine(targetTriple, "generic", "", targetOptions, {});
    }

    module->setTargetTriple(targetMachine->getTargetTriple().str());
    module->setDataLayout(targetMachine->createDataLayout());
}

std::nullptr_t Eisdrache::complain(std::string message) {
    std::cerr << "\033[31mError\033[0m: " << message << "\n"; 
    exit(1);
    return nullptr; // for warnings
} 

} // namespace llvm