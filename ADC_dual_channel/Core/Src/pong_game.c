/*
 * pong_game.c
 *
 *  Created on: Nov 7, 2025
 *      Author: julio
 */
#include "pong_game.h"
#include "game_settings.h"
#include "nextion.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

bool x_dir = true, y_dir = true;
uint16_t x_pos = 0, y_pos = 0;

uint16_t P1_A0 = 0;
uint16_t P2_A1 = 0;

static void read_controllers(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // Config comum para ambos
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;

    /* -------- Canal 3 (PA6) -------- */
    sConfig.Channel = ADC_CHANNEL_3;                    // seleciona canal 3
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);            // aplica config

    HAL_ADC_Start(&hadc1);                              // inicia conversão
    HAL_ADC_PollForConversion(&hadc1, 10);              // espera terminar
    P1_A0 = HAL_ADC_GetValue(&hadc1);                 // lê resultado
    HAL_ADC_Stop(&hadc1);                               // para ADC

    /* -------- Canal 10 (PC0) -------- */
    sConfig.Channel = ADC_CHANNEL_10;                   // agora canal 10
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    P2_A1 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
}

void init_game(void)
{
	nextion_init();
	nextion_set_page(0);
}

void run_game(void)
{
	if (x_dir)
	{
		x_pos += 10;
	}
	else
	{
		x_pos -= 10;
	}
	if (y_dir)
	{
		y_pos += 10;
	}
	else
	{
		y_pos -= 10;
	}
	if (x_pos > X_LIM_PLUS)
	{
		x_dir = false;

	}

	if (x_pos <= X_LIM_MINUS)
	{
		x_dir = true;

	}

	if (y_pos > Y_LIM_PLUS)
	{
		y_dir = false;

	}

	if (y_pos <= Y_LIM_MINUS)
	{
		y_dir = true;

	}

	// move a bola
	nextion_move_component(BALL, x_pos, y_pos);

	// ler o a posicao dos controles, potenciometros ligados em A0 e P1
	read_controllers();

	// move as "raquetes" no display em escala
	nextion_move_component(P1, P1_X_FIXED_POS, (float)P1_A0*(PLAYER_Y_PLUS_LIM/4095.0));
	nextion_move_component(P2, P2_X_FIXED_POS, (float)P2_A1*(PLAYER_Y_PLUS_LIM/4095.0));


	// esperar algum tempo ate o proximo loop
	HAL_Delay(GAME_LOOP_SPEED);
}
