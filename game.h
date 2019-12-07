#pragma once

#include "doublefann.h"
enum {
  ACTION_LEFT,
  ACTION_RIGHT,
  ACTION_UP,
  ACTION_DOWN,
  ACTION_NUM_ACTIONS
};

#define countof(x) (sizeof(x)/sizeof(x[0]))
#define GAME_MAP_SIZE_X             10
#define GAME_MAP_SIZE_Y             10
#define PLAYER_Q_REPLAY_MEMORY_SIZE 500
#define PLAYER_Q_REPLAY_BATCH_SIZE  400
#define PLAYER_SIZEOF_STATE         ((GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y)+ACTION_NUM_ACTIONS)

typedef struct {
  int x;
  int y;
} position_t;

typedef struct {
  int score;
  bool new_game;
  position_t start_position;
  position_t cheese;
  position_t pit;
  int moves;
  int run;
  int last_moves;
  int min_moves;
} game_t;

typedef struct {
  fann_type state[PLAYER_SIZEOF_STATE];
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

  fann_type discount;
  fann_type epsilon;
  fann_type max_epsilon;
  fann_type epsilon_increase_factor;
  fann_type last_e;

  int replay_memory_size;
  int replay_memory_pointer;
  replay_memory_t replay_memory[PLAYER_Q_REPLAY_MEMORY_SIZE];
  int replay_memory_index;
  int replay_batch_size;

  int runs;
  int old_score;

  input_state_t old_input_state;
  struct fann *q_nn_model;
} player_t;

typedef struct {
  input_state_t input;
  fann_type output;
} training_element_t;


void
player_q_initialize(player_t* player);

fann_type frand(void);
