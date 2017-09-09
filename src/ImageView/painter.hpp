#pragma once

#if defined(USE_DIRECT2D_PAINTER) && defined(USE_GDI_PAINTER)
static_assert(false, "Please choose only one painter to use.");
#endif

#ifdef USE_DIRECT2D_PAINTER
#include "painter_direct2d.hpp"
#endif

#ifdef USE_GDI_PAINTER
//#include "painter_gdi.hpp"
#endif

