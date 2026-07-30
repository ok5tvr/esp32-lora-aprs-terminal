#include <cassert>
#include <cstdio>

#include "ui/aprs_symbol_lookup.h"

int main() {
    using Ui::AprsSymbols::resolve;

    const auto primaryCar = resolve('/', '>', true);
    assert(primaryCar.valid);
    assert(!primaryCar.alternate);
    assert(primaryCar.index == static_cast<unsigned>('>' - '!'));
    assert(primaryCar.overlay == '\0');

    const auto alternateCar = resolve('\\', '>', true);
    assert(alternateCar.valid);
    assert(alternateCar.alternate);
    assert(alternateCar.index == primaryCar.index);

    const auto loraIgate = resolve('L', '&', true);
    assert(loraIgate.valid);
    assert(loraIgate.alternate);
    assert(loraIgate.overlay == 'L');

    const auto compressedNumericOverlay = resolve('c', '#', true);
    assert(compressedNumericOverlay.valid);
    assert(compressedNumericOverlay.alternate);
    assert(compressedNumericOverlay.overlay == '2');

    assert(!resolve('/', ' ', true).valid);
    assert(!resolve('/', '>', false).valid);
    assert(!resolve('?', '>', true).valid);

    std::puts("aprs_symbol_lookup tests passed");
    return 0;
}
