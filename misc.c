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
    state.state[0] = (number_t)x/(number_t)GAME_MAP_SIZE_X;
    const number_t *q_table_row =  player->q_nn_model->run(player->q_nn_model, state.state);
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
    state.state[1] = (number_t)x/(number_t)GAME_MAP_SIZE_X;
    number_t q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      state_set_action(&state, a);
      q_table_row[a] = player->q_nn_model->run(player->q_nn_model, state.state)[0];
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
    state.state[0] = (number_t)x/(number_t)GAME_MAP_SIZE_X;
    const number_t *q_table_row = player->q_nn_model->run(player->q_nn_model, state.state);
    printf("%d: %.2f %.2f ", x, q_table_row[ACTION_LEFT], q_table_row[ACTION_RIGHT]);
  }
  printf("\n");
#else
  input_state_t state;
  state_setup(&state, player);
  printf("\n");
  for (int x = 0; x < GAME_MAP_SIZE_X-1; x++) {
    state.state[1] = (number_t)x/(number_t)GAME_MAP_SIZE_X;
    number_t q_table_row[ACTION_NUM_ACTIONS];
    printf("%d: ", x);
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      state_set_action(&state, a);
      q_table_row[a] =  player->q_nn_model->run(player->q_nn_model, state.state)[0];
      printf("%.2f ", q_table_row[a]);
    }

    printf(" ");
  }
  printf("\n");
#endif
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

number_t
misc_q_table_row_max(const number_t *row, number_t decision_threshold)
{
  number_t max = -FLT_MAX;
  number_t second_place = -FLT_MAX;

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
misc_q_table_row_max_index(const number_t *row, number_t decision_threshold)
{
  number_t max = -FLT_MAX;;
  number_t second_place = -FLT_MAX;
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


number_t
frand(void)
{
  number_t r = ((number_t)rand()/(float)(RAND_MAX/(number_t)1.0));
  return r;
}
