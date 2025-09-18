# Sensor Data Module — nRF52832 (Zephyr SDK 2.9.1)

This project demonstrates the integration of multiple sensors and a real-time clock (RTC) into the Nordic nRF52832 using **Zephyr RTOS SDK 2.9.1**.  
The main control file `sensor_data.c` manages sensor initialization, periodic readings, and provides a snapshot API for retrieving the latest measurements.

---

## Overview

- **`sensor_data.c`**
  - Creates a **dedicated background thread** that periodically reads all sensors.
  - Stores the latest values in a global `sensor_snapshot_t` structure.
  - Can operate in **dummy data mode** for testing without hardware.
  - Provides an API function `sensor_data_get_snapshot()` to retrieve the last sensor readings.
  - Initializes each sensor driver exactly once, logging failures and successes.

---

## DHT11 — Temperature & Humidity Sensor

### Wiring
| Pin              | Connection                       |
|------------------|----------------------------------|
| Data             | GPIO P0.13 (configured in device tree) |
| VCC              | 3.3V                             |
| GND              | Ground                           |
| Pull-up resistor | 10kΩ on data line                |

### Protocol
- **Start signal:** MCU drives the line **LOW for ≥18 ms**, then **HIGH for 20–40 µs**.  
- **Sensor response:** 80 µs LOW + 80 µs HIGH.  
- **Data:** 40 bits total:  
  - 8 bits integral humidity  
  - 8 bits decimal humidity  
  - 8 bits integral temperature  
  - 8 bits decimal temperature  
  - 8 bits checksum  
- **Bit encoding:** duration of HIGH pulse → ~26 µs for logic `0`, ~70 µs for logic `1`.  
- **Timing critical:** implemented with `k_busy_wait()` for microsecond precision.  
- **Minimum interval between readings:** 1 second recommended.  

### Driver Implementation
- **File:** `sensors/dht11_sensor.c`  
- **Interface:** `sensors/dht11_sensor.h`  
- **Main functions:**  
  - `dht11_sensor_init()` → Configure GPIO and idle state.  
  - `dht11_sensor_read(uint8_t *temperature, uint8_t *humidity)` → Perform the start sequence, capture 40 bits, validate checksum, return values.  
- **Debugging:** Use a logic analyzer to verify pulse durations and timing sequence.  

---

## BME280 — Temperature, Humidity & Pressure Sensor

### Wiring
Connected to **I²C0** bus:  
| Pin | Connection |
|-----|------------|
| SDA | P0.26 (I²C0 SDA on nRF52 DK default) |
| SCL | P0.27 (I²C0 SCL on nRF52 DK default) |
| VCC | 3.3V |
| GND | Ground |

### Protocol
- **Interface:** I²C, 7-bit device address = `0x76`.  
- Supports standard and fast I²C modes.  
- Provides compensated sensor data using factory calibration registers.  
- Measured channels:  
  - `SENSOR_CHAN_AMBIENT_TEMP` (°C)  
  - `SENSOR_CHAN_HUMIDITY` (%)  
  - `SENSOR_CHAN_PRESS` (hPa)  
- Zephyr provides a native `bosch,bme280` driver, simplifying integration.  

### Driver Implementation
- **File:** `sensors/bme280_sensor.c`  
- **Interface:** `sensors/bme280_sensor.h`  
- **Main functions:**  
  - `bme280_sensor_init()` → Bind device from devicetree, validate readiness.  
  - `bme280_sensor_read(struct bme280_data *data)` → Trigger sample fetch, retrieve values via Zephyr’s sensor API.  
- **Debugging:** Use `sensor_channel_get()` return codes to verify data availability.  

---

## DS3231M — Real-Time Clock

### Wiring
Connected to **I²C0** bus:  
| Pin | Connection |
|-----|------------|
| SDA | P0.26 (I²C0 SDA on nRF52 DK default) |
| SCL | P0.27 (I²C0 SCL on nRF52 DK default) |
| VCC | 3.3V |
| GND | Ground |

### Protocol
- **Interface:** I²C, 7-bit device address = `0x68`.  
- Time/date stored in **BCD (Binary-Coded Decimal)** registers.  
- Provides year, month, day, hour, minute, and second.  
- Registers must be converted from BCD → decimal when reading, and decimal → BCD when writing.  
- Supports battery backup for keeping time during power loss.  

### Driver Implementation
- **File:** `sensors/rtc_ds3231m.c`  
- **Interface:** `sensors/rtc_ds3231m.h`  
- **Main functions:**  
  - `rtc_ds3231m_init()` → Initialize RTC, bind to `i2c0` from device tree.  
  - `rtc_ds3231m_get_time(struct rtc_time *time)` → Read registers, convert BCD → integers, populate struct.  
  - `rtc_ds3231m_set_time(const struct rtc_time *time)` → Convert integers → BCD, write to registers.  
- **Debugging:** Enable raw buffer logging (`LOG_HEXDUMP_INF`) to inspect I²C data.  

---

## Execution Flow

1. **Initialization**  
   - `sensors_init()` initializes DHT11, BME280, and DS3231M.  
   - If enabled, dummy data mode bypasses hardware access.  

2. **Background Thread**  
   - A dedicated thread (`sensor_thread`) wakes every `SENSOR_READ_INTERVAL_SECONDS` (default: 60 s).  
   - Reads all sensors sequentially and updates the global snapshot.  

3. **Data Retrieval**  
   - Other modules call `sensor_data_get_snapshot(sensor_snapshot_t *dest)` to fetch the last measurements.  

4. **Logging**  
   - Sensor readings and failures are logged via Zephyr’s `LOG_INF` and `LOG_ERR` APIs.  
