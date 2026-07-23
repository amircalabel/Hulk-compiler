#include <iostream>
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

static void __setup() {
}

int main() {
    __setup();
    auto env = std::make_shared<Environment>();
    ([&]() -> HulkValue { auto __env0 = std::make_shared<Environment>(env); __env0->define("a", ([&]() -> HulkValue { auto __arr = std::make_shared<HulkArray>(); __arr->values.clear(); std::vector<HulkValue> __dims = {HulkValue(static_cast<double>(5))}; size_t __size = 1; for (auto& __d : __dims) { if (std::holds_alternative<std::nullptr_t>(__d)) continue; __size *= static_cast<size_t>(asNum(__d)); } for (size_t __i = 0; __i < __size; ++__i) { auto __env1 = std::make_shared<Environment>(env); __env1->define("i", HulkValue(static_cast<double>(__i))); __arr->values.push_back(hAdd(__env1->get("i"), HulkValue(static_cast<double>(1)))); } return HulkValue(__arr); })()); return ([&]() -> HulkValue { return ([&]() -> HulkValue { auto __env2 = std::make_shared<Environment>(__env0); __env2->define("total", HulkValue(static_cast<double>(0))); return ([&]() -> HulkValue { auto __env3 = std::make_shared<Environment>(__env2); __env3->define("j", HulkValue(static_cast<double>(0))); return ([&]() -> HulkValue { ([&]() -> HulkValue { auto __env4 = std::make_shared<Environment>(__env3); HulkValue __r5 = nullptr; while (isTruthy(hLt(__env4->get("j"), callMethod(__env4->get("a"), "size", {})))) { __r5 = ([&]() -> HulkValue { ([&]() -> HulkValue { HulkValue __v6 = hAdd(__env4->get("total"), arrayGet(__env4->get("a"), __env4->get("j"))); __env4->assign("total", __v6); return __v6; })(); return ([&]() -> HulkValue { HulkValue __v7 = hAdd(__env4->get("j"), HulkValue(static_cast<double>(1))); __env4->assign("j", __v7); return __v7; })(); })(); } return __r5; })(); return (isTruthy(hEq(__env3->get("total"), HulkValue(static_cast<double>(15)))) ? (([&]() -> HulkValue { HulkValue __v9 = (HulkValue(std::string("ok"))); std::cout << stringify(__v9) << std::endl; return __v9; })()) : (([&]() -> HulkValue { HulkValue __v8 = (HulkValue(std::string("fail"))); std::cout << stringify(__v8) << std::endl; return __v8; })())); })(); })(); })(); })(); })();
    return 0;
}
