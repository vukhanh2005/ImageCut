import copy
from typing import List, Callable
import numpy as np
from app.utils.logger import logger

class Command:
    """Encapsulates an undoable operation."""
    def execute(self):
        pass

    def undo(self):
        pass

class MaskEditCommand(Command):
    """Command for mask modifications (Brush, Eraser, Auto Remove, Magic Wand, Filters)."""
    def __init__(self, document, old_mask: np.ndarray, new_mask: np.ndarray, layer_id: str = None, description: str = "Edit Mask"):
        self.document = document
        self.layer_id = layer_id
        self.old_mask = old_mask.copy() if old_mask is not None else None
        self.new_mask = new_mask.copy() if new_mask is not None else None
        self.description = description

    def execute(self):
        target = self.document.get_layer_by_id(self.layer_id) if self.layer_id else self.document.active_layer
        if target is not None:
            target.mask = self.new_mask.copy() if self.new_mask is not None else None
            self.document.notify_changed()
        elif hasattr(self.document, '_mask'):
            self.document._mask = self.new_mask.copy() if self.new_mask is not None else None
            self.document.notify_changed()

    def undo(self):
        target = self.document.get_layer_by_id(self.layer_id) if self.layer_id else self.document.active_layer
        if target is not None:
            target.mask = self.old_mask.copy() if self.old_mask is not None else None
            self.document.notify_changed()
        elif hasattr(self.document, '_mask'):
            self.document._mask = self.old_mask.copy() if self.old_mask is not None else None
            self.document.notify_changed()

class DocumentActionCommand(Command):
    """Generic action command for undoing/redoing document layer state changes."""
    def __init__(self, document, undo_fn: Callable, redo_fn: Callable, description: str = "Action"):
        self.document = document
        self._undo_fn = undo_fn
        self._redo_fn = redo_fn
        self.description = description

    def execute(self):
        self._redo_fn()
        self.document.notify_changed()

    def undo(self):
        self._undo_fn()
        self.document.notify_changed()


class UndoStack:
    """Manages command history stack for Undo / Redo functionality."""

    def __init__(self, max_depth: int = 30):
        self.max_depth = max_depth
        self._undo_stack: List[Command] = []
        self._redo_stack: List[Command] = []
        self._on_change_callbacks: List[Callable] = []

    def push(self, command: Command):
        command.execute()
        self._undo_stack.append(command)
        if len(self._undo_stack) > self.max_depth:
            self._undo_stack.pop(0)
        self._redo_stack.clear()
        self._notify()

    def undo(self):
        if not self.can_undo():
            return
        cmd = self._undo_stack.pop()
        cmd.undo()
        self._redo_stack.append(cmd)
        self._notify()

    def redo(self):
        if not self.can_redo():
            return
        cmd = self._redo_stack.pop()
        cmd.execute()
        self._undo_stack.append(cmd)
        self._notify()

    def can_undo(self) -> bool:
        return len(self._undo_stack) > 0

    def can_redo(self) -> bool:
        return len(self._redo_stack) > 0

    def clear(self):
        self._undo_stack.clear()
        self._redo_stack.clear()
        self._notify()

    def add_change_listener(self, callback: Callable):
        self._on_change_callbacks.append(callback)

    def _notify(self):
        for cb in self._on_change_callbacks:
            try:
                cb()
            except Exception as e:
                logger.error(f"Error in UndoStack change listener: {e}")
