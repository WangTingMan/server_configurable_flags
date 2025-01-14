/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <sys/un.h>

#include "com_android_aconfig_new_storage.h"
#include "aconfigd.h"
#include "aconfigd_util.h"

using namespace android::aconfigd;

static int aconfigd_init() {
  auto init_result = InitializeInMemoryStorageRecords();
  if (!init_result.ok()) {
    LOG(ERROR) << "Failed to initialize persistent storage records in memory: "
               << init_result.error();
    return 1;
  }

  // TODO: remove this check once b/330134027 is fixed. This is temporary as android
  // on chrome os vm does not have /metadata partition at the moment.
  DIR* dir = opendir("/metadata/aconfig");
  if (!dir) {
    return {};
  }
  closedir(dir);

  // clear boot dir to start fresh at each boot
  auto remove_result = RemoveFilesInDir("/metadata/aconfig/boot");
  if (!remove_result.ok()) {
    LOG(ERROR) <<"failed to clear boot dir: " << remove_result.error();
    return 1;
  }

  auto plat_result = InitializePlatformStorage();
  if (!plat_result.ok()) {
    LOG(ERROR) << "failed to initialize storage records: " << plat_result.error();
    return 1;
  }

  return 0;
}

static int aconfigd_start() {
  auto init_result = InitializeInMemoryStorageRecords();
  if (!init_result.ok()) {
    LOG(ERROR) << "Failed to initialize persistent storage records in memory: "
               << init_result.error();
    return 1;
  }

  auto aconfigd_fd = android::base::unique_fd(android_get_control_socket(kAconfigdSocket));
  if (aconfigd_fd == -1) {
    PLOG(ERROR) << "failed to get aconfigd socket";
    return 1;
  }

  if (listen(aconfigd_fd, 8) < 0) {
    PLOG(ERROR) << "failed to listen to socket";
    return 1;
  };

  auto addr = sockaddr_un();
  addr.sun_family = AF_UNIX;
  auto path = std::string("/dev/socket/") + kAconfigdSocket;
  strlcpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
  socklen_t addr_len = sizeof(addr);

  while(true) {
    LOG(INFO) << "start accepting client requests";
    auto client_fd = android::base::unique_fd(accept4(
        aconfigd_fd, reinterpret_cast<sockaddr*>(&addr), &addr_len, SOCK_CLOEXEC));
    if (client_fd == -1) {
      PLOG(ERROR) << "failed to establish connection";
      break;
    }
    LOG(INFO) << "received a client requests";

    char buffer[kBufferSize] = {};
    auto num_bytes = TEMP_FAILURE_RETRY(recv(client_fd, buffer, sizeof(buffer), 0));
    if (num_bytes < 0) {
      PLOG(ERROR) << "failed to read from aconfigd socket";
      break;
    } else if (num_bytes == 0) {
      LOG(ERROR) << "failed to read from aconfigd socket, empty message";
      break;
    }
    auto msg = std::string(buffer, num_bytes);

    auto messages = StorageRequestMessages{};
    if (!messages.ParseFromString(msg)) {
      LOG(ERROR) << "Could not parse message from aconfig storage init socket";
      continue;
    }

    auto return_messages = StorageReturnMessages();
    for (auto& msg : messages.msgs()) {
      auto* return_msg = return_messages.add_msgs();
      HandleSocketRequest(msg, *return_msg);
      if (!return_msg->has_error_message()) {
        LOG(ERROR) << "failed to handle socket request: " << return_msg->error_message();
      }
    }

    auto return_content = std::string();
    if (!return_messages.SerializeToString(&return_content)) {
      LOG(ERROR) << "failed to serialize return messages to string";
      continue;
    }

    auto num = TEMP_FAILURE_RETRY(
        send(client_fd, return_content.c_str(), return_content.size(), 0));
    if (num != static_cast<long>(return_content.size())) {
      PLOG(ERROR) << "failed to send return message";
    }
  }

  return 1;

}

int main(int argc, char** argv) {
  if (!com::android::aconfig_new_storage::enable_aconfig_storage_daemon()) {
    return 0;
  }

  android::base::InitLogging(argv, &android::base::KernelLogger);

  if (argc > 2 || (argc == 2 && strcmp("--initialize", argv[1]) != 0)) {
    LOG(ERROR) << "invalid aconfigd command";
    return 1;
  }

  if (argc == 2 && strcmp("--initialize", argv[1]) == 0) {
    return aconfigd_init();
  }

  return aconfigd_start();
}
