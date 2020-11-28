#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define elementsof(__x) ({ \
    (sizeof(__x) / sizeof((__x)[0])); \
})

#endif // TYPES_H
