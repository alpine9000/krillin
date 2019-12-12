#include "game.h"

char* state_action_names[] = {
#ifndef GAME_ONE_LINE_MAP
  [ACTION_DOWN] = "DOWN",
  [ACTION_UP] = "UP",
#endif
  [ACTION_LEFT] = "LEFT",
  [ACTION_RIGHT] = "RIGHT"
};


#ifndef GAME_ACTION_OUTPUTS
void
state_set_action(input_state_t* state, int a)
{
#ifdef GAME_POSITION_STATES
  state->state[0] = (fann_type)a/(fann_type)ACTION_NUM_ACTIONS;
#else
  state->state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + a] = 1;
#endif
}
#endif

void
state_dump(player_t* player, int action)
{
  printf("state: player: %d %d, cheese: %d %d, pit: %d %d, action: %s\n",
	 player->x,
	 player->y,
	 player->game->cheese.x,
	 player->game->cheese.y,
	 player->game->pits[0].x,
	 player->game->pits[0].y,
	 state_action_names[action]);
}


void
state_setup(input_state_t* state, player_t* player)
{
#ifdef GAME_POSITION_STATES
#ifdef GAME_ACTION_OUTPUTS
  int index = 0;
#else
  int index = 1;
#endif
  state->state[index++] = (fann_type)player->x/(fann_type)GAME_MAP_SIZE_X;
  state->state[index++] = (fann_type)player->y/(fann_type)GAME_MAP_SIZE_Y;
  state->state[index++] = (fann_type)player->game->cheese.x/(fann_type)GAME_MAP_SIZE_X;
  state->state[index++] = (fann_type)player->game->cheese.y/(fann_type)GAME_MAP_SIZE_Y;
  for (int i = 0; i < countof(player->game->pits); i++) {
    state->state[index++] = (fann_type)player->game->pits[i].x/(fann_type)GAME_MAP_SIZE_X;
    state->state[index++] = (fann_type)player->game->pits[i].y/(fann_type)GAME_MAP_SIZE_Y;
  }
#else
  state->state[player->x + (GAME_MAP_SIZE_X*player->y)] = INPUT_VALUE_PLAYER;
  state->state[player->game->cheese.x + (GAME_MAP_SIZE_X*player->game->cheese.y)] = INPUT_VALUE_CHEESE;
  for (int p = 0; p < countof(player->game->pits); p++) {
    state->state[player->game->pits[p].x + (GAME_MAP_SIZE_X*player->game->pits[p].y)] = INPUT_VALUE_PIT;
  }
#endif
}
