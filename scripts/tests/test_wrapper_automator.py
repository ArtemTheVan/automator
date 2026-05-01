"""Автотесты для wrapper_automator: проверка корректности построения
последовательностей действий и конвертации параметров без зависимости
от libautomator.dll и WinAPI."""

import os
import sys
import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
PARENT = os.path.dirname(HERE)
if PARENT not in sys.path:
    sys.path.insert(0, PARENT)

import wrapper_automator as wa


# =========== Поиск DLL ===========


def test_candidate_dll_paths_includes_explicit():
    paths = wa._candidate_dll_paths(r"C:\custom\libautomator.dll")
    assert any("custom" in p for p in paths)


def test_candidate_dll_paths_excludes_temp():
    """%TEMP% не должен быть в пути поиска (DLL hijacking)."""
    paths = wa._candidate_dll_paths(None)
    temp = os.environ.get("TEMP") or os.environ.get("TMP") or ""
    if temp:
        for p in paths:
            assert not p.lower().startswith(temp.lower()), \
                f"%TEMP% найден в путях поиска: {p}"


def test_candidate_dll_paths_uses_env_var(monkeypatch):
    monkeypatch.setenv("AUTOMATOR_DLL_PATH", r"C:\env\libautomator.dll")
    paths = wa._candidate_dll_paths(None)
    assert any("env" in p for p in paths)


def test_candidate_dll_paths_uses_neighbour_lib():
    """Должен искать в ../lib/libautomator.dll рядом со скриптом."""
    paths = wa._candidate_dll_paths(None)
    assert any(p.endswith(os.path.join("lib", "libautomator.dll")) for p in paths)


# =========== Высокоуровневые методы ===========


def test_keystroke_encodes_utf8(fake_automator):
    fake_automator.keystroke("Привет")
    name, args, _ = fake_automator._fake.calls[0]
    assert name == "simulate_keystroke"
    assert args[0] == "Привет".encode("utf-8")


def test_keystroke_passes_bytes_unchanged(fake_automator):
    fake_automator.keystroke(b"already bytes")
    _, args, _ = fake_automator._fake.calls[0]
    assert args[0] == b"already bytes"


def test_click_left_produces_down_up(fake_automator):
    fake_automator.click(100, 200)
    name, args, _ = fake_automator._fake.calls[0]
    assert name == "simulate_mouse_sequence"
    actions, count = args
    assert count == 2
    assert actions[0].x == 100 and actions[0].y == 200
    assert actions[0].dwFlags == wa.MOUSEEVENTF_LEFTDOWN
    assert actions[1].dwFlags == wa.MOUSEEVENTF_LEFTUP


def test_click_right_produces_right_flags(fake_automator):
    fake_automator.click(50, 60, button="right")
    actions, _ = fake_automator._fake.calls[0][1]
    assert actions[0].dwFlags == wa.MOUSEEVENTF_RIGHTDOWN
    assert actions[1].dwFlags == wa.MOUSEEVENTF_RIGHTUP


def test_click_middle_produces_middle_flags(fake_automator):
    fake_automator.click(0, 0, button="middle")
    actions, _ = fake_automator._fake.calls[0][1]
    assert actions[0].dwFlags == wa.MOUSEEVENTF_MIDDLEDOWN
    assert actions[1].dwFlags == wa.MOUSEEVENTF_MIDDLEUP


def test_click_unknown_button_raises(fake_automator):
    with pytest.raises(ValueError):
        fake_automator.click(0, 0, button="quadruple")


def test_click_at_is_alias_for_click(fake_automator):
    """click_at должен делать то же самое, что click (баг #20)."""
    fake_automator.click_at(10, 20)
    actions, count = fake_automator._fake.calls[0][1]
    assert count == 2
    assert actions[0].x == 10 and actions[0].y == 20


def test_drag_three_steps(fake_automator):
    fake_automator.drag(0, 0, 100, 100)
    actions, count = fake_automator._fake.calls[0][1]
    assert count == 3
    assert actions[0].dwFlags == wa.MOUSEEVENTF_LEFTDOWN
    assert actions[2].dwFlags == wa.MOUSEEVENTF_LEFTUP


def test_rectangle_six_steps(fake_automator):
    fake_automator.rectangle(0, 0, 50, 30)
    _, count = fake_automator._fake.calls[0][1]
    assert count == 6


def test_move_absolute_includes_absolute_flag(fake_automator):
    fake_automator.move(500, 500, absolute=True)
    actions, _ = fake_automator._fake.calls[0][1]
    assert actions[0].dwFlags & wa.MOUSEEVENTF_ABSOLUTE


def test_move_relative_excludes_absolute_flag(fake_automator):
    fake_automator.move(10, 10, absolute=False)
    actions, _ = fake_automator._fake.calls[0][1]
    assert not (actions[0].dwFlags & wa.MOUSEEVENTF_ABSOLUTE)


def test_capture_screen_encodes_filename(fake_automator):
    fake_automator.capture_screen(0, 0, 100, 100, "out.bmp")
    name, args, _ = fake_automator._fake.calls[0]
    assert name == "capture_screen_region"
    assert args[-1] == b"out.bmp"


def test_mouse_sequence_accepts_tuples(fake_automator):
    fake_automator.mouse_sequence([
        (10, 20, wa.MOUSEEVENTF_LEFTDOWN),
        (30, 40, wa.MOUSEEVENTF_LEFTUP),
    ])
    actions, count = fake_automator._fake.calls[0][1]
    assert count == 2
    assert actions[1].x == 30 and actions[1].y == 40


def test_mouse_sequence_accepts_dicts(fake_automator):
    fake_automator.mouse_sequence([
        {"x": 1, "y": 2, "flags": wa.MOUSEEVENTF_LEFTDOWN},
    ])
    actions, _ = fake_automator._fake.calls[0][1]
    assert actions[0].x == 1 and actions[0].dwFlags == wa.MOUSEEVENTF_LEFTDOWN
