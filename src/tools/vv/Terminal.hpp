#pragma once

#include <string>

bool OpenTerminal();
void CloseTerminal();
std::string QueryTerminal( const char* query );
