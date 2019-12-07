#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"

static int
player_q_get_input(player_t* player);

void
player_q_initialize(player_t* player)
{

  player->get_input = player_q_get_input;
  //  player->x = 0;
  //  player->y = 0;
  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }
  player->first_run = true;

  player->discount = 0.9;
  player->epsilon = 0.1;
  player->max_epsilon = 0.9;

  //  player->epsilon_increase_factor = 1200.0; //800
  player->epsilon_increase_factor = 400.0; //800

  player->replay_memory_size = PLAYER_Q_REPLAY_MEMORY_SIZE;
  player->replay_memory_pointer = 0;
  player->replay_memory_index = 0;
  player->replay_batch_size = PLAYER_Q_REPLAY_BATCH_SIZE;

  player->runs = 0;
  player->old_score = 0;
}

static void
initialize_q_neural_network(player_t* player)
{
  // Setup model
  // Input is the size of the map + number of actions
  // Output size is one

  if (player->game->reload) {
    printf("reloading...\n");
    player->q_nn_model = fann_create_from_file("nn.txt");
  }  else {
    player->q_nn_model = fann_create_standard(3, PLAYER_SIZEOF_STATE, PLAYER_SIZEOF_STATE, 1);
    fann_set_learning_rate(player->q_nn_model, 0.2);
    fann_set_activation_function_hidden(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
  }

  player->train = fann_create_train(player->replay_batch_size, // num_data
				      PLAYER_SIZEOF_STATE, // num_input
				      1); //num_output
}


static bool
repeated_index(int index, int* indexes, int num_indexes)
{
  for (int i = 0; i < num_indexes; i++) {
    if (indexes[i] == index) {
      return true;
    }
  }

  return false;
}


static replay_memory_t*
create_random_sample(player_t* player)
{
  static replay_memory_t batch[PLAYER_Q_REPLAY_BATCH_SIZE];
  int indexes[player->replay_batch_size];
  int indexes_index = 0;

  for (int i = 0; i < player->replay_batch_size; i++) {
    int index;
    do {
      index = frand()*player->replay_memory_index;
    } while (repeated_index(index, indexes, indexes_index));
    batch[i] = player->replay_memory[index];
    indexes[indexes_index++] = index;
  }
  return batch;
}


static fann_type
q_table_row_max(fann_type *row)
{
  fann_type max = -1.0;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[i] > max) {
      max = row[i];
    }
  }
  return max;
}


int
q_table_row_max_index(fann_type *row)
{
  fann_type max = -2.0;
  int index = 0;

  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    if (row[i] >= max) {
      max = row[i];
      index = i;
    }
  }
  return index;
}


static int
player_q_get_input(player_t* player)
{
  player->runs += 1;

  if (player->first_run) {
    // If this is first run initialize the Q-neural network
    initialize_q_neural_network(player);
    player->first_run = false;
  } else {
    // If this is not the first
    // Evaluate what happened on last action and calculate reward
    fann_type r = 0; // default is 0
    if (!player->game->new_game && player->old_score < player->game->score) {
      r = 1.0; //reward is 1 if our score increased
    } else if (!player->game->new_game && player->old_score > player->game->score) {
      r = -1.0; // reward is -1 if our score decreased
    } else if (!player->game->new_game) {
      r = -0.1;
    }

    // Capture current state
    // Set input to network map_size_x * map_size_y + actions length vector with a 1 on the player position
    input_state_t input_state = {0};// = Array.new(player->game->map_size_x*player->game->map_size_y + @actions.length, 0)
    input_state.state[player->x + (GAME_MAP_SIZE_X*player->y)] = 1;
    input_state.state[player->game->cheese.x + + (GAME_MAP_SIZE_X*player->game->cheese.y)] = 2.0;
    input_state.state[player->game->pit.x + + (GAME_MAP_SIZE_X*player->game->pit.y)] = 10.0;

    // Add reward, old_state and input state to memory
    replay_memory_t* rmp = &player->replay_memory[player->replay_memory_pointer];
    rmp->reward =  r;
    rmp->old_input_state = player->old_input_state;
    rmp->input_state = input_state;

    // Increment memory pointer
    player->replay_memory_pointer = (player->replay_memory_pointer<player->replay_memory_size-1) ? player->replay_memory_pointer+1 : 0;
    if (player->replay_memory_pointer > player->replay_memory_index) {
      player->replay_memory_index = player->replay_memory_pointer;
    }

    // If replay memory is full train network on a batch of states from the memory
    if (player->replay_memory_index >= player->replay_memory_size-1) {
      // Randomly samply a batch of actions from the memory and train network with these actions
      replay_memory_t* batch = create_random_sample(player);
      int training_data_input_index = 0;
      int training_data_output_index = 0;

      // For each batch calculate new q_value based on current network and reward
      static fann_type q_table_row[ACTION_NUM_ACTIONS];
      for (int i = 0; i < player->replay_batch_size; i++) {
	// To get entire q table row of the current state run the network once for every posible action
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  // Create neural network input vector for this action
	  input_state_t input_state_action = batch[i].input_state;
	  // Set a 1 in the action location of the input vector
	  input_state_action.state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + a] = 1;
	  // Run the network for this action and get q table row entry
	  q_table_row[a] = fann_run(player->q_nn_model, input_state_action.state)[0];
	}

	// Update the q value
	fann_type updated_q_value = batch[i].reward + (player->discount * q_table_row_max(q_table_row));

	// Add to training set
	for (int ti = 0; ti < PLAYER_SIZEOF_STATE; ti++) {
	  (*player->train->input)[training_data_input_index++] = batch[i].old_input_state.state[ti];
	}
	(*player->train->output)[training_data_output_index++] = updated_q_value;
      }


      // Train network with batch

      fann_train_on_data(player->q_nn_model, player->train, 1, 0, 0.01);
    }
  }


  // Capture current state and score
  // Set input to network map_size_x * map_size_y vector with a 1 on the player position
  input_state_t input_state = {0};
  input_state.state[player->x + (GAME_MAP_SIZE_X*player->y)] = 1;
  input_state.state[player->game->cheese.x + + (GAME_MAP_SIZE_X*player->game->cheese.y)] = 2.0;
  input_state.state[player->game->pit.x + + (GAME_MAP_SIZE_X*player->game->pit.y)] = 10.0;
  // Chose action based on Q value estimates for state
  // If a random number is higher than epsilon we take a random action
  // We will slowly increase @epsilon based on runs to a maximum of @max_epsilon - this encourages early exploration
  fann_type epsilon_run_factor = (player->runs/player->epsilon_increase_factor) > (player->max_epsilon-player->epsilon) ? (player->max_epsilon-player->epsilon) : (player->runs/player->epsilon_increase_factor);
  int action_taken_index = 0;
  fann_type rr = frand();
  player->last_e = player->epsilon + epsilon_run_factor;
  if (rr > (player->epsilon + epsilon_run_factor)) {
    // Select random action
    action_taken_index = frand()*ACTION_NUM_ACTIONS;
  } else {
    // To get the entire q table row of the current state run the network once for every posible action
    fann_type q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      // Create neural network input vector for this action
      input_state_t input_state_action = input_state;
      // Set a 1 in the action location of the input vector
      input_state_action.state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + a] = 1;
      // Run the network for this action and get q table row entry
      q_table_row[a] = fann_run(player->q_nn_model, input_state_action.state)[0];
    }

    action_taken_index = q_table_row_max_index(q_table_row);
  }

  // Save score and current state
  player->old_score = player->game->score;

  // Set action taken in input state before storing it
  input_state.state[(GAME_MAP_SIZE_X*GAME_MAP_SIZE_Y) + action_taken_index] = 1;
  player->old_input_state = input_state;

  // Take action
  return player->actions[action_taken_index];
}
