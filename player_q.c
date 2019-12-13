#include <string.h>
#include <stdlib.h>
#include "game.h"
//#include "parallel_fann.h"

#ifndef GAME_ACTION_OUTPUTS

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
  player->replay_memory_size = GAME_Q_REPLAY_MEMORY_SIZE;
  player->replay_memory_pointer = 0;
  player->replay_memory_index = 0;
  player->replay_batch_size = GAME_Q_REPLAY_BATCH_SIZE;

  player->runs = 0;
  player->previous_score = 0;
}

static void
player_q_initialize_neural_network(player_t* player)
{
  // Setup model
  // Input is the size of the map + number of actions
  // Output size is one

  if (player->game->reload) {
    printf("reloading...\n");
    player->q_nn_model->load(player->q_nn_model, "nn.txt");
  } else {
    //    player->q_nn_model = fann_create_standard(3, PLAYER_Q_SIZEOF_STATE, PLAYER_Q_SIZEOF_STATE, 1);
    //    fann_set_training_algorithm(player->q_nn_model, FANN_TRAIN_INCREMENTAL);
    // fann_set_learning_rate(player->q_nn_model, 0.2);
    //fann_set_activation_function_hidden(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    //    fann_set_activation_function_output(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);

    //    player->q_nn_model = model_gen(PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS, KANN_C_CEM, 1, PLAYER_Q_SIZEOF_STATE, 0.0);
    player->q_nn_model->create_network(player->q_nn_model, PLAYER_Q_SIZEOF_STATE, PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS);
  }

  //  player->train = fann_create_train(player->replay_batch_size,// num_data
  //				    PLAYER_Q_SIZEOF_STATE,    // num_input
  //				    1);                       // num_output

  player->train =  player->q_nn_model->create_training( player->q_nn_model, player->replay_batch_size, PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS);
}


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
    number_t r = 0; // default is 0
    if (!player->game->new_game && player->previous_score < player->game->score) {
      r = 1.0; //reward is 1 if our score increased
    } else if (!player->game->new_game && player->previous_score > player->game->score) {
      r = -1.0; // reward is -1 if our score decreased
    } else if (!player->game->new_game) {
      r = -0.1;
    }

    player->game->total_reward += r;

    // Capture current state
    // Set input to network map_size_x * map_size_y + actions length vector with a 1 on the player position
    input_state_t next_state = {0};// = Array.new(player->game->map_size_x*player->game->map_size_y + @actions.length, 0)
    state_setup(&next_state, player);

    // Add reward, old_state and input state to memory
    replay_memory_t* rmp = &player->replay_memory[player->replay_memory_pointer];
    *rmp = player->state;
    rmp->reward =  r;
    rmp->next_state = next_state;

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

      // For each batch calculate new q_value based on current network and reward
      static number_t q_table_row[ACTION_NUM_ACTIONS];
      for (int i = 0; i < player->replay_batch_size; i++) {
	// To get entire q table row of the current state run the network once for every posible action
	for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
	  // Create neural network input vector for this action
	  input_state_t next_state = batch[i].next_state;
	  // Set a 1 in the action location of the input vector
	  state_set_action(&next_state, a);
	  // Run the network for this action and get q table row entry
	  q_table_row[a] =  player->q_nn_model->run(player->q_nn_model, next_state.state)[0];
	}

	// Update the q value
	//	number_t updated_q_value = batch[i].reward + (player->discount * player_q_table_row_max(q_table_row));


	number_t updated_q_value = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row, 0));
	//number_t current_q = fann_run(player->q_nn_model, batch[i].old_input_state.state)[0];
	//number_t updated_q_value = current_q + 0.2 * (batch[i].reward + player->discount * player_q_table_row_max(q_table_row) - current_q);

	// Add to training set
	for (int ti = 0; ti < PLAYER_Q_SIZEOF_STATE; ti++) {
	  player->q_nn_model->set_training_input_data(player->train, i, ti, batch[i].previous_state.state[ti]);
	}
	 player->q_nn_model->set_training_output_data(player->train, i, 0, updated_q_value);
      }

       player->q_nn_model->dump_train(player->train);

      // Train network with batch
      //fann_train_on_data(player->q_nn_model, player->train, 1, 0, 0.01);

       player->q_nn_model->train(player->q_nn_model,  player->train, 1);
    }
  }


  // Capture current state and score
  // Set input to network map_size_x * map_size_y vector with a 1 on the player position
  input_state_t state = {0};
  state_setup(&state, player);
  // Chose action based on Q value estimates for state
  // If a random number is higher than epsilon we take a random action
  // We will slowly increase @epsilon based on runs to a maximum of @max_epsilon - this encourages early exploration

  int action_taken_index = 0;
  number_t rr = frand();
  if (rr > (player->game->current_epsilon)) {
    // Select random action
    action_taken_index = frand()*ACTION_NUM_ACTIONS;
  } else {
    // To get the entire q table row of the current state run the network once for every posible action
    number_t q_table_row[ACTION_NUM_ACTIONS];
    for (int a = 0; a < ACTION_NUM_ACTIONS; a++) {
      // Create neural network input vector for this action
      input_state_t state_action = state;
      // Set a 1 in the action location of the input vector
      state_set_action(&state_action, a);
      // Run the network for this action and get q table row entry
      q_table_row[a] =  player->q_nn_model->run(player->q_nn_model, state_action.state)[0];
    }

    action_taken_index = misc_q_table_row_max_index(q_table_row, 0);
  }


  // Save score and current state
  player->previous_score = player->game->score;

  // Set action taken in input state before storing it
  state_set_action(&state, action_taken_index);
  player->state.previous_state = state;

  // Take action
  return player->actions[action_taken_index];
}
#endif
