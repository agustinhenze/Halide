#include <stdio.h>
#include <Halide.h>

using namespace Halide;

int main(int argc, char **argv) {
    if (!get_jit_target_from_environment().has_gpu_feature()) {
        printf("No gpu target enabled. Skipping test.\n");
        return 0;
    }

    Var x, y, z;
    Func f;

    Param<int> k;
    k.set(3);

    f(x, y, z) = x*y+z*k+1;

    f.gpu_tile(x, y, 16, 16, GPU_Default);
    f.parallel(z);

    Image<int> im = f.realize(64, 64, 64);

    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            for (int z = 0; z < 64; z++) {
                if (im(x, y, z) != x*y+z*3+1) {
                    printf("im(%d, %d, %d) = %d\n", x, y, z, im(x, y, z));
                    return -1;
                }
            }
        }
    }

    printf("Success!\n");
    return 0;
}
