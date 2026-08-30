#include "game.hpp"
#include "movegen.hpp"
#include "types.hpp"

void Game::setFEN(const std::string& fen) {
    recordInitFEN = fen;
    record.clear();

    board.fromFEN(fen);
    history.clear();
}

void Game::playMove(Move move) {
    MoveUndo undo;                  // discarded; committed moves never unwind
    makeMove(move, undo);
    record.push_back(move);
    if (board.halfMoveClock == 0) history.clear();
}

void Game::makeMove(Move move, MoveUndo& undo) {
    history.push(board.zobristHash);
    board.makeMove(move, undo);
}

void Game::undoMove(Move move, const MoveUndo& undo) {
    history.pop();
    board.unmakeMove(move, undo);
}

GameState Game::state(const MoveList& moves) const {
    return getState(board, moves, history);
}

GameState Game::state() {
    MoveList moves;
    generateLegal(board, moves);
    return state(moves);
}