/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

#pragma once

#include <server_configurable_flags/server_configurable_flags_export.h>

namespace server_configurable_flags {

enum ResetMode { BOOT_FAILURE, UPDATABLE_CRASHING };

// Check failed reboot count, if it exceeds the threshold, server configurable
// flags will be reset.
SERVERCONFIGURABLEFLAGS_API void ServerConfigurableFlagsReset(ResetMode reset_mode);

}  // namespace server_configurable_flags
