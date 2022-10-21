#include "game_state.h"
#include "interpolated_pattern.h"

struct GameState 
{
    InterpolatedPattern current_path;   
    v3i64 view_move;
};
