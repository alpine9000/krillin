#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nn_kann.h"
#include "nn_fann.h"

#define GAME_ONE_LINE_MAP
//#define GAME_CHEESE_LEFT

typedef enum {
  NN_FANN,
  NN_KANN
} game_nn_enum_t;

typedef float number_t;

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

#define GAME_MAP_SIZE_X             20
#define GAME_MAP_SIZE_Y             1
#define GAME_Q_MAX_EPSILON          0.9
#define GAME_Q_REPLAY_MEMORY_SIZE   400
#define GAME_Q_REPLAY_BATCH_SIZE    40
#define GAME_Q_CONFIDENCE_THRESHOLD 0.0
#define GAME_MOVES_PER_EPISODE      100
#define GAME_NUM_PITS               1
#define GAME_POSITION_STATES
//#define GAME_ACTION_OUTPUTS

#else

#define GAME_MAP_SIZE_X             5
#define GAME_MAP_SIZE_Y             5
#define GAME_Q_MAX_EPSILON          0.9
#define GAME_Q_REPLAY_MEMORY_SIZE   200000
#define GAME_Q_REPLAY_BATCH_SIZE    400
#define GAME_Q_CONFIDENCE_THRESHOLD 0.0
#define GAME_MOVES_PER_EPISODE      1000
#define GAME_NUM_PITS               1
//#define GAME_MOVING_PLAYER
//#define GAME_MOVING_CHEESE
#define GAME_POSITION_STATES
//#define GAME_ACTION_OUTPUTS

#endif

// == nothing more to configure from here ==


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
  game_nn_enum_t nn_type;
  int score;
  bool new_game;
  number_t total_reward;
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

  number_t current_epsilon;
  number_t epsilon;
  number_t max_epsilon;
  number_t epsilon_increase_factor;
} game_t;

typedef struct {
  number_t state[PLAYER_Q_SIZEOF_STATE];
} input_state_t;

typedef struct {
  number_t reward;
  input_state_t next_state;
  int previous_action;
  input_state_t previous_state;
  number_t previous_q[GAME_NUM_OUTPUTS];
} replay_memory_t;

typedef struct {
  void* _private_data;
} nn_training_data_t;

typedef struct nn {
  void (*create_network)(struct nn* nn, int num_input_neurons, int num_hidden_neurons, int num_output_neurons);
  nn_training_data_t* (*create_training)(struct nn* nn, int num_data, int num_input, int num_output);
  void (*set_training_input_data)(nn_training_data_t* train, int sample_index, int neuron_index, number_t value);
  void (*set_training_output_data)(nn_training_data_t* train, int sample_index, int neuron_index, number_t value);
  void (*train)(struct nn* model, nn_training_data_t* train, int num_epochs);
  const number_t* (*run)(struct nn* model, number_t* state);
  void (*load)(struct nn* nn, const char* filename);
  void (*save)(struct nn* nn, const char* filename);
  void (*dump_train)(nn_training_data_t* train);
  void* _private_data;
} nn_t;


typedef struct player{
  game_t* game;
  int (*get_input)(struct player* player);

  int x;
  int y;
  int actions[ACTION_NUM_ACTIONS];
  bool first_run;
  int runs;
  bool ready;

  number_t discount;

  int replay_memory_size;
  int replay_memory_pointer;
  int replay_memory_index;
  int replay_batch_size;
  replay_memory_t replay_memory[GAME_Q_REPLAY_MEMORY_SIZE];

  int previous_score;
  replay_memory_t state;
  nn_t *q_nn_model;
  nn_training_data_t* train;
} player_t;


nn_t*
nn_kann_construct(void);

nn_t*
nn_fann_construct(void);

void
player_q_initialize(player_t* player);

void
player_nn_initialize(player_t* player);

void
player_r_initialize(player_t* player);

int
misc_num_correct(player_t* player);

int
misc_q_table_row_max_index(const number_t *row, number_t decision_threshold);

number_t
misc_q_table_row_max(const number_t *row, number_t decision_threshold);

void
misc_pause_display(player_t* player);

void
misc_clear_console(void);

void
misc_dump_q(player_t* player);

number_t frand(void);

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
