#include "game.h"
#include <stdlib.h>
#include <string.h>

#ifndef GAME_ACTION_OUTPUTS

typedef struct {
  int num_input;
  int num_output;
  int num_data;
  input_state_t* input;
  number_t* output;
} q_table_training_t;

typedef struct {
  number_t** qtable;
  int width;
  int height;
  int num_actions;
} q_table_t;


static void
nn_q_create_network(game_t* game, nn_t* nn, int num_input_neurons, int num_hidden_neurons, int num_output_neurons, number_t learning_rate)
{
  nn->learning_rate = learning_rate;

  q_table_t* q_table = malloc(sizeof(q_table_t));
  q_table->num_actions = ACTION_NUM_ACTIONS;
  q_table->width = game->config.map_size_x;
  q_table->height = game->config.map_size_y;
  q_table->qtable = malloc(sizeof(number_t*)*ACTION_NUM_ACTIONS);
  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    q_table->qtable[i] = malloc(sizeof(number_t)*game->config.map_size_x*game->config.map_size_y);
  }
  nn->_private_data = q_table;
}


static nn_training_data_t*
nn_q_create_training(nn_t* nn, int num_data, int num_input, int num_output)
{
  nn_training_data_t* train = calloc(1, sizeof(nn_training_data_t));
  q_table_training_t* private = malloc(sizeof(q_table_training_t));
  private->input = malloc(num_data*num_input*sizeof(number_t));
  private->output = malloc(num_data*num_output*sizeof(number_t));
  private->num_input = num_input;
  private->num_output = num_output;
  private->num_data = num_data;
  train->_private_data = private;

  return train;
}


static void
nn_q_set_training_input_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  q_table_training_t* private = train->_private_data;
  input_state_t* state = &private->input[sample_index];
  number_t* input = (number_t*)state;
  input[neuron_index] = value;
}


static void
nn_q_set_training_output_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  q_table_training_t* private = train->_private_data;
  private->output[(sample_index*private->num_output)+neuron_index] = value;
}

void
nn_set_q(nn_t* nn, input_state_t* state, number_t q)
{
  q_table_t* q_table = nn->_private_data;
  int x = (state->player.x*(q_table->width-1));
  int y = (state->player.y*(q_table->height-1));
  int action = (state->action*(ACTION_NUM_ACTIONS-1));
  q_table->qtable[action][(y*q_table->width)+x] = q;
}

static void
nn_q_train(nn_t* nn, nn_training_data_t* train, int num_epochs)
{
  q_table_training_t* private_train = train->_private_data;

  input_state_t* input = private_train->input;
  number_t* output = private_train->output;
  for (int i = 0; i < private_train->num_data; i++) {
    const number_t* old_q = nn->run(nn, (input_state_t*)input);
    number_t q = output[0];
    number_t diff = q - old_q[0];
    q = old_q[0] + (diff*nn->learning_rate);
    nn_set_q(nn, (input_state_t*)input, q);
    input++;
    output += private_train->num_output;
  }
}


static number_t
nn_q_test(struct nn* nn, nn_training_data_t* train)
{
  q_table_training_t* private_train = train->_private_data;

  number_t error = 0;
  input_state_t* input = private_train->input;
  for (int i = 0; i < private_train->num_data; i++) {
    const number_t* q = nn->run(nn, (input_state_t*)input);
    printf("%d: %f -> %f\n", i, private_train->output[i], q[0]);
    input += private_train->num_input;
  }
  printf("\n");
  return error;
}


static const number_t*
nn_q_run(nn_t* nn, input_state_t* state)
{
  q_table_t* q_table = nn->_private_data;
  int x = (state->player.x*(q_table->width-1));
  int y = (state->player.y*(q_table->height-1));
  int action = (state->action*(ACTION_NUM_ACTIONS-1));
  number_t* q = &q_table->qtable[action][(y*q_table->width)+x];
  return q;
}


static void
nn_q_dump_train(nn_training_data_t* train)
{
  return;
}


static void
nn_q_load(nn_t* nn, const char* filename)
{
  FILE * fp = fopen(filename, "r");
  q_table_t* q_table = malloc(sizeof(q_table_t));
  fscanf(fp, "width: %d\n", &q_table->width);
  fscanf(fp, "height: %d\n", &q_table->height);
  fscanf(fp, "actions: %d\n", &q_table->num_actions);
  fscanf(fp, "learning_rate: %f\n", &nn->learning_rate);
  q_table->qtable = malloc(sizeof(number_t*)*q_table->num_actions);
  for (int i = 0; i < ACTION_NUM_ACTIONS; i++) {
    q_table->qtable[i] = malloc(sizeof(number_t)*q_table->width*q_table->height);
  }
  nn->_private_data = q_table;

  for (int a = 0; a < q_table->num_actions; a++) {
    for (int y = 0; y < q_table->height; y++) {
      for (int x = 0; x < q_table->width; x++) {
	fscanf(fp, "%f ", &q_table->qtable[a][(y*q_table->width)+x]);
      }
      fscanf(fp, "\n");
    }
  }
  fclose(fp);
}


static void
nn_q_save(nn_t* nn, const char* filename)
{
  q_table_t* q_table = nn->_private_data;
  FILE * fp = fopen(filename, "w");
  fprintf(fp, "width: %d\n", q_table->width);
  fprintf(fp, "height: %d\n", q_table->height);
  fprintf(fp, "actions: %d\n", q_table->num_actions);
  fprintf(fp, "learning_rate: %f\n", nn->learning_rate);
  for (int a = 0; a < q_table->num_actions; a++) {
    for (int y = 0; y < q_table->height; y++) {
      for (int x = 0; x < q_table->width; x++) {
	fprintf(fp, "%f ", q_table->qtable[a][(y*q_table->width)+x]);
      }
      fprintf(fp, "\n");
    }
  }
  fclose(fp);
}


static struct nn*
nn_q_clone(struct nn* nn)
{
  fprintf(stderr, "not implemented\n");
  abort();

  return 0;
}


static void
nn_q_destroy(struct nn* nn)
{

}


nn_t*
nn_q_table_construct(void)
{
  nn_t* nn = calloc(1, sizeof(nn_t));
  nn->create_network = nn_q_create_network;
  nn->create_training = nn_q_create_training;
  nn->set_training_input_data = nn_q_set_training_input_data;
  nn->set_training_output_data = nn_q_set_training_output_data;
  nn->train = nn_q_train;
  nn->run = nn_q_run;
  nn->load = nn_q_load;
  nn->save = nn_q_save;
  nn->dump_train = nn_q_dump_train;
  nn->clone = nn_q_clone;
  nn->destroy = nn_q_destroy;
  nn->test = nn_q_test;
  return nn;
}
#endif
