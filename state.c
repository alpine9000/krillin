#include "game.h"

char* state_action_names[] = {
  [ACTION_DOWN] = "DOWN",
  [ACTION_UP] = "UP",
  [ACTION_LEFT] = "LEFT",
  [ACTION_RIGHT] = "RIGHT"
};


void
state_set_action(input_state_t* state, int a)
{
#ifdef GAME_POSITION_STATES
#if 1
  state->state[0] = (fann_type)a/(fann_type)ACTION_NUM_ACTIONS;
#else
  state->state[0] = (fann_type)a;
#endif
#else
  state->state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + a] = 1;
#endif

#if 0
  for (int i = 0; i < countof(state->state); i++) {
    printf("%f ", state->state[i]);
  }
  printf("\n");
#endif
}


void
state_dump(player_t* player, int action)
{
  printf("player: %d %d, cheese: %d %d, pit: %d %d, action: %s\n",
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
#if 1
  int index = 1;
  state->state[index++] = (fann_type)player->x/(fann_type)GAME_MAP_SIZE_X;
  state->state[index++] = (fann_type)player->y/(fann_type)GAME_MAP_SIZE_Y;
  state->state[index++] = (fann_type)player->game->cheese.x/(fann_type)GAME_MAP_SIZE_X;
  state->state[index++] = (fann_type)player->game->cheese.y/(fann_type)GAME_MAP_SIZE_Y;
  for (int i = 0; i < countof(player->game->pits); i++) {
    state->state[index++] = (fann_type)player->game->pits[i].x/(fann_type)GAME_MAP_SIZE_X;
    state->state[index++] = (fann_type)player->game->pits[i].y/(fann_type)GAME_MAP_SIZE_Y;
  }
#else
  int index = 1;
  state->state[index++] = player->x;
  state->state[index++] = player->y;
  state->state[index++] = player->game->cheese.x;
  state->state[index++] = player->game->cheese.y;
  for (int i = 0; i < countof(player->game->pits); i++) {
    state->state[index++] = player->game->pits[i].x;
    state->state[index++] = player->game->pits[i].y;
  }
#endif
#else
  state->state[player->x + (GAME_MAP_SIZE_X*player->y)] = INPUT_VALUE_PLAYER;
  state->state[player->game->cheese.x + (GAME_MAP_SIZE_X*player->game->cheese.y)] = INPUT_VALUE_CHEESE;
  for (int p = 0; p < countof(player->game->pits); p++) {
    state->state[player->game->pits[p].x + (GAME_MAP_SIZE_X*player->game->pits[p].y)] = INPUT_VALUE_PIT;
  }
#endif
}
