#pragma once

#include <cstddef>

#include "app/app_types.h"

namespace App {
namespace MenuModel {

struct MenuItem {
    const char* title;
    const char* description;
    ScreenId target;
};

constexpr std::size_t ITEM_COUNT = 13;

std::size_t count();
const MenuItem& item(std::size_t index);

}  // namespace MenuModel
}  // namespace App
