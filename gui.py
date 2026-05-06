import os
import shutil
import subprocess
import tkinter as tk


CARD_WIDTH = 70
CARD_HEIGHT = 95
CARD_GAP_X = 20
CARD_GAP_Y = 28

WINDOW_WIDTH = 1000
WINDOW_HEIGHT = 700

BACKGROUND_COLOR = "#3f5f64"
CARD_COLOR = "#f2f2f2"
CARD_BACK_COLOR = "#16728a"
CARD_OUTLINE = "#dddddd"
FOUNDATION_OUTLINE = "#f2a51a"


def find_cmake_program():
    candidates = [
        shutil.which("cmake"),
        "/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake",
        "/opt/homebrew/bin/cmake",
        "/usr/local/bin/cmake",
    ]

    for candidate in candidates:
        if candidate and os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate

    return None


def find_backend_program():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(base_dir, "build-local", "yukon_backend"),
        os.path.join(base_dir, "cmake-build-debug", "yukon_backend"),
        os.path.join(base_dir, "build", "yukon_backend"),
        os.path.join(base_dir, "yukon_backend"),
    ]

    for candidate in candidates:
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate

    return None


def build_backend():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(base_dir, "build-local")
    cmake_program = find_cmake_program()

    if cmake_program is None:
        raise RuntimeError(
            "Could not find cmake. Install cmake or use the CLion bundled cmake."
        )

    subprocess.run(
        [cmake_program, "-S", base_dir, "-B", build_dir],
        check=True,
        cwd=base_dir,
    )
    subprocess.run(
        [cmake_program, "--build", build_dir, "--target", "yukon_backend"],
        check=True,
        cwd=base_dir,
    )


def ensure_backend_program():
    backend_program = find_backend_program()
    if backend_program is not None:
        return backend_program

    build_backend()
    backend_program = find_backend_program()
    if backend_program is None:
        raise RuntimeError("Built yukon_backend, but could not find the executable afterwards.")

    return backend_program


class ProjektGUI:
    def __init__(self, root, backend_program):
        self.root = root
        self.root.title("ProjektGUI")

        self.canvas = tk.Canvas(
            root,
            width=WINDOW_WIDTH,
            height=WINDOW_HEIGHT,
            bg=BACKGROUND_COLOR,
            highlightthickness=0,
        )
        self.canvas.pack()

        self.message_label = tk.Label(
            root,
            text="",
            font=("Arial", 14),
            bg=BACKGROUND_COLOR,
            fg="white",
        )
        self.message_label.pack(fill="x")

        self.selected_source = None
        self.card_positions = []
        self.foundation_positions = []

        self.backend = subprocess.Popen(
            [backend_program],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
        )

        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.canvas.bind("<Button-1>", self.handle_click)

        self.send_command("LD")
        self.send_command("SR")
        state_lines = self.send_command("P")
        self.columns, self.foundations = self.parse_state(state_lines)
        self.draw_board()

    def close(self):
        if self.backend is not None:
            try:
                self.send_command("QQ")
            except BrokenPipeError:
                pass
            self.backend.terminate()
            self.backend = None
        self.root.destroy()

    def send_command(self, command):
        if self.backend is None or self.backend.stdin is None or self.backend.stdout is None:
            return []

        self.backend.stdin.write(command + "\n")
        self.backend.stdin.flush()

        lines = []
        while True:
            line = self.backend.stdout.readline()
            if line == "":
                break

            line = line.strip()
            if line == "END":
                break

            lines.append(line)

        return lines

    def parse_state(self, lines):
        columns = [[], [], [], [], [], [], []]
        foundations = [[], [], [], []]

        for line in lines:
            if line.startswith("C"):
                name, cards_text = line.split(":", 1)
                index = int(name[1]) - 1
                columns[index] = [] if cards_text == "" else cards_text.split(",")

            if line.startswith("F"):
                name, cards_text = line.split(":", 1)
                index = int(name[1]) - 1
                foundations[index] = [] if cards_text == "" else cards_text.split(",")

        return columns, foundations

    def draw_board(self):
        self.canvas.delete("all")
        self.card_positions = []
        self.foundation_positions = []

        self.draw_labels()
        self.draw_columns()
        self.draw_foundations()

    def draw_labels(self):
        start_x = 40

        for i in range(7):
            x = start_x + i * (CARD_WIDTH + CARD_GAP_X)
            self.canvas.create_text(
                x + CARD_WIDTH / 2,
                20,
                text="C" + str(i + 1),
                fill="white",
                font=("Arial", 14, "bold"),
            )

        self.canvas.create_text(
            890,
            20,
            text="Foundations",
            fill="white",
            font=("Arial", 14, "bold"),
        )

    def draw_columns(self):
        start_x = 40
        start_y = 45

        for column_index in range(7):
            x = start_x + column_index * (CARD_WIDTH + CARD_GAP_X)

            for card_index, card in enumerate(self.columns[column_index]):
                y = start_y + card_index * CARD_GAP_Y
                self.draw_card(x, y, card)
                self.card_positions.append(
                    {
                        "x1": x,
                        "y1": y,
                        "x2": x + CARD_WIDTH,
                        "y2": y + CARD_HEIGHT,
                        "column": column_index,
                        "card_index": card_index,
                        "card": card,
                    }
                )

    def draw_foundations(self):
        x = 860
        y = 45

        for i in range(4):
            foundation_y = y + i * 145

            self.canvas.create_rectangle(
                x,
                foundation_y,
                x + CARD_WIDTH,
                foundation_y + CARD_HEIGHT,
                outline=FOUNDATION_OUTLINE,
                width=4,
            )

            self.foundation_positions.append(
                {
                    "x1": x,
                    "y1": foundation_y,
                    "x2": x + CARD_WIDTH,
                    "y2": foundation_y + CARD_HEIGHT,
                    "foundation": i,
                }
            )

            if self.foundations[i]:
                self.draw_card(x, foundation_y, self.foundations[i][-1])

    def draw_card(self, x, y, card):
        if card == "XX":
            self.canvas.create_rectangle(
                x,
                y,
                x + CARD_WIDTH,
                y + CARD_HEIGHT,
                fill=CARD_BACK_COLOR,
                outline=CARD_OUTLINE,
                width=3,
            )
            return

        self.canvas.create_rectangle(
            x,
            y,
            x + CARD_WIDTH,
            y + CARD_HEIGHT,
            fill=CARD_COLOR,
            outline=CARD_OUTLINE,
            width=2,
        )

        color = self.get_card_color(card)

        self.canvas.create_text(
            x + 8,
            y + 8,
            text=card,
            fill=color,
            font=("Arial", 14, "bold"),
            anchor="nw",
        )

        self.canvas.create_text(
            x + CARD_WIDTH - 8,
            y + CARD_HEIGHT - 8,
            text=card,
            fill=color,
            font=("Arial", 14, "bold"),
            anchor="se",
        )

    def get_card_color(self, card):
        return "red" if card[-1] in ("H", "D") else "black"

    def handle_click(self, event):
        clicked_card = self.get_clicked_card(event.x, event.y)
        clicked_foundation = self.get_clicked_foundation(event.x, event.y)
        clicked_column = self.get_clicked_column(event.x, event.y)

        if clicked_card is not None:
            if clicked_card["card"] == "XX":
                self.message_label.config(text="Du kan ikke vælge et face-down kort")
                return

            column_number = clicked_card["column"] + 1
            card = clicked_card["card"]
            self.selected_source = "C" + str(column_number) + card
            self.message_label.config(text="Valgt: " + card + " fra C" + str(column_number))
            return

        if self.selected_source is not None and clicked_foundation is not None:
            foundation_number = clicked_foundation + 1
            command = self.selected_source + "->F" + str(foundation_number)
            self.execute_move(command)
            return

        if self.selected_source is not None and clicked_column is not None:
            column_number = clicked_column + 1
            command = self.selected_source + "->C" + str(column_number)
            self.execute_move(command)

    def execute_move(self, command):
        lines = self.send_command(command)
        error_message = ""

        for line in lines:
            if line.startswith("ERROR"):
                error_message = line

        self.columns, self.foundations = self.parse_state(lines)
        self.draw_board()

        if error_message != "":
            self.message_label.config(text=error_message)
        else:
            self.message_label.config(text="Move: " + command)

        self.selected_source = None

    def get_clicked_card(self, x, y):
        for position in reversed(self.card_positions):
            if position["x1"] <= x <= position["x2"] and position["y1"] <= y <= position["y2"]:
                return position

        return None

    def get_clicked_foundation(self, x, y):
        for position in self.foundation_positions:
            if position["x1"] <= x <= position["x2"] and position["y1"] <= y <= position["y2"]:
                return position["foundation"]

        return None

    def get_clicked_column(self, x, y):
        start_x = 40

        for i in range(7):
            column_x = start_x + i * (CARD_WIDTH + CARD_GAP_X)
            if column_x <= x <= column_x + CARD_WIDTH:
                return i

        return None


def main():
    try:
        backend_program = ensure_backend_program()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"Could not start GUI backend: {exc}")
        return 1

    root = tk.Tk()
    ProjektGUI(root, backend_program)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
