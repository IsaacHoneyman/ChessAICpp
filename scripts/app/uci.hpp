#pragma once
#include "bot.hpp"

// Reads UCI commands from stdin until 'quit' or EOF, driving the given bot.
// stdout for uci, stderr for debug
void uciLoop(Bot& bot);
