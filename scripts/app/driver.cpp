#include "movegen.hpp"
#include "notation.hpp"
#include "random_bot.hpp"
#include "search_bot.hpp"
#include "uci.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

constexpr int SEARCH_DEPTH = 4;
constexpr int DEFAULT_GAMES = 10;

using Clock = std::chrono::steady_clock;

std::string resultText(GameState s, PieceColour toMove) {
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

struct Outcome {
    GameState state = GameState::ONGOING;
    PieceColour toMove = WHITE;  // side to move once the game is over
    int plies = 0;
    std::array<uint64_t, COLOUR_SIZE> nodes{};
    std::array<double, COLOUR_SIZE> seconds{};
};

int pointsFor(PieceColour c, const Outcome& o) {
    if (o.state != GameState::CHECKMATE) return 0;
    return o.toMove == c ? -1 : 1;
}

Outcome playGame(std::array<Bot*, COLOUR_SIZE>& players, bool verbose) {
    Outcome o;
    Game g;
    for (Bot* p : players) p->newGame();

    while (true) {
        MoveList legal;
        generateLegal(g.board, legal);

        o.state = g.state(legal);
        o.toMove = g.board.toMove;
        if (o.state != GameState::ONGOING) break;

        const PieceColour side = g.board.toMove;
        Bot& bot = *players[side];

        const auto start = Clock::now();
        const SearchResult r = bot.pick(g, legal);
        const std::chrono::duration<double> elapsed = Clock::now() - start;

        if (r.move == NO_MOVE) {  // a bot with legal moves should never do this
            std::cout << bot.name() << " returned no move\n";
            break;
        }

        o.nodes[side] += r.nodes;
        o.seconds[side] += elapsed.count();
        ++o.plies;

        if (verbose) {
            g.board.printBoard();
            std::cout << bot.name() << " plays " << toUCI(r.move) << "  (score "
                      << r.score << ", depth " << r.depth << ", nodes " << r.nodes
                      << ")\n\n";
        }

        g.playMove(r.move);
    }

    if (verbose) g.board.printBoard();
    return o;
}

void runArena(int games, bool verbose) {
    SearchBot searcher(SEARCH_DEPTH);
    RandomBot randomBot;

    int wins = 0, draws = 0, losses = 0;  // searcher's point of view
    uint64_t nodes = 0;
    double seconds = 0.0;
    int plies = 0;

    std::cout << searcher.name() << " v " << randomBot.name() << " -- " << games
              << " game(s)\n\n";

    for (int i = 0; i < games; ++i) {
        const PieceColour searcherColour = (i % 2 == 0) ? WHITE : BLACK;
        std::array<Bot*, COLOUR_SIZE> players{};
        players[searcherColour] = &searcher;
        players[other(searcherColour)] = &randomBot;

        const Outcome o = playGame(players, verbose);
        const int points = pointsFor(searcherColour, o);

        if (points > 0) ++wins;
        else if (points < 0) ++losses;
        else ++draws;

        nodes += o.nodes[searcherColour];
        seconds += o.seconds[searcherColour];
        plies += o.plies;

        std::cout << "game " << (i + 1) << ": searcher as "
                  << (searcherColour == WHITE ? "white" : "black") << "  "
                  << resultText(o.state, o.toMove) << "  (" << o.plies << " plies, "
                  << o.nodes[searcherColour] << " nodes in " << o.seconds[searcherColour]
                  << "s)\n";
    }

    const double nps = seconds > 0.0 ? double(nodes) / seconds : 0.0;
    std::cout << "\n" << searcher.name() << ": +" << wins << " =" << draws << " -"
              << losses << "  (" << (wins + 0.5 * draws) / games * 100.0 << "%)\n"
              << "searched " << nodes << " nodes in " << seconds << "s ("
              << uint64_t(nps) << " nps) over " << plies << " plies\n";
}

}  // namespace

int main(int argc, char** argv) {
    // No args -> UCI on stdin/stdout, which is how a GUI (or the lichess
    // bridge) launches the engine.
    //   ./chess arena        random v depth-4 search, DEFAULT_GAMES games
    //   ./chess arena 50     50 games
    //   ./chess arena 4 v    4 games, printing every board and move
    const std::string mode = (argc > 1) ? argv[1] : "uci";

    if (mode == "uci") {
        SearchBot bot(SEARCH_DEPTH);
        uciLoop(bot);
        return 0;
    }

    if (mode != "arena") {
        std::cerr << "usage: " << argv[0] << " [uci | arena [games] [v]]\n";
        return 1;
    }

    int games = DEFAULT_GAMES;
    bool verbose = false;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "v") verbose = true;
        else if (const int n = std::atoi(arg.c_str()); n > 0) games = n;
        else {
            std::cerr << "arena: ignoring unknown argument '" << arg << "'\n";
        }
    }

    runArena(games, verbose);
    return 0;
}
