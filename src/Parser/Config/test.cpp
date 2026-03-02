#include "Config.hpp"

int main(int ac, char **av) {
    if (ac != 2) {
        std::cerr << "Usage: ./test_parser <config_file>" << std::endl;
        return 1;
    }

    try {
        std::vector<Token> tokens = lexer(av[1]);
        Parser parser(tokens);
        Config config = parser.parse();
        config.debug();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
