#include "game.h"
#include <unistd.h>
#include <float.h>


int
misc_num_correct(player_t* player)
{
  if (player->game->config.misc_q_debug == MISC_DISPLAY_NONE ||
      !player->q_model ||!player->q_model->_private_data) {
    return 0;
  }
  int num = 0;

  int x_save = player->x;
  for (int x = 1; x < player->game->config.map_size_x-1; x++) {
    player->x = x;
    number_t q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      input_state_t state;
      state_setup(&state, player);
      state_set_action(player, &state, a);
      q_table_row[a] = player->q_model->run(player->q_model, &state)[0];
    }

    if ((player->game->cheese.x < player->game->pit.x &&
	 misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_LEFT) ||
	(player->game->cheese.x > player->game->pit.x &&
	 misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_RIGHT)) {
      num++;
    }
  }

  player->x = x_save;

  return (num*100)/(player->game->config.map_size_x-2);
}


void
misc_dump_q(player_t* player)
{
#ifdef GAME_ACTION_OUTPUTS

    if (player->game->config.misc_q_debug == MISC_DISPLAY_DETAILED_Q) {
    printf("\n");
    int player_x = player->x;
    int player_y = player->y;
    for (int y = 0; y < player->game->config.map_size_y; y++) {
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	const number_t *q_table_row;
	input_state_t state;
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	q_table_row = player->q_model->run(player->q_model, &state);
	char indicator = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_UP ? 'U' : ' ';
	printf("     % .3f%c   ", q_table_row[ACTION_UP], indicator);
      }
      printf("\n");
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	const number_t* q_table_row;
	input_state_t state;
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	q_table_row = player->q_model->run(player->q_model, &state);
	char ind_left = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_LEFT ? 'L' : ' ';
	char ind_right = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_RIGHT ? 'R' : ' ';
	printf("% .3f%c% .3f%c|", q_table_row[ACTION_LEFT], ind_left, q_table_row[ACTION_RIGHT], ind_right);
      }
      printf("\n");
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	const number_t* q_table_row;
	input_state_t state;
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	q_table_row = player->q_model->run(player->q_model, &state);

	char indicator = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_DOWN ? 'D' : ' ';
	printf("     % .3f%c   ", q_table_row[ACTION_DOWN], indicator);
      }
      printf("\n");
    }
    player->x = player_x;
    player->y = player_y;
    printf("\n");
  } else if (player->game->config.misc_q_debug == MISC_DISPLAY_DIRECTION_Q) {
    printf("\n");
    int player_x = player->x;
    int player_y = player->y;
    for (int y = 0; y < player->game->config.map_size_y; y++) {
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	const number_t *q_table_row;
	input_state_t state;
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	q_table_row = player->q_model->run(player->q_model, &state);
      }
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	const number_t *q_table_row;
	input_state_t state;
	player->x = x;
	player->y = y;
	state_setup(&state, player);
	q_table_row = player->q_model->run(player->q_model, &state);
	int maxa = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS);
	int tie = 0;
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  if (a != maxa && q_table_row[a] == q_table_row[maxa]) {
	    tie = 1;
	    break;
	  }
	}
	if (player->game->cheese.x == x && player->game->cheese.y == y) {
	  printf("%s", "C");
	}	else if (player->game->pit.x == x && player->game->pit.y == y) {
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
  }
#else

  if (player->game->config.misc_q_debug == MISC_DISPLAY_DETAILED_Q) {
    printf("\n");
    int player_x = player->x;
    int player_y = player->y;
    for (int y = 0; y < player->game->config.map_size_y; y++) {
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	number_t q_table_row[ACTION_NUM_ACTIONS];
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  input_state_t state;
	  player->x = x;
	  player->y = y;
	  state_setup(&state, player);
	  state_set_action(player, &state, a);
	  q_table_row[a] =  player->q_model->run(player->q_model, &state)[0];
	}
	char indicator = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_UP ? 'U' : ' ';
	printf("     % .3f%c   ", q_table_row[ACTION_UP], indicator);
      }
      printf("\n");
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	number_t q_table_row[ACTION_NUM_ACTIONS];
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  input_state_t state;
	  player->x = x;
	  player->y = y;
	  state_setup(&state, player);
	  state_set_action(player, &state, a);
	  q_table_row[a] =  player->q_model->run(player->q_model, &state)[0];
	}
	char ind_left = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_LEFT ? 'L' : ' ';
	char ind_right = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_RIGHT ? 'R' : ' ';
	printf("% .3f%c% .3f%c|", q_table_row[ACTION_LEFT], ind_left, q_table_row[ACTION_RIGHT], ind_right);
      }
      printf("\n");
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	number_t q_table_row[ACTION_NUM_ACTIONS];
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  input_state_t state;
	  player->x = x;
	  player->y = y;
	  state_setup(&state, player);
	  state_set_action(player, &state, a);
	  q_table_row[a] =  player->q_model->run(player->q_model, &state)[0];
	}
	char indicator = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS) == ACTION_DOWN ? 'D' : ' ';
	printf("     % .3f%c   ", q_table_row[ACTION_DOWN], indicator);
      }
      printf("\n");
    }
    player->x = player_x;
    player->y = player_y;
    printf("\n");
  } else if (player->game->config.misc_q_debug == MISC_DISPLAY_DIRECTION_Q) {
    printf("\n");
    int player_x = player->x;
    int player_y = player->y;
    for (int y = 0; y < player->game->config.map_size_y; y++) {
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	number_t q_table_row[ACTION_NUM_ACTIONS];
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  input_state_t state;
	  player->x = x;
	  player->y = y;
	  state_setup(&state, player);
	  state_set_action(player, &state, a);
	  q_table_row[a] =  player->q_model->run(player->q_model, &state)[0];
	}
      }
      for (int x = 0; x < player->game->config.map_size_x; x++) {
	number_t q_table_row[ACTION_NUM_ACTIONS];
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  input_state_t state;
	  player->x = x;
	  player->y = y;
	  state_setup(&state, player);
	  state_set_action(player, &state, a);
	  q_table_row[a] =  player->q_model->run(player->q_model, &state)[0];
	}
	int maxa = misc_q_table_row_max_index(q_table_row, ACTION_NUM_ACTIONS);
	int tie = 0;
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  if (a != maxa && q_table_row[a] == q_table_row[maxa]) {
	    tie = 1;
	    break;
	  }
	}
	if (player->game->cheese.x == x && player->game->cheese.y == y) {
	  printf("%s", "C");
	}	else if (player->game->pit.x == x && player->game->pit.y == y) {
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
  }
#endif
}


void
misc_clear_console(void)
{
    puts("\x1B[H\x1B[2J");
}


void
misc_pause_display(player_t* player)
{
  if (player->game->render) {
    usleep(player->game->render_pause_time);
  }
}


number_t
misc_q_table_row_max(const number_t *row, int row_size)
{
  number_t max = -FLT_MAX;

  for (int i = 0; i < row_size; i++) {
    if (row[i] > max) {
      max = row[i];
    }
  }

  return max;
}


int
misc_q_table_row_max_index(const number_t *row, int row_size)
{
  number_t max = -FLT_MAX;
  int index = 0;

  int n = rand() % row_size;

  for (int i = 0; i < row_size; i++) {
    if (row[n] >= max) {
      max = row[n];
      index = n;
    }

    if (++n == row_size) {
      n = 0;
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
