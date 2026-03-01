#pragma once

// ================================================================
//  SnakeGame  —  2-button relative turning
//
//  UP   → turn left  (relative to snake's heading)
//  DOWN → turn right (relative to snake's heading)
//
//  Turn table:
//    Heading RIGHT → left=UP,    right=DOWN
//    Heading UP    → left=LEFT,  right=RIGHT
//    Heading LEFT  → left=DOWN,  right=UP
//    Heading DOWN  → left=RIGHT, right=LEFT
//
//  SELECT → start / restart
//  BACK   → quit
//
//  Layout:
//  ┌──────────────────────────────────────────────────────────┐
//  │  SCORE  12                              HI  42    y=0..9 │
//  │ ──────────────────────────────────────────────────────── │
//  │  play area  128×53px  grid 16×6 cells of 8px   y=11..63 │
//  └──────────────────────────────────────────────────────────┘
// ================================================================

#include "../IScreen.h"
#include "../../../core/DisplayManager.h"
#include "../../../storage/GameScore.h"
#include <Arduino.h>
#include <vector>

class SnakeGame : public IScreen {
public:
    explicit SnakeGame(DisplayManager& d) : _display(d), _score("snake") {}

    void onEnter() override {
        _score.load();
        _state    = State::TITLE;
        _wantsPop = false;
        _dirty    = true;
    }

    bool needsContinuousUpdate() const override { return true; }
    bool wantsPop() const override { return _wantsPop; }

    // ── Buttons ──────────────────────────────────────────────
    void onButtonUp() override {
        if (_state == State::RUNNING)
            _nextDir = _turnLeft(_dir);
        else
            _startGame();
    }

    void onButtonDown() override {
        if (_state == State::RUNNING)
            _nextDir = _turnRight(_dir);
        else
            _startGame();
    }

    void onButtonSelect() override { _startGame(); }

    void onButtonBack() override {
        if (_state == State::RUNNING) _score.save();
        _wantsPop = true;
    }

    // ── Update ────────────────────────────────────────────────
    void update() override {
        if (_state == State::RUNNING) {
            unsigned long now = millis();
            if (now - _lastTick >= _tickMs) {
                _lastTick = now;
                _tick();
                _dirty = true;
            }
        }

        if (!_dirty) return;
        _dirty = false;
        _draw();
    }

private:
    enum class State { TITLE, RUNNING, DEAD };
    enum class Dir   { UP, DOWN, LEFT, RIGHT };
    struct Cell      { int x, y; };

    static constexpr int GW = 16;
    static constexpr int GH = 6;
    static constexpr int CS = 8;
    static constexpr int OY = 11;   // play area top y (below score bar)

    DisplayManager&   _display;
    GameScore         _score;
    State             _state    = State::TITLE;
    Dir               _dir      = Dir::RIGHT;
    Dir               _nextDir  = Dir::RIGHT;
    std::vector<Cell> _snake;
    Cell              _food     = {0, 0};
    unsigned long     _tickMs   = 200;
    unsigned long     _lastTick = 0;
    bool              _wantsPop = false;

    // ── Relative turning ──────────────────────────────────────
    Dir _turnLeft(Dir d) {
        switch (d) {
            case Dir::RIGHT: return Dir::UP;
            case Dir::UP:    return Dir::LEFT;
            case Dir::LEFT:  return Dir::DOWN;
            case Dir::DOWN:  return Dir::RIGHT;
        }
        return d;
    }

    Dir _turnRight(Dir d) {
        switch (d) {
            case Dir::RIGHT: return Dir::DOWN;
            case Dir::DOWN:  return Dir::LEFT;
            case Dir::LEFT:  return Dir::UP;
            case Dir::UP:    return Dir::RIGHT;
        }
        return d;
    }

    // ── Game control ──────────────────────────────────────────
    void _startGame() {
        if (_state == State::RUNNING) return;
        _snake.clear();
        _snake.push_back({6, 3});
        _snake.push_back({5, 3});
        _snake.push_back({4, 3});
        _dir      = Dir::RIGHT;
        _nextDir  = Dir::RIGHT;
        _tickMs   = 200;
        _lastTick = millis();
        _score.reset();
        _spawnFood();
        _state = State::RUNNING;
        _dirty = true;
    }

    void _spawnFood() {
        for (int attempt = 0; attempt < 200; attempt++) {
            int fx = random(0, GW);
            int fy = random(0, GH);
            bool hit = false;
            for (auto& s : _snake) if (s.x == fx && s.y == fy) { hit = true; break; }
            if (!hit) { _food = {fx, fy}; return; }
        }
    }

    // ── Game tick ─────────────────────────────────────────────
    void _tick() {
        _dir = _nextDir;

        Cell head = _snake.front();
        switch (_dir) {
            case Dir::UP:    head.y--; break;
            case Dir::DOWN:  head.y++; break;
            case Dir::LEFT:  head.x--; break;
            case Dir::RIGHT: head.x++; break;
        }

        if (head.x < 0 || head.x >= GW || head.y < 0 || head.y >= GH) {
            _die(); return;
        }

        for (int i = 0; i < (int)_snake.size() - 1; i++) {
            if (_snake[i].x == head.x && _snake[i].y == head.y) {
                _die(); return;
            }
        }

        _snake.insert(_snake.begin(), head);

        if (head.x == _food.x && head.y == _food.y) {
            _score.add(1);
            if (_score.getCurrentScore() % 5 == 0 && _tickMs > 60) _tickMs -= 20;
            _spawnFood();
        } else {
            _snake.pop_back();
        }
    }

    void _die() {
        _state = State::DEAD;
        _score.save();
        _dirty = true;
    }

    // ── Draw ──────────────────────────────────────────────────
    void _draw() {
        auto& u = _display.raw();
        u.clearBuffer();

        switch (_state) {
            case State::TITLE:
                _drawTitle(u);
                break;
            case State::RUNNING:
                _drawRunning(u);
                break;
            case State::DEAD:
                _drawRunning(u);
                _drawGameOver(u);
                break;
        }

        u.sendBuffer();
    }

    void _drawTitle(U8G2& u) {
        u.setFont(u8g2_font_ncenB14_tr);
        int w = u.getStrWidth("SNAKE");
        u.drawStr((128 - w) / 2, 24, "SNAKE");

        u.setFont(u8g2_font_5x7_tr);
        char hi[16];
        snprintf(hi, sizeof(hi), "Best: %d", _score.getHighScore());
        w = u.getStrWidth(hi);
        u.drawStr((128 - w) / 2, 36, hi);

        u.drawStr(22, 50, "UP/DN to turn");
        u.drawStr(20, 60, "[SEL] to start");
    }

    void _drawRunning(U8G2& u) {
        _score.draw(u);

        u.drawFrame(0, OY, GW * CS, GH * CS);

        for (int i = 0; i < (int)_snake.size(); i++) {
            int px = _snake[i].x * CS;
            int py = _snake[i].y * CS + OY;
            if (i == 0)
                u.drawBox(px + 1, py + 1, CS - 2, CS - 2);
            else
                u.drawBox(px + 2, py + 2, CS - 4, CS - 4);
        }

        // Food — diamond
        int fx = _food.x * CS + CS / 2;
        int fy = _food.y * CS + CS / 2 + OY;
        u.drawLine(fx,     fy - 2, fx + 2, fy    );
        u.drawLine(fx + 2, fy,     fx,     fy + 2);
        u.drawLine(fx,     fy + 2, fx - 2, fy    );
        u.drawLine(fx - 2, fy,     fx,     fy - 2);
    }

    void _drawGameOver(U8G2& u) {
        constexpr int BX = 20, BY = 16, BW = 88, BH = 36;
        u.setDrawColor(0);
        u.drawBox(BX, BY, BW, BH);
        u.setDrawColor(1);
        u.drawFrame(BX, BY, BW, BH);

        u.setFont(u8g2_font_6x10_tr);
        int w = u.getStrWidth("GAME OVER");
        u.drawStr(BX + (BW - w) / 2, BY + 12, "GAME OVER");

        _score.drawGameOver(u, BX, BY, BW, BH);

        u.setFont(u8g2_font_5x7_tr);
        w = u.getStrWidth("[SEL] Retry");
        u.drawStr(BX + (BW - w) / 2, BY + BH + 10, "[SEL] Retry");
    }
};