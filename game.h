#pragma once
#include <stdbool.h>
#include "doublefann.h"

#define GAME_ONE_LINE_MAP
//#define GAME_CHEESE_LEFT

#ifdef GAME_ONE_LINE_MAP
enum {
  ACTION_LEFT,
  ACTION_RIGHT,
  ACTION_NUM_ACTIONS
};
#else
enum {
  ACTION_LEFT,
  ACTION_RIGHT,
  ACTION_UP,
  ACTION_DOWN,
  ACTION_NUM_ACTIONS
};
#endif

#define INPUT_VALUE_PLAYER          1.0
#define INPUT_VALUE_CHEESE          0.5
#define INPUT_VALUE_PIT             0.1

#ifdef GAME_ONE_LINE_MAP

#define GAME_MAP_SIZE_X             40
#define GAME_MAP_SIZE_Y             1
#define GAME_Q_MAX_EPSILON          0.9
#define GAME_Q_REPLAY_MEMORY_SIZE   400
#define GAME_Q_REPLAY_BATCH_SIZE    40
#define GAME_Q_CONFIDENCE_THRESHOLD 0.0
#define GAME_MOVES_PER_EPISODE      100
#define GAME_NUM_PITS               1
#define GAME_POSITION_STATES
#define GAME_ACTION_OUTPUTS

#else

#define GAME_MAP_SIZE_X             5
#define GAME_MAP_SIZE_Y             5
#define GAME_Q_MAX_EPSILON          0.9
#define GAME_Q_REPLAY_MEMORY_SIZE   4000
#define GAME_Q_REPLAY_BATCH_SIZE    400
#define GAME_Q_CONFIDENCE_THRESHOLD 0.0
#define GAME_MOVES_PER_EPISODE      100
#define GAME_NUM_PITS               1
//#define GAME_MOVING_PLAYER
//#define GAME_MOVING_CHEESE
#define GAME_POSITION_STATES
//#define GAME_ACTION_OUTPUTS

#endif

//=======================================


#ifdef GAME_ACTION_OUTPUTS

#ifdef GAME_POSITION_STATES
#define PLAYER_Q_SIZEOF_STATE       ((GAME_NUM_PITS*2)+2+2)
#else
#define PLAYER_Q_SIZEOF_STATE       ((GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y))
#endif
#define GAME_NUM_OUTPUTS            ACTION_NUM_ACTIONS

#else //!GAME_ACTION_OUTPUTS

#define GAME_NUM_OUTPUTS            1
#ifdef GAME_POSITION_STATES
#define PLAYER_Q_SIZEOF_STATE       ((GAME_NUM_PITS*2)+2+2+1)
#else //!GAME_POSITION_STATES
#define PLAYER_Q_SIZEOF_STATE       ((GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y)+ACTION_NUM_ACTIONS)
#endif

#endif //GAME_ACTION_OUTPUTS


typedef struct {
  int x;
  int y;
} position_t;

typedef struct {
  int score;
  bool new_game;
  fann_type total_reward;
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
  int loops;
  bool reset_player;

  int won;
  int played;

  int total_cheese;
  int total_pit;

  fann_type current_epsilon;
  fann_type epsilon;
  fann_type max_epsilon;
  fann_type epsilon_increase_factor;
} game_t;

typedef struct {
  fann_type state[PLAYER_Q_SIZEOF_STATE];
} input_state_t;

typedef struct {
  fann_type reward;
  input_state_t next_state;
  int previous_action;
  input_state_t previous_state;
  fann_type previous_q[GAME_NUM_OUTPUTS];
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

  int replay_memory_size;
  int replay_memory_pointer;
  int replay_memory_index;
  int replay_batch_size;
  replay_memory_t replay_memory[GAME_Q_REPLAY_MEMORY_SIZE];

  int previous_score;
  replay_memory_t state;
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
misc_num_correct(player_t* player);

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

void
misc_dump_q(player_t* player);

fann_type frand(void);

#ifndef GAME_ACTION_OUTPUTS
void
state_set_action(input_state_t* state, int a);
#endif

void
state_setup(input_state_t* state, player_t* player);

void
state_dump(player_t* player, int action);

extern char* state_action_names[];

#define countof(x) (sizeof(x)/sizeof(x[0]))
