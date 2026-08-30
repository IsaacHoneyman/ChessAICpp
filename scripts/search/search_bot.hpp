#pragma once
#include "bot.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include <optional>
#include <string>

struct SearchBot : Bot {
    explicit SearchBot(SearchLimits l = SearchLimits{6, 0, 0}) : fallback(l) {}
    explicit SearchBot(int d) : fallback(SearchLimits{d, 0, 0}) {} 

    SearchResult pick(Game& g, const MoveList& legal, std::optional<SearchLimits> limits) override {
        return searcher.search(g, legal, limits.value_or(fallback));
    }   
    std::string name() const override { return "Searcher Bot - d" + std::to_string(fallback.maxDepth); }
    void newGame() override { searcher.clear(); }

private:
    Searcher searcher;
    SearchLimits fallback;
};