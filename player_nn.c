#include <string.h>
#include <stdlib.h>
#include "game.h"

static int
player_nn_get_input(player_t* player);

void
player_nn_initialize(player_t* player)
{
  player->ready = true;
  player->get_input = player_nn_get_input;
  //  player->x = 0;
  //  player->y = 0;
  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }
  player->first_run = true;

  player->runs = 0;
  player->old_score = 0;
}

static void
player_nn_initialize_neural_network(player_t* player)
{
  // Setup model

  player->q_nn_model = fann_create_from_file("nn.txt");

}


static int
player_nn_get_input(player_t* player)
{
  misc_pause_display(player);
  player->runs += 1;

  if (player->first_run) {
    // If this is first run initialize the Q-neural network
    player_nn_initialize_neural_network(player);
    player->first_run = false;
  }

  int action_taken_index;
  if (0 && frand() > 0.95) {
    action_taken_index = frand() * ACTION_NUM_ACTIONS;
  } else {
    // Capture current state
    // Set input to network map_size_x * map_size_y + actions length vector with a 1 on the player position
    input_state_t input_state = {0};// = Array.new(player->game->map_size_x*player->game->map_size_y + @actions.length, 0)
    input_state.state[player->x + (GAME_MAP_SIZE_X*player->y)] = INPUT_VALUE_PLAYER;
    input_state.state[player->game->cheese.x + + (GAME_MAP_SIZE_X*player->game->cheese.y)] = INPUT_VALUE_CHEESE;
    for (int p = 0; p < countof(player->game->pits); p++) {
      input_state.state[player->game->pits[p].x + + (GAME_MAP_SIZE_X*player->game->pits[p].y)] = INPUT_VALUE_PIT;
    }

    fann_type q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      // Create neural network input vector for this action
      input_state_t input_state_action = input_state;
      // Set a 1 in the action location of the input vector
      input_state_action.state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + a] = 1;
      // Run the network for this action and get q table row entry
      q_table_row[a] = fann_run(player->q_nn_model, input_state_action.state)[0];
    }
    action_taken_index = misc_q_table_row_max_index(q_table_row);
  }


  // Take action
  return player->actions[action_taken_index];
}
