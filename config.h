#pragma once

typedef enum {
  MISC_DISPLAY_NONE,
  MISC_DISPLAY_DETAILED_Q,
  MISC_DISPLAY_DIRECTION_Q,
} misc_debug_display_t;

typedef struct {
  int train;
  int random;
  int ann;
  int num_episodes;
  int memory_size;
  int batch_size;
  int moves_per_episode;
  int map_size_x;
  int map_size_y;
  int cheese_x;
  int cheese_y;
  int pit_x;
  int pit_y;
  number_t discount;
  number_t learning_rate;
  number_t epsilon_gradient;
  number_t epsilon_max;
  int render;
  int reload;
  game_nn_enum_t nn_library;
  misc_debug_display_t misc_q_debug;
} config_t;
