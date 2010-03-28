#ifndef MANAGER_H_
#define MANAGER_H_

#include <QObject>
#include <QString>


class GameScene;
class Engine;
class Move;
class Player;

class Manager : public QObject {
public:
  Manager (Engine& engine, GameScene *scene, QObject * parent = 0);

  virtual ~Manager ();

  void refreshBoard ();

public slots:
  void handleMousePress (int x, int y, Qt::MouseButtons buttons);
  void handleHooverEntered (int x, int y);
  void setKomi (double);
  void playMove ();
  void undoMove ();
  void showGammas (int);

signals:
  void stateChanged (const Player&);
  void statusChanged (QString);

private:
  GameScene *m_gameScene;
  Engine& engine_;

private:
  Q_OBJECT
};

#endif /* MANAGER_H_ */
