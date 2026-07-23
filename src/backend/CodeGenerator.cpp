// src/backend/CodeGenerator.cpp
//
// Backend de HULK: transpila el AST a un programa C++ autocontenido que
// implementa la semántica de HULK (valores dinámicos, entornos léxicos,
// funciones, clases con herencia y despacho dinámico) y lo compila a un
// binario nativo (`./output`).

#include "CodeGenerator.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>

// Reporte de errores semánticos (definido en main.cpp)
extern void semanticError(int line, int col, const std::string& message);

namespace hulk::backend {

// ============================================================
// Runtime de HULK (encabezado del programa generado)
// ============================================================
static const char* RUNTIME_HEADER = R"RT(#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <map>

struct Instance;
struct HulkArray;
using HulkValue = std::variant<double, std::string, bool, std::nullptr_t, std::shared_ptr<Instance>, std::shared_ptr<HulkArray>>;

struct Instance {
    std::string klass;
    std::unordered_map<std::string, HulkValue> fields;
};

struct HulkArray {
    std::vector<HulkValue> values;
};

static bool isTruthy(const HulkValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return false;
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0.0;
    return true;
}
static double asNum(const HulkValue& v) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
    if (std::holds_alternative<std::string>(v)) { try { return std::stod(std::get<std::string>(v)); } catch (...) { return 0.0; } }
    return 0.0;
}
static std::string numToStr(double d) {
    if (std::isfinite(d) && d == static_cast<long long>(d)) return std::to_string(static_cast<long long>(d));
    return std::to_string(d);
}
static std::string stringify(const HulkValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "nil";
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<double>(v)) return numToStr(std::get<double>(v));
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<std::shared_ptr<Instance>>(v)) {
        auto o = std::get<std::shared_ptr<Instance>>(v);
        return o ? (std::string("<") + o->klass + " instance>") : "nil";
    }
    if (std::holds_alternative<std::shared_ptr<HulkArray>>(v)) {
        return "[array]";
    }
    return "unknown";
}
static bool valuesEqual(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<std::nullptr_t>(a) && std::holds_alternative<std::nullptr_t>(b)) return true;
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) return std::get<std::string>(a) == std::get<std::string>(b);
    if ((std::holds_alternative<double>(a) || std::holds_alternative<bool>(a)) &&
        (std::holds_alternative<double>(b) || std::holds_alternative<bool>(b))) return asNum(a) == asNum(b);
    if (std::holds_alternative<std::shared_ptr<Instance>>(a) && std::holds_alternative<std::shared_ptr<Instance>>(b))
        return std::get<std::shared_ptr<Instance>>(a) == std::get<std::shared_ptr<Instance>>(b);
    return false;
}
static HulkValue hAdd(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) + asNum(b)); }
static HulkValue hSub(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) - asNum(b)); }
static HulkValue hMul(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) * asNum(b)); }
static HulkValue hDiv(const HulkValue& a, const HulkValue& b) { double d = asNum(b); return HulkValue(d == 0.0 ? 0.0 : asNum(a) / d); }
static HulkValue hMod(const HulkValue& a, const HulkValue& b) { double d = asNum(b); return HulkValue(d == 0.0 ? 0.0 : std::fmod(asNum(a), d)); }
static HulkValue hPow(const HulkValue& a, const HulkValue& b) { return HulkValue(std::pow(asNum(a), asNum(b))); }
static HulkValue arraySize(const HulkValue& a) { 
    if (std::holds_alternative<std::shared_ptr<HulkArray>>(a)) {
        return HulkValue(static_cast<double>(std::get<std::shared_ptr<HulkArray>>(a)->values.size()));
    }
    return HulkValue(0.0);
}
static HulkValue arrayGet(const HulkValue& a, const HulkValue& idx) {
    if (std::holds_alternative<std::shared_ptr<HulkArray>>(a)) {
        auto arr = std::get<std::shared_ptr<HulkArray>>(a);
        long long i = static_cast<long long>(asNum(idx));
        if (i >= 0 && i < static_cast<long long>(arr->values.size())) return arr->values[static_cast<size_t>(i)];
    }
    return HulkValue(nullptr);
}
static void arraySet(const HulkValue& a, const HulkValue& idx, const HulkValue& val) {
    if (std::holds_alternative<std::shared_ptr<HulkArray>>(a)) {
        auto arr = std::get<std::shared_ptr<HulkArray>>(a);
        long long i = static_cast<long long>(asNum(idx));
        if (i >= 0) {
            if (static_cast<size_t>(i) >= arr->values.size()) {
                arr->values.resize(static_cast<size_t>(i) + 1, HulkValue(nullptr));
            }
            arr->values[static_cast<size_t>(i)] = val;
        }
    }
}
static HulkValue hConcat(const HulkValue& a, const HulkValue& b) { return HulkValue(stringify(a) + stringify(b)); }
static HulkValue hConcatSp(const HulkValue& a, const HulkValue& b) { return HulkValue(stringify(a) + " " + stringify(b)); }
static HulkValue hLt(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) < asNum(b)); }
static HulkValue hLe(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) <= asNum(b)); }
static HulkValue hGt(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) > asNum(b)); }
static HulkValue hGe(const HulkValue& a, const HulkValue& b) { return HulkValue(asNum(a) >= asNum(b)); }
static HulkValue hEq(const HulkValue& a, const HulkValue& b) { return HulkValue(valuesEqual(a, b)); }
static HulkValue hNeq(const HulkValue& a, const HulkValue& b) { return HulkValue(!valuesEqual(a, b)); }

class Environment {
public:
    Environment(std::shared_ptr<Environment> enclosing = nullptr) : enclosing(enclosing) {}
    void define(const std::string& n, const HulkValue& v) { values[n] = v; }
    HulkValue get(const std::string& n) const {
        auto it = values.find(n);
        if (it != values.end()) return it->second;
        if (enclosing) return enclosing->get(n);
        return nullptr;
    }
    void assign(const std::string& n, const HulkValue& v) {
        auto it = values.find(n);
        if (it != values.end()) { it->second = v; return; }
        if (enclosing) { enclosing->assign(n, v); return; }
        values[n] = v;
    }
private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, HulkValue> values;
};

using Func = std::function<HulkValue(std::vector<HulkValue>&)>;
using Method = std::function<HulkValue(std::shared_ptr<Instance>, std::vector<HulkValue>&)>;
using Ctor = std::function<std::shared_ptr<Instance>(std::vector<HulkValue>&)>;
static std::map<std::string, Func> __funcs;
static std::map<std::string, std::map<std::string, Method>> __methods;
static std::map<std::string, std::string> __parent;
static std::map<std::string, Ctor> __ctors;

static HulkValue callFunction(const std::string& name, std::vector<HulkValue> args) {
    auto it = __funcs.find(name);
    if (it != __funcs.end()) return it->second(args);
    return nullptr;
}
static HulkValue newInstance(const std::string& klass, std::vector<HulkValue> args) {
    auto it = __ctors.find(klass);
    if (it != __ctors.end()) return HulkValue(it->second(args));
    return nullptr;
}
static HulkValue callMethod(const HulkValue& objv, const std::string& name, std::vector<HulkValue> args) {
    // Allow array-type values to respond to simple methods like `size()`.
    if (std::holds_alternative<std::shared_ptr<HulkArray>>(objv)) {
        if (name == "size") return arraySize(objv);
        // Future: add other array helpers (push, pop, etc.) here.
        return nullptr;
    }
    if (!std::holds_alternative<std::shared_ptr<Instance>>(objv)) return nullptr;
    auto obj = std::get<std::shared_ptr<Instance>>(objv);
    std::string k = obj->klass;
    while (!k.empty()) {
        auto ci = __methods.find(k);
        if (ci != __methods.end()) {
            auto mi = ci->second.find(name);
            if (mi != ci->second.end()) return mi->second(obj, args);
        }
        auto pi = __parent.find(k);
        k = (pi != __parent.end()) ? pi->second : std::string();
    }
    return nullptr;
}
static HulkValue getField(const HulkValue& v, const std::string& name) {
    if (std::holds_alternative<std::shared_ptr<Instance>>(v)) {
        auto o = std::get<std::shared_ptr<Instance>>(v);
        auto it = o->fields.find(name);
        if (it != o->fields.end()) return it->second;
    }
    return nullptr;
}
static void setField(const HulkValue& v, const std::string& name, const HulkValue& val) {
    if (std::holds_alternative<std::shared_ptr<Instance>>(v))
        std::get<std::shared_ptr<Instance>>(v)->fields[name] = val;
}

)RT";

// ============================================================
// Utilidades
// ============================================================

std::string CodeGenerator::fresh(const std::string& prefix) {
    return "__" + prefix + std::to_string(counter++);
}

std::string CodeGenerator::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string CodeGenerator::literalToValueExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) return "HulkValue(nullptr)";
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "HulkValue(true)" : "HulkValue(false)";
    if (std::holds_alternative<double>(value)) {
        std::ostringstream os;
        os << "HulkValue(static_cast<double>(" << std::get<double>(value) << "))";
        return os.str();
    }
    if (std::holds_alternative<std::string>(value))
        return "HulkValue(std::string(\"" + escapeString(std::get<std::string>(value)) + "\"))";
    return "HulkValue(nullptr)";
}

bool CodeGenerator::isBuiltin(const std::string& name) {
    static const std::set<std::string> builtins = {
        "print", "sqrt", "sin", "cos", "tan", "exp", "log",
        "abs", "floor", "ceil", "pow", "min", "max", "rand"
    };
    return builtins.count(name) > 0;
}

// ============================================================
// Generación de expresiones
// ============================================================

std::string CodeGenerator::genExpr(const Expr& expr, const std::string& env) {
    if (auto* e = dynamic_cast<const LiteralExpr*>(&expr))
        return literalToValueExpr(e->value);

    if (auto* e = dynamic_cast<const GroupingExpr*>(&expr))
        return "(" + genExpr(*e->expression, env) + ")";

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr))
        return env + "->get(\"" + e->name.lexeme + "\")";

    if (dynamic_cast<const SelfExpr*>(&expr))
        return env + "->get(\"self\")";

    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        std::string r = genExpr(*e->right, env);
        if (e->op.type == TokenType::TOKEN_MINUS) return "HulkValue(-asNum(" + r + "))";
        return "HulkValue(!isTruthy(" + r + "))"; // ! / not
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        std::string l = genExpr(*e->left, env);
        std::string r = genExpr(*e->right, env);
        switch (e->op.type) {
            case TokenType::TOKEN_PLUS:          return "hAdd(" + l + ", " + r + ")";
            case TokenType::TOKEN_MINUS:         return "hSub(" + l + ", " + r + ")";
            case TokenType::TOKEN_STAR:          return "hMul(" + l + ", " + r + ")";
            case TokenType::TOKEN_SLASH:         return "hDiv(" + l + ", " + r + ")";
            case TokenType::TOKEN_PERCENT:       return "hMod(" + l + ", " + r + ")";
            case TokenType::TOKEN_CARET:         return "hPow(" + l + ", " + r + ")";
            case TokenType::TOKEN_AT:            return "hConcat(" + l + ", " + r + ")";
            case TokenType::TOKEN_AT_AT:         return "hConcatSp(" + l + ", " + r + ")";
            case TokenType::TOKEN_EQUAL_EQUAL:   return "hEq(" + l + ", " + r + ")";
            case TokenType::TOKEN_BANG_EQUAL:    return "hNeq(" + l + ", " + r + ")";
            case TokenType::TOKEN_LESS:          return "hLt(" + l + ", " + r + ")";
            case TokenType::TOKEN_LESS_EQUAL:    return "hLe(" + l + ", " + r + ")";
            case TokenType::TOKEN_GREATER:       return "hGt(" + l + ", " + r + ")";
            case TokenType::TOKEN_GREATER_EQUAL: return "hGe(" + l + ", " + r + ")";
            case TokenType::TOKEN_AND:           return "HulkValue(isTruthy(" + l + ") && isTruthy(" + r + "))";
            case TokenType::TOKEN_OR:            return "HulkValue(isTruthy(" + l + ") || isTruthy(" + r + "))";
            default: return l;
        }
    }

    if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) {
        std::string t = fresh("v");
        return "([&]() -> HulkValue { HulkValue " + t + " = " + genExpr(*e->value, env) +
               "; " + env + "->assign(\"" + e->name.lexeme + "\", " + t + "); return " + t + "; })()";
    }

    if (auto* e = dynamic_cast<const SetExpr*>(&expr)) {
        std::string t = fresh("v");
        return "([&]() -> HulkValue { HulkValue " + t + " = " + genExpr(*e->value, env) +
               "; setField(" + genExpr(*e->object, env) + ", \"" + e->name.lexeme + "\", " + t +
               "); return " + t + "; })()";
    }

    if (auto* e = dynamic_cast<const SetIndexExpr*>(&expr)) {
        std::string t = fresh("v");
        return "([&]() -> HulkValue { HulkValue " + t + " = " + genExpr(*e->value, env) +
               "; arraySet(" + genExpr(*e->object, env) + ", " + genExpr(*e->index, env) + ", " + t +
               "); return " + t + "; })()";
    }

    if (auto* e = dynamic_cast<const GetExpr*>(&expr))
        return "getField(" + genExpr(*e->object, env) + ", \"" + e->name.lexeme + "\")";

    if (auto* e = dynamic_cast<const IndexExpr*>(&expr))
        return "arrayGet(" + genExpr(*e->object, env) + ", " + genExpr(*e->index, env) + ")";

    if (auto* e = dynamic_cast<const NewExpr*>(&expr)) {
        std::string args;
        for (size_t i = 0; i < e->arguments.size(); ++i) {
            if (i) args += ", ";
            args += genExpr(*e->arguments[i], env);
        }
        return "newInstance(\"" + e->className.lexeme + "\", {" + args + "})";
    }

    if (auto* e = dynamic_cast<const NewArrayExpr*>(&expr)) {
        std::string dims;
        for (size_t i = 0; i < e->dimensions.size(); ++i) {
            if (i) dims += ", ";
            dims += genExpr(*e->dimensions[i], env);
        }
        std::string init = e->initializer ? genExpr(*e->initializer, env) : "HulkValue(nullptr)";
        return "([&]() -> HulkValue { auto __arr = std::make_shared<HulkArray>(); __arr->values.clear(); "
               "std::vector<HulkValue> __dims = {" + dims + "}; size_t __size = 1; for (auto& __d : __dims) { "
               "if (std::holds_alternative<std::nullptr_t>(__d)) continue; "
               "__size *= static_cast<size_t>(asNum(__d)); } "
               "for (size_t __i = 0; __i < __size; ++__i) __arr->values.push_back(" + init + "); return HulkValue(__arr); })()";
    }

    if (auto* e = dynamic_cast<const ArrayLiteralExpr*>(&expr)) {
        std::string elems;
        for (size_t i = 0; i < e->elements.size(); ++i) {
            if (i) elems += ", ";
            elems += genExpr(*e->elements[i], env);
        }
        return "([&]() -> HulkValue { auto __arr = std::make_shared<HulkArray>(); __arr->values = {" + elems + "}; return HulkValue(__arr); })()";
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        // Llamada a método:  obj.metodo(args)
        if (auto* getE = dynamic_cast<const GetExpr*>(e->callee.get())) {
            std::string args;
            for (size_t i = 0; i < e->arguments.size(); ++i) {
                if (i) args += ", ";
                args += genExpr(*e->arguments[i], env);
            }
            return "callMethod(" + genExpr(*getE->object, env) + ", \"" + getE->name.lexeme +
                   "\", {" + args + "})";
        }
        // Llamada a función/builtin por nombre
        if (auto* varE = dynamic_cast<const VariableExpr*>(e->callee.get())) {
            const std::string& name = varE->name.lexeme;
            if (name == "print") {
                std::string arg = e->arguments.empty() ? "HulkValue(nullptr)" : genExpr(*e->arguments[0], env);
                std::string t = fresh("v");
                return "([&]() -> HulkValue { HulkValue " + t + " = " + arg +
                       "; std::cout << stringify(" + t + ") << std::endl; return " + t + "; })()";
            }
            static const std::map<std::string, std::string> unary = {
                {"sqrt", "std::sqrt"}, {"sin", "std::sin"}, {"cos", "std::cos"},
                {"tan", "std::tan"}, {"exp", "std::exp"}, {"log", "std::log"},
                {"abs", "std::fabs"}, {"floor", "std::floor"}, {"ceil", "std::ceil"}
            };
            auto uit = unary.find(name);
            if (uit != unary.end() && e->arguments.size() == 1)
                return "HulkValue(" + uit->second + "(asNum(" + genExpr(*e->arguments[0], env) + ")))";
            if (name == "pow" && e->arguments.size() == 2)
                return "hPow(" + genExpr(*e->arguments[0], env) + ", " + genExpr(*e->arguments[1], env) + ")";
            if (name == "size" && e->arguments.size() == 1)
                return "arraySize(" + genExpr(*e->arguments[0], env) + ")";
            if ((name == "min" || name == "max") && e->arguments.size() == 2) {
                std::string a = genExpr(*e->arguments[0], env), b = genExpr(*e->arguments[1], env);
                std::string cmp = (name == "min") ? "<" : ">";
                return "(asNum(" + a + ") " + cmp + " asNum(" + b + ") ? " + a + " : " + b + ")";
            }
            std::string args;
            for (size_t i = 0; i < e->arguments.size(); ++i) {
                if (i) args += ", ";
                args += genExpr(*e->arguments[i], env);
            }
            return "callFunction(\"" + name + "\", {" + args + "})";
        }
        return "HulkValue(nullptr)";
    }

    if (auto* e = dynamic_cast<const LetExpr*>(&expr)) {
        // Tras el desazucarado, cada LetExpr tiene una sola ligadura.
        const auto& b = e->bindings[0];
        std::string child = fresh("env");
        std::string init = b.initializer ? genExpr(*b.initializer, env) : "HulkValue(nullptr)";
        return "([&]() -> HulkValue { auto " + child + " = std::make_shared<Environment>(" + env +
               "); " + child + "->define(\"" + b.name.lexeme + "\", " + init + "); return " +
               genExpr(*e->body, child) + "; })()";
    }

    if (auto* e = dynamic_cast<const IfExpr*>(&expr)) {
        std::string result = e->elseBranch ? genExpr(*e->elseBranch, env) : "HulkValue(nullptr)";
        for (auto it = e->elifBranches.rbegin(); it != e->elifBranches.rend(); ++it) {
            result = "(isTruthy(" + genExpr(*it->first, env) + ") ? (" + genExpr(*it->second, env) +
                     ") : (" + result + "))";
        }
        result = "(isTruthy(" + genExpr(*e->condition, env) + ") ? (" + genExpr(*e->thenBranch, env) +
                 ") : (" + result + "))";
        return result;
    }

    if (auto* e = dynamic_cast<const WhileExpr*>(&expr)) {
        std::string child = fresh("env");
        std::string r = fresh("r");
        return "([&]() -> HulkValue { auto " + child + " = std::make_shared<Environment>(" + env +
               "); HulkValue " + r + " = nullptr; while (isTruthy(" + genExpr(*e->condition, child) + ")) { " +
               r + " = " + genExpr(*e->body, child) + "; } return " + r + "; })()";
    }

    if (auto* e = dynamic_cast<const ForExpr*>(&expr)) {
        std::string child = fresh("env");
        std::string r = fresh("r");
        std::string init;
        if (auto* asg = dynamic_cast<const AssignExpr*>(e->initializer.get())) {
            init = child + "->define(\"" + asg->name.lexeme + "\", " + genExpr(*asg->value, child) + "); ";
        } else if (e->initializer) {
            init = genExpr(*e->initializer, child) + "; ";
        }
        std::string cond = e->condition ? genExpr(*e->condition, child) : "HulkValue(true)";
        std::string incr = e->increment ? genExpr(*e->increment, child) : "HulkValue(nullptr)";
         return "([&]() -> HulkValue { auto " + child + " = std::make_shared<Environment>(" + env +
             "); " + init + "HulkValue " + r + " = nullptr; while (isTruthy(" + cond + ")) { " +
             r + " = " + genExpr(*e->body, child) + "; " + incr + "; } return " + r + "; })()";
    }

    if (auto* e = dynamic_cast<const BlockExpr*>(&expr)) {
        if (e->expressions.empty()) return "HulkValue(nullptr)";
        std::string body;
        for (size_t i = 0; i + 1 < e->expressions.size(); ++i)
            body += genExpr(*e->expressions[i], env) + "; ";
        body += "return " + genExpr(*e->expressions.back(), env) + ";";
        return "([&]() -> HulkValue { " + body + " })()";
    }

    return "HulkValue(nullptr)";
}

// ============================================================
// Generación de statements
// ============================================================

std::string CodeGenerator::genStmts(const std::vector<std::unique_ptr<Stmt>>& stmts, const std::string& env) {
    std::string out;
    for (const auto& s : stmts)
        if (s) out += genStmt(*s, env);
    return out;
}

std::string CodeGenerator::genStmt(const Stmt& stmt, const std::string& env) {
    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt))
        return "    std::cout << stringify(" + genExpr(*s->expression, env) + ") << std::endl;\n";

    if (auto* s = dynamic_cast<const ExpressionStmt*>(&stmt))
        return "    " + genExpr(*s->expression, env) + ";\n";

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt))
        return "    return " + (s->value ? genExpr(*s->value, env) : std::string("HulkValue(nullptr)")) + ";\n";

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt))
        return "    " + env + "->define(\"" + s->name.lexeme + "\", " +
               (s->initializer ? genExpr(*s->initializer, env) : std::string("HulkValue(nullptr)")) + ");\n";

    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        std::string child = fresh("env");
        return "    {\n    auto " + child + " = std::make_shared<Environment>(" + env + ");\n" +
               genStmts(s->statements, child) + "    }\n";
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        std::string out = "    if (isTruthy(" + genExpr(*s->condition, env) + ")) {\n" +
                          genStmt(*s->thenBranch, env) + "    }\n";
        if (s->elseBranch)
            out += "    else {\n" + genStmt(*s->elseBranch, env) + "    }\n";
        return out;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt))
        return "    while (isTruthy(" + genExpr(*s->condition, env) + ")) {\n" +
               genStmt(*s->body, env) + "    }\n";

    if (auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        std::string child = fresh("env");
        std::string out = "    {\n    auto " + child + " = std::make_shared<Environment>(" + env + ");\n";
        if (s->initializer) out += genStmt(*s->initializer, child);
        std::string cond = s->condition ? genExpr(*s->condition, child) : "HulkValue(true)";
        out += "    while (isTruthy(" + cond + ")) {\n";
        out += genStmt(*s->body, child);
        if (s->increment) out += "    " + genExpr(*s->increment, child) + ";\n";
        out += "    }\n    }\n";
        return out;
    }

    // Declaraciones (function/type/protocol/def) se manejan en __setup().
    return "";
}

// ============================================================
// Funciones y clases
// ============================================================

std::string CodeGenerator::genFunction(const FunctionDeclStmt& fn) {
    std::string out = "    __funcs[\"" + fn.name.lexeme + "\"] = [](std::vector<HulkValue>& __args) -> HulkValue {\n";
    out += "        auto env = std::make_shared<Environment>();\n";
    out += "        (void)__args;\n";
    for (size_t i = 0; i < fn.parameters.size(); ++i)
        out += "        env->define(\"" + fn.parameters[i].name.lexeme + "\", __args.size() > " +
               std::to_string(i) + " ? __args[" + std::to_string(i) + "] : HulkValue(nullptr));\n";
    out += genStmts(fn.body, "env");
    out += "        return HulkValue(nullptr);\n    };\n";
    return out;
}

std::string CodeGenerator::genClass(const ClassDeclStmt& cls) {
    std::string name = cls.name.lexeme;
    std::string out;

    if (cls.superclass.type != TokenType::TOKEN_ERROR)
        out += "    __parent[\"" + name + "\"] = \"" + cls.superclass.lexeme + "\";\n";

    // Constructor
    out += "    __ctors[\"" + name + "\"] = [](std::vector<HulkValue>& __args) -> std::shared_ptr<Instance> {\n";
    out += "        auto self = std::make_shared<Instance>();\n";
    out += "        self->klass = \"" + name + "\";\n";
    out += "        auto env = std::make_shared<Environment>();\n";
    out += "        (void)__args;\n";
    out += "        env->define(\"self\", HulkValue(self));\n";
    for (size_t i = 0; i < cls.typeArguments.size(); ++i)
        out += "        env->define(\"" + cls.typeArguments[i].lexeme + "\", __args.size() > " +
               std::to_string(i) + " ? __args[" + std::to_string(i) + "] : HulkValue(nullptr));\n";
    if (cls.superclass.type != TokenType::TOKEN_ERROR) {
        std::string pa;
        for (size_t i = 0; i < cls.superclassArguments.size(); ++i) {
            if (i) pa += ", ";
            pa += genExpr(*cls.superclassArguments[i], "env");
        }
        out += "        { std::vector<HulkValue> __pa = {" + pa + "};\n";
        out += "          auto __p = __ctors[\"" + cls.superclass.lexeme + "\"](__pa);\n";
        out += "          for (auto& __kv : __p->fields) self->fields[__kv.first] = __kv.second; }\n";
    }
    for (size_t i = 0; i < cls.attributes.size(); ++i) {
        std::string init = (i < cls.attributeInitializers.size() && cls.attributeInitializers[i])
                               ? genExpr(*cls.attributeInitializers[i], "env")
                               : "HulkValue(nullptr)";
        out += "        self->fields[\"" + cls.attributes[i].first.lexeme + "\"] = " + init + ";\n";
    }
    out += "        return self;\n    };\n";

    // Métodos
    for (const auto& m : cls.methods) {
        out += "    __methods[\"" + name + "\"][\"" + m->name.lexeme + "\"] = "
               "[](std::shared_ptr<Instance> self, std::vector<HulkValue>& __args) -> HulkValue {\n";
        out += "        auto env = std::make_shared<Environment>();\n";
        out += "        (void)__args; (void)self;\n";
        out += "        env->define(\"self\", HulkValue(self));\n";
        for (size_t i = 0; i < m->parameters.size(); ++i)
            out += "        env->define(\"" + m->parameters[i].name.lexeme + "\", __args.size() > " +
                   std::to_string(i) + " ? __args[" + std::to_string(i) + "] : HulkValue(nullptr));\n";
        out += genStmts(m->body, "env");
        out += "        return HulkValue(nullptr);\n    };\n";
    }
    return out;
}

// ============================================================
// Análisis semántico
// ============================================================

void CodeGenerator::collectDeclarations(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& s : statements) {
        if (auto* fn = dynamic_cast<const FunctionDeclStmt*>(s.get()))
            functionNames.insert(fn->name.lexeme);
        else if (auto* cls = dynamic_cast<const ClassDeclStmt*>(s.get()))
            classNames.insert(cls->name.lexeme);
    }
}

void CodeGenerator::checkExpr(const Expr& expr) {
    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        if (auto* varE = dynamic_cast<const VariableExpr*>(e->callee.get())) {
            const std::string& n = varE->name.lexeme;
            if (!isBuiltin(n) && functionNames.count(n) == 0) {
                semanticOk = false;
                semanticError(e->paren.line, 0, "Undefined function '" + n + "'.");
            }
        } else {
            checkExpr(*e->callee);
        }
        for (const auto& a : e->arguments) checkExpr(*a);
        return;
    }
    if (auto* e = dynamic_cast<const NewExpr*>(&expr)) {
        if (classNames.count(e->className.lexeme) == 0) {
            semanticOk = false;
            semanticError(e->className.line, 0, "Undefined type '" + e->className.lexeme + "'.");
        }
        for (const auto& a : e->arguments) checkExpr(*a);
        return;
    }
    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) { checkExpr(*e->left); checkExpr(*e->right); return; }
    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) { checkExpr(*e->right); return; }
    if (auto* e = dynamic_cast<const GroupingExpr*>(&expr)) { checkExpr(*e->expression); return; }
    if (auto* e = dynamic_cast<const AssignExpr*>(&expr)) { checkExpr(*e->value); return; }
    if (auto* e = dynamic_cast<const SetExpr*>(&expr)) { checkExpr(*e->object); checkExpr(*e->value); return; }
    if (auto* e = dynamic_cast<const GetExpr*>(&expr)) { checkExpr(*e->object); return; }
    if (auto* e = dynamic_cast<const LetExpr*>(&expr)) {
        for (const auto& b : e->bindings) if (b.initializer) checkExpr(*b.initializer);
        checkExpr(*e->body); return;
    }
    if (auto* e = dynamic_cast<const IfExpr*>(&expr)) {
        checkExpr(*e->condition); checkExpr(*e->thenBranch);
        for (const auto& br : e->elifBranches) { checkExpr(*br.first); checkExpr(*br.second); }
        if (e->elseBranch) checkExpr(*e->elseBranch);
        return;
    }
    if (auto* e = dynamic_cast<const WhileExpr*>(&expr)) { checkExpr(*e->condition); checkExpr(*e->body); return; }
    if (auto* e = dynamic_cast<const ForExpr*>(&expr)) {
        if (e->initializer) checkExpr(*e->initializer);
        if (e->condition) checkExpr(*e->condition);
        if (e->increment) checkExpr(*e->increment);
        checkExpr(*e->body); return;
    }
    if (auto* e = dynamic_cast<const BlockExpr*>(&expr)) {
        for (const auto& x : e->expressions) checkExpr(*x);
        return;
    }
}

void CodeGenerator::checkStmt(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) { checkExpr(*s->expression); return; }
    if (auto* s = dynamic_cast<const ExpressionStmt*>(&stmt)) { checkExpr(*s->expression); return; }
    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) { if (s->value) checkExpr(*s->value); return; }
    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) { if (s->initializer) checkExpr(*s->initializer); return; }
    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) { for (const auto& x : s->statements) if (x) checkStmt(*x); return; }
    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        checkExpr(*s->condition); if (s->thenBranch) checkStmt(*s->thenBranch);
        if (s->elseBranch) checkStmt(*s->elseBranch); return;
    }
    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) { checkExpr(*s->condition); if (s->body) checkStmt(*s->body); return; }
    if (auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        if (s->initializer) checkStmt(*s->initializer);
        if (s->condition) checkExpr(*s->condition);
        if (s->increment) checkExpr(*s->increment);
        if (s->body) checkStmt(*s->body); return;
    }
    if (auto* s = dynamic_cast<const FunctionDeclStmt*>(&stmt)) { for (const auto& x : s->body) if (x) checkStmt(*x); return; }
    if (auto* s = dynamic_cast<const ClassDeclStmt*>(&stmt)) {
        for (const auto& in : s->attributeInitializers) if (in) checkExpr(*in);
        for (const auto& m : s->methods) for (const auto& x : m->body) if (x) checkStmt(*x);
        return;
    }
}

bool CodeGenerator::checkSemantics(const std::vector<std::unique_ptr<Stmt>>& statements) {
    semanticOk = true;
    collectDeclarations(statements);
    for (const auto& s : statements)
        if (s) checkStmt(*s);
    return semanticOk;
}

// ============================================================
// Driver
// ============================================================

bool CodeGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements, const std::string& outputPath) {
    if (functionNames.empty() && classNames.empty())
        collectDeclarations(statements);

    std::string sourcePath = outputPath + ".cpp";
    std::ofstream out(sourcePath);
    if (!out.is_open()) {
        std::cerr << "Could not create source file: " << sourcePath << std::endl;
        return false;
    }

    out << RUNTIME_HEADER;

    // Registro de funciones y clases
    out << "static void __setup() {\n";
    for (const auto& s : statements) {
        if (auto* fn = dynamic_cast<const FunctionDeclStmt*>(s.get())) out << genFunction(*fn);
        else if (auto* cls = dynamic_cast<const ClassDeclStmt*>(s.get())) out << genClass(*cls);
    }
    out << "}\n\n";

    // main con los statements de nivel superior
    out << "int main() {\n";
    out << "    __setup();\n";
    out << "    auto env = std::make_shared<Environment>();\n";
    for (const auto& s : statements) {
        if (!s) continue;
        if (dynamic_cast<const FunctionDeclStmt*>(s.get())) continue;
        if (dynamic_cast<const ClassDeclStmt*>(s.get())) continue;
        if (dynamic_cast<const ProtocolDeclStmt*>(s.get())) continue;
        if (dynamic_cast<const MacroDeclStmt*>(s.get())) continue;
        out << genStmt(*s, "env");
    }
    out << "    return 0;\n}\n";
    out.close();

    return compileToBinary(sourcePath, outputPath);
}

bool CodeGenerator::compileToBinary(const std::string& sourcePath, const std::string& outputPath) {
    std::string command = "g++ -std=c++17 -O2 " + sourcePath + " -o " + outputPath + " 2> " + outputPath + ".log";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Internal error: failed to compile generated code." << std::endl;
        std::ifstream log(outputPath + ".log");
        if (log.is_open()) { std::cerr << log.rdbuf(); }
        return false;
    }
    chmod(outputPath.c_str(), 0755);
    return true;
}

} // namespace hulk::backend
