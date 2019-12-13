#include "game.h"

static void
nn_fann_create_network(nn_t* nn, int num_input_neurons, int num_hidden_neurons, int num_output_neurons)
{
  struct fann* private = fann_create_standard(3, PLAYER_Q_SIZEOF_STATE, PLAYER_Q_SIZEOF_STATE, GAME_NUM_OUTPUTS);
  fann_set_training_algorithm(private, FANN_TRAIN_INCREMENTAL);
  fann_set_learning_rate(private, 0.2);

  //fann_set_activation_function_hidden(private, FANN_SIGMOID_SYMMETRIC);
  //fann_set_activation_function_hidden(private, FANN_LINEAR_PIECE_SYMMETRIC);
  fann_set_activation_function_hidden(private, FANN_SIGMOID_SYMMETRIC);
  fann_set_activation_function_output(private, FANN_SIGMOID_SYMMETRIC);
  nn->_private_data = private;

}


static nn_training_data_t*
nn_fann_create_training(nn_t* nn, int num_data, int num_input, int num_output)
{
  nn_training_data_t* train = malloc(sizeof(nn_training_data_t));

  train->_private_data = fann_create_train(num_data,
					   num_input,
					   num_output);
  return train;
}


static void
nn_fann_set_training_input_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  struct fann_train_data* private = train->_private_data;
  (*private->input)[(sample_index*private->num_input)+neuron_index] = value;
}


static void
nn_fann_set_training_output_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  struct fann_train_data* private = train->_private_data;
  (*private->output)[(sample_index*private->num_output)+neuron_index] = value;
}


static void
nn_fann_train(nn_t* nn, nn_training_data_t* train, int num_epochs)
{
  struct fann* private_nn = nn->_private_data;
  struct fann_train_data* private_train = train->_private_data;
  fann_train_on_data(private_nn, private_train, num_epochs, 0, 0.01);
}


static const number_t*
nn_fann_run(nn_t* nn, number_t* state)
{
  struct fann* private_nn = nn->_private_data;
  return fann_run(private_nn, state);
}


static void
nn_fann_dump_train(nn_training_data_t* train)
{
  return;
#if 0
#ifdef GAME_ACTION_OUTPUTS
  for (int i = 0; i < train->num_data; i++) {
    number_t* input = &(*train->input)[i*train->num_input];
    number_t* output = &(*train->output)[i*train->num_output];
    printf("player:%2d,%2d  cheese:%2d,%2d pit:%2d,%2d %s: % 4.4f  %s % 4.4f %c\n",
	   (int)(input[0]*GAME_MAP_SIZE_X),
	   (int)(input[1]*GAME_MAP_SIZE_Y),
	   (int)(input[2]*GAME_MAP_SIZE_X),
	   (int)(input[3]*GAME_MAP_SIZE_Y),
	   (int)(input[4]*GAME_MAP_SIZE_X),
	   (int)(input[5]*GAME_MAP_SIZE_Y),
	   state_action_names[0],
	   output[0],
	   state_action_names[1],
	   output[1],
	   output[0] <  output[1] ? '*' : ' ');
  }
#else
  for (int i = 0; i < train->num_data; i++) {
    number_t* input = &(*train->input)[i*train->num_input];
    number_t* output = &(*train->output)[i*train->num_output];
    printf("player:%2d,%2d  cheese:%2d,%2d pit:%2d,%2d  Q: % 4.4f Action: %s:\n",
      (int)(input[1]*GAME_MAP_SIZE_X),
      (int)(input[2]*GAME_MAP_SIZE_Y),
      (int)(input[3]*GAME_MAP_SIZE_X),
      (int)(input[4]*GAME_MAP_SIZE_Y),
      (int)(input[5]*GAME_MAP_SIZE_X),
      (int)(input[6]*GAME_MAP_SIZE_Y),
      output[0],
      state_action_names[(int)input[0]]);
  }
#endif
#endif
}


static void
nn_fann_load(nn_t* nn, const char* filename)
{
  nn->_private_data = fann_create_from_file(filename);
}


static void
nn_fann_save(nn_t* nn, const char* filename)
{
  struct fann* private_nn = nn->_private_data;
  fann_save(private_nn, filename);
}


nn_t*
nn_fann_construct(void)
{
  nn_t* nn = malloc(sizeof(nn_t));
  nn->create_network = nn_fann_create_network;
  nn->create_training = nn_fann_create_training;
  nn->set_training_input_data = nn_fann_set_training_input_data;
  nn->set_training_output_data = nn_fann_set_training_output_data;
  nn->train = nn_fann_train;
  nn->run = nn_fann_run;
  nn->load = nn_fann_load;
  nn->save = nn_fann_save;
  nn->dump_train = nn_fann_dump_train;
  return nn;
}
