#pragma once
#include "board.hpp"
#include "history.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include <string>
#include <vector>

inline const std::string START_FEN = 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 

struct Game {
    // playback
    std::string recordInitFEN;
    std::vector<Move> record;

    // game state
    Board board;
    PositionHistory history;

    Game() { setFEN(START_FEN); }
    explicit Game(const std::string& fen) { setFEN(fen); }
    void reset() { setFEN(recordInitFEN); }
    void setFEN(const std::string& fen);
    
    void playMove(Move move); // final choice can't be undone  
    void makeMove(Move move, MoveUndo& undo); // move in search
    void undoMove(Move move, const MoveUndo& undo); // undo in search

    GameState state(const MoveList& moves) const;
    GameState state();
};