#pragma once

#include "hardware/i2c.h"
#include <cstddef>
#include <cstdint>

class BME280 {
public:
    BME280(i2c_inst_t* i2c, uint8_t address);

    bool init();

    bool readSensor(
        float& temperature,
        float& pressure,
        float& humidity
    );

private:
    i2c_inst_t* _i2c;
    uint8_t _address;

    // Temperature calibration
    uint16_t dig_T1 = 0;
    int16_t  dig_T2 = 0;
    int16_t  dig_T3 = 0;

    // Pressure calibration
    uint16_t dig_P1 = 0;
    int16_t  dig_P2 = 0;
    int16_t  dig_P3 = 0;
    int16_t  dig_P4 = 0;
    int16_t  dig_P5 = 0;
    int16_t  dig_P6 = 0;
    int16_t  dig_P7 = 0;
    int16_t  dig_P8 = 0;
    int16_t  dig_P9 = 0;

    // Humidity calibration
    uint8_t dig_H1 = 0;
    int16_t dig_H2 = 0;
    uint8_t dig_H3 = 0;
    int16_t dig_H4 = 0;
    int16_t dig_H5 = 0;
    int8_t  dig_H6 = 0;

    int32_t t_fine = 0;

    bool readCalibrationData();

    bool readRegister(uint8_t reg, uint8_t& value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);
    bool writeRegister(uint8_t reg, uint8_t value);

    int32_t compensateTemperature(int32_t adc_T);
    uint32_t compensatePressure(int32_t adc_P);
    uint32_t compensateHumidity(int32_t adc_H);
};