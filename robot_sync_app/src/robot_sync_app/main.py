import argparse

from robot_sync_app.bootstrap.container import build_orchestrator, build_voice_session


def main() -> None:
    parser = argparse.ArgumentParser(description="Robot speech+gesture+face orchestrator")
    parser.add_argument("--config", default="config/config.yaml", help="Path to YAML config")
    parser.add_argument("--text", default="", help="Text to speak (text mode)")
    parser.add_argument("--intent", default="chat", help="Intent label (chat|quiz|arm_calibration)")
    parser.add_argument("--voice", action="store_true", help="Run voice loop (mic->ASR->LLM->TTS)")
    parser.add_argument("--max-turns", type=int, default=0, help="Max turns for voice loop (0=infinite)")
    args = parser.parse_args()

    if args.voice:
        session = build_voice_session(args.config)
        session.run(intent=args.intent, max_turns=args.max_turns)
        return

    if not args.text.strip():
        raise ValueError("In text mode, --text is required")

    orchestrator = build_orchestrator(args.config)
    orchestrator.run_once(text=args.text, intent=args.intent)


if __name__ == "__main__":
    main()
