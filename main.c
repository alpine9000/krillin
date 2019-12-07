#include <stdio.h>
#include <stdbool.h>
#include "game.h"

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
  puts("\e[H\e[2J");
}

static void
reset(void)
{
  player.x = game.start_position.x;
  player.y = game.start_position.y;
  game.cheese.x = 8; //rand(@map_size_x)
  game.cheese.y = 6;//#rand(@map_size_y)
  game.pit.x = 3;
  game.pit.y = 5;
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


  printf("Score %d | Run %d | Last %d | Min %d | e %f\n", game.score, game.run, game.last_moves, game.min_moves, player.last_e);

  puts("############");
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
  puts("############");
}

static void
run(void)
{
  draw();
  while (game.score < 5 && game.score > -5) {
    draw();
    gameloop();
    game.moves += 1;
  }

  // Draw one last time to update the
  draw();

  if (game.score >= 5) {
    printf("  You win in %d moves!\n", game.moves);
    game.last_moves = game.moves;
    if (game.last_moves < game.min_moves) {
      game.min_moves = game.last_moves;
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
  initialize();
  player_q_initialize(&player);
  for (int i = 0; i < 15; i++) {
    run();
    reset();
  }
  return 0;
}
