#include "game.h"

char* state_action_names[] = {
  [ACTION_DOWN] = "D",
  [ACTION_UP] = "U",
  [ACTION_LEFT] = "L",
  [ACTION_RIGHT] = "R"
};


#ifndef GAME_ACTION_OUTPUTS
void
state_set_action(player_t* player, input_state_t* state, int a)
{
#ifdef GAME_POSITION_STATES
  state->state[0] = (number_t)a/(number_t)(ACTION_NUM_ACTIONS-1);
#else
  state->state[(player->game->map_size_x*player->game->map_size_y) + a] = 1;
#endif
}
#endif

void
state_dump(input_state_t* state, player_t* player, int action)
{

#ifdef GAME_POSITION_STATES
#ifdef GAME_ACTION_OUTPUTS
  int index = 0;
#else
  int index = 1;
  printf("action: %f ",  state->state[0]);
#endif
  printf("state: player: ");
  printf("%f,", state->state[index++]);
  printf("%f ", state->state[index++]);
  printf("cheese: %f,", state->state[index++]);
  printf("%f ", state->state[index++]);
  printf("pit: ");
  for (int i = 0; i < countof(player->game->pits); i++) {
    printf("%f,", state->state[index++]);
    printf("%f ", state->state[index++]);
  }

  printf("action: \n");
#else
  state->state[player->x + (player->game->map_size_x*player->y)] = INPUT_VALUE_PLAYER;
  state->state[player->game->cheese.x + (player->game->map_size_x*player->game->cheese.y)] = INPUT_VALUE_CHEESE;
  for (int p = 0; p < countof(player->game->pits); p++) {
    state->state[player->game->pits[p].x + (player->game->map_size_x*player->game->pits[p].y)] = INPUT_VALUE_PIT;
  }
#endif
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
  number_t scalex = player->game->map_size_x;//1.0;
  number_t scaley = player->game->map_size_y;//1.0;
  state->state[index++] = (number_t)player->x/scalex;
  state->state[index++] = (number_t)player->y/scaley;
  state->state[index++] = (number_t)player->game->cheese.x/scalex;
  state->state[index++] = (number_t)player->game->cheese.y/scaley;
  for (int i = 0; i < countof(player->game->pits); i++) {
    state->state[index++] = (number_t)player->game->pits[i].x/scalex;
    state->state[index++] = (number_t)player->game->pits[i].y/scaley;
  }

#else
  state->state[player->x + (player->game->map_size_x*player->y)] = INPUT_VALUE_PLAYER;
  state->state[player->game->cheese.x + (player->game->map_size_x*player->game->cheese.y)] = INPUT_VALUE_CHEESE;
  for (int p = 0; p < countof(player->game->pits); p++) {
    state->state[player->game->pits[p].x + (player->game->map_size_x*player->game->pits[p].y)] = INPUT_VALUE_PIT;
  }
#endif
}
