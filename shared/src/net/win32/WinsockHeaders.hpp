// Interne à shared/ : inclusion unique des en-têtes Winsock, avec les bonnes options.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // évite d'inclure tout windows.h
#endif
#ifndef NOMINMAX
#define NOMINMAX // empêche windows.h de définir des macros min/max qui cassent std::min/std::max
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
