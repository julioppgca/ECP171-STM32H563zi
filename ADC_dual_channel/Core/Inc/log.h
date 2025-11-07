/*
 * log.h  —  STM32H563ZI (HAL) - Logger com UART e ITM/SWO
 * Autor: julio (ajustado)
 * Data: Oct 31, 2025
 */

#ifndef INC_LOG_H_
#define INC_LOG_H_

#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ============================================================
 * ===============     CONFIGURAÇÕES GERAIS     ===============
 * ============================================================ */

/* Ative/desative os destinos de saída */
#ifndef LOG_USE_UART
#define LOG_USE_UART 1   /* 1 = envia logs via UART */
#endif

#ifndef LOG_USE_ITM
#define LOG_USE_ITM  0   /* 1 = envia logs via ITM/SWO (SWV Viewer) */
#endif

/* Buffer de formatação (tamanho total da linha) */
#ifndef LOG_TX_BUFFER_SIZE
#define LOG_TX_BUFFER_SIZE 256
#endif

/* Timestamp automático antes da TAG (ex.: [00012345 ms][TAG]) */
#ifndef LOG_ADD_TIMESTAMP
#define LOG_ADD_TIMESTAMP 0   /* 1 = habilita, 0 = desabilita */
#endif

/* Níveis de log */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4   /* desativa todos os logs */

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#ifndef LOG_TAG
#define LOG_TAG "DEFAULT"
#endif


/* ============================================================
 * ===============     SELEÇÃO DE UART (LOG)    ===============
 * ============================================================ */

#if LOG_USE_UART
/* Altere aqui a UART usada para logs */
#define LOG_UART_HANDLE huart3             /* <<< troque para huart1/huart2/... conforme seu projeto */
extern UART_HandleTypeDef LOG_UART_HANDLE; /* declarado no main.c gerado pelo CubeMX */
#define LOG_UART (&LOG_UART_HANDLE)
#endif


/* ============================================================
 * ===============     SUPORTE A ITM / SWO       ===============
 * ============================================================ */

#if LOG_USE_ITM
  /* Para Cortex-M33 (STM32H5), ITM_SendChar vem do CMSIS core */
  #include "core_cm33.h"
  /* Caso seu include path não puxe core_cm33.h automaticamente,
     inclua o header CMSIS apropriado do seu device pack. */
#endif


/* ============================================================
 * ===============     TIMESTAMP (HOOK fraco)    ===============
 * ============================================================ */

/* Hook fraco: por padrão usa HAL_GetTick (ms).
   Você pode definir esta função em outro .c para usar RTC ou timer HW. */
__attribute__((weak))
uint32_t log_get_time_ms(void) {
    return HAL_GetTick();
}

/* Formata o prefixo do log: "[xxxxxxxx ms][TAG] " ou "[TAG] " */
static inline int log_format_prefix(char *buf, size_t sz, const char *tag)
{
#if LOG_ADD_TIMESTAMP
    uint32_t ms = log_get_time_ms();
    return snprintf(buf, sz, "[%08lu ms][%s] ", (unsigned long)ms, tag);
#else
    return snprintf(buf, sz, "[%s] ", tag);
#endif
}


/* ============================================================
 * ===============    SAÍDAS DE TRANSMISSÃO      ===============
 * ============================================================ */

#if LOG_USE_UART
static inline void log_uart_write(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    HAL_UART_Transmit(LOG_UART, (uint8_t*)data, (uint16_t)len, HAL_MAX_DELAY);
}
#endif

#if LOG_USE_ITM
static inline void log_itm_write(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        ITM_SendChar(data[i]);
    }
}
#endif


/* ============================================================
 * ===============     Núcleo de impressão        ===============
 * ============================================================ */

/* Função central: formata prefixo + mensagem e envia às saídas ativas */
static inline void log_vprint_core(int level, const char *tag, const char *fmt, va_list ap)
{
    (void)level; /* reservado p/ filtros futuros dinâmicos */

    char b[LOG_TX_BUFFER_SIZE];
    int off = log_format_prefix(b, sizeof(b), tag);
    if (off < 0) off = 0;
    if (off >= (int)sizeof(b)) off = (int)sizeof(b) - 1;

    int n = vsnprintf(b + off, sizeof(b) - (size_t)off, fmt, ap);
    if (n < 0) n = 0;

    int len = off + n;
    if (len < 0) len = 0;
    if (len >= (int)sizeof(b)) len = (int)sizeof(b) - 1;

    /* Garante CRLF no final (sem duplicar) */
    if (len == 0 || (b[len - 1] != '\n' && b[len - 1] != '\r')) {
        if (len <= (int)sizeof(b) - 3) {
            b[len++] = '\r';
            b[len++] = '\n';
            b[len]   = '\0';
        }
    }

    /* Transmite para os destinos habilitados */
#if LOG_USE_UART
    log_uart_write((uint8_t*)b, (size_t)len);
#endif
#if LOG_USE_ITM
    log_itm_write((uint8_t*)b, (size_t)len);
#endif
}

/* API pública: printf estilo printf */
static inline void log_print(int level, const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprint_core(level, tag, fmt, ap);
    va_end(ap);
}


/* ============================================================
 * ===============  Redirecionamento do printf    ===============
 * ============================================================ */

/* Faz o printf() padrão ir para os mesmos destinos (UART/ITM) */
#ifdef __GNUC__
int _write(int file, char *ptr, int len)
{
    (void)file;
#if LOG_USE_UART
    log_uart_write((uint8_t*)ptr, (size_t)len);
#endif
#if LOG_USE_ITM
    log_itm_write((uint8_t*)ptr, (size_t)len);
#endif
    return len;
}
#else
int fputc(int ch, FILE *f)
{
    (void)f;
    uint8_t c = (uint8_t)ch;
#if LOG_USE_UART
    log_uart_write(&c, 1);
#endif
#if LOG_USE_ITM
    log_itm_write(&c, 1);
#endif
    return ch;
}
#endif


/* ============================================================
 * ===============          MACROS DE LOG         ===============
 * ============================================================ */

#define LOG(level, tag, fmt, ...) \
    do { \
        if ((level) >= LOG_LEVEL && LOG_LEVEL != LOG_LEVEL_NONE) { \
            log_print((level), (tag), fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, LOG_TAG, "DEBUG: " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(LOG_LEVEL_INFO,  LOG_TAG, "INFO: "  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(LOG_LEVEL_WARN,  LOG_TAG, "WARN: "  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, LOG_TAG, "ERROR: " fmt, ##__VA_ARGS__)

#endif /* INC_LOG_H_ */
