#include "wire.h"
#include "gpio.h"
#include "tim.h"
#include "usart.h"

//This version of the code uses UART instead of bit-banging
//Prons - lowers the usage of the CPU, allows for more interrupts, simpler, no delays
//Cons - requires free UART port, may increase power consumption

//Still uses only one wire

HAL_StatusTypeDef wire_init(void)
{
  return HAL_TIM_Base_Start(&htim6);
}

static void set_baudrate(uint32_t baudrate)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = baudrate;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  huart3.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;

  if (HAL_HalfDuplex_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

HAL_StatusTypeDef wire_reset(void)
{
  uint8_t data_out = 0xF0;
  uint8_t data_in = 0;

  set_baudrate(9600);
  HAL_UART_Transmit(&huart3, &data_out, 1, HAL_MAX_DELAY);
  HAL_UART_Receive(&huart3, &data_in, 1, HAL_MAX_DELAY);
  set_baudrate(115200);

  if (data_in != 0xF0)
    return HAL_OK;
  else
    return HAL_ERROR;
}


static int read_bit(void)
{
  uint8_t data_out = 0xFF;
  uint8_t data_in = 0;
  HAL_UART_Transmit(&huart3, &data_out, 1, HAL_MAX_DELAY);
  HAL_UART_Receive(&huart3, &data_in, 1, HAL_MAX_DELAY);

  return data_in & 0x01;
}

//reads a byte - merges bits (quite useful)
//starts from LSB to MSB
uint8_t wire_read(void)
{
  uint8_t value = 0;
  int i;
  for (i = 0; i < 8; i++)
  {
	value = value >> 1;  //gdyby od najbardziej -> <<
    if (read_bit())
    {
      value = value | 0x80; //gdyby od najbardziej -> 0x01
    }
  }
  return value;
}


//Sends a bit
static void write_bit(int value)
{
  if (value)
  {
	  uint8_t data_out = 0xff;
	  HAL_UART_Transmit(&huart3, &data_out, 1, HAL_MAX_DELAY);
  }
  else
  {
	  uint8_t data_out = 0x0;
	  HAL_UART_Transmit(&huart3, &data_out, 1, HAL_MAX_DELAY);
  }
}


//Sends a byte
void wire_write(uint8_t byte)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    write_bit(byte & 0x01); //bit by bit
    byte = byte >> 1;
  }
}

uint8_t static byte_crc(uint8_t crc, uint8_t byte) //calculates CRC for a byte
{
  int i;
  for (i = 0; i < 8; i++)
  {
    uint8_t b = crc ^ byte;
    crc = crc >> 1;
    if (b & 0x01)
    {
      crc = crc ^ 0x8c;
    }
    byte = byte >> 1;
  }
  return crc;
}

uint8_t wire_crc(const uint8_t* data, int len) //calculates CRC for a data block
{
  int i;
  uint8_t crc = 0;
  for (i = 0; i < len; i++)
  {
	  crc = byte_crc(crc, data[i]);
  }
  return crc;
}
