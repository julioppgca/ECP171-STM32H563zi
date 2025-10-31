/*
 * nextion.c
 *
 *  Created on: Oct 31, 2025
 *      Author: julio
 */

#include "nextion.h"
#include <string.h>
#include <stdio.h>
#include "usart.h"

/************************************************************************************************/
/**************** Mudar a porta serial utilzada pela ihm nextion aqui!!! ************************/
/************************************************************************************************/

#define NEXTION_UART huart3

/************************************************************************************************/
/************************************************************************************************/

// ajusta a porta serial para o componente escolhido
extern UART_HandleTypeDef NEXTION_UART;
#define NX_UART	(&NEXTION_UART)

// tamanho e declaração do buffer de recepçao
#define NEXTION_RX_BUFFER_SIZE	128
uint8_t nextion_rx_buffer[NEXTION_RX_BUFFER_SIZE];

nextion_ihm_t nextion_ihm;

// inicializa a interface, deve ser chamada antes do envio de comandos.
void nextion_init()
{
	HAL_UARTEx_ReceiveToIdle_IT(NX_UART, nextion_rx_buffer,
			sizeof(nextion_rx_buffer));

	nextion_ihm.active_page = -1;
	nextion_ihm.touch_event.component_id = -1;
	nextion_ihm.touch_event.event_type = -1;
	nextion_ihm.touch_event.page_id = -1;
}

// envia comandos para a ihm, coloca o terminador ff ff ff de forma automática ao final do comando.
void nex_send_cmd(const char *cmd)
{
	HAL_UART_Transmit(NX_UART, (uint8_t*) cmd, strlen(cmd), HAL_MAX_DELAY);
	const uint8_t t[3] =
	{ 0xFF, 0xFF, 0xFF };
	HAL_UART_Transmit(NX_UART, t, 3, HAL_MAX_DELAY);
    HAL_Delay(5);
}

// solicita qual a pagina está ativa na IHM, a respota chega no callback da porta serial escolhida (HAL_UARTEx_RxEventCallback)
void nextion_get_active_page()
{
	nex_send_cmd("sendme");
}

// quando chegam novos dados na serial, este callback é chamado, os dados são processados, e o callback é reiniciado para permitir novas recepções
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	// garante que a origem foi a ihm
	if (huart->Instance == NEXTION_UART.Instance && Size > 0)
	{
		// Processa os dados recebidos
		nextion_parse_command(nextion_rx_buffer, Size);

		// ⭐⭐ IMPORTANTE: Reinicia a recepção ⭐⭐
		HAL_UARTEx_ReceiveToIdle_IT(NX_UART, nextion_rx_buffer,
				sizeof(nextion_rx_buffer));
	}
}

// Função para processar comandos Nextion
void nextion_parse_command(uint8_t *data, uint16_t size)
{
	if (size < 1)
		return;

	uint8_t command = data[0]; //primeiro byte é sempre o comando
	// consultar: https://wiki.iteadstudio.com/Nextion_Instruction_Set#page:_Refresh_page
	// para obter a lista de comandos validos

	switch (command)
	{
	case 0x66: //pagina atual - estrutura: 0x66 + Page ID
	{
		if (size >= 2)
		{
			nextion_ihm.active_page = data[1];
		}
		break;
	}
	case 0x65: //// Touch Event - estrutura: 0x65 + Page ID + Component ID + Event (0x01 press, 0x00 release)
	{
		if (size >= 2)
		{
			nextion_ihm.touch_event.page_id = data[1];
			nextion_ihm.touch_event.component_id = data[2];
			nextion_ihm.touch_event.event_type = data[1];
		}
		break;
	}

	}
}


// Funções auxiliares para interface com o Nextion
void nextion_set_page(uint8_t page_id)
{
	char cmd[20];
	snprintf(cmd, sizeof(cmd), "page %d", page_id);
	nex_send_cmd(cmd);
}

void nextion_set_component_value(const char *component, uint32_t value)
{
	char cmd[30];
	snprintf(cmd, sizeof(cmd), "%s.val=%lu", component, value);
	nex_send_cmd(cmd);
}

void nextion_set_component_color(const char *component, uint32_t value)
{
	char cmd[30];
	snprintf(cmd, sizeof(cmd), "%s.pco=%lu", component, value);
	nex_send_cmd(cmd);
}

void nextion_set_component_text(const char *component, const char *text)
{
	char cmd[50];
	snprintf(cmd, sizeof(cmd), "%s.txt=\"%s\"", component, text);
	nex_send_cmd(cmd);
}

// Mover componente para posição específica (ambas coordenadas em um comando)
void nextion_move_component(const char *component_name, uint16_t x, uint16_t y)
{
    char cmd[50];
    snprintf(cmd, sizeof(cmd), "%s.x=%u", component_name, x);
    nex_send_cmd(cmd);
    snprintf(cmd, sizeof(cmd), "%s.y=%u", component_name, y);
    nex_send_cmd(cmd);
}
