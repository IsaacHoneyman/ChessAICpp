#pragma once
#include "bot.hpp"
#include "notation.hpp"
#include <iostream>

// we treat a human as another type of bot
struct HumanBot : Bot {
    SearchResult pick(Game& g, const MoveList& legal) override {
        while (true) {
            std::cout << (g.board.toMove == WHITE ? "white" : "black") << "> "
                      << std::flush;
 
            std::string line;
            if (!std::getline(std::cin, line)) return {};  // EOF -> NO_MOVE
 
            if (line == "quit") return {};
            if (line == "fen") { std::cout << g.board.toFEN() << '\n'; continue; }
            if (line == "moves") {
                for (const Move m : legal) std::cout << toUCI(m) << ' ';
                std::cout << "\n(" << legal.size() << ")\n";
                continue;
            }
            if (line == "help") {
                std::cout << "  <move>  UCI, e.g. e2e4 or e7e8q\n"
                             "  moves   list legal moves\n"
                             "  fen     print current FEN\n"
                             "  quit    exit\n";
                continue;
            }
 
            SearchResult r;
            r.move = fromUCI(legal, line);
            if (r.move == NO_MOVE) {
                std::cout << "illegal or unparseable: " << line << '\n';
                continue;
            }
            return r;
        }
    }
    std::string name() const override { return "Human"; }
};