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
game_initialize(game_nn_enum_t nn_library, bool render, bool reload, int num_episodes)
{
  srand(10);
  memset(&game, 0, sizeof(game));
  memset(&player, 0, sizeof(player));

  player.game = &game;
  switch (nn_library) {
  case NN_FANN:
    player.q_nn_model = nn_fann_construct();
    break;
  case NN_KANN:
    player.q_nn_model = nn_kann_construct();
    break;
  default:
    game_error("game_initialize: unknown nn library type");
    break;
  }

  game.min_moves = 10000;
  game.render = render;
  game.reload = reload;

  game.epsilon = 0.1;
  game.max_epsilon = GAME_Q_MAX_EPSILON;
  game.epsilon_increase_factor = num_episodes*0.75;

#ifndef GAME_ONE_LINE_MAP
  game.start_position.x = 0;
  game.start_position.y = 0;
  //  game.cheese.x = GAME_MAP_SIZE_X/2;
  //  game.cheese.y = GAME_MAP_SIZE_Y/3;
  game.cheese.x = GAME_MAP_SIZE_X-1;
  game.cheese.y = GAME_MAP_SIZE_Y-2;
  game.pits[0].x = game.cheese.x-2;
  game.pits[0].y = game.cheese.y-1;

#if 0
  srand(countof(game.pits));
  for (int i = 0; i < countof(game.pits); i++) {
    do {
      game.pits[i].x = rand() % GAME_MAP_SIZE_X;
      game.pits[i].y = rand() % GAME_MAP_SIZE_Y;
    } while ((game.pits[i].x == game.cheese.x && game.pits[i].y == game.cheese.y) ||
	     (game.pits[i].x == game.cheese.x && game.pits[i].y == game.cheese.y));
  }
#else

#endif

#else



  game.start_position.x = GAME_MAP_SIZE_X/2;
  game.start_position.y = GAME_MAP_SIZE_Y/2;
#ifdef GAME_CHEESE_LEFT
  game.cheese.x = 0;
  game.cheese.y = 0;
  game.pits[0].x = GAME_MAP_SIZE_X-1;
  game.pits[0].y = GAME_MAP_SIZE_Y-1;
#else
  game.cheese.x = GAME_MAP_SIZE_X-1;
  game.cheese.y = GAME_MAP_SIZE_Y-1;
  game.pits[0].x = 0;
  game.pits[0].y = 0;
#endif

#endif
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
    game.start_position.x = player.x = rand() % GAME_MAP_SIZE_X;
    game.start_position.y = player.y = rand() % GAME_MAP_SIZE_Y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) || game_pit_collision(player.x, player.y));
#else
  player.x = game.start_position.x;
  player.y = game.start_position.y;
#endif

#ifdef GAME_MOVING_CHEESE
  do {
    game.cheese.x = rand() % GAME_MAP_SIZE_X;
    game.cheese.y = rand() % GAME_MAP_SIZE_Y;
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
    //player.x = player.x > 0 ?player.x-1 : GAME_MAP_SIZE_X-1;
    player.x = player.x > 0 ? player.x-1 : player.x;
  } else if (move == ACTION_RIGHT) {
    //    player.x = player.x < GAME_MAP_SIZE_X-1 ? player.x+1 : 0;
    player.x = player.x < GAME_MAP_SIZE_X-1 ? player.x+1 : player.x;
  }
#ifndef GAME_ONE_LINE_MAP
  else if (move == ACTION_DOWN) {
    //    player.y = player.y < GAME_MAP_SIZE_Y-1 ? player.y+1 : 0;
    player.y = player.y < GAME_MAP_SIZE_Y-1 ? player.y+1 : player.y;
  }
  else if (move == ACTION_UP) {
    //    player.y = player.y > 0 ?player.y-1 : GAME_MAP_SIZE_Y-1;
    player.y = player.y > 0 ?player.y-1 : player.y;
  }
#endif

  if (player.x == game.cheese.x && player.y == game.cheese.y) {
    game.total_cheese++;
    game.score += 1;
    game.reset_player = 1;
  }

  for (int p = 0; p < countof(game.pits); p++) {
    if (player.x == game.pits[p].x && player.y == game.pits[p].y) {
      game.total_pit++;
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

  printf("Score %d | Game %d | Last %d | Av %d | e %f\n", game.score, game.played, game.last_moves, game.average_moves, game.current_epsilon);

  for (int x = 0; x < GAME_MAP_SIZE_X+2; x++) {
    putchar('#');
  }
  putchar('\n');

  for (int y = 0; y < GAME_MAP_SIZE_Y; y++) {
    printf("#");
    for (int x = 0; x < GAME_MAP_SIZE_X; x++) {
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

  for (int x = 0; x < GAME_MAP_SIZE_X+2; x++) {
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
  number_t total = 0.0;
  for (int i = 0; i < countof(player.replay_memory); i++) {
    total += misc_q_table_row_max(player.replay_memory[i].previous_q, 0);
  }
  return total/(number_t)countof(player.replay_memory);
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
game_run(bool train, int episode_number)
{
  if (game.render) {
    game_draw();
  }


  game.total_reward = 0;

  if (!train) {
    int count = 0;
    while (game.score == 0 && count++ < GAME_MOVES_PER_EPISODE) {
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

  } else {
    game.current_epsilon = game.epsilon + ((episode_number/game.epsilon_increase_factor) > (game.max_epsilon-game.epsilon) ? (game.max_epsilon-game.epsilon) : (episode_number/game.epsilon_increase_factor));
    for (int i = 0; i < GAME_MOVES_PER_EPISODE; i++) {
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
	printf("\nGame %04d: PASS  in %4d Moves Av: %4d W/R: %03d%% Run: %6d Correct: %2d", game.played, game.moves, game.average_moves, win_percentage, game.loops, misc_num_correct(&player));
	game_print_bar_char((float)game.won/game.played, game_get_win_ratio_direction());
      } else {
	printf("\nGame: %04d R:% 7.2f AR:% 7.2f Q:% 1.2f Run: %6d e: %.2f Correct: %2d", game.played, game.total_reward, game_total_r_moving_average(), average_q, game.loops, game.current_epsilon,  misc_num_correct(&player));
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
	printf("\nGame %04d: *FAIL* in %4d Moves Av: %4d W/R: %3d%% Run: %6d Correct: %2d", game.played, game.moves, game.average_moves, win_percentage, game.loops,  misc_num_correct(&player));
	game_print_bar_char((float)game.won/game.played, game_get_win_ratio_direction());
      } else {
	printf("\nGame: %04d R:% 7.2f AR:% 7.2f Q:% 1.2f Run: %6d e: %.2f Correct: %2d", game.played, game.total_reward, game_total_r_moving_average(), average_q, game.loops, game.current_epsilon,  misc_num_correct(&player));
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
  static int render = 0;
  static int reload = 0;
  game_nn_enum_t nn_library = NN_KANN;


  while (1) {
    static struct option long_options[] = {
      {"render",     no_argument,       &render, 'd'},
      {"reload",     no_argument,       &reload, 'l'},
      {"train",      required_argument, 0, 't'},
      {"ann",        required_argument, 0, 'a'},
      {"random",     required_argument, 0, 'r'},
      {"nn_library", required_argument, 0, 'n'},
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
    case 'a':
      if (sscanf(optarg, "%d", &ann) != 1) {
	game_error("missing or incorrect argument for number of ann games");
      }
      break;
    case 'r':
      if (sscanf(optarg, "%d", &random) != 1) {
	game_error("missing or incorrect argument for number of random games");
      }
      break;
    case 't':
      if (sscanf(optarg, "%d", &train) != 1) {
	game_error("missing or incorrect argument for number of training games");
      }
      break;
    case '?':
      exit(1);
      break;
    default:
      abort ();
    }
  }


  double time_taken = 0;
  if (train) {
    printf("Training network with %d episodes using %s nn library\n", train, game_nn_enum_to_string(nn_library));
    clock_t t = clock();
    game_initialize(nn_library, render, reload, train);
    player_q_initialize(&player);
    for (int e = 0; e < train; e++) {
      game_run(true, e);
      game_reset();
      if (!player.ready) {
	e--;
      }
    }


    time_taken = ((double)(clock()-t))/CLOCKS_PER_SEC; // calculate the elapsed time

    player.q_nn_model->save(player.q_nn_model, "nn.txt");
    printf("\n");
  }

  if (ann || random) {
    int num_episodes = random ? random : ann;
    printf("Testing %d episodes using %s library\n", num_episodes,  game_nn_enum_to_string(nn_library));
    game_initialize(nn_library, render, reload, num_episodes);
    if (random) {
      player_r_initialize(&player);
    } else {
      player_nn_initialize(&player);
    }

    for (int e = 0; e < num_episodes; e++) {
      game_run(false, e);
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

  printf("C: %d P: %d\n", game.total_cheese, game.total_pit);

  return 0;
}
