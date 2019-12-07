#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "game.h"

static int
player_r_get_input(player_t* player);

void
player_r_initialize(player_t* player)
{
  player->get_input = player_r_get_input;

  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }

  player->first_run = true;
  player->runs = 0;
  player->old_score = 0;
}


static int
player_r_get_input(player_t* player)
{
  misc_pause_display(player);
  player->runs += 1;

  if (player->first_run) {
    // If this is first run initialize the Q-neural network
    player->first_run = false;
  }

  int action_taken_index;
  action_taken_index = frand() * ACTION_NUM_ACTIONS;

  // Take action
  return player->actions[action_taken_index];
}
