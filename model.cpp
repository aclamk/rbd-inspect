#include "model.h"

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>
#include <ctime>
#include <map>
#include <vector>
#include <assert.h>

void Learn_t::learn_object_op(Model_t &m, const op_io_t& op)
{
  bool seq_continues = false;
  op_type type = op_rand;
  if (stage == 1) {
    last_type = last_offset + last_len == op.offset ? op_seq : op_rand;
    stage++;
  }
  type = (last_offset + last_len == op.offset) ? op_seq : op_rand;

  if (stage >= 2) {
    if (last_client_no == op.client_no &&
	last_dir == op.opcode &&
	last_object == op.object_name &&
	(	(last_type == op_seq && last_offset + last_len == op.offset) ||
	    (last_type == op_rand && last_offset + last_len != op.offset) )
    ) {
      seq_continues = true;
    }
    last_offset = op.offset;
    last_len = op.len;

    if (op.opcode == op_read) {
      r_learn.learn(m.r_sequence, seq_continues);
    } else {
      w_learn.learn(m.w_sequence, seq_continues);
    }

    size_t slog2 = sizeof(op.len)*8 - __builtin_clz(op.len);
    if (slog2 < m.size_history_minval)
      slog2 = m.size_history_minval - 1; // range 0 .. 4093
    if (slog2 > m.size_history_maxval - 1)
      slog2 = m.size_history_maxval - 1;
    slog2 -= (m.size_history_minval - 1);
    m.size_history[slog2]++;

    if ((op.len & (op.len - 1)) == 0) {
      m.aligned_c.c_aligned++;
    } else {
      m.aligned_c.c_unaligned++;
    }

    if (seq_continues) {
      op_interval_count++;
      op_interval_cumulative += (op.tv - last_op_time);
    } else {
      seq_interval_count++;
      seq_interval_cumulative += (op.tv - last_op_time);
    }
    m.op_interval = op_interval_cumulative / op_interval_count;
    m.seq_change_interval = seq_interval_cumulative / seq_interval_count;

    last_op_time = op.tv;

    if (seq_continues && stage > 2) {
      return;
    }

    learn(m, op.opcode, op.object_name, type);

    if (type == op_seq) {
      m.type_c.c_seq++;
    } else {
      m.type_c.c_rand++;
    }
  }
  stage++;

  last_client_no = op.client_no;
  last_dir = op.opcode;
  last_type = type;
  last_object = op.object_name;
  last_offset = op.offset;
  last_len = op.len;
  last_op_time = op.tv;
}

void Learn_t::learn(Model_t &m, op_dir dir, const std::string& object, op_type type)
{
  ssize_t p;
  if (dir == op_read) {
    m.rw_c.c_read++;
    if (last_writes.has(object)) {
      m.r_object_c.c_write_history++;
    } else {
      if (last_reads.has(object)) {
	p = last_reads.put(object);
	assert(p >= 0);
	m.read_history[p]++;
	m.r_object_c.c_read_history++;
      } else {
	m.r_object_c.c_other++;
	if (last_reads.insert(object)) {
	}
      }
    }
  } else {
    m.rw_c.c_write++;
    if (last_writes.has(object)) {
      m.w_object_c.c_history++;
      p = last_writes.put(object);
      assert(p >= 0);
      m.write_history[p]++;
    } else {
      m.w_object_c.c_other++;
      if (last_writes.insert(object)) {
      }
    }
  }
}

ssize_t Learn_t::history::put(const std::string& obj_name)
{
  for (size_t i = 0; i < names.size(); i++) {
    if (names[i] == obj_name) {
      for (size_t j = i ; j > 0 ; j--) {
	names[j] = names[j - 1];
      }
      names[0] = obj_name;
      return i;
    }
  }
  return -1;
}

bool Learn_t::history::has(const std::string &obj_name)
{
  for (size_t i = 0; i < names.size(); i++) {
    if (names[i] == obj_name) {
      return true;
    }
  }
  return false;
}

bool Learn_t::history::insert(const std::string &obj_name)
{
  bool is_full = names[names.size() - 1] != "";
  for (size_t i = names.size() - 1; i > 0; i--) {
    names[i] = names[i - 1];
  }
  names[0] = obj_name;
  return is_full;
}




void Model_t::r_w_print()
{
  std::cout << "R";
  for (auto &i: r_sequence.seq) {
    std::cout << " " << i.next << "/" << i.escape;
  }
  std::cout << std::endl;

  std::cout << "W";
  for (auto &i: w_sequence.seq) {
    std::cout << " " << i.next << "/" << i.escape;
  }
  std::cout << std::endl;

  std::cout << "R=" << rw_c.c_read << " W=" << rw_c.c_write << std::endl;
  std::cout << "R-history read=" << r_object_c.c_read_history <<
      " write=" << r_object_c.c_write_history <<
      " other=" << r_object_c.c_other << std::endl;
  std::cout << "W-history write=" << w_object_c.c_history << " other=" << w_object_c.c_other << std::endl;
  std::cout << "seq=" << type_c.c_seq << " rand=" << type_c.c_rand << std::endl;

  std::cout << "Oh-w";
  for (auto &i: write_history) {
    std::cout << " " << i;
  }
  std::cout << std::endl;

  std::cout << "Oh-r";
  for (auto &i: read_history) {
    std::cout << " " << i;
  }
  std::cout << std::endl;

  std::cout << "sizes:";
  for (auto &i: size_history) {
    std::cout << " " << i;
  }
  std::cout << " aligned=" << aligned_c.c_aligned << " unaligned=" << aligned_c.c_unaligned << std::endl;

  std::cout << "op_interval=" << op_interval << " seq_interval=" << seq_change_interval << std::endl;
}

#if 0
op_dir Model_t::r_w_model()
{
  op_dir result;
  if (r_w_index_m > 0) {
    if (rand() % (r_seq[r_w_index_m - 1].next + r_seq[r_w_index_m - 1].escape)
	<
	r_seq[r_w_index_m - 1].next) {
      result = op_read;
      r_w_index_m ++;
      if (r_w_index_m > r_w_seq_depth)
	  r_w_index_m = r_w_seq_depth;
    } else {
      r_w_index_m = -1;
      result = op_write;
    }
  } else {
    if (rand() % (w_seq[ -r_w_index_m - 1].next + w_seq[ -r_w_index_m - 1].escape)
	<
	w_seq[ -r_w_index_m - 1].next) {
      result = op_write;
      r_w_index_m --;
      if (r_w_index_m < -r_w_seq_depth)
	  r_w_index_m = -r_w_seq_depth;
    } else {
      r_w_index_m = 1;
      result = op_read;
    }
  }
  return result;
}
#endif
