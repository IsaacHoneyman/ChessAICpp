#pragma once
#include "movegen.hpp"
#include "types.hpp"
#include <string>
#include <string_view>

std::string toUCI(Move m);
Move fromUCI(const MoveList& legal, std::string_view uci);