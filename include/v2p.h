#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum paging_mode {
    LEGACY = 2,
    PAE = 3,
} paging_mode_t;

typedef enum error {
    SUCCESS = 0,
    NO_TRANSLATION = -1,
    PAGE_FAULT = -2,
    READ_FAULT = -3,
    INVALID_TRANSLATION_TYPE = -4,
    GENERAL_PROTECTION_EXCEPTION = -5,
} error_t;

enum flags {
    // PDE
    P_PDE4KB = 0,
    P_PDE4MB = 0,
    PS_PDE4KB = 7,
    PS_PDE4MB = 7,

    // PTE
    P_PTE = 0,
    PAT_PTE = 7,
};

// чтение указанного количества байт физической памяти по заданному адресу в указанный буфер
// функция вернет количество прочитанных байт (меньшее или 0 означает ошибку - выход за пределы памяти)
typedef uint32_t (*pread_func_t)(void *buf, const uint32_t size, const uint64_t physical_addr);

typedef struct config {
    paging_mode_t level;
    uint32_t root_addr;
    pread_func_t read_func;

    // paging features
    bool pse;
    bool pse36;
    bool pat;
    uint8_t maxphyaddr;
} config_t;


// функция трансляции виртуального адреса в физический:
//
// virt_addr - виртуальный адрес подлежащий трансляции
// level - количество уровней трансляции 2 или 3 (соответствуют legacy и PAE трансляциям в x86)
// root_addr - физический адрес корня дерева трансляции (соответствует регистру CR3)
// read_func - функция для чтения физической памяти (см. объявление выше)
// функция возвращает успешность трансляции: 0 - успешно, не 0 - ошибки
// phys_addr - выходной оттранслированный физический адрес
int
va2pa(uint32_t virt_addr, const config_t *cfg, uint64_t *phys_addr);

