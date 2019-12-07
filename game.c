#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "game.h"

static player_t player;
static game_t game;

static void
game_reset(void);

static void
game_initialize(void)
{

  player.game = &game;
  game.run = 0;
  game.min_moves = 10000;
  game.start_position.x = 4;
  game.start_position.y = 1;

  game.cheese.x = GAME_MAP_SIZE_X/2;
  game.cheese.y = GAME_MAP_SIZE_Y/3;

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
  game.pits[0].x = game.cheese.x-1;
  game.pits[0].y = game.cheese.y+2;
#endif

  srand(time(0));
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
  do {
    game.start_position.x = player.x = rand() % GAME_MAP_SIZE_X;
    game.start_position.y = player.y = rand() % GAME_MAP_SIZE_Y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) || game_pit_collision(player.x, player.y));

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
  game.run += 1;
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
   player.x =player.x > 0 ?player.x-1 : GAME_MAP_SIZE_X-1;
  } else if (move == ACTION_RIGHT) {
    player.x = player.x < GAME_MAP_SIZE_X-1 ? player.x+1 : 0;
  } else if (move == ACTION_DOWN) {
    player.y = player.y < GAME_MAP_SIZE_Y-1 ? player.y+1 : 0;
  } else if (move == ACTION_UP) {
    player.y = player.y > 0 ?player.y-1 : GAME_MAP_SIZE_Y-1;
  }

  if (player.x == game.cheese.x && player.y == game.cheese.y) {
    game.score += 1;
    game.reset_player = 1;
  }

  for (int p = 0; p < countof(game.pits); p++) {
    if (player.x == game.pits[p].x && player.y == game.pits[p].y) {
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

  printf("Score %d | Run %d | Last %d | Av %d | e %f\n", game.score, game.run, game.last_moves, game.average_moves, player.last_e);

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
}


static void
game_print_bar_char(float ratio)
{
  printf("|");
  for (int i = 0; i < 20; i++) {
    if (i <= ratio*20) {
      putchar('=');
    } else {
      putchar('.');
    }
  }
  printf("|");
}


static void
game_print_q_chart(float q)
{
  q = (q + 1.0)*10.0;

  for (int i = 0; i < 20; i++) {
    if (i == (int)q) {
      printf("Q");
    } else {
      printf(".");
    }
  }
  printf("|\n");
}


static void
game_run(void)
{
  if (game.render) {
    game_draw();
  }
  int count = 0;
  while (game.score < GAME_SCORE_HIGH && game.score > GAME_SCORE_LOW && count++ < 2000) {
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

  if (game.render) {
    game_draw();
    misc_pause_display(&player);
  }

  if (game.score >= GAME_SCORE_HIGH) {
    game.last_moves = game.moves;
    if (game.last_moves < game.min_moves) {
      game.min_moves = game.last_moves;
    }
    game.total_moves += game.moves;
    game.average_moves = game.total_moves/++game.move_count;
    game.won++;
    printf("HIT in %4d moves Av: %3d W/R: %1.02f%% Q:% 1.2f Run: %6d e: %.2f", game.moves, game.average_moves, (float)game.won/game.run, game.average_q, game.loops, player.last_e);
    game_print_bar_char((float)game.won/game.run);
    game_print_q_chart(game.average_q);
  } else {
    printf("*** in %4d moves Av: %3d W/R: %1.02f%% Q:% 1.2f Run: %6d e: %.2f", game.moves, game.average_moves, (float)game.won/game.run, game.average_q, game.loops, player.last_e);
    game_print_bar_char((float)game.won/game.run);
    game_print_q_chart(game.average_q);
    game.last_moves = -1;
  }
}


int
main(int argc, char* argv[])
{
  int c;
  int train = 100;
  int random = 0;
  int ann = 0;

  while (1) {
    static struct option long_options[] = {
      {"render",  no_argument,       &game.render, 'd'},
      {"reload",  no_argument,       &game.reload, 'l'},
      {"train",   required_argument, 0, 't'},
      {"ann",     required_argument, 0, 'a'},
      {"random",  required_argument, 0, 'r'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "ard:t:", long_options, &option_index);

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
    case 'a':
      train = 0;
      ann = atoi(optarg);
      break;
    case 'r':
      random = atoi(optarg);
      train = 0;
      break;
    case 't':
      train = atoi(optarg);
      break;
    case '?':
      exit(1);
      break;
    default:
      abort ();
    }
  }

  if (train) {
    printf("Training network with %d iterations\n", train);

    game_initialize();
    player_q_initialize(&player);
    for (int i = 0; i < train; i++) {
      if (!game.render) {
	printf("Run %04d: ", i+1);
      }
      game_run();
      game_reset();
      if (!player.ready) {
	i--;
      }
    }

    fann_save(player.q_nn_model, "nn.txt");
  } else {

    game_initialize();
    int count;
    if (random) {
      count = random;
      player_r_initialize(&player);
    } else {
      count = ann;
      player_nn_initialize(&player);
    }
    for (int i = 0; i < count; i++) {
      if (!game.render) {
	printf("Run %04d: ", i+1);
      }
      game_run();
      game_reset();
    }
  }
  return 0;
}
