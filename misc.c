#include "game.h"
#include <unistd.h>

void
misc_clear_console(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  puts("\e[H\e[2J");
#pragma GCC diagnostic pop
}


void
misc_pause_display(player_t* player)
{
  if (player->game->render) {
    usleep(100000);
  }
}


int
misc_q_table_row_max_index(fann_type *row)
{
  fann_type max = -2.0;
  int index = 0;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[i] >= max) {
      max = row[i];
      index = i;
    }
  }
  return index;
}


fann_type
frand(void)
{
  fann_type r = ((fann_type)rand()/(float)(RAND_MAX/(fann_type)1.0));
  return r;
}
