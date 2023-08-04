cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

project(bot_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index_html.h
	COMMAND xxd -i index.html > ${CMAKE_CURRENT_BINARY_DIR}/index_html.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index.html
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index_js.h
	COMMAND xxd -i index.js > ${CMAKE_CURRENT_BINARY_DIR}/index_js.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index.js
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index_css.h
	COMMAND xxd -i index.css > ${CMAKE_CURRENT_BINARY_DIR}/index_css.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index.css
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(firmware
	firmware.c
	index_html.h
	index_js.h
	index_css.h
)

target_include_directories(firmware PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(firmware PRIVATE
	WIFI_SSID=\"${WIFI_SSID}\"
	WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

target_link_libraries(firmware
	pico_cyw43_arch_lwip_poll
	pico_stdlib
)

add_compile_options(-Wall)
pico_enable_stdio_uart(firmware 1)
pico_add_extra_outputs(firmware)

