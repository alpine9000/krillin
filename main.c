#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "game.h"
#include <getopt.h>

static player_t player;
static game_t game;

static void
reset(void);

static void
initialize(void)
{
  player.game = &game;
  game.run = 0;
  game.min_moves = 10000;
  game.start_position.x = 4;
  game.start_position.y = 1;
  reset();

  // Clear the console
  // puts("\e[H\e[2J");
}

static void
reset(void)
{

#if 1


  game.cheese.x = GAME_MAP_SIZE_X/2; //rand(@map_size_x)
  game.cheese.y = GAME_MAP_SIZE_Y/3;//#rand(@map_size_y)
  game.pit.x = GAME_MAP_SIZE_X/4;
  game.pit.y = GAME_MAP_SIZE_Y*0.75;

#else
  game.cheese.x = rand() % GAME_MAP_SIZE_X;
  game.cheese.y = rand() % GAME_MAP_SIZE_Y;

  do {
    game.pit.x = rand() % GAME_MAP_SIZE_X;//game.start_position.x;
    game.pit.y = rand() % GAME_MAP_SIZE_Y;//game.start_position.y;
  } while ((game.pit.x == game.cheese.x && game.pit.y == game.cheese.y) ||
	   (game.pit.x == game.cheese.x && game.pit.y == game.cheese.y));
#endif

#if 1
  do {
    game.start_position.x = player.x = rand() % GAME_MAP_SIZE_X;//game.start_position.x;
    game.start_position.y = player.y = rand() % GAME_MAP_SIZE_Y;//game.start_position.y;
  } while ((player.x == game.cheese.x && player.y == game.cheese.y) ||
	   (player.x == game.pit.x && player.y == game.pit.y));
#else
  player.x = game.start_position.x;
  player.y = game.start_position.y;
#endif

  player.runs = 0;
  game.score = 0;
  game.run += 1;
  game.moves = 0;
  game.new_game = true;
}

static void
gameloop(void)
{
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
    player.x = game.start_position.x;
    player.y = game.start_position.y;
    //@cheese_x = rand(GAME_MAP_SIZE_X)
    //@cheese_y = rand(GAME_MAP_SIZE_Y)
  }

  if (player.x == game.pit.x && player.y == game.pit.y) {
    game.score -= 1;
    player.x = game.start_position.x;
    player.y = game.start_position.y;
  }


  if (game.new_game) {
    game.new_game = false;// No longer a new game after the first input has been received
  }
}

static void
draw(void)
{
  //Clear the console
  puts("\e[H\e[2J");


  printf("Score %d | Run %d | Last %d | Av %d | e %f\n", game.score, game.run, game.last_moves, game.average_moves, player.last_e);

  for (int x = 0; x < GAME_MAP_SIZE_X+2; x++) {
    putchar('#');
  }
  putchar('\n');
  //Compute map line
  for (int y = 0; y < GAME_MAP_SIZE_Y; y++) {
    printf("#");
    for (int x = 0; x < GAME_MAP_SIZE_X; x++) {
      if (player.x == x && player.y == y) {
	putchar('P');
      } else if (game.cheese.x == x && game.cheese.y == y) {
	putchar('C');
      } else if (game.pit.x == x && game.pit.y == y) {
	putchar('O');
      } else {
	putchar('=');
      }
    }
    puts("#");
    // Draw to console
  }

  for (int x = 0; x < GAME_MAP_SIZE_X+2; x++) {
    putchar('#');
  }
  putchar('\n');
}

static void
run(void)
{
  if (game.render) {
    draw();
  }
  int count = 0;
  while (game.score < 5 && game.score > -5 && count++ < 2000) {
    if (game.render) {
      draw();
    }
    gameloop();
    game.moves += 1;
  }

  if (game.render) {
    // Draw one last time to update the
    draw();
  }

  if (game.score >= 5) {

    game.last_moves = game.moves;
    if (game.last_moves < game.min_moves) {
      game.min_moves = game.last_moves;
    }
    game.total_moves += game.moves;
    game.average_moves = game.total_moves/++game.move_count;
    if (player.q_nn_model) {
      printf("  You win in %d moves Av: %d MSE %f\n", game.moves, game.average_moves, fann_get_MSE(player.q_nn_model));
    } else {
      printf("  You win in %d moves\n", game.moves);
    }
  } else {
    puts("  Game over");
    game.last_moves = -1;
  }
}

fann_type
frand(void)
{
  fann_type r = ((fann_type)rand()/(float)(RAND_MAX/(fann_type)1.0));
  return r;
}

int
main(int argc, char* argv[])
{
  int c;
  int train = 100;
  int random = 0;

  while (1) {
    static struct option long_options[] = {
	/* These options set a flag. */
      {"ann",     no_argument,       0, 'a'},
      {"random",  no_argument,       0, 'r'},
      {"render",  no_argument,       0, 'd'},
      {"reload",  no_argument,       0, 'l'},
      {"train",   required_argument, 0, 't'},
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
      break;
    case 'r':
      random = 1;
      train = 0;
      break;
    case 'd':
      game.render = 1;
      break;
    case 'l':
      game.reload = 1;
      break;
    case 't':
      train = atoi(optarg);
      break;
    case '?':
          /* getopt_long already printed an error message. */
      break;

    default:
      abort ();
    }
  }

  if (train) {
    printf("Training network with %d iterations\n", train);

    initialize();
    player_q_initialize(&player);
    for (int i = 0; i < train; i++) {
      if (!game.render) {
	printf("Run %d: ", i);
      }
      run();
      reset();
    }

    fann_save(player.q_nn_model, "nn.txt");
  } else {

    initialize();
    if (random) {
      player_r_initialize(&player);
    } else {
      player_nn_initialize(&player);
    }
    for (int i = 0; i < 15; i++) {
      run();
      reset();
    }
  }
  return 0;
}
