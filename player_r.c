#include "game.h"

static int
player_r_get_input(player_t* player)
{
  misc_pause_display(player);
  player->runs += 1;

  return rand() % ACTION_NUM_ACTIONS;
}


void
player_r_initialize(player_t* player)
{
  player->get_input = player_r_get_input;
  player->ready = true;
  player->first_run = false;
  player->runs = 0;
  player->previous_score = 0;
}
