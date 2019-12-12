#include <string.h>
#include <stdlib.h>
#include "game.h"
//#include "parallel_fann.h"

#ifdef GAME_ACTION_OUTPUTS

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
    player->q_nn_model = fann_create_from_file("nn.txt");
  } else {
    //    player->q_nn_model = fann_create_standard(3, PLAYER_Q_SIZEOF_STATE, PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS);
    player->q_nn_model = fann_create_standard(2, PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS);

    fann_set_training_algorithm(player->q_nn_model, FANN_TRAIN_INCREMENTAL);
    fann_set_learning_rate(player->q_nn_model, 0.2);


    //    fann_set_activation_function_hidden(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    //    fann_set_activation_function_output(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    //    fann_set_activation_function_hidden(player->q_nn_model, FANN_LINEAR_PIECE_SYMMETRIC);
    //    fann_set_activation_function_output(player->q_nn_model, FANN_LINEAR_PIECE_SYMMETRIC);

    //fann_set_activation_function_hidden(player->q_nn_model, FANN_LINEAR_PIECE_SYMMETRIC);
    fann_set_activation_function_hidden(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(player->q_nn_model, FANN_SIGMOID_SYMMETRIC);
  }

  player->train = fann_create_train(player->replay_batch_size,// num_data
				    PLAYER_Q_SIZEOF_STATE,    // num_input
				    GAME_NUM_OUTPUTS);        // num_output

  for (int i = 0; i < countof(player->replay_memory); i++) {
    for (int qi = 0; qi < GAME_NUM_OUTPUTS; qi++) {
      //player->replay_memory[i].previous_q[qi] = (frand()*0.2)-0.1;
    }
  }
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
    fann_type r = 0; // default is 0
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

      int training_data_input_index = 0;
      int training_data_output_index = 0;

      for (int i = 0; i < player->replay_batch_size; i++) {
	input_state_t batch_state = batch[i].next_state;
	// Run the network, get q values for next state

	// Add to training set
	for (int ti = 0; ti < PLAYER_Q_SIZEOF_STATE; ti++) {
	  (*player->train->input)[training_data_input_index++] = batch[i].previous_state.state[ti];
	}

	for (int qi = 0; qi < GAME_NUM_OUTPUTS; qi++) {
	  fann_type updated_q;
	  if (qi ==  batch[i].previous_action) {
	    fann_type *q_table_row = fann_run(player->q_nn_model, batch_state.state);
	    fann_type maxq = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row, 0));
	    //	    fann_type tderror = maxq - batch[i].previous_q[qi];
	    updated_q = maxq; //tderror;
	  } else {
	    updated_q = batch[i].previous_q[qi];
	  }

	  (*player->train->output)[training_data_output_index++] = updated_q;
	}
      }

      misc_dump_train(player->train);

      // Train network with batch
      fann_train_on_data(player->q_nn_model, player->train, 1, 0, 0.01);
      //fann_train_epoch_irpropm_parallel(player->q_nn_model, player->train, 2);
    }
  }


  // Capture current state and score
  input_state_t next_state = {0};
  state_setup(&next_state, player);
  int action_taken_index = 0;
  fann_type rr = frand();
  fann_type* q_table_row = fann_run(player->q_nn_model, next_state.state);

  if (rr > player->game->current_epsilon) {
    // Select random action
    action_taken_index = frand()*ACTION_NUM_ACTIONS;
  } else {
    // Chose action based on Q value estimates for state
    action_taken_index = misc_q_table_row_max_index(q_table_row, 0);
  }

  // Save score and current state
  player->previous_score = player->game->score;
  memcpy(player->state.previous_q,  q_table_row, sizeof(player->state.previous_q));
  player->state.previous_state = next_state;
  player->state.previous_action = action_taken_index;

  // Take action
  return action_taken_index;
}

#endif
