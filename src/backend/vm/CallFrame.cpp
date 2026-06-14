// src/backend/vm/CallFrame.cpp
#include "CallFrame.hpp"
#include "Value.hpp"

namespace hulk::backend {

CallFrame::CallFrame() 
    : closure(nullptr), ip(nullptr), slots(nullptr), slotCount(0) {}

CallFrame::CallFrame(ObjClosure* closure, Value* slots)
    : closure(closure), ip(closure->function->code.data()), 
      slots(slots), slotCount(0) {}

void CallFrame::reset() {
    closure = nullptr;
    ip = nullptr;
    slots = nullptr;
    slotCount = 0;
}

uint8_t CallFrame::readByte() {
    return *ip++;
}

uint16_t CallFrame::readShort() {
    uint16_t value = (static_cast<uint16_t>(ip[0]) << 8) | ip[1];
    ip += 2;
    return value;
}

Value CallFrame::readConstant() {
    uint8_t index = readByte();
    return closure->function->constants[index];
}

std::string CallFrame::readString() {
    Value value = readConstant();
    if (value.isObj() && value.asObj()->type == ObjType::OBJ_STRING) {
        return static_cast<ObjString*>(value.asObj())->chars;
    }
    return "";
}

} // namespace hulk::backend