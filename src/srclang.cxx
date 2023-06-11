#include <utility>
#include <fstream>

#include "Language.hxx"
#include "ProjectManager.hxx"

using namespace srclang;

const auto LOGO = R"(
                       .__                         
  _____________   ____ |  | _____    ____    ____  
 /  ___/\_  __ \_/ ___\|  | \__  \  /    \  / ___\
 \___ \  |  | \/\  \___|  |__/ __ \|   |  \/ /_/  >
/____  > |__|    \___  >____(____  /___|  /\___  /
     \/              \/          \/     \//_____/

)";

int compile(Language *language, std::optional<std::string> path, std::optional<std::string> output) {
    if (path == std::nullopt) {
        std::cerr << "No input file specified" << std::endl;
        return 1;
    }
    return language->compile(*path, std::move(output));
}

int run(Language *language, std::optional<std::string> path) {
    if (path == std::nullopt) {
        std::cerr << "No input file specified" << std::endl;
        return 1;
    }
    auto result = language->execute(std::filesystem::path(*path));
    if (SRCLANG_VALUE_GET_TYPE(result) == ValueType::Error) {
        std::cerr << (char *) SRCLANG_VALUE_AS_OBJECT(result)->pointer << std::endl;
        return 1;
    }
    return 0;
}

int interactive(Language *language) {
    std::cout << LOGO << std::endl;

    while (true) {
        std::cout << "> ";
        std::string input;
        if (!std::getline(std::cin, input, '\n')) {
            continue;
        }

        if (input == ".exit") break;

        auto result = language->execute(input, "<script>");
        std::cout << ":: " << SRCLANG_VALUE_GET_STRING(result) << std::endl;
    }

    return 0;
}

int printHelp() {
    std::cout << LOGO << std::endl;
    std::cout << "Source Programming Language" << std::endl;
    std::cout << "Copyright (C) 2021 rlxos" << std::endl;

    std::cout << "\n"
                 " COMMANDS:" << std::endl;
    std::cout << "   run                    Run srclang script and bytecode (source ends with .src)\n"
              << "   interactive            Start srclang interactive shell\n"
              << "   compile                Compile srclang script in bytecode\n"
              << "   new <name>             Setup run srclang project\n"
              << "   help                   Print this help message\n"
              << '\n'
              << " FLAGS:\n"
              << "  -debug                  Enable debugging outputs\n"
              << "  -breakpoint             Enable breakpoint at instructions\n"
              << "  -search-path <path>     Append module search path\n"
              << "  -define <key>=<value>   Define variable from command line\n"
              << std::endl;

    return 1;
}

int main(int argc, char **argv) {
    Language language;

    auto args = new SrcLangList();

    std::string task = "help";
    std::optional<std::string> filename, output;
    std::filesystem::path project_path = std::filesystem::current_path();

    if (argc >= 2) { task = argv[1]; }

    for (int i = 2; i < argc; i++) {
        if (filename == std::nullopt && argv[i][0] == '-') {
            argv[i]++;
#define CHECK_COUNT(c) if (argc <= c + i) { std::cout << "expecting " << c << " arguments" << std::endl; return 1; }
            if (strcmp(argv[i], "debug") == 0) {
                language.options["DEBUG"] = true;
            } else if (strcmp(argv[i], "breakpoint") == 0) {
                language.options["BREAK"] = true;
            } else if (strcmp(argv[i], "define") == 0) {
                CHECK_COUNT(1)
                auto value = std::string(argv[++i]);
                auto idx = value.find_first_of('=');
                if (idx == std::string::npos) {
                    language.define(value, SRCLANG_VALUE_TRUE);
                } else {
                    auto key = value.substr(0, idx);
                    value = value.substr(idx + 1);
                    language.define(key, SRCLANG_VALUE_STRING(strdup(value.c_str())));
                }
            } else if (strcmp(argv[i], "search-path") == 0) {
                CHECK_COUNT(1)
                language.appendSearchPath(argv[++i]);
            } else if (strcmp(argv[i], "o") == 0) {
                CHECK_COUNT(1);
                output = argv[++i];
            } else if (strcmp(argv[i], "project-path") == 0) {
                CHECK_COUNT(1);
                project_path = argv[++i];
            } else {
                std::cerr << "ERROR: invalid flag '-" << argv[i] << "'" << std::endl;
                return 1;
            }
#undef CHECK_COUNT
        } else if (filename == std::nullopt) {
            filename = argv[i];
        } else {
            args->push_back(SRCLANG_VALUE_STRING(strdup(argv[i])));
        }
    }

    language.define("__ARGS__", SRCLANG_VALUE_LIST(args));
    ProjectManager projectManager(&language, project_path);

    if (task == "help") return printHelp();
    else if (task == "run") return run(&language, filename);
    else if (task == "interactive") return interactive(&language);
    else if (task == "compile") return compile(&language, filename, output);

    try {
        if (task == "new") {
            if (args->empty()) throw std::runtime_error("no project name specified");
            projectManager.create((const char *) SRCLANG_VALUE_AS_OBJECT(args->at(0))->pointer);
        } else if (task == "test") {
            projectManager.test();
        }
    } catch (std::exception const &ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }


    return printHelp();
}