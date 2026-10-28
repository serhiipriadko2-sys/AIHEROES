#!/usr/bin/env python3
"""Install a verified, immutable SoT30 v5.5.7 copy for this workspace."""
import argparse
import pathlib
import shutil
import sys
import tempfile
import zipfile

from verify_sot30 import ZIP_PREFIX, verify


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zip", type=pathlib.Path, required=True)
    parser.add_argument(
        "--destination", type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[1] / ".iskra" / "sot30",
    )
    args = parser.parse_args()
    source, dest = args.zip.resolve(), args.destination.resolve()

    if not verify(source, is_zip=True):
        print("INSTALL_BLOCKED: source integrity failed")
        return 1
    if dest.is_dir():
        if verify(dest, is_zip=False):
            print("ALREADY_INSTALLED", str(dest))
            return 0
        print("INSTALL_BLOCKED: existing canon differs; refusing overwrite")
        return 1
    if dest.exists():
        print("INSTALL_BLOCKED: destination exists and is not a directory")
        return 1

    dest.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".sot30-stage-", dir=dest.parent) as temp:
        stage = pathlib.Path(temp) / "sot30"
        stage.mkdir()
        with zipfile.ZipFile(source) as archive:
            for entry in archive.infolist():
                if entry.is_dir():
                    continue
                relative = entry.filename.removeprefix(ZIP_PREFIX)
                path = stage.joinpath(*relative.split("/"))
                path.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(entry, "r") as src, path.open("xb") as dst:
                    shutil.copyfileobj(src, dst)

        if not verify(stage, is_zip=False):
            print("INSTALL_BLOCKED: staging verification failed")
            return 1
        stage.rename(dest)
    print("INSTALLED_VERIFIED_SOT30", str(dest))
    return 0


if __name__ == "__main__":
    sys.exit(main())
