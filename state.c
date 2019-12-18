#include "game.h"

char* state_action_names[] = {
  [ACTION_DOWN] = "D",
  [ACTION_UP] = "U",
  [ACTION_LEFT] = "L",
  [ACTION_RIGHT] = "R"
};


void
state_set_action(player_t* player, input_state_t* state, int a)
{
#ifndef GAME_ACTION_OUTPUTS
#ifdef GAME_POSITION_STATES
  state->action = (number_t)a/(number_t)(ACTION_NUM_ACTIONS-1);
#else
  state->action = 1;
#endif
#endif
}

void
state_dump(input_state_t* state, player_t* player, int action)
{
  number_t* state_ptr = (void*)state;

  for (int ti = 0; ti < GAME_INPUT_STATE_SIZE; ti++, state_ptr++) {
    printf("%f ", *state_ptr);
  }
  printf("\n");
}

void
state_setup(input_state_t* state, player_t* player)
{
  number_t scale_x = player->game->config.map_size_x;
  number_t scale_y = player->game->config.map_size_y;
  state->player.x = (number_t)player->x/scale_x;
  state->player.y = (number_t)player->y/scale_y;
  state->cheese.x = (number_t)player->game->cheese.x/scale_x;
  state->cheese.y = (number_t)player->game->cheese.y/scale_y;
  state->pit.x = (number_t)player->game->pit.x/scale_x;
  state->pit.y = (number_t)player->game->pit.y/scale_y;
#ifndef GAME_ACTION_OUTPUTS
  state->action = 0;
#endif
}


void
state_setup_training_sample(player_t* player, nn_training_data_t* train, input_state_t* state, number_t* q, int sample_index)
{
  number_t* state_ptr = (void*)state;

  for (int ti = 0; ti < GAME_INPUT_STATE_SIZE; ti++, state_ptr++) {
    player->q_model->set_training_input_data(player->train, sample_index, ti, *state_ptr);
    //    printf("%f ", *state_ptr);
  }

  for (int i = 0; i < GAME_NUM_OUTPUTS; i++) {
    player->q_model->set_training_output_data(player->train, sample_index, 0, q[i]);
  }
  //  printf("=>% f\n", q);
}
