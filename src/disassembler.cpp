#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include "quickjs-libc.h"
#include "quickjs.h"
#include "utilities.h"

#include "quickjs_bytecode.h"

namespace po = boost::program_options;

struct instruction {
    std::string name;
    size_t size;
};

instruction instructions[] = {
#define DEF(ID, SIZE, N_POP, N_PUSH, F) \
    {                                   \
        .name = #ID,                    \
        .size = SIZE                    \
    },
#define def(id, size, n_pop, n_push, f)
#include "quickjs-opcode.h"
};

void dump_bytecode(const JSFunctionBytecode * b, const int indent) {
    uint8_t * bytecode = b->byte_code_buf;
    const size_t bytecode_len = b->byte_code_len;

    size_t i = 0;

    while (i < bytecode_len) {
        instruction op = instructions[bytecode[i]];
        size_t args = op.size - 1;

        std::cout << std::string(indent, ' ') << std::format("{:#04x}", bytecode[i]) << " (" << op.name << ") ";

        for (size_t arg = 1; arg <= args; arg++) {
            std::cout << std::format("{:#04x}", bytecode[i+arg]) << " ";
        }

        std::cout << std::endl;

        if (op.name == "fclosure8") {
            const uint8_t loc = bytecode[i+1];
            const auto * closure_bytecode = static_cast<JSFunctionBytecode *>(JS_VALUE_GET_PTR(b->cpool[loc]));
            dump_bytecode(closure_bytecode, indent + 4);
        } else if (op.name == "fclosure") {
            uint32_t loc;
            memcpy(&loc, bytecode + i + 1, sizeof(loc));
            const auto * closure_bytecode = static_cast<JSFunctionBytecode *>(JS_VALUE_GET_PTR(b->cpool[loc]));
            dump_bytecode(closure_bytecode, indent + 4);
        }

        i += op.size;
    }
}

void dump_bytecode(const JSFunctionBytecode * b) {
    dump_bytecode(b, 0);
}

int main(const int argc, char * argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print help message")
        ("file,f", po::value<std::string>(), "input file containing code");
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch (po::unknown_option& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    po::notify(vm);

    if (vm.contains("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (!vm.contains("file")) {
        std::cerr << "No input file provided with -f. Exiting." << std::endl;
        return 1;
    }

    std::string filename = vm["file"].as<std::string>();
    std::ifstream file;
    file.open(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << filename << std::endl;
        return 1;
    }

    std::string code = read_ifstream(&file);
    file.close();

    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, nullptr);

    const JSValue obj = JS_Eval(ctx, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(obj)) {
        js_std_dump_error(ctx);
        JS_FreeValue(ctx, obj);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    auto * b = static_cast<JSFunctionBytecode *>(JS_VALUE_GET_PTR(obj));

    dump_bytecode(b);

    JS_FreeValue(ctx, obj);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    return 0;
}
