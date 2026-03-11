from datetime import datetime, timezone
from typing import Callable, Optional, Tuple

from robot_sync_app.application.orchestrator_service import OrchestratorService
from robot_sync_app.ports.asr_port import ASRPort
from robot_sync_app.ports.llm_port import LLMPort
from robot_sync_app.ports.storage_port import StoragePort


class VoiceSessionService:
    def __init__(
        self,
        asr: ASRPort,
        llm: LLMPort,
        orchestrator: OrchestratorService,
        storage: StoragePort,
    ) -> None:
        self._asr = asr
        self._llm = llm
        self._orchestrator = orchestrator
        self._storage = storage

    def run(self, intent: str = "chat", max_turns: int = 0) -> None:
        if intent == "arm_calibration":
            self._run_arm_calibration(max_turns=max_turns)
            return

        turn = 0
        print("🎙️ Voice session started. Say 'stop' to end.")

        # Startup greeting: ask user's name instead of using a fixed one.
        self._orchestrator.run_once(
            text="Hi! Nice to meet you. What is your name?",
            intent=intent,
        )

        while True:
            if max_turns > 0 and turn >= max_turns:
                print("✓ Reached max turns")
                break

            user_text = self._asr.listen_and_transcribe()
            if not user_text:
                continue

            if user_text.lower().strip() in {"stop", "exit", "quit", "bye"}:
                print("👋 Ending voice session")
                break

            reply = self._llm.generate_reply(user_text=user_text, intent=intent)
            self._orchestrator.run_once(text=reply, intent=intent)

            turn += 1
            self._storage.put_json(
                "sessions/latest_turn.json",
                {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "turn": turn,
                    "intent": intent,
                    "user_text": user_text,
                    "assistant_reply": reply,
                },
            )

    @staticmethod
    def _normalize(text: str) -> str:
        normalized = text.lower().strip()
        normalized = normalized.replace("shoulder 1", "shoulder1")
        normalized = normalized.replace("shoulder one", "shoulder1")
        normalized = normalized.replace("shoulder 2", "shoulder2")
        normalized = normalized.replace("shoulder two", "shoulder2")
        normalized = normalized.replace("some more", "more")
        normalized = normalized.replace("a little", "little")
        return " ".join(normalized.split())

    @staticmethod
    def _is_stop(text: str) -> bool:
        return VoiceSessionService._normalize(text) in {"stop", "exit", "quit", "bye"}

    def _say(self, text: str) -> None:
        self._orchestrator.run_once(text=text, intent="arm_calibration_prompt")

    def _listen_for_choice(
        self,
        prompt: str,
        parser: Callable[[str], Optional[str]],
    ) -> Optional[str]:
        while True:
            self._say(prompt)
            print(f"🎯 Waiting for keyword response to: {prompt}")
            user_text = self._asr.listen_and_transcribe()
            if not user_text:
                continue

            print(f"📝 User said: {user_text}")
            if self._is_stop(user_text):
                return None

            choice = parser(user_text)
            if choice is not None:
                return choice

            self._say("I did not catch one of the expected keywords. Please try again.")

    def _parse_joint(self, text: str) -> Optional[str]:
        normalized = self._normalize(text)
        if "elbow" in normalized or "elbo" in normalized or "elb" in normalized:
            return "elbow"
        if "shoulder1" in normalized:
            return "shoulder1"
        if "shoulder2" in normalized:
            return "shoulder2"
        return None

    def _parse_side(self, text: str) -> Optional[str]:
        normalized = self._normalize(text)
        if "left" in normalized:
            return "left"
        if "right" in normalized:
            return "right"
        return None

    def _parse_direction(self, text: str) -> Optional[str]:
        normalized = self._normalize(text)
        if "up" in normalized or "raise" in normalized:
            return "up"
        if "down" in normalized or "lower" in normalized:
            return "down"
        return None

    def _parse_amount(self, text: str) -> Optional[str]:
        normalized = self._normalize(text)
        if "little" in normalized or "small" in normalized or "more" in normalized:
            return "small"
        return None

    def _parse_follow_up(self, text: str) -> Optional[str]:
        normalized = self._normalize(text)
        if "more" in normalized or "again" in normalized:
            return "more"
        if "reverse" in normalized or "other way" in normalized:
            return "reverse"
        if "enough" in normalized or "menu" in normalized:
            return "menu"
        if "another" in normalized or "change" in normalized or "different" in normalized:
            return "menu"
        if self._is_stop(normalized):
            return "stop"
        return None

    @staticmethod
    def _invert_direction(direction: str) -> str:
        return "down" if direction == "up" else "up"

    @staticmethod
    def _format_joint(joint: str) -> str:
        return {
            "elbow": "elbow",
            "shoulder1": "shoulder one",
            "shoulder2": "shoulder two",
        }[joint]

    def _build_arm_command(self, side: str, joint: str, direction: str, amount: str) -> str:
        return f"arm_cal_{side}_{joint}_{direction}_{amount}"

    def _execute_arm_command(self, side: str, joint: str, direction: str, amount: str, turn: int) -> None:
        command_name = self._build_arm_command(side=side, joint=joint, direction=direction, amount=amount)
        self._say(f"Moving your {side} {self._format_joint(joint)} {direction} a little.")
        self._orchestrator.send_command(command_name, intent="arm_calibration")
        self._storage.put_json(
            "sessions/latest_arm_calibration.json",
            {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "turn": turn,
                "intent": "arm_calibration",
                "side": side,
                "joint": joint,
                "direction": direction,
                "amount": amount,
                "command": command_name,
            },
        )

    def _run_arm_calibration(self, max_turns: int = 0) -> None:
        turn = 0
        last_selection: Optional[Tuple[str, str, str, str]] = None

        print("🦾 Arm calibration session started. Say 'stop' to end.")
        self._say(
            "Calibration mode is ready. I will ask for keywords like elbow, shoulder one, shoulder two, left, right, up, down, and a little."
        )

        while True:
            if max_turns > 0 and turn >= max_turns:
                print("✓ Reached max turns")
                break

            self._say("Main menu. Choose elbow, shoulder one, or shoulder two.")

            joint = self._listen_for_choice(
                "Do you want to calibrate elbow, shoulder one, or shoulder two?",
                self._parse_joint,
            )
            if joint is None:
                break

            side = self._listen_for_choice("Left or right?", self._parse_side)
            if side is None:
                break

            direction = self._listen_for_choice("Up or down?", self._parse_direction)
            if direction is None:
                break

            amount = self._listen_for_choice(
                "How much? For now, say a little.",
                self._parse_amount,
            )
            if amount is None:
                break

            turn += 1
            self._execute_arm_command(side=side, joint=joint, direction=direction, amount=amount, turn=turn)
            last_selection = (side, joint, direction, amount)

            while True:
                follow_up = self._listen_for_choice(
                    "Say some more, reverse, enough for main menu, or stop.",
                    self._parse_follow_up,
                )
                if follow_up is None or follow_up == "stop":
                    print("👋 Ending arm calibration session")
                    return

                if follow_up == "menu":
                    break

                if follow_up == "more" and last_selection is not None:
                    side, joint, direction, amount = last_selection
                    turn += 1
                    self._execute_arm_command(side=side, joint=joint, direction=direction, amount=amount, turn=turn)
                    continue

                if follow_up == "reverse" and last_selection is not None:
                    side, joint, direction, amount = last_selection
                    direction = self._invert_direction(direction)
                    last_selection = (side, joint, direction, amount)
                    turn += 1
                    self._execute_arm_command(side=side, joint=joint, direction=direction, amount=amount, turn=turn)
                    continue

        print("👋 Ending arm calibration session")

