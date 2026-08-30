#include "uci.hpp"
#include "game.hpp"
#include "movegen.hpp"
#include "notation.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr char ENGINE_NAME[] = "Cera";
constexpr char ENGINE_AUTHOR[] = "Isaac Honeyman";

std::vector<std::string> split(const std::string& line) {
    std::istringstream ss(line);
    std::vector<std::string> out;
    for (std::string tok; ss >> tok;) out.push_back(tok);
    return out;
}

void handlePosition(Game& g, const std::vector<std::string>& t) {
    size_t i = 1;
    if (i >= t.size()) return;

    if (t[i] == "startpos") {
        g.setFEN(START_FEN);
        ++i;
    } else if (t[i] == "fen") {
        ++i;
        std::string fen;
        for (; i < t.size() && t[i] != "moves"; ++i) {
            if (!fen.empty()) fen += ' ';
            fen += t[i];
        }
        if (fen.empty()) return;
        g.setFEN(fen);
    } else {
        return;  // malformed
    }

    if (i >= t.size() || t[i] != "moves") return;

    for (++i; i < t.size(); ++i) {
        MoveList legal;
        generateLegal(g.board, legal);
        const Move m = fromUCI(legal, t[i]);
        if (m == NO_MOVE) {
            std::cerr << "uci: rejected move '" << t[i] << "'\n";
            return;
        }
        g.playMove(m);
    }
}

void handleGo(Game& g, Bot& bot) {
    MoveList legal;
    generateLegal(g.board, legal);

    if (legal.size() == 0) {  // mated or stalemated
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    const SearchResult r = bot.pick(g, legal);
    const Move best = (r.move == NO_MOVE) ? legal.moves[0] : r.move;

    std::cout << "info depth " << r.depth << " score cp " << r.score
              << " nodes " << r.nodes << " pv " << toUCI(best) << std::endl;
    std::cout << "bestmove " << toUCI(best) << std::endl;
}

}  // namespace

void uciLoop(Bot& bot) {
    std::ios::sync_with_stdio(false);

    Game g;
    std::string line;

    while (std::getline(std::cin, line)) {
        const std::vector<std::string> t = split(line);
        if (t.empty()) continue;
        const std::string& cmd = t[0];

        if (cmd == "uci") {
            std::cout << "id name " << ENGINE_NAME << '\n'
                      << "id author " << ENGINE_AUTHOR << '\n'
                      << "uciok" << std::endl;
        } else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (cmd == "ucinewgame") {
            bot.newGame();
            g.setFEN(START_FEN);
        } else if (cmd == "position") {
            handlePosition(g, t);
        } else if (cmd == "go") {
            handleGo(g, bot);
        } else if (cmd == "stop" || cmd == "setoption" || cmd == "debug") {
            // stop: nothing to interrupt while search is synchronous
            // setoption: no options declared yet
        } else if (cmd == "quit") {
            break;
        } else if (cmd == "d") {
            g.board.printBoard();  // stderr, so it can't corrupt the protocol
        }
    }
}
