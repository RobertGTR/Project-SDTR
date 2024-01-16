project(wifi_test C CXX ASM)
add_executable(wifi_test wifi_test.c)
target_include_directories(wifi_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..)
target_compile_definitions(wifi_test PRIVATE
        WIFI_SSID=\"eror\"
        WIFI_PASSWORD=\"AnaAre1Mere\"
        NO_SYS=0)
target_link_libraries(wifi_test pico_cyw43_arch_lwip_sys_freertos pico_stdlib pico_lwip_iperf FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)

# enable uart@115200
pico_enable_stdio_usb(wifi_test 1)
pico_enable_stdio_uart(wifi_test 0)

pico_add_extra_outputs(wifi_test)