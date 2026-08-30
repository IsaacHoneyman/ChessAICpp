#include "board.hpp"
#include "human_bot.hpp"
#include "movegen.hpp"
#include "bot.hpp"
#include "random_bot.hpp"
#include "uci.hpp"
#include <memory>
#include <iostream>

namespace {
const std::string resultText(GameState s, PieceColour toMove) {
    switch (s) {
    case GameState::CHECKMATE:
        return toMove == WHITE ? "0-1 (black mates)" : "1-0 (white mates)";
    case GameState::STALEMATE:
        return "1/2-1/2 (stalemate)";
    case GameState::FIFTYMOVE:
        return "1/2-1/2 (fifty-move rule)";
    case GameState::INSUFFICIENT:
        return "1/2-1/2 (insufficient material)";
    case GameState::REPETITION:
        return "1/2-1/2 (threefold repetition)";
    default:
        return "";
    }
}

std::unique_ptr<Bot> makeBot(char c) {
    if (c == 'h')
        return std::make_unique<HumanBot>();
    return std::make_unique<RandomBot>();
}
} // namespace

int main(int argc, char** argv) {
    // No args -> UCI on stdin/stdout, which is what a GUI expects, since
    // that is how a GUI launches the engine.
    //   ./chess hr   human white, random black
    //   ./chess rh   random white, human black
    //   ./chess rr   self-play      ./chess hh  both human
    const std::string mode = (argc > 1) ? argv[1] : "uci";
    if (mode == "uci") {
        RandomBot bot;
        uciLoop(bot);
        return 0;
    }
 
    std::unique_ptr<Bot> players[COLOUR_SIZE] = {
        makeBot(mode[0]), makeBot(mode.size() > 1 ? mode[1] : 'r')};
    for (auto& p : players) p->newGame();
 
    Game g;
    std::cout << players[WHITE]->name() << " (white) v "
              << players[BLACK]->name() << " (black)\n";
 
    while (true) {
        MoveList legal;
        generateLegal(g.board, legal);
        g.board.printBoard();
 
        const GameState s = g.state(legal);
        if (s != GameState::ONGOING) {
            std::cout << resultText(s, g.board.toMove) << '\n';
            break;
        }
 
        Bot& bot = *players[g.board.toMove];
        const SearchResult r = bot.pick(g, legal);
        if (r.move == NO_MOVE) { std::cout << "aborted\n"; break; }
 
        if (bot.name() != "Human")
            std::cout << bot.name() << " plays " << toUCI(r.move)
                      << "  (score " << r.score << ", depth " << r.depth
                      << ", nodes " << r.nodes << ")\n\n";
 
        g.playMove(r.move);
    }
}