#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>
#include <ctime>
#include <map>
#include <vector>
#include <set>
#include <assert.h>
#include <memory>
#include "parse_log.h"

#include "model.h"

int main(int argc, char** argv)
{
  Model_t M;
  std::map<uint32_t, Model_t> Models;
  std::map<uint32_t, Learn_t> Learns;
  std::map<uint32_t, Recorder_t> Recorders;
  std::vector<Player_t> all_players;

  struct play
  {
    Playback_t playback;
    int64_t base_time;
  };
  std::multimap<int64_t, std::shared_ptr<Playback_t> > active_players;

  FILE* f;
  f = fopen("test-record", "r");
  while (true)
  {
    int ch = fgetc(f);
    if (feof(f)) break;
    ungetc(ch, f);
    Player_t p;
    assert(p.load(f));
    all_players.push_back(p);
    Player_t::entry e;
    while(p.pop_next(e)) {
      //std::cout << "TIME=" << e.msec << " obj=" << e.obj_id << " " << e.operation << " o=" << e.offset << " l=" << e.len << std::endl;
    }
  }
  fclose(f);

  std::cout << "fio version 2 iolog" << std::endl;

  int64_t base_time = 0;
  static constexpr size_t players_count = 50;
  Playback_objects_t object_pool("rbd_data.____.");
  while (active_players.size() < players_count)
  {
    std::shared_ptr<Playback_t> pb(new Playback_t(all_players[rand() % all_players.size()], object_pool, base_time));
    uint64_t next_op;
    pb->blktrace_get_next_time(next_op);
    active_players.emplace(next_op, std::shared_ptr<Playback_t>(pb));
  }

  while (true)
  {
    auto first = active_players.begin();
    std::string commands;
    assert(first->second->blktrace_get_commands(commands));
    std::cout << commands;
    uint64_t next_time;
    if (first->second->blktrace_get_next_time(next_time)) {
      active_players.emplace(next_time, first->second);
      active_players.erase(first);
    } else {
      next_time = first->first;
      std::shared_ptr<Playback_t> pb(new Playback_t(all_players[rand() % all_players.size()], object_pool, next_time));
      uint64_t next_op;
      pb->blktrace_get_next_time(next_op);
      active_players.emplace(next_op, std::shared_ptr<Playback_t>(pb));
      active_players.erase(first);
    }

  }

}
