#include <stdio.h>
#include <string.h>
#include <time.h>
#include "game.h"

static player_t player;
static game_t game;

static void
game_reset(void);

void
game_error(const char* error)
{
  fprintf(stderr, "error: %s\n", error);
  exit(1);
}


static const char*
game_nn_enum_to_string(game_nn_enum_t nn_library)
{
 switch (nn_library) {
  case NN_FANN:
    return "fann";
    break;
  case NN_KANN:
    return "kann";
    break;
 case NN_Q_TABLE:
   return "q";
   break;
 default:
    return "unknown";
    break;
  }
}

static void
game_initialize(config_t* config)
{
  srand(10);
  memset(&game, 0, sizeof(game));
  memset(&player, 0, sizeof(player));
  game.config = *config;

  player.game = &game;
  switch (config->nn_library) {
  case NN_FANN:
    player.q_model = nn_fann_construct();
    break;
  case NN_KANN:
    player.q_model = nn_kann_construct();
    break;
#ifndef GAME_ACTION_OUTPUTS
  case NN_Q_TABLE:
    player.q_model = nn_q_table_construct();
    break;
#endif
  default:
    game_error("game_initialize: unknown nn library type");
    break;
  }

  game.min_moves = 10000;
  game.render = config->render;
  game.reload = config->reload;

  game.epsilon = 0.1;
  game.epsilon_max = config->epsilon_max;
  game.epsilon_increase_factor = config->num_episodes*config->epsilon_gradient;

  game.start_position.x = game.config.map_size_x/2;
  game.start_position.y = game.config.map_size_y/2;
  game.cheese.x = config->cheese_x;
  game.cheese.y = config->cheese_y;
  game.pit.x = config->pit_x;
  game.pit.y = config->pit_y;


  // srand(time(0));
  game_reset();
}


static bool
game_pit_collision(int x, int y)
{
  if (game.pit.x == x && game.pit.y == y) {
    return true;
  }
  return false;
}


static void
game_reset(void)
{
#ifdef GAME_MOVING_PLAYER
  do {
    game.start_position.x = player.x = rand() %game.config.map_size_x;
    game.start_position.y = player.y = rand() %game.config.map_size_y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) || game_pit_collision(player.x, player.y));
#else
  player.x = game.start_position.x;
  player.y = game.start_position.y;
#endif

#ifdef GAME_MOVING_CHEESE
  do {
    game.cheese.x = rand() %game.config.map_size_x;
    game.cheese.y = rand() %game.config.map_size_y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) || game_pit_collision(game.cheese.x, game.cheese.y));
#endif

  if (!player.ready) {
    player.runs = 0;
  }
  game.score = 0;

  game.moves = 0;
  game.new_game = true;
}


static void
game_loop(void)
{
  if (player.ready) {
    game.loops++;
  }

  int move = player.get_input(&player);

  if (move == ACTION_LEFT) {
    //player.x = player.x > 0 ?player.x-1 :game.config.map_size_x-1;
    player.x = player.x > 0 ? player.x-1 : player.x;
  } else if (move == ACTION_RIGHT) {
    //player.x = player.x <game.config.map_size_x-1 ? player.x+1 : 0;
    player.x = player.x <game.config.map_size_x-1 ? player.x+1 : player.x;
  }
  else if (move == ACTION_DOWN) {
    //player.y = player.y <game.config.map_size_y-1 ? player.y+1 : 0;
    player.y = player.y <game.config.map_size_y-1 ? player.y+1 : player.y;
  }
  else if (move == ACTION_UP) {
    //player.y = player.y > 0 ?player.y-1 :game.config.map_size_y-1;
    player.y = player.y > 0 ? player.y-1 : player.y;
  }

  if (player.x == game.cheese.x && player.y == game.cheese.y) {
    game.episode_cheese++;
    game.score += 1;
    game.reset_player = 1;
  }

  if (player.x == game.pit.x && player.y == game.pit.y) {
    game.episode_pit++;
    game.score -= 1;
    game.reset_player = 1;
  }

  if (game.new_game) {
    game.new_game = false;
  }
}


static void
game_draw(void)
{
  misc_clear_console();

  printf("Score %d | Episode %d | Last %d | Av %d | e %f\n", game.score, game.played, game.last_moves, game.average_moves, game.current_epsilon);

  for (int x = 0; x <game.config.map_size_x+2; x++) {
    putchar('#');
  }
  putchar('\n');

  for (int y = 0; y <game.config.map_size_y; y++) {
    printf("#");
    for (int x = 0; x <game.config.map_size_x; x++) {
      if (game_pit_collision(x, y)) {
	if (player.x == x && player.y == y)  {
	  putchar('X');
	} else {
	  putchar('O');
	}
      } else if (player.x == x && player.y == y) {
	if (game.cheese.x == x && game.cheese.y == y) {
	  putchar('*');
	} else {
	  putchar('P');
	}
      } else if (game.cheese.x == x && game.cheese.y == y) {
	putchar('C');
      } else {
	putchar('=');
      }
    }
    puts("#");
  }

  for (int x = 0; x <game.config.map_size_x+2; x++) {
    putchar('#');
  }
  putchar('\n');

  misc_pause_display(&player);
}


static void
game_print_bar_char(float ratio, int direction)
{
  printf("|");
  for (int i = 0; i < 20; i++) {
    if (i <= ratio*20) {
      if (direction > 0) {
	putchar('>');
      } else if (direction < 0) {
	putchar('<');
      } else {
	putchar('=');
      }
    } else {
      putchar('.');
    }
  }
  printf("|");
}


static void
game_print_q_chart(float q)
{
  q = (q + 1.0)*20.0;
  printf("|");
  for (int i = 0; i < 40; i++) {
    if (i == (int)q) {
      printf("Q");
    } else {
      printf(".");
    }
  }
  printf("|");
}

static int
game_get_win_ratio_direction(void)
{
  static float last_win_ratio = 0;
  static int win_direction = 0;
  float win_ratio = (float)game.won/game.played;
  if (win_ratio != last_win_ratio) {
    if (win_ratio > last_win_ratio) {
      win_direction = 1;
    } else if (win_ratio < last_win_ratio) {
      win_direction = -1;
    } else {
      win_direction = 0;
    }
    last_win_ratio = win_ratio;
  }
  return win_direction;
}

number_t
game_average_q(void)
{
  if (!player.replay_memory_size) {
    return 0;
  }

  number_t total = 0.0;
  for (int i = 0; i < player.replay_memory_size; i++) {
    total += misc_q_table_row_max(player.replay_memory[i].previous_q, GAME_NUM_OUTPUTS);
  }
  return total/(number_t)player.replay_memory_size;
}

number_t
game_total_r_moving_average(void)
{
  static number_t window[50] = {0};
  static int index = 0;

  window[index] = game.total_reward;
  index++;
  if (index >= countof(window)) {
    index = 0;
  }

  number_t average = 0;
  for (int i = 0; i < countof(window); i++) {
    average += window[i];
  }

  return average/(number_t)countof(window);
}

static void
episode_run(bool train, int episode_number, int moves_per_episode)
{
  if (game.render) {
    game_draw();
  }


  game.total_reward = 0;

  if (!train) {
    game.render_pause_time = 0.5*1000000;
    game.episode_cheese = 0;
    game.episode_pit = 0;
    int count = 0;
    while (game.score == 0 && count++ < moves_per_episode) {
      if (game.render) {
	game_draw();
      }
      if (game.reset_player) {
	player.x = game.start_position.x;
	player.y = game.start_position.y;
	game.reset_player = 0;
	game.new_game = true;
      }
      game_loop();
      game.moves += 1;
    }

  } else {
    game.render_pause_time = 0;
    game.current_epsilon = game.epsilon + ((episode_number/game.epsilon_increase_factor) > (game.epsilon_max-game.epsilon) ? (game.epsilon_max-game.epsilon) : (episode_number/game.epsilon_increase_factor));
#ifdef GAME_TARGET_MODEL
    player_q_copy_target_model(&player);
#endif
    game.episode_cheese = 0;
    game.episode_pit = 0;
    //    for (int i = 0; i < moves_per_episode; i++) {
    for (; game.score < 20 && game.score > -20;) {
      if (game.render) {
	game_draw();
      }
      if (game.reset_player) {
	player.x = game.start_position.x;
	player.y = game.start_position.y;
	game.reset_player = 0;
      }
      game_loop();
      game.moves += 1;
    }
  }

  if (player.ready) {
    game.played++;
  }

  if (game.render) {
    game_draw();
  }


  const number_t average_q = game_average_q();
  if (game.score > 0) {
    game.last_moves = game.moves;
    if (game.last_moves < game.min_moves) {
      game.min_moves = game.last_moves;
    }


    if (player.ready) {
      game.total_moves += game.moves;
      game.average_moves = game.total_moves/++game.move_count;
      game.won++;

      if (!train) {
	int win_percentage = (int)((float)(100*game.won)/game.played);
	printf("\nEpisode: %04d:  PASS  in %4d Moves Av: %4d W/R: %3d%% Run: %6d Correct: %2d", game.played, game.moves, game.average_moves, win_percentage, game.loops, misc_num_correct(&player));
	game_print_bar_char((float)game.won/game.played, game_get_win_ratio_direction());
      } else {
	printf("\nEpisode: %04d C: %2d P: %2d R:% 7.2f AR:% 7.2f Q:% 1.2f Run: %6d e: %.2f Correct: %2d", game.played, game.episode_cheese, game.episode_pit, game.total_reward, game_total_r_moving_average(), average_q, game.loops, game.current_epsilon,  misc_num_correct(&player));
	game_print_q_chart(average_q);
	misc_dump_q(&player);
      }


    } else {
      printf(".");
    }
  } else {
    if (player.ready) {
      if (!train) {
	int win_percentage = (int)((float)(100*game.won)/game.played);
	printf("\nEpisode: %04d: *FAIL* in %4d Moves Av: %4d W/R: %3d%% Run: %6d Correct: %2d", game.played, game.moves, game.average_moves, win_percentage, game.loops,  misc_num_correct(&player));
	game_print_bar_char((float)game.won/game.played, game_get_win_ratio_direction());
      } else {
	printf("\nEpisode: %04d C: %2d P: %2d R:% 7.2f AR:% 7.2f Q:% 1.2f Run: %6d e: %.2f Correct: %2d", game.played, game.episode_cheese, game.episode_pit, game.total_reward, game_total_r_moving_average(), average_q, game.loops, game.current_epsilon,  misc_num_correct(&player));
	game_print_q_chart(average_q);
	misc_dump_q(&player);
      }

    } else {
      printf(".");
      fflush(stdout);
    }
    game.last_moves = -1;
  }
}


int
main(int argc, char* argv[])
{
  config_t config;

  config_parse_args(&config, argc, argv);

  double time_taken = 0;
  if (config.train) {
    config.memory_size = config.memory_size ? config.memory_size : (config.moves_per_episode*config.train*2);
    if (config.memory_size < config.batch_size) {
      config.memory_size = config.batch_size;
    }
    config.num_episodes = config.train;

    printf("\nConfig:\n\ttrain: %d, random: %d, ann: %d, memory_size(memory): %d, batch_size(batch): %d\n",
	   config.train, config.random, config.ann, config.memory_size, config.batch_size);
    printf("\tmoves_per_episode(moves): %d, discount: %f, epsilon_max: %f, epsilon_gradient: %f, learning_rate: %f, library(nn): %s\n", config.moves_per_episode, config.discount, config.epsilon_max, config.epsilon_gradient, config.learning_rate, game_nn_enum_to_string(config.nn_library));
    printf("\tmap_size_x: %d, map_size_y %d\n", config.map_size_x, config.map_size_y);
    printf("\tcheese_x: %d, cheese_y %d\n", config.cheese_x, config.cheese_y);
    printf("\tpit_x: %d, pit_y %d\n\n", config.pit_x, config.pit_y);


    clock_t t = clock();
    game_initialize(&config);
    player_q_initialize(&player, config.memory_size ? config.memory_size : config.batch_size*config.train, config.batch_size, config.discount, config.learning_rate);
    for (int e = 0; e < config.num_episodes; e++) {
      episode_run(true, e, config.moves_per_episode);
      game_reset();
      if (!player.ready) {
	e--;
      }
    }


    time_taken = ((double)(clock()-t))/CLOCKS_PER_SEC; // calculate the elapsed time

    player.q_model->save(player.q_model, "nn.txt");
    printf("\n");
  }

  if (config.ann || config.random) {
    config.num_episodes = config.random ? config.random : config.ann;
    game_initialize(&config);
    if (config.random) {
      player_r_initialize(&player);
    } else {
      player_nn_initialize(&player);
    }

    for (int e = 0; e < config.num_episodes; e++) {
      episode_run(false, e, config.moves_per_episode);
      game_reset();
    }
  }

  printf("\n");

  if (config.train) {
    printf("\nTraining time: %0.2lfs", time_taken);
  }

  if (config.ann || config.random) {
    if (game.played) {
      printf("\nRuns: %d, W/R: %d%% Average Moves: %d\n", game.played, (int)((float)(100*game.won)/game.played), game.average_moves);
    }
  }

  printf("\n");

  return 0;
}
