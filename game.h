#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum {
  NN_FANN,
  NN_KANN
} game_nn_enum_t;

typedef float number_t;

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

#define GAME_TARGET_MODEL
#define GAME_MAX_STATE_SIZE         ((10*10)+ACTION_NUM_ACTIONS)
#define GAME_Q_CONFIDENCE_THRESHOLD 0.0
#define GAME_NUM_PITS               1
//#define GAME_MOVING_PLAYER
//#define GAME_MOVING_CHEESE
#define GAME_POSITION_STATES
//#define GAME_ACTION_OUTPUTS


// == nothing more to configure from here ==


#ifdef GAME_ACTION_OUTPUTS

#define GAME_NUM_OUTPUTS            ACTION_NUM_ACTIONS

#else

#define GAME_NUM_OUTPUTS            1

#endif



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
  long render_pause_time;
  int reload;
  int loops;
  int map_size_x;
  int map_size_y;
  bool reset_player;

  int won;
  int played;

  int episode_cheese;
  int episode_pit;


  number_t current_epsilon;
  number_t epsilon;
  number_t epsilon_max;
  number_t epsilon_increase_factor;
} game_t;

typedef struct {
  number_t state[GAME_MAX_STATE_SIZE];
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
  void (*create_network)(struct nn* nn, int num_input_neurons, int num_hidden_neurons, int num_output_neurons, number_t learning_rate);
  nn_training_data_t* (*create_training)(struct nn* nn, int num_data, int num_input, int num_output);
  void (*set_training_input_data)(nn_training_data_t* train, int sample_index, int neuron_index, number_t value);
  void (*set_training_output_data)(nn_training_data_t* train, int sample_index, int neuron_index, number_t value);
  void (*train)(struct nn* model, nn_training_data_t* train, int num_epochs);
  const number_t* (*run)(struct nn* model, number_t* state);
  void (*load)(struct nn* nn, const char* filename);
  void (*save)(struct nn* nn, const char* filename);
  void (*dump_train)(nn_training_data_t* train);
  struct nn* (*clone)(struct nn* nn);
  void (*destroy)(struct nn* nn);
  void* _private_data;
  number_t learning_rate;
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
  number_t learning_rate;

  int replay_memory_size;
  int replay_memory_pointer;
  int replay_memory_index;
  int replay_batch_size;
  replay_memory_t* replay_memory;
  int state_size;

  int previous_score;
  replay_memory_t state;
  nn_t *q_model;
#ifdef GAME_TARGET_MODEL
  nn_t *q_target_model;
#endif
  nn_training_data_t* train;
} player_t;


nn_t*
nn_kann_construct(void);

nn_t*
nn_fann_construct(void);

void
player_q_initialize(player_t* player, int memory_size, int batch_size, number_t discount, number_t learning_rate);

void
player_nn_initialize(player_t* player);

void
player_r_initialize(player_t* player);

void
player_q_copy_target_model(player_t* player);

int
misc_num_correct(player_t* player);

int
misc_q_table_row_max_index(const number_t *row, number_t decision_threshold);

number_t
misc_q_table_row_max(const number_t *row);

void
misc_pause_display(player_t* player);

void
misc_clear_console(void);

void
misc_dump_q(player_t* player);

number_t frand(void);

#ifndef GAME_ACTION_OUTPUTS
void
state_set_action(player_t* player, input_state_t* state, int a);
#endif

void
state_setup(input_state_t* state, player_t* player);

void
state_dump(input_state_t* state, player_t* player, int action);

extern char* state_action_names[];

#define countof(x) (sizeof(x)/sizeof(x[0]))
