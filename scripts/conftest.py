"""Глобальные pytest-фикстуры для wrapper_automator."""

import os
import sys
import types
import pytest

# Делаем импорт wrapper_automator работающим из подкаталогов tests/.
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)


class _FakeLib:
    """Заглушка ctypes-библиотеки: фиксирует все вызовы для проверки."""

    def __init__(self):
        self.calls = []
        # Атрибуты, которые wrapper_automator настраивает через _setup_functions.
        for name in (
            "simulate_keystroke",
            "simulate_mouse_sequence",
            "simulate_mouse_click_at",
            "capture_screen_region",
        ):
            self.__dict__[name] = self._make_recorder(name)

    def _make_recorder(self, name):
        recorder = types.SimpleNamespace()
        recorder.argtypes = None
        recorder.restype = None

        def __call__(*args, **kwargs):
            self.calls.append((name, args, kwargs))
            # capture_screen_region возвращает int.
            if name == "capture_screen_region":
                return 1
            return None

        recorder.__call__ = __call__
        # Делаем сам recorder вызываемым.
        return _Callable(name, self.calls)


class _Callable:
    """Отдельный класс, чтобы можно было задавать argtypes/restype как атрибуты."""

    def __init__(self, name, journal):
        self._name = name
        self._journal = journal
        self.argtypes = None
        self.restype = None

    def __call__(self, *args, **kwargs):
        self._journal.append((self._name, args, kwargs))
        if self._name == "capture_screen_region":
            return 1
        return None


@pytest.fixture
def fake_automator(monkeypatch):
    """
    Возвращает свежий экземпляр Automator с заглушенной DLL.
    Сбрасывает синглтон, поэтому изменения не утекают между тестами.
    """
    import wrapper_automator as wa

    # Сбрасываем синглтон.
    wa.Automator._instance = None
    wa.Automator._initialized = False
    wa._LazyAutomator._real = None

    fake = _FakeLib()

    def fake_load(self):
        self.lib = fake
        self._loaded_path = "<fake>"

    monkeypatch.setattr(wa.Automator, "_load_library", fake_load)
    monkeypatch.setattr(wa.Automator, "_setup_functions", lambda self: None)

    instance = wa.Automator()
    instance._fake = fake  # удобный доступ из тестов
    yield instance

    # Cleanup.
    wa.Automator._instance = None
    wa.Automator._initialized = False
    wa._LazyAutomator._real = None
