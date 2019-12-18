#include "game.h"
#include <string.h>
#include <getopt.h>


void
config_parse_args(config_t* config, int argc, char** argv)
{
  config->train = 0;
  config->random = 0;
  config->ann = 0;
  config->memory_size = 0;
  config->batch_size = 400;
  config->moves_per_episode = 200;
  config->map_size_x = 20;
  config->map_size_y = 1;
  config->cheese_x = -1;
  config->cheese_y = -1;
  config->pit_x = -1;
  config->pit_y = -1;
  config->discount = 0.9;
  config->learning_rate = 0.01;
  config->epsilon_gradient = 0.75;
  config->epsilon_max = 0.9;
  config->render = 0;
  config->reload = 0;
  config->nn_library = NN_KANN;
  config->misc_q_debug = MISC_DISPLAY_DIRECTION_Q;

  int c;

  struct option long_options[] = {
    {"render",        no_argument,       &config->render, 'd'},
    {"reload",        no_argument,       &config->reload, 'l'},
    {"train",         required_argument, 0,       't'},
    {"ann",           required_argument, 0,       'a'},
    {"random",        required_argument, 0,       'r'},
    {"nn",            required_argument, 0,       'n'},
    {"memory",        required_argument, 0,       'm'},
    {"batch",         required_argument, 0,       'b'},
    {"moves",         required_argument, 0,       'p'},
    {"discount",      required_argument, 0,       'i'},
    {"learning_rate", required_argument, 0,       'z'},
    {"epsilon_gradient", required_argument, 0,    'e'},
    {"epsilon_max",   required_argument, 0,       'y'},
    {"map_size_x",    required_argument, 0,       '1'},
    {"map_size_y",    required_argument, 0,       '2'},
    {"cheese_x",      required_argument, 0,       '3'},
    {"cheese_y",      required_argument, 0,       '4'},
    {"pit_x",         required_argument, 0,       '5'},
    {"pit_y",         required_argument, 0,       '6'},
    {"q_debug",       required_argument, 0,       '7'},
    {0, 0, 0, 0}
  };


  while (1) {
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "d", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
	break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
	printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'n':
      if (strcmp(optarg, "fann") == 0) {
	config->nn_library = NN_FANN;
      } else if (strcmp(optarg, "kann") == 0) {
	config->nn_library = NN_KANN;
      } else if (strcmp(optarg, "q") == 0) {
	config->nn_library = NN_Q_TABLE;
      } else {
	game_error("unknown nn library");
      }
      break;
    case 'p':
      if (sscanf(optarg, "%d", &config->moves_per_episode) != 1) {
	game_error("missing or incorrect argument for moves per episode");
      }
      break;
    case 'm':
      if (sscanf(optarg, "%d", &config->memory_size) != 1) {
	game_error("missing or incorrect argument for memory size");
      }
      break;
    case 'b':
      if (sscanf(optarg, "%d", &config->batch_size) != 1) {
	game_error("missing or incorrect argument for batch size");
      }
      break;
    case 'a':
      if (sscanf(optarg, "%d", &config->ann) != 1) {
	game_error("missing or incorrect argument for number of ann games");
      }
      break;
    case 'i':
      if (sscanf(optarg, "%f", &config->discount) != 1) {
	game_error("missing or incorrect argument for discount");
      }
      break;
    case 'r':
      if (sscanf(optarg, "%d", &config->random) != 1) {
	game_error("missing or incorrect argument for number of random games");
      }
      break;
    case 'z':
      if (sscanf(optarg, "%f", &config->learning_rate) != 1) {
	game_error("missing or incorrect argument for learning rate");
      }
      break;
    case 'e':
      if (sscanf(optarg, "%f", &config->epsilon_gradient) != 1) {
	game_error("missing or incorrect argument for epsilon_gradient");
      }
      break;
    case 'y':
      if (sscanf(optarg, "%f", &config->epsilon_max) != 1) {
	game_error("missing or incorrect argument for epsilon_max");
      }
      break;
    case 't':
      if (sscanf(optarg, "%d", &config->train) != 1) {
	game_error("missing or incorrect argument for number of training games");
      }
      break;
    case '1':
      if (sscanf(optarg, "%d", &config->map_size_x) != 1) {
	game_error("missing or incorrect argument for map_size_x");
      }
      break;
    case '2':
      if (sscanf(optarg, "%d", &config->map_size_y) != 1) {
	game_error("missing or incorrect argument for map_size_y");
      }
      break;
    case '3':
      if (sscanf(optarg, "%d", &config->cheese_x) != 1) {
	game_error("missing or incorrect argument for cheese_x");
      }
      break;
    case '4':
      if (sscanf(optarg, "%d", &config->cheese_y) != 1) {
	game_error("missing or incorrect argument for cheese_y");
      }
      break;
    case '5':
      if (sscanf(optarg, "%d", &config->pit_x) != 1) {
	game_error("missing or incorrect argument for pit_x");
      }
      break;
    case '6':
      if (sscanf(optarg, "%d", &config->pit_y) != 1) {
	game_error("missing or incorrect argument for pit_y");
      }
      break;
    case '7':
      if (strcmp(optarg, "detailed") == 0) {
	config->misc_q_debug = MISC_DISPLAY_DETAILED_Q;
      } else if (strcmp(optarg, "none") == 0) {
        config->misc_q_debug = MISC_DISPLAY_NONE;
      }
      break;
    case '?':
      exit(1);
      break;
    default:
      abort ();
    }
  }

  if (config->cheese_x < 0) {
    config->cheese_x = 0;
  }

  if (config->cheese_y < 0) {
    config->cheese_y = 0;
  }

  if (config->pit_x < 0) {
    config->pit_x = config->map_size_x-1;
  }

  if (config->pit_y < 0) {
    config->pit_y = config->map_size_y-1;
  }
}
