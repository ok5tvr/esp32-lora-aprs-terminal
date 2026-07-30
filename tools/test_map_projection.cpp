#include <cassert>
#include <cmath>
#include <iostream>

#include "services/map_projection.h"

namespace {

bool near(double left, double right, double tolerance = 1e-6) {
    return std::fabs(left - right) <= tolerance;
}

}  // namespace

int main() {
    using namespace Services::MapProjection;

    const WorldPixel origin = toWorldPixel(0.0, 0.0, 0);
    assert(origin.valid);
    assert(near(origin.x, 128.0));
    assert(near(origin.y, 128.0));
    assert(near(origin.worldSize, 256.0));

    const WorldPixel pilsen = toWorldPixel(49.786333, 13.285000, 13);
    assert(pilsen.valid);
    const ScreenPoint centered = projectToViewport(
        49.786333, 13.285000, 13, pilsen.x, pilsen.y, 480, 202);
    assert(centered.valid);
    assert(near(centered.x, 240.0));
    assert(near(centered.y, 101.0));

    const ScreenPoint east = projectToViewport(
        49.786333, 13.295000, 13, pilsen.x, pilsen.y, 480, 202);
    assert(east.valid);
    assert(east.x > centered.x);

    const WorldPixel datelineCenter = toWorldPixel(0.0, 179.9, 4);
    const ScreenPoint wrapped = projectToViewport(
        0.0, -179.9, 4, datelineCenter.x, datelineCenter.y, 480, 202);
    assert(wrapped.valid);
    assert(std::fabs(wrapped.x - 240.0) < 10.0);

    assert(!toWorldPixel(100.0, 0.0, 10).valid);
    assert(!projectToViewport(0.0, 0.0, 10, NAN, 0.0, 480, 202).valid);

    std::cout << "map projection tests passed\n";
    return 0;
}
