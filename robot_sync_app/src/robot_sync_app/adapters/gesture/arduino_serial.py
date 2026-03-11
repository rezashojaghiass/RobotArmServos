import json
import time
from typing import Any, Dict, List, Optional

try:
    import serial
except Exception:  # pragma: no cover
    serial = None

from robot_sync_app.ports.gesture_port import GesturePort


class ArduinoSerialGestureAdapter(GesturePort):
    """
    Sends gesture commands to Arduino as JSON lines.

    SAFETY RULE:
    - Main arm servos are blocked by default.
    - Only finger gestures pass when enable_main_arms is False.
    """

    ARM_PREFIXES = ("arm_", "l_sh", "r_sh", "l_elb", "r_elb")

    def __init__(self, port: str, baud_rate: int, enable_main_arms: bool, allowed_finger_gestures: List[str]) -> None:
        self._enable_main_arms = enable_main_arms
        self._allowed_fingers = set(allowed_finger_gestures)
        self._serial = None

        if serial is not None:
            self._serial = serial.Serial(port=port, baudrate=baud_rate, timeout=0.1)
            time.sleep(2.0)
            try:
                self._serial.reset_input_buffer()
                self._serial.reset_output_buffer()
            except Exception:
                pass
            print(f"[GESTURE] Connected to Arduino on {port} @ {baud_rate}")

    def _is_arm_command(self, name: str) -> bool:
        n = name.lower()
        return n.startswith(self.ARM_PREFIXES)

    def _validate(self, name: str) -> None:
        n = name.lower()
        if self._is_arm_command(n) and not self._enable_main_arms:
            raise RuntimeError("Safety block: main arm servo command is disabled")
        if not self._is_arm_command(n) and n not in self._allowed_fingers:
            raise RuntimeError(f"Gesture not whitelisted: {name}")

    @staticmethod
    def _build_arm_calibration_line(name: str) -> str:
        parts = name.lower().split("_")
        if len(parts) != 6 or parts[0] != "arm" or parts[1] != "cal":
            raise RuntimeError(f"Unsupported arm calibration command: {name}")

        _, _, side, joint, direction, amount = parts
        if side not in {"left", "right"}:
            raise RuntimeError(f"Unsupported side in command: {name}")
        if joint not in {"elbow", "shoulder1", "shoulder2"}:
            raise RuntimeError(f"Unsupported joint in command: {name}")
        if direction not in {"up", "down"}:
            raise RuntimeError(f"Unsupported direction in command: {name}")
        if amount not in {"small"}:
            raise RuntimeError(f"Unsupported amount in command: {name}")

        return f"ARM_CAL:{side.upper()}:{joint.upper()}:{direction.upper()}:{amount.upper()}"

    @staticmethod
    def build_wire_line(name: str, is_start: bool) -> str:
        normalized_name = name.lower()
        if normalized_name == "fingers_wave":
            return "TALK_ON" if is_start else "TALK_OFF"
        if normalized_name.startswith("arm_cal_"):
            return ArduinoSerialGestureAdapter._build_arm_calibration_line(normalized_name)
        payload = {
            "type": "gesture_start" if is_start else "gesture_stop",
            "name": name,
        }
        return json.dumps(payload, separators=(",", ":"))

    def _drain_serial_logs(self, deadline_seconds: float = 0.8) -> bool:
        if self._serial is None:
            return False
        deadline = time.time() + deadline_seconds
        saw_line = False
        while time.time() < deadline:
            waiting = getattr(self._serial, "in_waiting", 0)
            if waiting <= 0:
                time.sleep(0.02)
                continue

            incoming = self._serial.readline().decode("utf-8", errors="ignore").strip()
            if incoming:
                saw_line = True
                print(f"[GESTURE] <- {incoming}")
        return saw_line

    def _send(self, payload: Dict[str, Any], wire_line: Optional[str] = None) -> None:
        line = wire_line if wire_line is not None else json.dumps(payload, separators=(",", ":"))
        if self._serial is None:
            print(f"[GESTURE-MOCK] {line}")
            return
        print(f"[GESTURE] -> {line}")
        try:
            self._serial.reset_input_buffer()
        except Exception:
            pass

        try:
            self._serial.write((line + "\n").encode("utf-8"))
            self._serial.flush()
            time.sleep(0.05)
            saw_line = self._drain_serial_logs(deadline_seconds=1.0)

            if not saw_line and wire_line in {"TALK_ON", "TALK_OFF"}:
                print(f"[GESTURE] retry -> {line}")
                self._serial.write((line + "\n").encode("utf-8"))
                self._serial.flush()
                time.sleep(0.05)
                self._drain_serial_logs(deadline_seconds=1.0)
        except Exception:
            pass

    def start_gesture(self, name: str) -> None:
        self._validate(name)
        wire_line = self.build_wire_line(name, is_start=True)
        self._send({"type": "gesture_start", "name": name}, wire_line=wire_line)

    def stop_gesture(self, name: str) -> None:
        self._validate(name)
        if name.lower().startswith("arm_cal_"):
            return
        wire_line = self.build_wire_line(name, is_start=False)
        self._send({"type": "gesture_stop", "name": name}, wire_line=wire_line)
