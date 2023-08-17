cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

project(bot_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(firmware
	firmware.c
	dhcpserver/dhcpserver.c
)

target_include_directories(firmware PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/dhcpserver
)

target_link_libraries(firmware
	pico_cyw43_arch_lwip_poll
	pico_stdlib
	pico_unique_id
	pico_time
	hardware_gpio
	hardware_pwm
)

add_compile_options(-Wall)
pico_enable_stdio_uart(firmware 1)
pico_enable_stdio_usb(firmware 1)
pico_add_extra_outputs(firmware)

