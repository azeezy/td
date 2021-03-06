//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/files/ResourceState.h"
#include "td/telegram/net/NetQuery.h"

namespace td {

struct LocalFileLocation;
class ResourceManager;

class FileLoaderActor : public NetQueryCallback {
 public:
  virtual void set_resource_manager(ActorShared<ResourceManager>) = 0;
  virtual void update_priority(int32 priority) = 0;
  virtual void update_resources(const ResourceState &other) = 0;

  // TODO: existence of this function is a dirty hack. Refactoring is highly appreciated
  virtual void update_local_file_location(const LocalFileLocation &local) {
  }
};

}  // namespace td
