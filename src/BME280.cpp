#include "BME280.h"

BME280::BME280(i2c_inst_t* i2c, uint8_t address)
    : _i2c(i2c),
      _address(address) {
}

bool BME280::init() {
    uint8_t chipId = 0;

    if (!readRegister(0xD0, chipId)) {
        return false;
    }

    if (chipId != 0x60) {
        return false;
    }

    if (!readCalibrationData()) {
        return false;
    }

    /*
     * ctrl_hum 0xF2
     *
     * humidity oversampling x1
     */
    if (!writeRegister(0xF2, 0x01)) {
        return false;
    }

    /*
     * config 0xF5
     *
     * t_sb   = 1000 ms
     * filter = off
     * spi3w  = off
     *
     * 10100000 = 0xA0
     */
    if (!writeRegister(0xF5, 0xA0)) {
        return false;
    }

    /*
     * ctrl_meas 0xF4
     *
     * temperature oversampling x1
     * pressure oversampling x1
     * normal mode
     *
     * 00100111 = 0x27
     */
    if (!writeRegister(0xF4, 0x27)) {
        return false;
    }

    return true;
}

bool BME280::readCalibrationData() {
    uint8_t calib1[26];

    if (!readRegisters(0x88, calib1, sizeof(calib1))) {
        return false;
    }

    dig_T1 = static_cast<uint16_t>(
        (static_cast<uint16_t>(calib1[1]) << 8) |
        calib1[0]
    );

    dig_T2 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[3]) << 8) |
        calib1[2]
    );

    dig_T3 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[5]) << 8) |
        calib1[4]
    );

    dig_P1 = static_cast<uint16_t>(
        (static_cast<uint16_t>(calib1[7]) << 8) |
        calib1[6]
    );

    dig_P2 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[9]) << 8) |
        calib1[8]
    );

    dig_P3 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[11]) << 8) |
        calib1[10]
    );

    dig_P4 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[13]) << 8) |
        calib1[12]
    );

    dig_P5 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[15]) << 8) |
        calib1[14]
    );

    dig_P6 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[17]) << 8) |
        calib1[16]
    );

    dig_P7 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[19]) << 8) |
        calib1[18]
    );

    dig_P8 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[21]) << 8) |
        calib1[20]
    );

    dig_P9 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib1[23]) << 8) |
        calib1[22]
    );

    if (!readRegister(0xA1, dig_H1)) {
        return false;
    }

    uint8_t calib2[7];

    if (!readRegisters(0xE1, calib2, sizeof(calib2))) {
        return false;
    }

    dig_H2 = static_cast<int16_t>(
        (static_cast<uint16_t>(calib2[1]) << 8) |
        calib2[0]
    );

    dig_H3 = calib2[2];

    /*
     * H4 is a signed 12-bit value:
     *
     * E4 = bits 11..4
     * E5 = bits 3..0
     *
     * Cast E4 to int8_t first so its sign is extended.
     */
    dig_H4 = static_cast<int16_t>(
        (static_cast<int16_t>(
            static_cast<int8_t>(calib2[3])
        ) << 4) |
        (calib2[4] & 0x0F)
    );

    /*
     * H5 is a signed 12-bit value:
     *
     * E6 = bits 11..4
     * E5 = bits 7..4
     */
    dig_H5 = static_cast<int16_t>(
        (static_cast<int16_t>(
            static_cast<int8_t>(calib2[5])
        ) << 4) |
        (calib2[4] >> 4)
    );

    dig_H6 = static_cast<int8_t>(calib2[6]);

    return true;
}

bool BME280::readSensor(
    float& temperature,
    float& pressure,
    float& humidity
) {
    uint8_t data[8];

    if (!readRegisters(0xF7, data, sizeof(data))) {
        return false;
    }

    const int32_t adc_P =
        (static_cast<int32_t>(data[0]) << 12) |
        (static_cast<int32_t>(data[1]) << 4) |
        (static_cast<int32_t>(data[2]) >> 4);

    const int32_t adc_T =
        (static_cast<int32_t>(data[3]) << 12) |
        (static_cast<int32_t>(data[4]) << 4) |
        (static_cast<int32_t>(data[5]) >> 4);

    const int32_t adc_H =
        (static_cast<int32_t>(data[6]) << 8) |
        data[7];

    /*
     * Temperature must be compensated first because it calculates
     * t_fine, which pressure and humidity both depend upon.
     */
    const int32_t t = compensateTemperature(adc_T);
    const uint32_t p = compensatePressure(adc_P);
    const uint32_t h = compensateHumidity(adc_H);

    /*
     * Temperature result:
     *   5123 = 51.23 °C
     */
    temperature = static_cast<float>(t) / 100.0f;

    /*
     * Pressure result is Pa.
     *
     * Convert to hPa.
     */
    pressure = static_cast<float>(p) / 100.0f;

    /*
     * Humidity result:
     *   1024 = 1 %RH
     */
    humidity = static_cast<float>(h) / 1024.0f;

    return true;
}

bool BME280::readRegister(uint8_t reg, uint8_t& value) {
    const int written = i2c_write_blocking(
        _i2c,
        _address,
        &reg,
        1,
        true
    );

    if (written != 1) {
        return false;
    }

    const int read = i2c_read_blocking(
        _i2c,
        _address,
        &value,
        1,
        false
    );

    return read == 1;
}

bool BME280::readRegisters(
    uint8_t reg,
    uint8_t* buffer,
    size_t length
) {
    const int written = i2c_write_blocking(
        _i2c,
        _address,
        &reg,
        1,
        true
    );

    if (written != 1) {
        return false;
    }

    const int read = i2c_read_blocking(
        _i2c,
        _address,
        buffer,
        length,
        false
    );

    return read == static_cast<int>(length);
}

bool BME280::writeRegister(uint8_t reg, uint8_t value) {
    const uint8_t buffer[2] = {
        reg,
        value
    };

    const int written = i2c_write_blocking(
        _i2c,
        _address,
        buffer,
        sizeof(buffer),
        false
    );

    return written == sizeof(buffer);
}

/*
 * Bosch BME280 integer compensation algorithm.
 *
 * Returns temperature in hundredths of a degree C.
 *
 * For example:
 *
 *   2345 = 23.45 °C
 */
int32_t BME280::compensateTemperature(int32_t adc_T) {
    const int32_t var1 =
        (((adc_T >> 3) -
          (static_cast<int32_t>(dig_T1) << 1)) *
         static_cast<int32_t>(dig_T2)) >> 11;

    const int32_t var2 =
        (((((adc_T >> 4) -
            static_cast<int32_t>(dig_T1)) *
           ((adc_T >> 4) -
            static_cast<int32_t>(dig_T1))) >> 12) *
         static_cast<int32_t>(dig_T3)) >> 14;

    t_fine = var1 + var2;

    return (t_fine * 5 + 128) >> 8;
}

/*
 * Returns pressure in Pa.
 */
uint32_t BME280::compensatePressure(int32_t adc_P) {
    int64_t var1 =
        static_cast<int64_t>(t_fine) - 128000;

    int64_t var2 =
        var1 *
        var1 *
        static_cast<int64_t>(dig_P6);

    var2 +=
        (var1 * static_cast<int64_t>(dig_P5)) << 17;

    var2 +=
        static_cast<int64_t>(dig_P4) << 35;

    var1 =
        ((var1 *
          var1 *
          static_cast<int64_t>(dig_P3)) >> 8) +
        ((var1 *
          static_cast<int64_t>(dig_P2)) << 12);

    var1 =
        (((static_cast<int64_t>(1) << 47) + var1) *
         static_cast<int64_t>(dig_P1)) >> 33;

    if (var1 == 0) {
        return 0;
    }

    int64_t p =
        1048576 - adc_P;

    p =
        (((p << 31) - var2) * 3125) /
        var1;

    var1 =
        (static_cast<int64_t>(dig_P9) *
         (p >> 13) *
         (p >> 13)) >> 25;

    var2 =
        (static_cast<int64_t>(dig_P8) *
         p) >> 19;

    p =
        ((p + var1 + var2) >> 8) +
        (static_cast<int64_t>(dig_P7) << 4);

    return static_cast<uint32_t>(p >> 8);
}

/*
 * Returns humidity in units of 1/1024 %RH.
 *
 * For example:
 *
 *   51200 / 1024 = 50.0 %RH
 */
uint32_t BME280::compensateHumidity(int32_t adc_H) {
    int32_t v_x1 =
        t_fine - static_cast<int32_t>(76800);

    v_x1 =
        (((((adc_H << 14) -
            (static_cast<int32_t>(dig_H4) << 20) -
            (static_cast<int32_t>(dig_H5) * v_x1)) +
           static_cast<int32_t>(16384)) >> 15) *

         (((((((v_x1 *
               static_cast<int32_t>(dig_H6)) >> 10) *

             (((v_x1 *
                static_cast<int32_t>(dig_H3)) >> 11) +
              static_cast<int32_t>(32768))) >> 10) +

            static_cast<int32_t>(2097152)) *

           static_cast<int32_t>(dig_H2) +
           8192) >> 14));

    v_x1 =
        v_x1 -
        (((((v_x1 >> 15) *
            (v_x1 >> 15)) >> 7) *
          static_cast<int32_t>(dig_H1)) >> 4);

    if (v_x1 < 0) {
        v_x1 = 0;
    }

    if (v_x1 > 419430400) {
        v_x1 = 419430400;
    }

    return static_cast<uint32_t>(v_x1 >> 12);
}