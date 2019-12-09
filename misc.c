#include "game.h"
#include <unistd.h>
#include <float.h>

void
misc_dump_train(struct fann_train_data* train)
{
#if 0
  for (int i = 0; i < train->num_input; i++) {
    printf("%f ", (*train->input)[i]);
  }
  for (int i = 0; i < train->num_output; i++) {
    printf("=> %f\n", (*train->output)[i]);
  }
#endif
}

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

fann_type
misc_q_table_row_max(fann_type *row, fann_type decision_threshold)
{
  fann_type max = -DBL_MAX;
  fann_type second_place = -DBL_MAX;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[i] > max) {
      second_place = max;
      max = row[i];
    }
  }

  if (fabs(second_place-max) < decision_threshold) {
    if (rand() % 2) {
      return second_place;
    }
  }
  return max;
}


int
misc_q_table_row_max_index(fann_type *row, fann_type decision_threshold)
{
  fann_type max = -DBL_MAX;
  fann_type second_place = -DBL_MAX;
  int second_place_index;
  int index = 0;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[i] >= max) {
      second_place = max;
      second_place_index = index;
      max = row[i];
      index = i;
    }
  }

  if (fabs(second_place-max) < decision_threshold) {
    if (rand() % 2) {
      return second_place_index;
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
