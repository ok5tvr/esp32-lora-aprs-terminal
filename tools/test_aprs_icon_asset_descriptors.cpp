#include <cassert>
#include <cstddef>
#include <cstring>
#include <cstdio>

#include "ui/icons/aprs_icon_assets.h"

int main() {
    using namespace Ui::AprsIconAssets;
    static_assert(SYMBOL_COUNT == 94U, "APRS !..~ range must contain 94 symbols");

    for (std::size_t index = 0; index < SYMBOL_COUNT; ++index) {
        assert(primary[index].header.w == 30U);
        assert(primary[index].header.h == 30U);
        assert(primary[index].data_size == 30U * 30U * 2U);
        assert(primary[index].data != nullptr);
        assert(alternate[index].header.w == 30U);
        assert(alternate[index].header.h == 30U);
        assert(alternate[index].data_size == 30U * 30U * 2U);
        assert(alternate[index].data != nullptr);
    }

    const std::size_t jeep = static_cast<std::size_t>('j' - '!');
    assert(std::memcmp(
        primary[jeep].data,
        alternate[jeep].data,
        primary[jeep].data_size) != 0);

    assert(car.header.w == 24U && car.header.h == 24U);
    assert(digipeater.header.w == 24U && digipeater.header.h == 24U);
    assert(gateway.header.w == 24U && gateway.header.h == 24U);

    std::puts("aprs_icon_asset descriptor tests passed");
    return 0;
}
