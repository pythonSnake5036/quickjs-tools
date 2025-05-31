#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include "quickjs-libc.h"
#include "quickjs.h"

namespace po = boost::program_options;

struct interrupt_handler_data {
    bool verbose;
    int num_interrupts;

    int interrupt_at;
};

int interrupt_handler(JSRuntime * rt, void * opaque) {
    auto *data = static_cast<interrupt_handler_data *>(opaque);

    if (data->verbose) {
        std::cout << "Interruption Point " << data->num_interrupts << std::endl;
    }

    return data->num_interrupts++ == data->interrupt_at ? 1 : 0;
}

int main(const int argc, char * argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print help message")
        ("verbose,v", "verbose output, log interruption points when hit")
        ("interrupt,i", po::value<int>(), "interrupt at interruption point")
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

    bool verbose = vm.contains("verbose");

    if (vm.contains("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (!vm.contains("file")) {
        std::cerr << "No input file provided with -f. Exiting." << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "Running in verbose mode" << std::endl;
    }

    std::string filename = vm["file"].as<std::string>();
    std::ifstream file;
    file.open(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << filename << std::endl;
        return 1;
    }

    std::stringstream code_stream;
    code_stream << file.rdbuf();
    file.close();
    std::string code = code_stream.str();

    interrupt_handler_data handler_data{
        .verbose = verbose,
        .num_interrupts = 0,
        .interrupt_at = vm.contains("interrupt") ? vm["interrupt"].as<int>() : -1,
    };

    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, nullptr);
    JS_SetInterruptHandler(rt, interrupt_handler, &handler_data);

    const JSValue val = JS_Eval(ctx, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
    }

    std::cout << handler_data.num_interrupts << " possible interrupt(s)." << std::endl;

    JS_FreeValue(ctx, val);

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}