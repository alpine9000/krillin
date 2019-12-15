#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
//#include "parallel_fann.h"

#ifndef GAME_ACTION_OUTPUTS

static int
player_q_get_input(player_t* player);

void
player_q_initialize(player_t* player, int memory_size, int batch_size, number_t discount, number_t learning_rate)
{

  player->ready = false;
  player->get_input = player_q_get_input;
  //  player->x = 0;
  //  player->y = 0;
  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }
  player->first_run = true;

  player->discount = discount;
  player->learning_rate = learning_rate;
  player->replay_memory_size = memory_size;
  player->replay_memory = malloc(sizeof(replay_memory_t)*memory_size);
  player->replay_memory_pointer = 0;
  player->replay_memory_index = 0;
  player->replay_batch_size = batch_size;

#ifdef GAME_POSITION_STATES
  player->state_size = ((GAME_NUM_PITS*2)+2+2);
#else
  player->state_size = ((player->game->map_size_x*player->game->map_size_y)+ACTION_NUM_ACTIONS);
#endif


  player->runs = 0;
  player->previous_score = 0;
}


#ifdef GAME_TARGET_MODEL
void
player_q_copy_target_model(player_t* player)
{
  if (player->q_model->_private_data) {
    if (player->q_target_model) {
      player->q_target_model->destroy(player->q_target_model);
    }

    player->q_target_model = player->q_model->clone(player->q_model);
  }
}
#endif

static void
player_q_initialize_neural_network(player_t* player)
{
  if (player->game->reload) {
    printf("reloading...\n");
    player->q_model->load(player->q_model, "nn.txt");
  } else {
    player->q_model->create_network(player->q_model, player->state_size, player->state_size, GAME_NUM_OUTPUTS, player->learning_rate);
  }

#ifdef GAME_TARGET_MODEL
  player->q_target_model = player->q_model->clone(player->q_model);
#endif

  player->train =  player->q_model->create_training( player->q_model, player->replay_batch_size, player->state_size, GAME_NUM_OUTPUTS);
}


static replay_memory_t*
player_q_create_random_sample(player_t* player)
{
  static replay_memory_t* batch = 0;
  static int batch_size = 0;
  if (batch_size != player->replay_batch_size) {
    batch = malloc(sizeof(replay_memory_t)*player->replay_batch_size);
    batch_size = player->replay_batch_size;
  }
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

    if (!player->game->new_game) {
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
	      state_set_action(player, &next_state, a);
	      // Run the network for this action and get q table row entry
#ifdef GANE_TARGET_MODEL
	      q_table_row[a] = player->q_target_model->run(player->q_target_model, next_state.state)[0];
#else
	      q_table_row[a] =  player->q_model->run(player->q_model, next_state.state)[0];
#endif

	    }

	    // Update the q value
	    number_t updated_q_value = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row));

	    // Add to training set
	    for (int ti = 0; ti < player->state_size; ti++) {
	      player->q_model->set_training_input_data(player->train, i, ti, batch[i].previous_state.state[ti]);
	    }
	    player->q_model->set_training_output_data(player->train, i, 0, updated_q_value);

	    //	 state_dump(&batch[i].previous_state, player, 0);
	  }

	  player->q_model->dump_train(player->train);

	  // Train network with batch

	  player->q_model->train(player->q_model,  player->train, 1);
	}
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
      state_set_action(player, &state_action, a);
      // Run the network for this action and get q table row entry
      q_table_row[a] =  player->q_model->run(player->q_model, state_action.state)[0];
    }

    action_taken_index = misc_q_table_row_max_index(q_table_row, 0);
    memcpy(player->state.previous_q,  q_table_row, sizeof(player->state.previous_q));
  }


  // Save score and current state
  player->previous_score = player->game->score;

  // Set action taken in input state before storing it
  state_set_action(player, &state, action_taken_index);
  player->state.previous_state = state;
  player->state.previous_action = action_taken_index;

  // Take action
  return player->actions[action_taken_index];
}
#endif
