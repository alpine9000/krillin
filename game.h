#pragma once
#include <stdbool.h>
#include "doublefann.h"

enum {
  ACTION_LEFT,
  ACTION_RIGHT,
  ACTION_UP,
  ACTION_DOWN,
  ACTION_NUM_ACTIONS
};

#define INPUT_VALUE_PLAYER          1.0
#define INPUT_VALUE_CHEESE          0.5
#define INPUT_VALUE_PIT             0.1

#define GAME_SCORE_HIGH             1
#define GAME_SCORE_LOW              -2
#define GAME_MAP_SIZE_X             10
#define GAME_MAP_SIZE_Y             10

#define PLAYER_Q_REPLAY_MEMORY_SIZE 50000
#define PLAYER_Q_REPLAY_BATCH_SIZE  400
#define PLAYER_Q_SIZEOF_STATE       ((GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y)+ACTION_NUM_ACTIONS)

typedef struct {
  int x;
  int y;
} position_t;

typedef struct {
  int score;
  bool new_game;
  position_t start_position;
  position_t cheese;
  position_t pits[1];
  int moves;
  int run;
  int last_moves;
  int move_count;
  int min_moves;
  int average_moves;
  int total_moves;
  int render;
  int reload;
  int won;
  fann_type average_q;
  int loops;
  bool reset_player;
} game_t;

typedef struct {
  fann_type state[PLAYER_Q_SIZEOF_STATE];
} input_state_t;

typedef struct {
  fann_type reward;
  input_state_t old_input_state;
  input_state_t input_state;
} replay_memory_t;

typedef struct player{
  game_t* game;
  int (*get_input)(struct player* player);

  int x;
  int y;
  int actions[ACTION_NUM_ACTIONS];
  bool first_run;
  int runs;
  bool ready;

  fann_type discount;
  fann_type epsilon;
  fann_type max_epsilon;
  fann_type epsilon_increase_factor;
  fann_type last_e;

  int replay_memory_size;
  int replay_memory_pointer;
  int replay_memory_index;
  int replay_batch_size;
  replay_memory_t replay_memory[PLAYER_Q_REPLAY_MEMORY_SIZE];

  int old_score;
  input_state_t old_input_state;
  struct fann *q_nn_model;
  struct fann_train_data* train;
} player_t;


void
player_q_initialize(player_t* player);

void
player_nn_initialize(player_t* player);

void
player_r_initialize(player_t* player);

int
misc_q_table_row_max_index(fann_type *row);

void
misc_pause_display(player_t* player);

void
misc_clear_console(void);

fann_type frand(void);

#define countof(x) (sizeof(x)/sizeof(x[0]))
