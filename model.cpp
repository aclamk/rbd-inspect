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


bool Learn_t::in_batch(const op_io_t& hold, const op_io_t& op)
{
  assert(hold.client_no == op.client_no);

  if ((hold.object_name == op.object_name) &&
      (hold.opcode == op.opcode) &&
      ( (last_type == op_seq && hold.offset + hold.len == op.offset) ||
	(last_type == op_rand && hold.offset + hold.len != op.offset) ||
	(last_type == op_unknown)) ) {
    return true;
  }
  return false;
}


void Learn_t::learn_size(Model_t& m, const op_io_t& op)
{
  size_t slog2 = sizeof(op.len)*8 - __builtin_clz(op.len);
  if (slog2 < m.size_history_minval)
    slog2 = m.size_history_minval - 1; // range 0 .. 4093
  if (slog2 > m.size_history_maxval)
    slog2 = m.size_history_maxval;
  slog2 -= (m.size_history_minval - 1);
  m.size_history[slog2]++;

}

void Learn_t::learn_alignment(Model_t& m, const op_io_t& op)
{
  if ((op.len & (op.len - 1)) == 0) {
    m.aligned_c.c_p2++;
  } else {
    if ( ((op.len < m.normal_align) && ((op.len & (m.small_align - 1)) == 0)) ||
	  ((op.len >= m.normal_align) && ((op.len & (m.normal_align - 1)) == 0)) )
    {
	m.aligned_c.c_aligned++;
    } else {
	m.aligned_c.c_unaligned++;
    }
  }
}

void Learn_t::learn_sequence(Model_t& m, const op_io_t& op, bool seq_continues)
{
  if (op.opcode == op_read) {
    r_learn.learn(m.r_sequence, seq_continues);
  } else {
    w_learn.learn(m.w_sequence, seq_continues);
  }
}

void Learn_t::learn_timings(Model_t& m, const op_io_t& op, bool seq_continues)
{
  if (last_op_time != 0) {
    if (seq_continues) {
      op_interval_count++;
      op_interval_cumulative += (op.tv - last_op_time);
    } else {
      seq_interval_count++;
      seq_interval_cumulative += (op.tv - last_op_time);
    }
    m.op_interval = op_interval_cumulative / op_interval_count;
    m.seq_change_interval = seq_interval_cumulative / seq_interval_count;
  }
  last_op_time = op.tv;
}

bool Learn_t::process_new(Model_t &m, const op_io_t& op)
{
  // if hold in batch with op
  //   - start new sequence from hold
  //   - consume op
  //   - move op to hold
  //   - set hold processed
  // otherwise
  //   - consume hold as 1-element seq
  //   - move op to hold
  //   - set hold unprocessed

  bool is_batch = in_batch(hold, op);
  r_learn.pos = 0;
  w_learn.pos = 0;

  learn_object_history(m, hold.opcode, hold.object_name);
  learn_size(m, hold);
  learn_alignment(m, hold);
  learn_timings(m, hold, is_batch);
  if (!is_batch) {
    //single item sequence ends
    learn_sequence(m, hold, false);

  }
  last_type = is_batch && (hold.offset + hold.len == op.offset) ? op_seq : op_rand;
  if (last_type == op_seq) {
    m.type_c.c_seq++;
  } else {
    m.type_c.c_rand++;
  }
  if (!is_batch)
    last_type = op_unknown;
  return is_batch;
}

bool Learn_t::process_continue(Model_t &m, const op_io_t& op)
{
  learn_sequence(m, op, true);
  learn_size(m, op);
  learn_alignment(m, op);
  learn_timings(m, op, true);
  return true;
}


void Learn_t::learn_object_op(Model_t &m, const op_io_t& op)
{
  // if hold does not exist (initial condition)
  //   - store to hold
  //   - set hold not processed
  // otherwise
  //   if hold processed
  //     if hold in batch with current:
  //       - store current to hold
  //       - process_continue(hold)
  //       - set hold processed
  //     otherwise
  //       - store current to hold
  //       - set hold unprocessed (means beginning of sequence)
  //   otherwise (hold not processed)
  //     if process_new hold->current forms sequence
  //       - process_continue(current)
  //       - move current to hold
  //       - set hold processed
  //     otherwise
  //       - move current to hold
  //       - set hold unprocessed
  if (hold.client_no == 0) {
    hold = op;
    hold_processed = false;
    return;
  } else {
    if (hold_processed) {
      if (in_batch(hold, op)) {
	process_continue(m, op);
	hold = op;
	hold_processed = true;
      } else {
	//current sequence ends
	learn_sequence(m, hold, false);
	last_type == op_unknown;
	hold = op;
	hold_processed = false;
	learn_timings(m, op, false);
      }
    } else {
      bool is_batch = process_new(m, op);
      if (is_batch) {
	process_continue(m, op);
      }
      hold = op;
      hold_processed = is_batch;
    }
  }
}

void Learn_t::learn_object_history(Model_t &m, op_dir dir, const std::string& object)
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

void Learn_t::learn_end(Model_t &m) {
  assert (hold.client_no != 0);
  if (hold_processed == false) {
    learn_object_history(m, hold.opcode, hold.object_name);
    learn_size(m, hold);
    learn_alignment(m, hold);
    learn_timings(m, hold, false);
    m.type_c.c_rand++;
  }
  learn_sequence(m, hold, false);
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


uint32_t Model_t::get_length()
{
  return aligned_c.c_p2 + aligned_c.c_aligned + aligned_c.c_unaligned;
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
  std::cout << " p2=" << aligned_c.c_p2 << " aligned=" << aligned_c.c_aligned << " unaligned=" << aligned_c.c_unaligned << std::endl;

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


void Generator_t::get_op(op_io_t& op)
{
  //1. if no seq, select one
  //2. apply existing seq
  //3. progress seq, possibly ending it

  //check seq
  bool same_seq = true;
  op.len = get_length();

  if (rw_index == -1 || curr_pos == m.max_object_size) {
    same_seq = false;
    //new sequence starts
    if (rand() % (m.rw_c.c_read + m.rw_c.c_write) <
	m.rw_c.c_read)
      dir = op_read;
    else
      dir = op_write;
    rw_index = 0;

    if (rand() % (m.type_c.c_rand + m.type_c.c_seq) <
	m.type_c.c_rand)
      type = op_rand;
    else
      type = op_seq;

    //if (rand() % (m.r))
    //curr_pos = ((1 << m.size_history_maxval) - op.len + 1);
    curr_pos = ( rand() % ((1 << m.size_history_maxval) - op.len + 1) ) & ( ~(1 << m.size_history_minval) - 1);

    //std::cout << (dir==op_read?"read  ":"write ") << (type==op_rand?"rand ":"seq  ") <<
//	"offset=" << curr_pos << std::endl;
  }

  if (type == op_rand) {
    curr_pos = ( rand() % ((1 << m.size_history_maxval) - op.len + 1) ) & ( ~(1 << m.size_history_minval) - 1);
  }



  //apply seq
  op.tv = tv;
  op.client_no = 0;
  op.object_name = "";
  op.opcode = dir;
  op.offset = curr_pos;

  if (curr_pos + op.len > m.max_object_size)
    op.len = m.max_object_size - curr_pos;

  //std::cout << "op.offset=" << op.offset << "op.len=" <<  op.len << " " << (1 << m.size_history_maxval) - op.len + 1 << std::endl;

  //update
  if (dir == op_read) {
    if (rand() % (m.r_sequence.seq[rw_index].next + m.r_sequence.seq[rw_index].escape)
	< m.r_sequence.seq[rw_index].next ) {
      rw_index ++;
      if (rw_index >= m.r_w_seq_depth)
	rw_index = m.r_w_seq_depth - 1;
    } else {
      rw_index = -1;
    }
  } else {
    if (rand() % (m.w_sequence.seq[rw_index].next + m.w_sequence.seq[rw_index].escape)
	< m.w_sequence.seq[rw_index].next ) {
      rw_index ++;
      if (rw_index >= m.r_w_seq_depth)
	rw_index = m.r_w_seq_depth - 1;
    } else {
      rw_index = -1;
    }
  }
  if (type == op_seq) {
    curr_pos += op.len;
    if (curr_pos == m.max_object_size) {
      rw_index = -1;
    }
  } else {
    //curr_pos = ( rand() % ((1 << m.size_history_maxval) - 1) ) & ( ~(1 << m.size_history_minval) - 1);
  }

  if (same_seq)
    tv = tv + (rand() % int(m.op_interval * 2 * 1000000)) / 1000000.;
  else
    tv = tv + (rand() % int(m.seq_change_interval * 2 * 1000000)) / 1000000.;
}

size_t Generator_t::get_length()
{
  size_t rval = rand() % (m.aligned_c.c_p2 + m.aligned_c.c_aligned + m.aligned_c.c_unaligned);
  bool is_p2 = rval < m.aligned_c.c_p2;
  bool is_aligned = rval < m.aligned_c.c_p2 + m.aligned_c.c_aligned;

  size_t total = 0;
  for (auto cnt: m.size_history) {
    total += cnt;
  }
  size_t v = rand() % total;
  size_t pos;
  size_t size;
  for (pos = 0; pos < m.size_history_depth; pos++) {
    if (v < m.size_history[pos]) break;
    v -= m.size_history[pos];
  }
  if (pos == 0) {
    if (is_p2) {
      size = 512 << (rand() % 3);
    } else {
      size = rand() % (m.normal_align - 1);
      if (is_aligned) {
	size = size & ~(m.small_align - 1);
      }
      if (size < 512)
	size = 512;
    }
  } else {
    if (is_p2) {
      size = 4096 << (pos - 1);
    } else {
      size = rand() % ((m.normal_align / 2) << (pos - 1)) +
	  ((m.normal_align / 2) << (pos - 1));
      if (is_aligned) {
	size = size & ~(m.normal_align - 1);
      }
    }
  }
  return size;
}
