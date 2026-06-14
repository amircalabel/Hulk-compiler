// src/backend/banner/BannerIR.cpp
#include "BannerIR.hpp"
#include <sstream>
#include <iomanip>

namespace hulk::backend {

std::string BannerInstr::toString() const {
    switch (kind) {
        case Kind::LOAD:
            if (!label.empty()) return "LOAD " + label;
            if (!varName.empty()) return "LOAD " + varName;
            return "LOAD " + std::to_string(number);
        case Kind::STORE: return "STORE " + varName;
        case Kind::COPY: return "COPY " + varName + " " + std::to_string(index);
        case Kind::ADD: return "ADD";
        case Kind::SUB: return "SUB";
        case Kind::MUL: return "MUL";
        case Kind::DIV: return "DIV";
        case Kind::POW: return "POW";
        case Kind::ALLOCATE: return "ALLOCATE " + typeName;
        case Kind::ARRAY: return "ARRAY " + std::to_string(index);
        case Kind::GETATTR: return "GETATTR " + varName + " " + typeName;
        case Kind::SETATTR: return "SETATTR " + varName + " " + typeName;
        case Kind::LABEL: return "LABEL " + label;
        case Kind::GOTO: return "GOTO " + label;
        case Kind::IF_GOTO: return "IF_GOTO " + label;
        case Kind::PARAM: return "PARAM " + varName;
        case Kind::CALL: return "CALL " + varName;
        case Kind::VCALL: return "VCALL " + typeName + " " + varName;
        case Kind::RETURN: return "RETURN";
        case Kind::PRINT: return "PRINT";
        case Kind::HALT: return "HALT";
        default: return "UNKNOWN";
    }
}

std::string BannerProgram::toString() const {
    std::stringstream ss;
    
    ss << ".TYPES\n";
    for (const auto& type : types) {
        ss << "  type " << type.name << " {\n";
        for (const auto& attr : type.attributes) {
            ss << "    attribute " << attr << ";\n";
        }
        for (const auto& [method, label] : type.methods) {
            ss << "    method " << method << " : " << label << ";\n";
        }
        ss << "  }\n";
    }
    ss << "\n";
    
    ss << ".DATA\n";
    for (const auto& data : this->data) {
        ss << "  " << data.label << " = \"" << data.value << "\";\n";
    }
    ss << "\n";
    
    ss << ".CODE\n";
    for (const auto& func : functions) {
        ss << "  function " << func.name << " {\n";
        
        if (!func.parameters.empty()) {
            ss << "    PARAM";
            for (const auto& param : func.parameters) {
                ss << " " << param;
            }
            ss << ";\n";
        }
        
        if (!func.locals.empty()) {
            ss << "    LOCAL";
            for (const auto& local : func.locals) {
                ss << " " << local;
            }
            ss << ";\n";
        }
        
        for (const auto& instr : func.instructions) {
            ss << "    " << instr.toString() << ";\n";
        }
        
        ss << "  }\n";
    }
    
    return ss.str();
}

} // namespace hulk::backend