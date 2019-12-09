#include <string.h>
#include <stdlib.h>
#include "game.h"
//#include "parallel_fann.h"

static int
player_q_get_input(player_t* player);

void
player_q_initialize(player_t* player)
{

  player->ready = false;
  player->get_input = player_q_get_input;
  //  player->x = 0;
  //  player->y = 0;
  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }
  player->first_run = true;

  player->discount = 0.9;
  player->epsilon = 0.1;
  player->max_epsilon = GAME_Q_MAX_EPSILON;

  player->epsilon_increase_factor = GAME_E_INCREASE_FACTOR;
  player->replay_memory_size = GAME_Q_REPLAY_MEMORY_SIZE;
  player->replay_memory_pointer = 0;
  player->replay_memory_index = 0;
  player->replay_batch_size = GAME_Q_REPLAY_BATCH_SIZE;

  player->runs = 0;
  player->old_score = 0;
}

static void
player_q_initialize_neural_network(player_t* player)
{
  // Setup model
  // Input is the size of the map + number of actions
  // Output size is one

  if (player->game->reload) {
    printf("reloading...\n");
    player->q_nn_model = fann_create_from_file("nn.txt");
  } else {
    player->q_nn_model = fann_create_standard(3, PLAYER_Q_SIZEOF_STATE, PLAYER_Q_SIZEOF_STATE, 1);
    fann_set_training_algorithm(player->q_nn_model, FANN_TRAIN_INCREMENTAL);
    fann_set_learning_rate(player->q_nn_model, 0.2);

    fann_set_activation_function_hidden(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
  }

  player->train = fann_create_train(player->replay_batch_size,// num_data
				    PLAYER_Q_SIZEOF_STATE,    // num_input
				    1);                       // num_output
}

#if 0
static bool
player_q_repeated_index(int index, int* indexes, int num_indexes)
{
  for (int i = 0; i < num_indexes; i++) {
    if (indexes[i] == index) {
      return true;
    }
  }

  return false;
}


static replay_memory_t*
player_q_create_random_sample(player_t* player)
{
  static replay_memory_t batch[GAME_Q_REPLAY_BATCH_SIZE];
  int indexes[player->replay_batch_size];
  int indexes_index = 0;

  for (int i = 0; i < player->replay_batch_size; i++) {
    int index;
    do {
      index = frand()*player->replay_memory_index;
    } while (player_q_repeated_index(index, indexes, indexes_index));
    batch[i] = player->replay_memory[index];
    indexes[indexes_index++] = index;
  }
  return batch;
}

#else

static replay_memory_t*
player_q_create_random_sample(player_t* player)
{
  static replay_memory_t batch[GAME_Q_REPLAY_BATCH_SIZE];
  int k = player->replay_batch_size;
  int n = player->replay_memory_index;
  replay_memory_t *array = player->replay_memory;
  replay_memory_t *output = batch;
  int i;

  for (i = 0; i < k; i++) {
    output[i] = array[i];
  }

  //  srand(time(NULL));    //use time function to get different seed value

  while(i < n) {
    int j = rand() % (i+1); //random index from 0 to i
    if (j < k) {            //copy ith element to jth element in the output array
      output[j] = array[i];
    }
    i++;
  }

  return batch;
}

#endif



static int
player_q_get_input(player_t* player)
{
  player->runs += 1;

  if (player->first_run) {
    // If this is first run initialize the Q-neural network
    player_q_initialize_neural_network(player);
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
    state_setup(&input_state, player);

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
    //    if (player->replay_memory_index >= player->replay_batch_size*2) {
      player->ready = true;
      // Randomly samply a batch of actions from the memory and train network with these actions
      replay_memory_t* batch = player_q_create_random_sample(player);

      int training_data_input_index = 0;
      int training_data_output_index = 0;

      player->game->average_q = 0;
      // For each batch calculate new q_value based on current network and reward
      static fann_type q_table_row[ACTION_NUM_ACTIONS];
      for (int i = 0; i < player->replay_batch_size; i++) {
	// To get entire q table row of the current state run the network once for every posible action
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  // Create neural network input vector for this action
	  input_state_t input_state_action = batch[i].input_state;
	  // Set a 1 in the action location of the input vector
	  state_set_action(&input_state_action, a);
	  // Run the network for this action and get q table row entry
	  q_table_row[a] = fann_run(player->q_nn_model, input_state_action.state)[0];
	}

	// Update the q value
	//	fann_type updated_q_value = batch[i].reward + (player->discount * player_q_table_row_max(q_table_row));


	fann_type updated_q_value = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row, 0));
	//fann_type current_q = fann_run(player->q_nn_model, batch[i].old_input_state.state)[0];
	//fann_type updated_q_value = current_q + 0.2 * (batch[i].reward + player->discount * player_q_table_row_max(q_table_row) - current_q);

	player->game->average_q += updated_q_value;

	// Add to training set
	for (int ti = 0; ti < PLAYER_Q_SIZEOF_STATE; ti++) {
	  (*player->train->input)[training_data_input_index++] = batch[i].old_input_state.state[ti];
	}
	(*player->train->output)[training_data_output_index++] = updated_q_value;
      }

      misc_dump_train(player->train);
      player->game->average_q = player->game->average_q / player->replay_batch_size;
      player->game->q_history[player->game->q_history_index] = player->game->average_q;
      player->game->q_history_index++;
      if (player->game->q_history_index == countof(player->game->q_history)) {
	player->game->q_history_index = 0;
      }

      // Train network with batch
      fann_train_on_data(player->q_nn_model, player->train, 1, 0, 0.01);
      //fann_train_epoch_irpropm_parallel(player->q_nn_model, player->train, 2);
    }
  }


  // Capture current state and score
  // Set input to network map_size_x * map_size_y vector with a 1 on the player position
  input_state_t input_state = {0};
  state_setup(&input_state, player);
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
      state_set_action(&input_state_action, a);
      // Run the network for this action and get q table row entry
      q_table_row[a] = fann_run(player->q_nn_model, input_state_action.state)[0];
    }

    action_taken_index = misc_q_table_row_max_index(q_table_row, 0);
  }

  // Save score and current state
  player->old_score = player->game->score;

  // Set action taken in input state before storing it
  state_set_action(&input_state, action_taken_index);
  player->old_input_state = input_state;

  // Take action
  return player->actions[action_taken_index];
}
