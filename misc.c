#include "game.h"
#include <unistd.h>
#include <float.h>


int
misc_num_correct(player_t* player)
{
  if (!player->q_model ||!player->q_model->_private_data) {
    return 0;
  }
  int num = 0;

  int x_save = player->x;
  for (int x = 1; x < player->game->map_size_x-1; x++) {
    player->x = x;
    number_t q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      input_state_t state = {0};
      state_setup(&state, player);
      state_set_action(player, &state, a);
      q_table_row[a] = player->q_model->run(player->q_model, state.state)[0];
    }

    if ((player->game->cheese.x < player->game->pits[0].x &&
	misc_q_table_row_max_index(q_table_row, 0.0) == ACTION_LEFT) ||
	(player->game->cheese.x > player->game->pits[0].x &&
	 misc_q_table_row_max_index(q_table_row, 0.0) == ACTION_RIGHT)) {
      num++;
    }
  }

  player->x = x_save;

  return (num*100)/(player->game->map_size_x-2);
}


void
misc_dump_q(player_t* player)
{
#ifdef GAME_ACTION_OUTPUTS
  input_state_t state;
  state_setup(&state, player);
  printf("\n");
  for (int x = 1; x < player->game->map_size_x-1; x++) {
    state.state[0] = (number_t)x/(number_t)player->game->map_size_x;
    const number_t *q_table_row = player->q_model->run(player->q_model, state.state);
    printf("%d: %.2f %.2f ", x, q_table_row[ACTION_LEFT], q_table_row[ACTION_RIGHT]);
  }
  printf("\n");
#else
#if 0
  printf("\n");
  int player_x = player->x;
  int player_y = player->y;
  for (int y = 0; y < player->game->map_size_y; y++) {
    for (int x = 0; x < player->game->map_size_x; x++) {
      number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	input_state_t state = {0};
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	state_set_action(player, &state, a);
	q_table_row[a] =  player->q_model->run(player->q_model, state.state)[0];
      }
      char indicator = misc_q_table_row_max_index(q_table_row, 0) == ACTION_UP ? 'U' : ' ';
      printf("   % .3f%c   ", q_table_row[ACTION_UP], indicator);
    }
    printf("\n");
    for (int x = 0; x < player->game->map_size_x; x++) {
      number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	input_state_t state = {0};
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	state_set_action(player, &state, a);
	q_table_row[a] =  player->q_model->run(player->q_model, state.state)[0];
      }
      char ind_left = misc_q_table_row_max_index(q_table_row, 0) == ACTION_LEFT ? 'L' : ' ';
      char ind_right = misc_q_table_row_max_index(q_table_row, 0) == ACTION_RIGHT ? 'R' : ' ';
      printf("% .3f%c% .3f%c|", q_table_row[ACTION_LEFT], ind_left, q_table_row[ACTION_RIGHT], ind_right);
    }
    printf("\n");
    for (int x = 0; x < player->game->map_size_x; x++) {
      number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	input_state_t state = {0};
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	state_set_action(player, &state, a);
	q_table_row[a] =  player->q_model->run(player->q_model, state.state)[0];
      }
      char indicator = misc_q_table_row_max_index(q_table_row, 0) == ACTION_DOWN ? 'D' : ' ';
      printf("   % .3f%c   ", q_table_row[ACTION_DOWN], indicator);
    }
    printf("\n");
  }
  player->x = player_x;
  player->y = player_y;
  printf("\n");
#else
  printf("\n");
  int player_x = player->x;
  int player_y = player->y;
  for (int y = 0; y < player->game->map_size_y; y++) {
    for (int x = 0; x < player->game->map_size_x; x++) {
      number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	input_state_t state = {0};
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	state_set_action(player, &state, a);
	q_table_row[a] =  player->q_model->run(player->q_model, state.state)[0];
      }
    }
    for (int x = 0; x < player->game->map_size_x; x++) {
      number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	input_state_t state = {0};
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	state_set_action(player, &state, a);
	q_table_row[a] =  player->q_model->run(player->q_model, state.state)[0];
      }
      int maxa = misc_q_table_row_max_index(q_table_row, 0.0);
      int tie = 0;
      for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	if (a != maxa && q_table_row[a] == q_table_row[maxa]) {
	  tie = 1;
	  break;
	}
      }
      if (player->game->cheese.x == x && player->game->cheese.y == y) {
	printf("%s", "C");
      }	else if (player->game->pits[0].x == x && player->game->pits[0].y == y) {
	printf("%s", "O");
      } else {
	if (tie) {
	  printf("?");
	} else {
	  printf("%s", state_action_names[maxa]);
	}
      }
    }
    printf("\n");
  }
  player->x = player_x;
  player->y = player_y;
  printf("\n");
#endif
#endif
}


void
misc_clear_console(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

    puts("\x1B[H\x1B[2J");
#pragma GCC diagnostic pop
}


void
misc_pause_display(player_t* player)
{
  if (player->game->render) {
    usleep(player->game->render_pause_time);
  }
}

number_t
misc_q_table_row_max(const number_t *row)
{
  number_t max = -FLT_MAX;

  for (int i = 0; i < GAME_NUM_OUTPUTS; i++) {
    if (row[i] > max) {
      max = row[i];
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

  int n = rand() % ACTION_NUM_ACTIONS;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[n] >= max) {
      second_place = max;
      second_place_index = index;
      max = row[n];
      index = n;
    }

    if (++n == ACTION_NUM_ACTIONS) {
      n = 0;
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
