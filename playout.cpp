/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                           *
 *  This file is part of GoBoard library,                                    *
 *                                                                           *
 *  Copyright 2006 Lukasz Lew                                                *
 *                                                                           *
 *  GoBoard library is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  GoBoard library is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with GoBoard library; if not, write to the Free Software           *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor,                           *
 *  Boston, MA  02110-1301  USA                                              *
 *                                                                           *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "board.cpp"


#ifdef NDEBUG
static const bool playout_ac = false;
#endif


#ifdef DEBUG
static const bool playout_ac = true;
#endif


static const bool playout_print       = false;
static const uint max_playout_length  = board_area * 2;
static const uint mercy_threshold     = 25;

// class playout_t


class playout_t { // board with incremental fast random move generator

public:

  board_t*    board;

  v::t        empty_v[board_area];
  uint        empty_v_cnt;

  v::t        rejected_v[board_area];
  uint        rejected_v_cnt;

  void check () {
    if (!playout_ac) return;

    bool noticed[v::cnt];

    assert    (empty_v_cnt <= board_area);
    assert (rejected_v_cnt <= board_area);

    v_for_each_all (v) noticed[v] = false;

    rep (k, empty_v_cnt) {
      assert (noticed[empty_v[k]] == false);
      noticed[empty_v[k]] = true;
    }

    rep (k, rejected_v_cnt) {
      assert (noticed[rejected_v[k]] == false);
      noticed[rejected_v[k]] = true;
    }

    v_for_each_faster (v)
      assert ((board->color_at[v] == color::empty) == noticed[v]);
  }

  playout_t (board_t* board) { // copying constructor makes no improvement as randomization is a key
    this->board = board;

    init ();
  }

  void init () {
    empty_v_cnt    = 0;
    rejected_v_cnt = 0;

    v_for_each_faster (v) {
      if (board->color_at[v] == color::empty)
        add_empty_v (v);
    }
  }

  void add_empty_v (v::t v) {
    uint ii;

    empty_v_cnt++;
    ii = pm::rand_int (empty_v_cnt);         // TODO improve speed here
    empty_v[empty_v_cnt-1] = empty_v[ii];
    empty_v[ii] = v;
    assertc (playout_ac, empty_v_cnt <= board_area);
  }

  player::t run (player::t first_pl) {
    v::t  v;
    bool  was_pass[player::cnt];
    uint  move_no;
    bool  do_mercy;

    #define do_play(player) {               \
      v = play_one (player);                \
      if (playout_print) {                  \
        cout << endl;                       \
        board->print (cout, v);             \
        getchar ();                         \
      }                                     \
      was_pass[player] = (v == v::pass);    \
    }

    player_for_each (pl) 
      was_pass [pl] = 0;

    move_no = 0;

    if (first_pl == player::white) 
      do_play (player::white);

    do { // to avoid rare loopy playouts
      move_no += 2;
      player_for_each (pl) 
        do_play (pl);

      do_mercy = (uint (abs (board->approx_score ())) > mercy_threshold);
      if ((was_pass [player::black] & was_pass [player::white]) | 
          (move_no > max_playout_length) |
          (do_mercy)) 
        break;

    } while (true);

    if (do_mercy)
      return board->approx_winner ();
    else
      return board->winner ();

    #undef do_play
  }


  v::t play_one (player::t player) {
    v::t v;

    #define loop(sign)                                                         \
    while (empty_v_cnt > 0) {                                                  \
      v = empty_v [--empty_v_cnt];                                             \
                                                                               \
      if (board->is_eyelike (player, v) ||                                     \
          board->play (player, v) == board_t::play_ss_suicide)                 \
      {                                                                        \
        rejected_v[rejected_v_cnt++] = v;                                      \
        if (playout_print) {                                                   \
          cout << "REJECT" << sign << endl;                                    \
          board->print (cout, v);                                              \
          getchar ();                                                          \
        }                                                                      \
      } else {                                                                 \
        v::t* first = board->empty_v + board->last_empty_v_cnt - 1;            \
        v::t* last = board->empty_v + board->empty_v_cnt;                      \
        while (first < last) {                                                 \
          add_empty_v (*first);                                                \
          first++;                                                             \
          if (playout_print) cout << "C" << sign << " ";                       \
        }                                                                      \
                                                                               \
        if (playout_print) cout << "OK" << sign;                               \
                                                                               \
        return v;                                                              \
      }                                                                        \
    }                                                                          


    loop ("");

    memcpy (empty_v, rejected_v, sizeof(v::cnt) * rejected_v_cnt);
    empty_v_cnt = rejected_v_cnt;
    rejected_v_cnt = 0;
    if (playout_print) cout << "MEMCPY " << empty_v_cnt << endl;

    loop (" 2");

    board->check_no_more_legal (player); // powerfull check

    #undef loop

    return v::pass;
  }

};



// class simple_playout_t


namespace simple_playout {

  static v::t play_one (board_t* board, player::t player) {

    v::t v;
    uint start;

    // find random place in vector of empty vertices
    start = pm::rand_int (board->empty_v_cnt); 

    // search for a move in start ... board->empty_v_cnt-1 interval
    for (uint ev_i = start; ev_i != board->empty_v_cnt; ev_i++) {   
      v = board->empty_v [ev_i];
      if (!board->is_eyelike (player, v) &&
          board->play (player, v) != board_t::play_ss_suicide) return v;
    }

    // search for a move in 0 ... start interval
    for (uint ev_i = 0; ev_i != start; ev_i++) {
      v = board->empty_v [ev_i];
      if (!board->is_eyelike (player, v) &&
          board->play (player, v) != board_t::play_ss_suicide) return v;
    }

    board->check_no_more_legal (player); // powerfull check
    return v::pass;
  }


  player::t run (board_t* board, player::t first_player) {

    v::t  v;
    bool  was_pass[player::cnt];
    uint  move_no;
    player::t act_player;

    act_player = first_player;

    player_for_each (pl)
      was_pass [pl] = false;

    move_no = 0;

    do {

      v = play_one (board, act_player);

      was_pass [act_player] = (v == v::pass);
      act_player = player::other (act_player);
      move_no++;

      if ((was_pass [player::black] & was_pass [player::white]) | 
          (move_no > max_playout_length)) break;

    } while (true);

    return board->winner ();

  }

}




void playout_benchmark (board_t const * start_board, 
                uint playout_cnt, 
                player::t first_player, 
                ostream& out) 
{
  board_t    mc_board;
  float      seconds_begin;
  float      seconds_end;
  float      seconds_total;

  player::t  winner;
  uint       win_cnt [player::cnt];

  seconds_begin = get_seconds ();

  rep (ii, playout_cnt) {
    mc_board.load (start_board);

    playout_t playout (&mc_board);
    winner = playout.run (first_player);
    
    win_cnt [winner] ++; // TODO this is a way too costly // (we have empty tab, there is a better way)
  }

  seconds_end = get_seconds ();

  out << "Initial board:" << endl;
  out << "komi " << -start_board->komi << endl;

  start_board->print (out);

  seconds_total = seconds_end - seconds_begin;

  out << "Performance: " << endl
      << "  " << playout_cnt << " playouts" << endl
      << "  " << seconds_total << " seconds" << endl
      << "  " << float (playout_cnt) / seconds_total / 1000.0 << " kpps" << endl;

  out << "Black wins = " << win_cnt [player::black] << endl
      << "White wins = " << win_cnt [player::white] << endl
      << "P(black win) = " << float (win_cnt [player::black]) / float (win_cnt [player::black] + win_cnt [player::white]) << endl;

}

#ifdef PLAYOUT_TEST

static const uint playout_cnt = 200000;

board_t arch_board;

void usage (char* argv[]) {
  cout << "Usage: " << argv[0] << " initial_board" << endl;
  exit (1);
}

int main (int argc, char* argv[]) {
  if (argc != 2) usage (argv);

  ifstream board_cin (argv[1]);
  if (!cin) usage (argv);
  if (!arch_board.load (board_cin)) {
    cout << "Wrong file format" << endl;
    exit (1);
  }

  playout_benchmark (&arch_board, playout_cnt, player::black, cout);

  return 0;
}

#endif


/*

  Mam bardzo rozgaleziony algorytm, dlaczego?
1. limit na dlugosc playoutu 2 pass
3. szukamy ruchu while empty_v_cnt > 0
4. eyelike state
5. play result
6. while captured

Potrzebujemy:
  Dobrego kryterium konca gry.

Idealny algorytm:
 1. good_move_cnt > 0 ?

*/
