#define for_each_8_nbr(center_v, nbr_v, block) {                \
    Vertex nbr_v = Vertex::Invalid();                           \
    nbr_v = center_v.N (); block;                               \
    nbr_v = center_v.W (); block;                               \
    nbr_v = center_v.E (); block;                               \
    nbr_v = center_v.S (); block;                               \
    nbr_v = center_v.NW (); block;                              \
    nbr_v = center_v.NE (); block;                              \
    nbr_v = center_v.SW (); block;                              \
    nbr_v = center_v.SE (); block;                              \
  }


namespace Param {
  bool mcmc_use    = false;
  bool mcmc_update = true;
  float mcmc_update_fraction = 0.5;
  float mcmc_explore_coeff = 1000.0;
  float mcmc_prob_8_nbr = 0.8;
}

struct McmcNode {
  NatMap <Move, Stat> move_stats;
  NatMap <Player, Stat> light;
};

// -----------------------------------------------------------------------------

class Mcmc {
public:
  Mcmc () {
    Reset ();
  }

  void Reset () {
    ForEachNat (Move, m) {
      // TODO prior, pass
      ForEachNat (Move, m1) mcmc[m].move_stats [m1].reset (1.0, 0.0);
      ForEachNat (Player, pl) mcmc[m].light[pl].reset (1.0, 0.0);    
      ForEachNat (Vertex, v) ownage[v].reset (0.0, 0.0);
    }
  }

  void NewPlayout () {
    to_update.clear();
    to_update_pl.clear();
    prob_8mcmc_1024 = 1024 * Param::mcmc_prob_8_nbr;
  }

  void Update (Board& board) {
    float score = board.PlayoutScore ();
    rep (ii, to_update.size() * Param::mcmc_update_fraction) {
      to_update[ii] -> update (score);
      to_update[ii] -> UpdateUcb (to_update_pl [ii], Param::mcmc_explore_coeff);
    }

    ForEachNat (Vertex, v) {
      Color c = board.ColorAt (v);
      if (c == Color::OffBoard()) continue;
      if (c.IsPlayer ())
        ownage [v].update (c.ToPlayer().ToScore());
      else 
        ownage [v].update (board.EyeScore(v));
    }
  }

  Vertex Choose8Move (const Board& board, const NatMap<Vertex, uint>& v_seen, FastRandom& random) {
    if (!Param::mcmc_use) return Vertex::Invalid();

    Player pl = board.ActPlayer();
    Vertex best_v; // invalid == light move
    float best_value = - 1E20;

    Vertex last_v = board.LastVertex();
    if (last_v.IsValid() &&
        v_seen [last_v] == 1 && // only once played so far
        last_v != Vertex::Pass () &&
        random.GetNextUint(1024) < prob_8mcmc_1024)
      {
        McmcNode& act_mcmc = mcmc [board.LastMove ()];

        for_each_8_nbr (last_v, nbr, {
          if (v_seen[nbr] == 0 &&
              board.IsLegal (pl, nbr) &&
              !board.IsEyelike (pl, nbr))
          {
            Stat& stat = act_mcmc.move_stats [Move(pl, nbr)];
            float value = stat.Ucb();
            if (best_value < value) {
              best_value = value;
              best_v = nbr;
            }
          }
        });
      }

    return best_v;
  }

  void MovePlayed (Move m_pre, Move m, const NatMap<Vertex, uint>& v_seen) {
    if (v_seen [m_pre.GetVertex ()] != 1) return;
    if (v_seen [m    .GetVertex ()] != 0) return;
    McmcNode& act_mcmc = mcmc [m_pre];
    to_update.push_back (&act_mcmc.move_stats [m]);
    to_update_pl.push_back (m.GetPlayer ());
  }
  
  void MoveProbGfx (Move pre_move,
                    Player player,
                    const Board& board,
                    Gtp::GoguiGfx* gfx)
  {
    unused (pre_move);
    unused (player);
    unused (board);
    unused (gfx);
  }

  void MoveValueGfx (Move pre_move,
                     Player player,
                     const Board& board,
                     Gtp::GoguiGfx* gfx)
  {
    Stat stat(0.01, 0.0);
    Vertex pre_v = pre_move.GetVertex();

    for_each_8_nbr (pre_v, v, {
      if (board.ColorAt (v) == Color::Empty () && v != pre_move.GetVertex()) {
        Move move = Move (player, v);
        float mean = mcmc [pre_move].move_stats [move].mean();
        stat.update (mean);
      }
    });

    cerr << "light" << " : "
         << mcmc [pre_move].light [player].to_string () << endl;

    for_each_8_nbr (pre_v, v, {
        //ForEachNat (Vertex, v) {
      if (board.ColorAt (v) == Color::Empty () &&
          v != pre_move.GetVertex()) {
        Move move = Move (player, v);
        float mean = mcmc [pre_move].move_stats [move].mean();
        float val = (mean - stat.mean()) / stat.std_dev () / 4;
        gfx->SetInfluence (v.ToGtpString (), val);
        cerr << v.ToGtpString () << " : "
             << mcmc [pre_move].move_stats [move].to_string () << endl;
      }
    });
  }

  void OwnageGfx (Gtp::GoguiGfx* gfx) {
    ForEachNat (Vertex, v) {
      if (v.IsOnBoard()) {
        gfx->SetInfluence (v.ToGtpString (), ownage[v].mean());
      }
    }
  }

  NatMap <Move, McmcNode> mcmc;
  vector <Stat*> to_update;
  vector <Player> to_update_pl;
  NatMap <Vertex, Stat> ownage;
  uint prob_8mcmc_1024;
};
// -----------------------------------------------------------------------------

