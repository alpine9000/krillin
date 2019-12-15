#include "game.h"
#include <float.h>
#include <string.h>
#include <stdio.h>
#include "kann/kann.h"

typedef struct {
  float** _input;
  float** _output;
  int num_input;
  int num_output;
  int num_data;
} nn_kann_training_data_t;


static int
nn_train_fnn1(kann_t *ann, float lr, int mini_size, int max_epoch, int n, float **x, float **y)
{
  int i, *shuf, n_train, n_in, n_out, n_var;
  float *x1, *y1, *r;

  n_in = kann_dim_in(ann);
  n_out = kann_dim_out(ann);
  if (n_in < 0 || n_out < 0) return -1;
  n_var = kann_size_var(ann);
  //  n_const = kann_size_const(ann);
  r = (float*)calloc(n_var, sizeof(float));
  shuf = (int*)malloc(n * sizeof(int));
  kann_shuffle(n, shuf);
  n_train = n;

  x1 = (float*)malloc(n_in  * mini_size * sizeof(float));
  y1 = (float*)malloc(n_out * mini_size * sizeof(float));

  kann_feed_bind(ann, KANN_F_IN,    0, &x1);
  kann_feed_bind(ann, KANN_F_TRUTH, 0, &y1);

  for (i = 0; i < max_epoch; ++i) {
    int n_proc = 0, n_train_err = 0,  n_train_base = 0;
    kann_shuffle(n_train, shuf);
    kann_switch(ann, 1);
    while (n_proc < n_train) {
      int b, c, ms = n_train - n_proc < mini_size? n_train - n_proc : mini_size;
      for (b = 0; b < ms; ++b) {
	memcpy(&x1[b*n_in],  x[shuf[n_proc+b]], n_in  * sizeof(float));
	memcpy(&y1[b*n_out], y[shuf[n_proc+b]], n_out * sizeof(float));
      }
      kann_set_batch_size(ann, ms);
      kann_cost(ann, 0, 1);
      c = kann_class_error(ann, &b);
      n_train_err += c, n_train_base += b;
      kann_RMSprop(n_var, lr, 0, 0.9f, ann->g, ann->x, r);
      n_proc += ms;
    }
    kann_switch(ann, 0);
    n_proc = 0;
  }


  free(y1); free(x1); free(shuf); free(r);
  return i;
}


static kann_t*
model_gen(int n_in, int n_out, int loss_type, int n_h_layers, int n_h_neurons, float h_dropout)
{
  int i;
  kann_verbose = 0;
  kad_node_t *t;
  t = kann_layer_input(n_in);
  for (i = 0; i < n_h_layers; ++i) {
    t = kann_layer_dropout(kad_relu(kann_layer_dense(t, n_h_neurons)), h_dropout);
    //t = kann_layer_dropout(kad_sigm(kann_layer_dense(t, n_h_neurons)), h_dropout);
  }
  return kann_new(kann_layer_cost(t, n_out, loss_type), 0);
}


static void
nn_create_network(nn_t* nn, int num_input_neurons, int num_hidden_neurons, int num_output_neurons, number_t learning_rate)
{
  nn->learning_rate = learning_rate;
  //#ifdef GAME_ACTION_OUTPUTS
  //  nn->_private_data = model_gen(num_input_neurons, num_output_neurons, KANN_C_CEM, 1, num_hidden_neurons, 0.0);
  //#else
  //   nn->_private_data = model_gen(num_input_neurons, num_output_neurons, KANN_C_MSE, 1, num_hidden_neurons, 0.0);
  //#endi
  nn->_private_data = model_gen(num_input_neurons, num_output_neurons, KANN_C_MSE, 1, num_hidden_neurons, 0.001);
}


static nn_training_data_t*
nn_create_training(nn_t* nn, int num_data, int num_input, int num_output)
{
  nn_kann_training_data_t* private = malloc(sizeof(nn_kann_training_data_t));
  private->num_data = num_data;
  private->num_input = num_input;
  private->num_output = num_output;
  private->_input = malloc(num_data*sizeof(number_t*));
  private->_output = malloc(num_data*sizeof(number_t*));
  for (int i = 0; i < num_data; i++) {
    private->_input[i] = malloc(sizeof(number_t)*num_input);
    private->_output[i] = malloc(sizeof(number_t)*num_output);
  }
  nn_training_data_t* train = malloc(sizeof(nn_training_data_t));
  train->_private_data = private;
  return train;
}

static void
nn_set_training_input_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  nn_kann_training_data_t* private = train->_private_data;
  private->_input[sample_index][neuron_index] = value;
}


static void
nn_set_training_output_data(nn_training_data_t* train, int sample_index, int neuron_index, number_t value)
{
  nn_kann_training_data_t* private = train->_private_data;
  private->_output[sample_index][neuron_index] = value;
}


static void
nn_train(nn_t* nn, nn_training_data_t* train, int num_epochs)
{
  kann_t *private_nn = nn->_private_data;
  nn_kann_training_data_t* private_train = train->_private_data;
  nn_train_fnn1(private_nn, nn->learning_rate, private_train->num_data, num_epochs, private_train->num_data, private_train->_input, private_train->_output);
}


static const number_t*
nn_run(nn_t* nn, number_t* state)
{
  kann_t *private_nn = nn->_private_data;
  const number_t* result = kann_apply1(private_nn, state);
  return result;
}


static void
nn_dump_train(nn_training_data_t* train)
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
nn_load(nn_t* nn, const char* filename)
{
  nn->_private_data = kann_load(filename);
}


static void
nn_save(nn_t* nn, const char* filename)
{
  kann_t* private_nn = nn->_private_data;
  kann_save(filename, private_nn);
}

static struct nn*
nn_clone(struct nn* nn)
{
  nn_t* clone = malloc(sizeof(nn_t));
  memcpy(clone, nn, sizeof(nn_t));
  clone->_private_data = kann_clone(nn->_private_data, 0);
  return clone;
}


static void
nn_destroy(struct nn* nn)
{
  kann_delete(nn->_private_data);
  free(nn);
}

nn_t*
nn_kann_construct(void)
{
  nn_t* nn = calloc(1, sizeof(nn_t));
  nn->create_network = nn_create_network;
  nn->create_training = nn_create_training;
  nn->set_training_input_data = nn_set_training_input_data;
  nn->set_training_output_data = nn_set_training_output_data;
  nn->train = nn_train;
  nn->run = nn_run;
  nn->load = nn_load;
  nn->save = nn_save;
  nn->dump_train = nn_dump_train;
  nn->clone = nn_clone;
  nn->destroy = nn_destroy;
  return nn;
}
