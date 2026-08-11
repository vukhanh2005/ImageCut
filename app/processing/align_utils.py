from typing import List
from app.core.image_document import ImageDocument
from app.core.layer import Layer

class AlignUtils:
    """
    Utility methods for aligning and distributing layers relative to Canvas or Selection.
    """

    @staticmethod
    def align_layers(doc: ImageDocument, mode: str, target: str = "Canvas"):
        """
        Aligns active selected layers.
        Modes: 'left', 'center', 'right', 'top', 'middle', 'bottom'.
        Target: 'Canvas' or 'Selection'.
        """
        active_layers = doc.active_layers
        if not active_layers:
            return

        cw = doc.canvas_width
        ch = doc.canvas_height

        if target == "Selection" and len(active_layers) > 1:
            min_x = min(l.offset_x for l in active_layers)
            max_x = max(l.offset_x + l.width() * l.scale_x for l in active_layers)
            min_y = min(l.offset_y for l in active_layers)
            max_y = max(l.offset_y + l.height() * l.scale_y for l in active_layers)
            sel_w = max_x - min_x
            sel_h = max_y - min_y
        else:
            min_x, min_y = 0.0, 0.0
            sel_w, sel_h = float(cw), float(ch)

        for l in active_layers:
            if l.locked:
                continue
            lw = l.width() * l.scale_x
            lh = l.height() * l.scale_y

            if mode == "left":
                l.offset_x = min_x
            elif mode == "center":
                l.offset_x = min_x + (sel_w - lw) / 2.0
            elif mode == "right":
                l.offset_x = min_x + sel_w - lw
            elif mode == "top":
                l.offset_y = min_y
            elif mode == "middle":
                l.offset_y = min_y + (sel_h - lh) / 2.0
            elif mode == "bottom":
                l.offset_y = min_y + sel_h - lh

        doc.notify_changed()

    @staticmethod
    def distribute_layers(doc: ImageDocument, orientation: str = "horizontal"):
        """
        Distributes 3 or more selected layers evenly horizontally or vertically.
        """
        active_layers = [l for l in doc.active_layers if not l.locked]
        if len(active_layers) < 3:
            return

        if orientation == "horizontal":
            # Sort by offset_x
            active_layers.sort(key=lambda l: l.offset_x)
            first = active_layers[0]
            last = active_layers[-1]

            start_x = first.offset_x + first.width() * first.scale_x
            end_x = last.offset_x
            total_dist = end_x - start_x

            middle_layers = active_layers[1:-1]
            middle_widths = sum(l.width() * l.scale_x for l in middle_layers)
            gap = (total_dist - middle_widths) / (len(active_layers) - 1)

            curr_x = start_x + gap
            for l in middle_layers:
                l.offset_x = curr_x
                curr_x += (l.width() * l.scale_x) + gap

        elif orientation == "vertical":
            active_layers.sort(key=lambda l: l.offset_y)
            first = active_layers[0]
            last = active_layers[-1]

            start_y = first.offset_y + first.height() * first.scale_y
            end_y = last.offset_y
            total_dist = end_y - start_y

            middle_layers = active_layers[1:-1]
            middle_heights = sum(l.height() * l.scale_y for l in middle_layers)
            gap = (total_dist - middle_heights) / (len(active_layers) - 1)

            curr_y = start_y + gap
            for l in middle_layers:
                l.offset_y = curr_y
                curr_y += (l.height() * l.scale_y) + gap

        doc.notify_changed()
