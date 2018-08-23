#include <vector>
#include <map>
#include <set>
#include <inttypes.h>
#include "parse_log.h"

struct choice
{
  uint32_t next = 0;
  uint32_t escape = 0;
};

struct aligned_choice
{
  uint32_t c_p2 = 0;
  uint32_t c_aligned = 0;
  uint32_t c_unaligned = 0;
};

struct rw_choice
{
  uint32_t c_read = 0;
  uint32_t c_write = 0;
};

struct type_choice
{
  uint32_t c_seq = 0;
  uint32_t c_rand = 0;
};

struct startpos_choice
{
  uint32_t c_from0 = 0;
  uint32_t c_fromrand = 0;
};
struct write_object_choice
{
  uint32_t c_history = 0;
  uint32_t c_other = 0;
};

struct read_object_choice
{
  uint32_t c_read_history = 0;
  uint32_t c_write_history = 0;
  uint32_t c_other = 0;
};

void print_history();

struct sequence
{
  sequence(size_t length) {
    seq.resize(length);
  }
  std::vector<choice> seq;

  struct learn_t
  {
    size_t pos = 0;
    void learn(sequence& ch_seq, bool advance) {
      if (advance) {
	ch_seq.seq[pos].next++;
	if (pos + 1 < ch_seq.seq.size())
	  pos++;
      } else {
	ch_seq.seq[pos].escape++;
	pos = 0;
      }
    }
  };

  struct generate_t
  {
    size_t pos = 0;
    bool generate(sequence& ch_seq);
  };
};

class Player_t
{
public:
  struct entry {
    uint32_t msec;
    uint32_t offset;
    uint32_t len;
    uint16_t obj_id;
    uint16_t operation;
  };
  bool load(FILE *f);
  bool pop_next(entry& e);
  uint32_t get_length();
  size_t get_object_count() { return object_count; }
private:
  std::vector<Player_t::entry> recorded;
  size_t object_count = 0;
  size_t pos = 0;
  friend class Playback_t;
};

class Playback_objects_t
{
public:
  Playback_objects_t(const std::string& name_prefix): name_prefix(name_prefix) {};
  std::tuple<std::string, bool> obtain_name();
  void release_name(const std::string&);
  size_t names_count();
private:
  std::set<std::string> unused_names;
  size_t names_generated = 0;
  std::string name_prefix;
};

class Playback_t
{
public:
  Playback_t(const Player_t& player, Playback_objects_t& object_pool, uint64_t base_time):
    player(player), object_pool(object_pool), base_time(base_time) {};
  bool blktrace_open(std::string& commands);
  bool blktrace_get_next_time(uint64_t& time_at);
  bool blktrace_get_commands(std::string& commands);
  bool blktrace_close(std::string& commands);
private:
  const Player_t& player;
  Playback_objects_t& object_pool;
  uint64_t base_time = 0;
  size_t pos = 0;
  struct object_t {
    std::string name;
    bool opened;
  };
  std::vector<object_t> objects;
 // size_t open_objects = 0;
};

class Recorder_t
{
public:
  void record(const op_io_t& specifics);

  void printall();
  bool save(FILE *f);
private:
  std::vector<Player_t::entry> recorded;
  double start_time = 0;
  std::map<std::string, uint32_t> objects;
};


class Model_t
{
public:
  static constexpr size_t max_object_size = 1 << 22; //4194304

  void add();

  //void r_w_learn(op_dir d);
  void r_w_print();

  op_dir r_w_model();
  Model_t() {};
  uint32_t get_length();
private:
  static constexpr size_t r_w_seq_depth = 10;
  static constexpr size_t object_history_depth = 10;

  static constexpr size_t size_history_minval = 12;//4096;
  static constexpr size_t size_history_maxval = 22;//4194304;
  static constexpr size_t size_history_depth = size_history_maxval + 1 - (size_history_minval - 1);

  static constexpr size_t normal_align = 1 << 12; //4096
  static constexpr size_t small_align = 1 << 9; //512

  rw_choice rw_c;
  read_object_choice r_object_c;
  write_object_choice w_object_c;
  type_choice type_c;
  startpos_choice startpos_c;
  aligned_choice aligned_c;

  double seq_change_interval;
  double op_interval;

  sequence w_sequence{r_w_seq_depth};
  sequence r_sequence{r_w_seq_depth};

  uint32_t write_history[object_history_depth] = {0};
  uint32_t read_history[object_history_depth] = {0};

  uint32_t size_history[size_history_depth] = {0};

  friend class Learn_t;
  friend class Generator_t;
};

struct history
{
  history(size_t size): names(size) {}
  std::vector<std::string> names;
  ssize_t put(const std::string &obj_name);
  bool has(const std::string &obj_name);
  bool insert(const std::string &obj_name); //true when all slots already filled
};

class Learn_t
{
public:
  //Learn_t(){};
  Learn_t() {};
  void next(Model_t& m, op_io_t& op);
  void next_r_w(Model_t &m, op_dir d);
  void next_object_write(Model_t &m, const std::string& object_name);
  void learn_object_op(Model_t &m, const op_io_t& op);
  void learn_object_history(Model_t &m, op_dir dir, const std::string& object);
  void learn_end(Model_t &m);

  void learn_size(Model_t& m, const op_io_t& op);
  void learn_alignment(Model_t& m, const op_io_t& op);
  void learn_sequence(Model_t& m, const op_io_t& op, bool seq_continues);
  void learn_timings(Model_t& m, const op_io_t& op, bool seq_continues);

  bool in_batch(const op_io_t& hold, const op_io_t& op);
  bool process(Model_t &m, const op_io_t& op, bool seq_continues);
  bool process_continue(Model_t &m, const op_io_t& op);
  bool process_new(Model_t &m, const op_io_t& op);
private:
  //Model_t& m;

  int stage = 0;

  op_io_t hold = {0};
  bool hold_processed = false;

  uint32_t last_client_no;
  op_dir last_dir;
  op_type last_type;
  std::string last_object;
  uint32_t last_offset;
  uint32_t last_len;
  double last_op_time = 0;

  uint32_t op_interval_count = 0;
  double op_interval_cumulative = 0;
  uint32_t seq_interval_count = 0;
  double seq_interval_cumulative = 0;

  sequence::learn_t r_learn;
  sequence::learn_t w_learn;

  std::vector<std::string> last_history;

  history last_writes{Model_t::object_history_depth};
  history last_reads{Model_t::object_history_depth};

};

class Generator_t
{
public:
  Generator_t(const Model_t& m): m(m) {};
  void get_op(op_io_t& op);
  size_t get_length();
private:
  ssize_t rw_index = -1;
  double tv = 0;
  op_dir dir;
  op_type type;
  size_t curr_pos = 0;
  std::string object_name;
  const Model_t& m;
};

