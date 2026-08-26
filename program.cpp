#include "board.hpp"

int main() {
    Board b;
    b.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.printBoard();
}