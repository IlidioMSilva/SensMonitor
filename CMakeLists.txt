cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(isilva_prvproject)

target_sources(app PRIVATE
  src/main.c
  sensores/dht11_sensor.c
  sensores/bme280_sensor.c
  sensores/rtc_ds3231m.c
  sensores/sensor_data.c
  ble/ble_manager.c
  ble/bme280_service.c
  #wifi/spi_manager.c
  wifi/uart_manager.c
)

# Inclui diretório 'sensores' para headers personalizados
zephyr_include_directories(sensores)
zephyr_include_directories(ble)
zephyr_include_directories(wifi)