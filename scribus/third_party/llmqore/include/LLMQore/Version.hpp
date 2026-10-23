// Copyright (C) 2026 Petr Mironychev
// SPDX-License-Identifier: MIT

#pragma once

#define LLMQORE_VERSION_MAJOR 0
#define LLMQORE_VERSION_MINOR 9
#define LLMQORE_VERSION_PATCH 0

#define LLMQORE_VERSION_STRING "0.9.0"

#define LLMQORE_VERSION_CHECK(major, minor, patch) \
    ((major << 16) | (minor << 8) | (patch))

#define LLMQORE_VERSION \
    LLMQORE_VERSION_CHECK(LLMQORE_VERSION_MAJOR, LLMQORE_VERSION_MINOR, LLMQORE_VERSION_PATCH)
