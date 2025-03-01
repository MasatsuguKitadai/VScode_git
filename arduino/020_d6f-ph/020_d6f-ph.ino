/*
 * MIT License
 * Copyright (c) 2019 - present OMRON Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* includes */
#include <Wire.h>

/* defines */
#define D6F_ADDR 0x6C // D6F-PH I2C client address at 7bit expression

uint8_t conv16_u8_h(int16_t a)
{
    return (uint8_t)(a >> 8);
}

uint8_t conv16_u8_l(int16_t a)
{
    return (uint8_t)(a & 0xFF);
}

uint16_t conv8us_u16_be(uint8_t *buf)
{
    return (uint16_t)(((uint32_t)buf[0] << 8) | (uint32_t)buf[1]);
}

/** <!-- i2c_write_reg16 {{{1 --> I2C write bytes with a 16bit register.
 */
bool i2c_write_reg16(uint8_t slave_addr, uint16_t register_addr,
                     uint8_t *write_buff, uint8_t len)
{
    Wire.beginTransmission(slave_addr);

    Wire.write(conv16_u8_h(register_addr));
    Wire.write(conv16_u8_l(register_addr));

    if (len != 0)
    {
        for (uint8_t i = 0; i < len; i++)
        {
            Wire.write(write_buff[i]);
        }
    }
    Wire.endTransmission();
    return false;
}

/** <!-- i2c_read_reg8 {{{1 --> I2C read bytes with a 8bit register.
 */
bool i2c_read_reg8(uint8_t slave_addr, uint8_t register_addr,
                   uint8_t *read_buff, uint8_t len)
{
    Wire.beginTransmission(slave_addr);
    Wire.write(register_addr);
    Wire.endTransmission();

    Wire.requestFrom(slave_addr, len);

    if (Wire.available() != len)
    {
        return true;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        read_buff[i] = Wire.read();
    }
    return false;
}

/** <!-- setup {{{1 -->
 * 1. initialize a Serial port for output.
 * 2. initialize an I2C peripheral.
 * 3. setup sensor settings.
 */
void setup()
{
    Serial.begin(115200);
    Serial.println("peripherals: I2C");
    Wire.begin(); // i2c master

    Serial.println("sensor: Differential pressure Sensor");
    delay(32);

    // 1. Initialize sensor (0Bh, 00h)
    i2c_write_reg16(D6F_ADDR, 0x0B00, NULL, 0);
}

/** <!-- loop - Differential pressure sensor {{{1 -->
 * 1. read and convert sensor.
 * 2. output results, format is: [Pa]
 */
void loop()
{
    delay(900);

    // 2. Trigger getting data (00h, D0h, 40h, 18h, 06h)
    uint8_t send0[] = {0x40, 0x18, 0x06};
    i2c_write_reg16(D6F_ADDR, 0x00D0, send0, 3);

    delay(50); // wait 50ms

    // 3. Read data (00h, D0h, 51h, 2Ch) (07h)
    uint8_t send1[] = {0x51, 0x2C};
    i2c_write_reg16(D6F_ADDR, 0x00D0, send1, 2);
    uint8_t rbuf[2];
    if (i2c_read_reg8(D6F_ADDR, 0x07, rbuf, 2))
    { // read from [07h]
        return;
    }
    uint16_t rd_flow = conv8us_u16_be(rbuf);
    float flow_rate;

    // calculation for 0-250[Pa] range
    flow_rate = ((float)rd_flow - 1024.0) * 250 / 60000.0;

//    Serial.print(flow_rate, 2); // print converted flow rate
    // Serial.println(" [Pa]");

    float c = 1;
    float rho = 1.29; // [kg/m3] | 標準状態：温度 0 度，圧力1気圧 | url:https://www.jsme.or.jp/jsme-medwiki/09:1012584

    float v = 0;

    v = c * sqrt(2 * flow_rate / rho) * 3600 / 1000;

    Serial.print(v, 2);
    Serial.println(" [km/h]");
}
    // vi: ft=arduino:fdm=marker:et:sw=4:tw=80
