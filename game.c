#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include "game.h"

static player_t player;
static game_t game;

static void
game_reset(void);

//static
void game_error(const char* error)
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
  default:
    return "unknown";
    break;
  }
}

static void
game_initialize(game_nn_enum_t nn_library, bool render, bool reload, int num_episodes,
		number_t epsilon_gradient, number_t epsilon_max, int map_size_x, int map_size_y,
		int cheese_x, int cheese_y, int pit_x, int pit_y)
{
  srand(10);
  memset(&game, 0, sizeof(game));
  memset(&player, 0, sizeof(player));

  player.game = &game;
  switch (nn_library) {
  case NN_FANN:
    player.q_model = nn_fann_construct();
    break;
  case NN_KANN:
    player.q_model = nn_kann_construct();
    break;
  default:
    game_error("game_initialize: unknown nn library type");
    break;
  }

  game.min_moves = 10000;
  game.render = render;
  game.reload = reload;

  game.epsilon = 0.1;
  game.epsilon_max = epsilon_max;
  game.epsilon_increase_factor = num_episodes*epsilon_gradient;

  game.map_size_x = map_size_x;
  game.map_size_y = map_size_y;

  game.start_position.x = game.map_size_x/2;
  game.start_position.y = game.map_size_y/2;
  game.cheese.x = cheese_x;
  game.cheese.y = cheese_y;
  game.pits[0].x = pit_x;
  game.pits[0].y = pit_y;


  // srand(time(0));
  game_reset();
}


static bool
game_pit_collision(int x, int y)
{
  for (int i = 0; i < countof(game.pits); i++) {
    if (game.pits[i].x == x && game.pits[i].y == y) {
      return true;
    }
  }
  return false;
}


static void
game_reset(void)
{
#ifdef GAME_MOVING_PLAYER
  do {
    game.start_position.x = player.x = rand() % game.map_size_x;
    game.start_position.y = player.y = rand() % game.map_size_y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) || game_pit_collision(player.x, player.y));
#else
  player.x = game.start_position.x;
  player.y = game.start_position.y;
#endif

#ifdef GAME_MOVING_CHEESE
  do {
    game.cheese.x = rand() % game.map_size_x;
    game.cheese.y = rand() % game.map_size_y;
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
    //player.x = player.x > 0 ?player.x-1 : game.map_size_x-1;
    player.x = player.x > 0 ? player.x-1 : player.x;
  } else if (move == ACTION_RIGHT) {
    //player.x = player.x < game.map_size_x-1 ? player.x+1 : 0;
    player.x = player.x < game.map_size_x-1 ? player.x+1 : player.x;
  }
  else if (move == ACTION_DOWN) {
    //player.y = player.y < game.map_size_y-1 ? player.y+1 : 0;
    player.y = player.y < game.map_size_y-1 ? player.y+1 : player.y;
  }
  else if (move == ACTION_UP) {
    //player.y = player.y > 0 ?player.y-1 : game.map_size_y-1;
    player.y = player.y > 0 ? player.y-1 : player.y;
  }

  if (player.x == game.cheese.x && player.y == game.cheese.y) {
    game.episode_cheese++;
    game.score += 1;
    game.reset_player = 1;
  }

  for (int p = 0; p < countof(game.pits); p++) {
    if (player.x == game.pits[p].x && player.y == game.pits[p].y) {
      game.episode_pit++;
      game.score -= 1;
      game.reset_player = 1;
      break;
    }
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

  for (int x = 0; x < game.map_size_x+2; x++) {
    putchar('#');
  }
  putchar('\n');

  for (int y = 0; y < game.map_size_y; y++) {
    printf("#");
    for (int x = 0; x < game.map_size_x; x++) {
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

  for (int x = 0; x < game.map_size_x+2; x++) {
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
    total += misc_q_table_row_max(player.replay_memory[i].previous_q);
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
    for (int i = 0; i < moves_per_episode; i++) {
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
  int c;
  int train = 0;
  int random = 0;
  int ann = 0;
  int memory_size = 0;
  int batch_size = 400;
  int moves_per_episode = 200;
  int map_size_x = 20;
  int map_size_y = 1;
  int cheese_x = -1;
  int cheese_y = -1;
  int pit_x = -1;
  int pit_y = -1;
  number_t discount = 0.9;
  number_t learning_rate = 0.01;
  number_t epsilon_gradient = 0.75;
  number_t epsilon_max = 0.9;
  static int render = 0;
  static int reload = 0;
  game_nn_enum_t nn_library = NN_KANN;


  while (1) {
    static struct option long_options[] = {
      {"render",        no_argument,       &render, 'd'},
      {"reload",        no_argument,       &reload, 'l'},
      {"train",         required_argument, 0,       't'},
      {"ann",           required_argument, 0,       'a'},
      {"random",        required_argument, 0,       'r'},
      {"nn",            required_argument, 0,       'n'},
      {"memory",        required_argument, 0,       'm'},
      {"batch",         required_argument, 0,       'b'},
      {"moves",         required_argument, 0,       'p'},
      {"discount",      required_argument, 0,       'i'},
      {"learning_rate", required_argument, 0,       'z'},
      {"epsilon_gradient", required_argument, 0,    'e'},
      {"epsilon_max",   required_argument, 0,       'y'},
      {"map_size_x",    required_argument, 0,       '1'},
      {"map_size_y",    required_argument, 0,       '2'},
      {"cheese_x",      required_argument, 0,       '3'},
      {"cheese_y",      required_argument, 0,       '4'},
      {"pit_x",         required_argument, 0,       '5'},
      {"pit_y",         required_argument, 0,       '6'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "d", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
	break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
	printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'n':
      if (strcmp(optarg, "fann") == 0) {
	nn_library = NN_FANN;
      } else if (strcmp(optarg, "kann") == 0) {
	nn_library = NN_KANN;
      } else {
	game_error("unknown nn library");
      }
      break;
    case 'p':
      if (sscanf(optarg, "%d", &moves_per_episode) != 1) {
	game_error("missing or incorrect argument for moves per episode");
      }
      break;
    case 'm':
      if (sscanf(optarg, "%d", &memory_size) != 1) {
	game_error("missing or incorrect argument for memory size");
      }
      break;
    case 'b':
      if (sscanf(optarg, "%d", &batch_size) != 1) {
	game_error("missing or incorrect argument for batch size");
      }
      break;
    case 'a':
      if (sscanf(optarg, "%d", &ann) != 1) {
	game_error("missing or incorrect argument for number of ann games");
      }
      break;
    case 'i':
      if (sscanf(optarg, "%f", &discount) != 1) {
	game_error("missing or incorrect argument for discount");
      }
      break;
    case 'r':
      if (sscanf(optarg, "%d", &random) != 1) {
	game_error("missing or incorrect argument for number of random games");
      }
      break;
    case 'z':
      if (sscanf(optarg, "%f", &learning_rate) != 1) {
	game_error("missing or incorrect argument for learning rate");
      }
      break;
    case 'e':
      if (sscanf(optarg, "%f", &epsilon_gradient) != 1) {
	game_error("missing or incorrect argument for epsilon_gradient");
      }
      break;
    case 'y':
      if (sscanf(optarg, "%f", &epsilon_max) != 1) {
	game_error("missing or incorrect argument for epsilon_max");
      }
      break;
    case 't':
      if (sscanf(optarg, "%d", &train) != 1) {
	game_error("missing or incorrect argument for number of training games");
      }
      break;
    case '1':
      if (sscanf(optarg, "%d", &map_size_x) != 1) {
	game_error("missing or incorrect argument for map_size_x");
      }
      break;
    case '2':
      if (sscanf(optarg, "%d", &map_size_y) != 1) {
	game_error("missing or incorrect argument for map_size_y");
      }
      break;
    case '3':
      if (sscanf(optarg, "%d", &cheese_x) != 1) {
	game_error("missing or incorrect argument for cheese_x");
      }
      break;
    case '4':
      if (sscanf(optarg, "%d", &cheese_y) != 1) {
	game_error("missing or incorrect argument for cheese_y");
      }
      break;
    case '5':
      if (sscanf(optarg, "%d", &pit_x) != 1) {
	game_error("missing or incorrect argument for pit_x");
      }
      break;
    case '6':
      if (sscanf(optarg, "%d", &pit_y) != 1) {
	game_error("missing or incorrect argument for pit_y");
      }
      break;

    case '?':
      exit(1);
      break;
    default:
      abort ();
    }
  }

  if (cheese_x < 0) {
    cheese_x = 0;
  }

  if (cheese_y < 0) {
    cheese_y = 0;
  }

  if (pit_x < 0) {
    pit_x = map_size_x-1;
  }

  if (pit_y < 0) {
    pit_y = map_size_y-1;
  }

  double time_taken = 0;
  if (train) {
    memory_size = memory_size ? memory_size : (moves_per_episode*train);
    if (memory_size < batch_size) {
      memory_size = batch_size;
    }

    printf("\nConfig:\n\ttrain: %d, random: %d, ann: %d, memory_size(memory): %d, batch_size(batch): %d\n",
	   train, random, ann, memory_size, batch_size);
    printf("\tmoves_per_episode(moves): %d, discount: %f, epsilon_max: %f, epsilon_gradient: %f, learning_rate: %f, library(nn): %s\n", moves_per_episode, discount, epsilon_max, epsilon_gradient, learning_rate, game_nn_enum_to_string(nn_library));
    printf("\tmap_size_x: %d, map_size_y %d\n", map_size_x, map_size_y);
    printf("\tcheese_x: %d, cheese_y %d\n", cheese_x, cheese_y);
    printf("\tpit_x: %d, pit_y %d\n\n", pit_x, pit_y);


    clock_t t = clock();
    game_initialize(nn_library, render, reload, train, epsilon_gradient, epsilon_max, map_size_x, map_size_y,
		    cheese_x, cheese_y, pit_x, pit_y);
    player_q_initialize(&player, memory_size ? memory_size : batch_size*train, batch_size, discount, learning_rate);
    for (int e = 0; e < train; e++) {
      episode_run(true, e, moves_per_episode);
      game_reset();
      if (!player.ready) {
	e--;
      }
    }


    time_taken = ((double)(clock()-t))/CLOCKS_PER_SEC; // calculate the elapsed time

    player.q_model->save(player.q_model, "nn.txt");
    printf("\n");
  }

  if (ann || random) {
    int num_episodes = random ? random : ann;
    game_initialize(nn_library, render, reload, num_episodes, epsilon_gradient, epsilon_max,
		    map_size_x, map_size_y, cheese_x, cheese_y, pit_x, pit_y);
    if (random) {
      player_r_initialize(&player);
    } else {
      player_nn_initialize(&player);
    }

    for (int e = 0; e < num_episodes; e++) {
      episode_run(false, e, moves_per_episode);
      game_reset();
    }
  }

  printf("\n");

  if (train) {
    printf("\nTraining time: %0.2lfs", time_taken);
  }

  if (ann || random) {
    if (game.played) {
      printf("\nRuns: %d, W/R: %d%% Average Moves: %d\n", game.played, (int)((float)(100*game.won)/game.played), game.average_moves);
    }
  }

  printf("\n");

  return 0;
}
