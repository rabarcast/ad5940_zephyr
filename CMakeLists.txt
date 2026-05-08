cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(proyectoSAC)

target_sources(app PRIVATE
    src/main.c

    # Drivers
    scripts/drivers/BNO055.c
    scripts/drivers/ble.c

    # Utils
    scripts/utils/fall_detection.c
    scripts/utils/utils.c
)

target_include_directories(app PRIVATE
    scripts/drivers
    scripts/utils
)