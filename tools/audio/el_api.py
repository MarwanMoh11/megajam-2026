"""ElevenLabs API calls for the audio step (stdlib only).

The key is read from ELEVENLABS_API_KEY: the process environment first, then the User environment in the registry
(HKCU\\Environment, where `setx` puts it). It is never printed, logged or put on a command line.
"""
import json
import os
import urllib.error
import urllib.request

API = "https://api.elevenlabs.io"


def _key():
    key = os.environ.get("ELEVENLABS_API_KEY", "").strip()
    if not key:
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as k:
                key = str(winreg.QueryValueEx(k, "ELEVENLABS_API_KEY")[0]).strip()
        except OSError:
            key = ""
    if not key:
        raise SystemExit("ELEVENLABS_API_KEY is not set (process or User environment).")
    return key


class ApiError(RuntimeError):
    pass


def _request(method, path, body=None, timeout=300):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(API + path, data=data, method=method)
    req.add_header("xi-api-key", _key())
    if data is not None:
        req.add_header("Content-Type", "application/json")
    # A connection that never got through (DNS, connect timeout: URLError, not HTTPError) is retried: nothing was
    # billed. A read timeout (a bare TimeoutError) is not: the server may have done the work.
    import time
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read(), dict(r.headers)
        except urllib.error.HTTPError as e:
            detail = e.read()[:400].decode("utf-8", "replace")
            raise ApiError(f"HTTP {e.code} on {path.split('?')[0]}: {detail}") from None
        except urllib.error.URLError:
            if attempt == 3:
                raise
            time.sleep(10 * (attempt + 1))


def get_json(path):
    raw, _ = _request("GET", path)
    return json.loads(raw)


def subscription():
    """character_count (credits used this period) and character_limit."""
    s = get_json("/v1/user/subscription")
    return {"used": s.get("character_count"), "limit": s.get("character_limit"), "tier": s.get("tier"),
            "status": s.get("status")}


def voices():
    return get_json("/v1/voices").get("voices", [])


def tts(voice_id, text, model_id, settings, output_format="mp3_44100_128"):
    body = {"text": text, "model_id": model_id, "voice_settings": settings}
    raw, _ = _request("POST", f"/v1/text-to-speech/{voice_id}?output_format={output_format}", body)
    return raw


def sound_effect(text, duration_seconds=None, prompt_influence=0.5, loop=False, output_format="mp3_44100_128"):
    body = {"text": text, "prompt_influence": prompt_influence, "loop": bool(loop),
            "model_id": "eleven_text_to_sound_v2"}
    if duration_seconds:
        body["duration_seconds"] = float(duration_seconds)
    raw, _ = _request("POST", f"/v1/sound-generation?output_format={output_format}", body)
    return raw


def music(prompt, length_ms, output_format="mp3_44100_128", model_id="music_v1"):
    body = {"prompt": prompt, "music_length_ms": int(length_ms), "model_id": model_id, "force_instrumental": True}
    raw, _ = _request("POST", f"/v1/music?output_format={output_format}", body, timeout=600)
    return raw


def design_voice(description, text, model_id="eleven_multilingual_ttv_v2", guidance_scale=3.5, seed=None,
                 output_format="mp3_44100_128"):
    """Voice Design: previews (generated_voice_id, audio bytes) of new voices for a description."""
    import base64
    body = {"voice_description": description, "text": text, "model_id": model_id, "guidance_scale": guidance_scale,
            "loudness": 0.5}
    if seed is not None:
        body["seed"] = int(seed)
    raw, _ = _request("POST", f"/v1/text-to-voice/design?output_format={output_format}", body)
    d = json.loads(raw)
    return [(p["generated_voice_id"], base64.b64decode(p["audio_base_64"])) for p in d.get("previews", [])]


def save_designed_voice(name, description, generated_voice_id):
    """Keeps a Voice Design preview as a voice in the account; returns its voice_id."""
    body = {"voice_name": name, "voice_description": description, "generated_voice_id": generated_voice_id}
    raw, _ = _request("POST", "/v1/text-to-voice", body)
    return json.loads(raw)["voice_id"]
