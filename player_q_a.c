#include <string.h>
#include <stdlib.h>
#include "game.h"

#ifdef GAME_ACTION_OUTPUTS

static int
player_q_get_input(player_t* player);

void
player_q_initialize(player_t* player, int memory_size, int batch_size, number_t discount, number_t learning_rate)
{

  player->discount = discount;
  player->learning_rate = learning_rate;
  player->ready = false;
  player->get_input = player_q_get_input;
  //  player->x = 0;
  //  player->y = 0;
  for (unsigned int i = 0; i < countof(player->actions); i++) {
    player->actions[i] = i;
  }
  player->first_run = true;

  player->discount = 0.9;
  player->replay_memory_size = memory_size;
  player->replay_memory = malloc(sizeof(replay_memory_t)*memory_size);
  player->replay_memory_pointer = 0;
  player->replay_memory_index = 0;
  player->replay_batch_size = batch_size;

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
    player->q_model->load(player->q_model, "nn.txt");
  } else {
    //    player->q_model = model_gen(PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS, KANN_C_CEM, 1, PLAYER_Q_SIZEOF_STATE, 0.0);

    player->q_model->create_network(player->game, player->q_model, GAME_INPUT_STATE_SIZE, GAME_INPUT_STATE_SIZE, GAME_NUM_OUTPUTS, player->learning_rate);

    printf("GAME_NUM_OUTPUTS: %d\n", GAME_NUM_OUTPUTS);

  }


  player->train = player->q_model->create_training(player->q_model, player->replay_batch_size, GAME_INPUT_STATE_SIZE, GAME_NUM_OUTPUTS);
  //  player->train = fann_create_train(player->replay_batch_size,// num_data
  //				    PLAYER_Q_SIZEOF_STATE,    // num_input
  //				    GAME_NUM_OUTPUTS);        // num_output
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

    player->game->total_reward += r;

    // Capture current state
    // Set input to network map_size_x * map_size_y + actions length vector with a 1 on the player position
    input_state_t next_state;// = Array.new(player->game->map_size_x*player->game->map_size_y + @actions.length, 0)
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


      for (int i = 0; i < player->replay_batch_size; i++) {
	input_state_t batch_state = batch[i].next_state;
	// Run the network, get q values for next state

#if 0
	// Add to training set
	for (int ti = 0; ti < PLAYER_Q_SIZEOF_STATE; ti++) {
	  player->q_model->set_training_input_data(player->train, i, ti, batch[i].previous_state.state[ti]);
	}

	for (int qi = 0; qi < GAME_NUM_OUTPUTS; qi++) {
	  number_t updated_q;
	  if (qi ==  batch[i].previous_action) {
	    const number_t *q_table_row = player->q_model->run(player->q_model, batch_state.state);

	    number_t maxq = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row, GAME_NUM_OUTPUTS));
	    //	    number_t tderror = maxq - batch[i].previous_q[qi];
	    updated_q = maxq; //tderror;
	  } else {
	    updated_q = batch[i].previous_q[qi];
	  }

	  player->q_model->set_training_output_data(player->train, i, qi, updated_q);
	}
#else
	number_t updated_q[GAME_NUM_OUTPUTS];
	for (int qi = 0; qi < GAME_NUM_OUTPUTS; qi++) {
	  if (qi == batch[i].previous_action) {
	    const number_t *q_table_row = player->q_model->run(player->q_model, &batch_state);
	    number_t maxq = batch[i].reward + (player->discount * misc_q_table_row_max(q_table_row, GAME_NUM_OUTPUTS));
	    //	    number_t tderror = maxq - batch[i].previous_q[qi];
	    updated_q[qi] = maxq; //tderror;
	  } else {
	    updated_q[qi] = batch[i].previous_q[qi];
	  }
	}
	state_setup_training_sample(player, player->train, &batch[i].previous_state, updated_q, i);
#endif

      }

      player->q_model->dump_train(player->train);

      // Train network with batch
      player->q_model->train(player->q_model, player->train, 1);

    }
  }


  // Capture current state and score
  input_state_t next_state;
  state_setup(&next_state, player);
  int action_taken_index = 0;
  number_t rr = frand();
  const number_t* q_table_row = player->q_model->run(player->q_model, &next_state);

  if (rr > player->game->current_epsilon) {
    // Select random action
    action_taken_index = rand() % ACTION_NUM_ACTIONS;
  } else {
    // Chose action based on Q value estimates for state
    action_taken_index = misc_q_table_row_max_index(q_table_row, GAME_NUM_OUTPUTS);
  }

  // Save score and current state
  player->previous_score = player->game->score;
  memcpy(player->state.previous_q, q_table_row, sizeof(player->state.previous_q));
  player->state.previous_state = next_state;
  player->state.previous_action = action_taken_index;

  // Take action
  return action_taken_index;
}

#endif
