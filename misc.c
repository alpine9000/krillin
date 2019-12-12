#include "game.h"
#include <unistd.h>
#include <float.h>


int
misc_num_correct(player_t* player)
{
  if (!player->q_nn_model) {
    return 0;
  }
  int num = 0;
#ifdef GAME_ONE_LINE_MAP
#ifdef GAME_ACTION_OUTPUTS
  input_state_t state;
  state_setup(&state, player);
  for (int x = 1; x < GAME_MAP_SIZE_X-1; x++) {
    state.state[0] = (fann_type)x/(fann_type)GAME_MAP_SIZE_X;
    fann_type *q_table_row = fann_run(player->q_nn_model, state.state);
#ifdef GAME_CHEESE_LEFT
    if (q_table_row[ACTION_RIGHT] < q_table_row[ACTION_LEFT]) {
#else
    if (q_table_row[ACTION_RIGHT] > q_table_row[ACTION_LEFT]) {
#endif
      num++;
    }
  }
#else
  input_state_t state;
  state_setup(&state, player);
  for (int x = 1; x < GAME_MAP_SIZE_X-1; x++) {
    state.state[1] = (fann_type)x/(fann_type)GAME_MAP_SIZE_X;
    fann_type q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      state_set_action(&state, a);
      q_table_row[a] = fann_run(player->q_nn_model, state.state)[0];
    }

#ifdef GAME_CHEESE_LEFT
    if (q_table_row[ACTION_RIGHT] < q_table_row[ACTION_LEFT]) {
#else
    if (q_table_row[ACTION_RIGHT] > q_table_row[ACTION_LEFT]) {
#endif
      num++;
    }
  }
#endif
#endif
    return (num*100)/(GAME_MAP_SIZE_X-2);

}


void
misc_dump_q(player_t* player)
{
#ifdef GAME_ONE_LINE_MAP
#ifdef GAME_ACTION_OUTPUTS
  input_state_t state;
  state_setup(&state, player);
  printf("\n");
  for (int x = 1; x < GAME_MAP_SIZE_X-1; x++) {
    state.state[0] = (fann_type)x/(fann_type)GAME_MAP_SIZE_X;
    fann_type *q_table_row = fann_run(player->q_nn_model, state.state);
    printf("%d: %.2f %.2f ", x, q_table_row[ACTION_LEFT], q_table_row[ACTION_RIGHT]);
  }
  printf("\n");
#else
  input_state_t state;
  state_setup(&state, player);
  printf("\n");
  for (int x = 0; x < GAME_MAP_SIZE_X-1; x++) {
    state.state[1] = (fann_type)x/(fann_type)GAME_MAP_SIZE_X;
    fann_type q_table_row[ACTION_NUM_ACTIONS];
    printf("%d: ", x);
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      state_set_action(&state, a);
      q_table_row[a] = fann_run(player->q_nn_model, state.state)[0];
      printf("%.2f ", q_table_row[a]);
    }

    printf(" ");
  }
  printf("\n");
#endif
#endif
}

void
misc_dump_train(struct fann_train_data* train)
{
  return;
#ifdef GAME_ACTION_OUTPUTS
  for (int i = 0; i < train->num_data; i++) {
    fann_type* input = &(*train->input)[i*train->num_input];
    fann_type* output = &(*train->output)[i*train->num_output];
    printf("player:%2d,%2d  cheese:%2d,%2d pit:%2d,%2d %s: % 4.4f  %s % 4.4f %c\n",
	   (int)(input[0]*GAME_MAP_SIZE_X),
	   (int)(input[1]*GAME_MAP_SIZE_Y),
	   (int)(input[2]*GAME_MAP_SIZE_X),
	   (int)(input[3]*GAME_MAP_SIZE_Y),
	   (int)(input[4]*GAME_MAP_SIZE_X),
	   (int)(input[5]*GAME_MAP_SIZE_Y),
	   state_action_names[0],
	   output[0],
	   state_action_names[1],
	   output[1],
	   output[0] <  output[1] ? '*' : ' ');
  }
#else
  for (int i = 0; i < train->num_data; i++) {
    fann_type* input = &(*train->input)[i*train->num_input];
    fann_type* output = &(*train->output)[i*train->num_output];
    printf("player:%2d,%2d  cheese:%2d,%2d pit:%2d,%2d  Q: % 4.4f Action: %s:\n",
      (int)(input[1]*GAME_MAP_SIZE_X),
      (int)(input[2]*GAME_MAP_SIZE_Y),
      (int)(input[3]*GAME_MAP_SIZE_X),
      (int)(input[4]*GAME_MAP_SIZE_Y),
      (int)(input[5]*GAME_MAP_SIZE_X),
      (int)(input[6]*GAME_MAP_SIZE_Y),
      output[0],
      state_action_names[(int)input[0]]);
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
    //    usleep(1000000);
    //sleep(1);
    usleep(0.5*1000000);
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
