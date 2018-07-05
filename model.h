#include <vector>
#include <inttypes.h>
#include "parse_log.h"

struct choice
{
  uint32_t next = 0;
  uint32_t escape = 0;
};

struct aligned_choice
{
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
};


class Model_t
{
public:
  void add();

  //void r_w_learn(op_dir d);
  void r_w_print();

  op_dir r_w_model();
  Model_t() {};
private:
  static constexpr size_t r_w_seq_depth = 10;
  static constexpr size_t object_history_depth = 10;

  static constexpr size_t size_history_minval = 12;//4096;
  static constexpr size_t size_history_maxval = 22;//4194304;
  static constexpr size_t size_history_depth = size_history_maxval - (size_history_minval - 1);

  rw_choice rw_c;
  read_object_choice r_object_c;
  write_object_choice w_object_c;
  type_choice type_c;
  aligned_choice aligned_c;

  sequence w_sequence{r_w_seq_depth};
  sequence r_sequence{r_w_seq_depth};

  uint32_t write_history[object_history_depth] = {0};
  uint32_t read_history[object_history_depth] = {0};

  uint32_t size_history[size_history_depth] = {0};

  friend class Learn_t;
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
  void learn(Model_t &m, op_dir dir, const std::string& object, op_type type);
private:
  //Model_t& m;

  int stage = 0;
  //op_io_t last_op;

  uint32_t last_client_no;
  op_dir last_dir;
  op_type last_type;
  std::string last_object;
  uint32_t last_offset;
  uint32_t last_len;

  sequence::learn_t r_learn;
  sequence::learn_t w_learn;

  std::vector<std::string> last_history;

  struct history
  {
    history(size_t size): names(size) {}
    std::vector<std::string> names;
    ssize_t put(const std::string &obj_name);
    bool has(const std::string &obj_name);
    bool insert(const std::string &obj_name); //true when all slots already filled
  };

  history last_writes{Model_t::object_history_depth};
  history last_reads{Model_t::object_history_depth};

};
