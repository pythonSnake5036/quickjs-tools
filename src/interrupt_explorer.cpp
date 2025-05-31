#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include <boost/program_options.hpp>

#include "quickjs-libc.h"
#include "quickjs.h"

namespace po = boost::program_options;

struct interrupt_handler_data {
    bool suppress;

    bool verbose;
    int num_interrupts;

    std::set<int> interrupt_at;
    double interrupt_chance;

    std::mt19937* generator;
    std::uniform_real_distribution<double> * random_distribution;
};

int interrupt_handler(JSRuntime * rt, void * opaque) {
    auto *data = static_cast<interrupt_handler_data *>(opaque);

    if (data->suppress) return 0;

    if (data->verbose) {
        std::cout << "Interruption Point " << data->num_interrupts << std::endl;
    }

    bool interrupt = data->interrupt_at.contains(data->num_interrupts);

    data->num_interrupts++;

    if (data->interrupt_chance >= 0) {
        if ((*data->random_distribution)(*data->generator) < data->interrupt_chance) {
            return 1;
        }
    }

    return interrupt ? 1 : 0;
}

int main(const int argc, char * argv[]) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print help message")
        ("verbose,v", "verbose output, log interruption points when hit")
        ("interrupt,i", po::value<std::vector<int>>(), "interrupt at interruption point(s)")
        ("interrupt-chance", po::value<double>(), "random chance to interrupt at each interruption point (0-1)")
        ("call,c", po::value<std::vector<std::string>>(), "function(s) to call after evaluating the script")
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

    std::set<int> interrupt_at;

    if (vm.contains("interrupt")) {
        std::vector<int> interrupt_at_vec = vm["interrupt"].as<std::vector<int>>();
        interrupt_at.insert(interrupt_at_vec.begin(), interrupt_at_vec.end());
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

    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, nullptr);

    interrupt_handler_data handler_data{
        .suppress = false,
        .verbose = verbose,
        .num_interrupts = 0,
        .interrupt_at = interrupt_at,
        .interrupt_chance = vm.contains("interrupt-chance") ? vm["interrupt-chance"].as<double>() : 0,
        .generator = &mt,
        .random_distribution = &dist,
    };

    JS_SetInterruptHandler(rt, interrupt_handler, &handler_data);

    const JSValue val = JS_Eval(ctx, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);

    if (vm.contains("call")) {
        JSValue global = JS_GetGlobalObject(ctx);

        std::vector<std::string> functions = vm["call"].as<std::vector<std::string>>();

        for (const auto& function : functions) {
            JSAtom function_atom = JS_NewAtom(ctx, function.c_str());
            JSValue function_value = JS_GetProperty(ctx, global, function_atom);

            const JSValue return_val = JS_Call(ctx, function_value, function_value, 0, nullptr);

            if (JS_IsException(return_val)) {
                handler_data.suppress = true;
                js_std_dump_error(ctx);
                handler_data.suppress = false;
            }

            JS_FreeAtom(ctx, function_atom);
            JS_FreeValue(ctx, function_value);
            JS_FreeValue(ctx, return_val);
        }

        JS_FreeValue(ctx, global);
    }

    if (JS_IsException(val)) {
        handler_data.suppress = true;
        js_std_dump_error(ctx);
        handler_data.suppress = false;
    }

    JS_FreeValue(ctx, val);

    std::cout << handler_data.num_interrupts << " total interruption point(s)." << std::endl;

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}