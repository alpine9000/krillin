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

#define GAME_TRAINING_SCORE_HIGH    10
#define GAME_TRAINING_SCORE_LOW     -20
#define GAME_MAP_SIZE_X             6
#define GAME_MAP_SIZE_Y             6
#define GAME_E_INCREASE_FACTOR      4000 //(80*GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y)
#define GAME_Q_MAX_EPSILON          0.9
#define GAME_Q_REPLAY_MEMORY_SIZE   150000
#define GAME_Q_REPLAY_BATCH_SIZE    4000
#define GAME_Q_CONFIDENCE_THRESHOLD 0.2
#define GAME_MAX_ATTEMPTS_PER_GAME  400
#define GAME_NUM_PITS               1
#define GAME_MOVING_CHEESE
//#define GAME_POSITION_STATES

#define GAME_Q_HISTORY_SIZE         2500
#ifdef GAME_POSITION_STATES
#define PLAYER_Q_SIZEOF_STATE       ((GAME_NUM_PITS*2)+2+2+1)
#else
#define PLAYER_Q_SIZEOF_STATE       ((GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y)+ACTION_NUM_ACTIONS)
#endif


typedef struct {
  int x;
  int y;
} position_t;

typedef struct {
  int winning_score;
  int losing_score;
  int score;
  bool new_game;
  position_t start_position;
  position_t cheese;
  position_t pits[GAME_NUM_PITS];
  int moves;
  int last_moves;
  int move_count;
  int min_moves;
  int average_moves;
  int total_moves;
  int render;
  int reload;
  fann_type average_q;
  fann_type q_history[GAME_Q_HISTORY_SIZE];
  int q_history_index;
  int loops;
  bool reset_player;

  int won;
  int played;
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
  replay_memory_t replay_memory[GAME_Q_REPLAY_MEMORY_SIZE];

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
misc_q_table_row_max_index(fann_type *row, fann_type decision_threshold);

fann_type
misc_q_table_row_max(fann_type *row, fann_type decision_threshold);

void
misc_pause_display(player_t* player);

void
misc_clear_console(void);

void
misc_dump_train(struct fann_train_data* train);

fann_type frand(void);

void
state_set_action(input_state_t* state, int a);

void
state_setup(input_state_t* state, player_t* player);

void
state_dump(player_t* player, int action);

extern char* state_action_names[];

#define countof(x) (sizeof(x)/sizeof(x[0]))
