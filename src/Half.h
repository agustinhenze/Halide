#ifndef HALIDE_HALF_H
#define HALIDE_HALF_H

#include "Type.h"

namespace Halide {

struct half {
};

template<> Type type_of<half>() {
    return Float(16);
}

}  // Halide

#endif  // HALIDE_HALF_H
