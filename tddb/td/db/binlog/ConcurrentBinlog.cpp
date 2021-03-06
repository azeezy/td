//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/db/binlog/ConcurrentBinlog.h"

#include "td/utils/logging.h"
#include "td/utils/OrderedEventsProcessor.h"
#include "td/utils/Time.h"

#include <map>

namespace td {
namespace detail {
class BinlogActor : public Actor {
 public:
  BinlogActor(std::unique_ptr<Binlog> binlog, uint64 seq_no) : binlog_(std::move(binlog)), processor_(seq_no) {
  }
  void close(Promise<> promise) {
    binlog_->close().ensure();
    promise.set_value(Unit());
    LOG(INFO) << "close: done";
    stop();
  }
  void close_and_destroy(Promise<> promise) {
    binlog_->close_and_destroy().ensure();
    promise.set_value(Unit());
    LOG(INFO) << "close_and_destroy: done";
    stop();
  }

  struct Event {
    BufferSlice raw_event;
    Promise<> sync_promise;
  };
  void add_raw_event(uint64 seq_no, BufferSlice &&raw_event, Promise<> &&promise) {
    processor_.add(seq_no, Event{std::move(raw_event), std::move(promise)}, [&](uint64 id, Event &&event) {
      if (!event.raw_event.empty()) {
        do_add_raw_event(std::move(event.raw_event));
      }
      do_lazy_sync(std::move(event.sync_promise));
    });
    flush_immediate_sync();
    try_flush();
  }

  void force_sync(Promise<> &&promise) {
    auto seq_no = processor_.max_unfinished_seq_no();
    if (processor_.max_finished_seq_no() == seq_no) {
      do_immediate_sync(std::move(promise));
    } else {
      immediate_sync_promises_.emplace(seq_no, std::move(promise));
    }
  }

  void force_flush() {
    // TODO: use same logic as in force_sync
    binlog_->flush();
    flush_flag_ = false;
  }

  void change_key(DbKey db_key, Promise<> promise) {
    binlog_->change_key(std::move(db_key));
    promise.set_value(Unit());
  }

 private:
  std::unique_ptr<Binlog> binlog_;

  OrderedEventsProcessor<Event> processor_;

  std::multimap<uint64, Promise<>> immediate_sync_promises_;
  std::vector<Promise<>> sync_promises_;
  bool force_sync_flag_ = false;
  bool lazy_sync_flag_ = false;
  bool flush_flag_ = false;
  double wakeup_at_ = 0;

  static constexpr int32 FLUSH_TIMEOUT = 1;  // 1s

  void wakeup_after(double after) {
    auto now = Time::now_cached();
    wakeup_at(now + after);
  }

  void wakeup_at(double at) {
    if (wakeup_at_ == 0 || wakeup_at_ > at) {
      wakeup_at_ = at;
      set_timeout_at(wakeup_at_);
    }
  }

  void do_add_raw_event(BufferSlice &&raw_event) {
    binlog_->add_raw_event(std::move(raw_event));
  }

  void try_flush() {
    auto need_flush_since = binlog_->need_flush_since();
    auto now = Time::now_cached();
    if (now > need_flush_since + FLUSH_TIMEOUT - 1e-9) {
      binlog_->flush();
    } else {
      if (!force_sync_flag_) {
        flush_flag_ = true;
        wakeup_at(need_flush_since + FLUSH_TIMEOUT);
      }
    }
  }

  void flush_immediate_sync() {
    auto seq_no = processor_.max_finished_seq_no();
    for (auto it = immediate_sync_promises_.begin(), end = immediate_sync_promises_.end();
         it != end && it->first <= seq_no; it = immediate_sync_promises_.erase(it)) {
      do_immediate_sync(std::move(it->second));
    }
  }

  void do_immediate_sync(Promise<> &&promise) {
    if (promise) {
      sync_promises_.emplace_back(std::move(promise));
    }
    if (!force_sync_flag_) {
      force_sync_flag_ = true;
      wakeup_after(0.003);
    }
  }

  void do_lazy_sync(Promise<> &&promise) {
    if (!promise) {
      return;
    }
    sync_promises_.emplace_back(std::move(promise));
    if (!lazy_sync_flag_ && !force_sync_flag_) {
      wakeup_after(30);
      lazy_sync_flag_ = true;
    }
  }

  void timeout_expired() override {
    bool need_sync = lazy_sync_flag_ || force_sync_flag_;
    lazy_sync_flag_ = false;
    force_sync_flag_ = false;
    bool need_flush = flush_flag_;
    flush_flag_ = false;
    wakeup_at_ = 0;
    if (need_sync) {
      binlog_->sync();
      // LOG(ERROR) << "BINLOG SYNC";
      for (auto &promise : sync_promises_) {
        promise.set_value(Unit());
      }
      sync_promises_.clear();
    } else if (need_flush) {
      try_flush();
      // LOG(ERROR) << "BINLOG FLUSH";
    }
  }
};
}  // namespace detail

ConcurrentBinlog::ConcurrentBinlog() = default;
ConcurrentBinlog::~ConcurrentBinlog() = default;
ConcurrentBinlog::ConcurrentBinlog(std::unique_ptr<Binlog> binlog, int scheduler_id) {
  init_impl(std::move(binlog), scheduler_id);
}

Result<BinlogInfo> ConcurrentBinlog::init(string path, const Callback &callback, DbKey db_key, DbKey old_db_key,
                                          int scheduler_id) {
  auto binlog = std::make_unique<Binlog>();
  TRY_STATUS(binlog->init(std::move(path), callback, std::move(db_key), std::move(old_db_key)));
  auto info = binlog->get_info();
  init_impl(std::move(binlog), scheduler_id);
  return info;
}

void ConcurrentBinlog::init_impl(std::unique_ptr<Binlog> binlog, int32 scheduler_id) {
  path_ = binlog->get_path().str();
  last_id_ = binlog->peek_next_id();
  binlog_actor_ =
      create_actor_on_scheduler<detail::BinlogActor>("Binlog " + path_, scheduler_id, std::move(binlog), last_id_);
}

void ConcurrentBinlog::close_impl(Promise<> promise) {
  send_closure(std::move(binlog_actor_), &detail::BinlogActor::close, std::move(promise));
}
void ConcurrentBinlog::close_and_destroy_impl(Promise<> promise) {
  send_closure(std::move(binlog_actor_), &detail::BinlogActor::close_and_destroy, std::move(promise));
}
void ConcurrentBinlog::add_raw_event_impl(uint64 id, BufferSlice &&raw_event, Promise<> promise) {
  send_closure(binlog_actor_, &detail::BinlogActor::add_raw_event, id, std::move(raw_event), std::move(promise));
}
void ConcurrentBinlog::force_sync(Promise<> promise) {
  send_closure(binlog_actor_, &detail::BinlogActor::force_sync, std::move(promise));
}
void ConcurrentBinlog::force_flush() {
  send_closure(binlog_actor_, &detail::BinlogActor::force_flush);
}
void ConcurrentBinlog::change_key(DbKey db_key, Promise<> promise) {
  send_closure(binlog_actor_, &detail::BinlogActor::change_key, std::move(db_key), std::move(promise));
}
}  // namespace td
