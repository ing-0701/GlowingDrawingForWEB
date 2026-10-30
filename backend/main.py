from pathlib import Path
from uuid import UUID, uuid4

from fastapi import FastAPI, HTTPException, UploadFile
from fastapi.responses import FileResponse

import logging
import subprocess
import tempfile

app = FastAPI()

VIDEO_DIR = Path("/data/videos")
VIDEO_DIR.mkdir(parents=True, exist_ok=True)

MAX_VIDEO_BYTES = 20 * 1024 * 1024


@app.get("/api/health")
def health():
    return {"status": "ok"}


@app.post("/api/videos")
def upload_video(file: UploadFile):
    try:
        # まずは現在の録画形式であるWebMだけを受け付ける
        content_type = (file.content_type or "").split(";")[0].strip()

        if content_type != "video/webm":
            raise HTTPException(
                status_code=415,
                detail="WebM動画を選択してください。",
            )

        # 上限より1バイト多く読み、容量超過を判定する
        data = file.file.read(MAX_VIDEO_BYTES + 1)

        if not data:
            raise HTTPException(
                status_code=400,
                detail="動画が空です。",
            )

        if len(data) > MAX_VIDEO_BYTES:
            raise HTTPException(
                status_code=413,
                detail="動画は20MiB以下にしてください。",
            )

        video_id = uuid4().hex
        output_path = VIDEO_DIR / f"{video_id}.mp4"

        # 変換途中のファイルは、一時ディレクトリに置く
        with tempfile.TemporaryDirectory(dir=VIDEO_DIR) as temp_dir:
            temp_path = Path(temp_dir)
            input_path = temp_path / "input.webm"
            converted_path = temp_path / "output.mp4"

            # ブラウザから受け取ったWebMを書き込む
            input_path.write_bytes(data)

            try:
                subprocess.run(
                    [
                        "ffmpeg",
                        "-hide_banner",
                        "-loglevel", "error",
                        "-nostdin",
                        "-i", str(input_path),
                        "-map", "0:v:0",
                        "-an",
                        "-vf", "pad=ceil(iw/2)*2:ceil(ih/2)*2",
                        "-c:v", "libx264",
                        "-preset", "fast",
                        "-crf", "20",
                        "-pix_fmt", "yuv420p",
                        "-movflags", "+faststart",
                        str(converted_path),
                    ],
                    check=True,
                    capture_output=True,
                    text=True,
                    timeout=45,
                )
            except subprocess.TimeoutExpired:
                logging.exception("動画変換がタイムアウトしました")
                raise HTTPException(
                    status_code=504,
                    detail="動画の変換に時間がかかりすぎました。",
                )
            except subprocess.CalledProcessError as error:
                logging.error("FFmpegの変換エラー: %s", error.stderr)
                raise HTTPException(
                    status_code=500,
                    detail="動画をMP4に変換できませんでした。",
                )

            # 変換が成功したMP4だけを、保存先に移す
            converted_path.replace(output_path)

        return {
            "id": video_id,
            "download_path": f"/api/videos/{video_id}",
        }
    finally:
        file.file.close()


@app.get("/api/videos/{video_id}")
def download_video(video_id: UUID):
    path = VIDEO_DIR / f"{video_id.hex}.mp4"
    media_type = "video/mp4"
    filename = "painting.mp4"

    # 変更前に保存したWebMも、従来のURLから取得できるようにする
    if not path.is_file():
        path = VIDEO_DIR / f"{video_id.hex}.webm"
        media_type = "video/webm"
        filename = "painting.webm"

    if not path.is_file():
        raise HTTPException(
            status_code=404,
            detail="動画が見つかりません。",
        )

    return FileResponse(
        path,
        media_type=media_type,
        filename=filename,
    )