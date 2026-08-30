#pragma once
#include "bot.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include <string>

struct SearchBot : Bot {
    explicit SearchBot(int d = 4) : depth(d) {} 
    SearchResult pick(Game& g, const MoveList& legal) override {
        return searcher.search(g, legal, depth);
    }   
    std::string name() const override { return "Searcher Bot - d" + std::to_string(depth); }
    void newGame() override { searcher.clear(); }

private:
    Searcher searcher;
    int depth;
};