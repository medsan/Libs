/*
  Arduino library for ADS7828, a 8-channel single-ended/4-channel differential
  12-bit ADC from TI with an I2C interface.
 */

#include "Arduino.h"
#include "ADS7828.h"
// Initialize the logging with the tag "TCA64xxA"
static const char *LOG_ADS7828 = "ADS7828";

// SemaphoreHandle_t ADS7828::i2cSemaphore = NULL;

ADS7828::ADS7828()
{
  // i2cSemaphore = xSemaphoreCreateMutex(); // Criar o semáforo de mutex
}

// i2caddr need only be the two address bits for the part (0 .. 3).
// twire defaults to a pointer to Wire, but it can be a pointer to a
// different TwoWire instance, such as Wire1, in a system with more than
// one i2c bus.

void ADS7828::begin(uint8_t i2caddr, TwoWire *twire)
{
  if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY) == pdTRUE)
  {
    ESP_LOGI(LOG_ADS7828, "Iniciando ... ADS7828!");

    _i2caddr = ADS7828_I2CADDR | (i2caddr & 0x3);
    _wire = twire;
    _wire->begin();

    xSemaphoreGive(i2cSemaphore); // Liberar o semáforo após a seção crítica
  }
  else
  {
    ESP_LOGE(LOG_ADS7828, "Erro ao adquirir o semáforo de mutex no método begin");
  }
}

// // Common code to read 12-bit data from channel
// uint16_t ADS7828::_readData(uint8_t channel, bool single)
// {
//   if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY) == pdTRUE)
//   {
//     uint8_t cmd = single ? SINGLE_ENDED : DIFFERENTIAL;
//     // force channel into 3 bits in case channel is out of range
//     channel = channel & 0x7;
//     uint8_t csel = (channel >> 1) | ((channel & 0x1) << 2);
//     cmd = cmd | csel << 4 | IREF_OFF_AD_ON;
//     // cmd = cmd | csel << 4 | PD_BTWN_CONV;
//     _wire->beginTransmission(_i2caddr);
//     _wire->write(cmd);
//     _wire->endTransmission();
//     _wire->requestFrom(_i2caddr, 2);
//    while (_wire->available() < 2)
//     {
//       // Wait for data to be available
//     }
//     uint16_t result = (_wire->read() & 0x0F) << 8;
//     result = result + _wire->read();
//     xSemaphoreGive(i2cSemaphore);
//     return result;
//   }
//   else
//   {
//     ESP_LOGE(LOG_ADS7828, "Erro ao adquirir o semáforo de mutex no método readI2CBuffer");
//     return 0; // Return a default value or handle the error accordingly
//   }
// }

// Common code to read 12-bit data from channel
uint16_t ADS7828::_readData(uint8_t channel, bool single)
{
  // Número máximo de tentativas de leitura
  const uint8_t maxRetries = 3;
  uint16_t result = 0;

  // Adquirir o semáforo de mutex
  for (uint8_t attempt = 0; attempt < maxRetries; ++attempt)
  {
    if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY) == pdTRUE)
    {
      uint8_t cmd = single ? SINGLE_ENDED : DIFFERENTIAL;
      // force channel into 3 bits in case channel is out of range
      channel = channel & 0x7;
      uint8_t csel = (channel >> 1) | ((channel & 0x1) << 2);
      cmd = cmd | csel << 4 | IREF_OFF_AD_ON;
      // cmd = cmd | csel << 4 | PD_BTWN_CONV;

      _wire->beginTransmission(_i2caddr);

      // Send the command
      _wire->write(cmd);
      if (_wire->endTransmission() != 0)
      {
        xSemaphoreGive(i2cSemaphore); // Release semaphore on failure
        continue;                     // Retry if endTransmission failed
      }

      // Request 2 bytes from the device
      _wire->requestFrom(_i2caddr, 2);

      // Wait for data to be available
      unsigned long startTime = millis();
      while (_wire->available() < 2)
      {
        if (millis() - startTime > 100) // Timeout after 100ms
        {
          xSemaphoreGive(i2cSemaphore); // Release semaphore on timeout
          break;
        }
      }

      // Check if data is available
      if (_wire->available() >= 2)
      {
        result = (_wire->read() & 0x0F) << 8;
        result = result + _wire->read();
        xSemaphoreGive(i2cSemaphore);
        return result; // Successful read, return result
      }

      // Release the semaphore before retry
      xSemaphoreGive(i2cSemaphore);
    }
    else
    {
      ESP_LOGE(LOG_ADS7828, "Erro ao adquirir o semáforo de mutex no método readI2CBuffer");
    }
  }

  // If we reach here, all attempts have failed
  ESP_LOGE(LOG_ADS7828, "Erro de leitura I2C após %d tentativas", maxRetries);
  return 0; // Return a default value or handle the error accordingly
}

// read single-ended channel data
uint16_t ADS7828::read(uint8_t channel)
{
  // xSemaphoreTake(i2cSemaphore, portMAX_DELAY); // Adquire o semáforo antes de acessar o barramento I2C
  uint16_t result = _readData(channel, true);
  // xSemaphoreGive(i2cSemaphore); // Libera o semáforo após o acesso ao barramento I2C
  return result;
}

// read differential channel data
uint16_t ADS7828::readdif(uint8_t channel)
{

  // xSemaphoreTake(i2cSemaphore, portMAX_DELAY); // Adquire o semáforo antes de acessar o barramento I2C

  uint16_t result = _readData(channel, false);

  // xSemaphoreGive(i2cSemaphore); // Libera o semáforo após o acesso ao barramento I2C
  return result;
}
