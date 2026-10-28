#!/usr/bin/env python3
"""Verify a pinned SoT30 v5.5.7 archive or extracted directory (read-only)."""
import argparse
import hashlib
import json
import pathlib
import stat
import sys
import zipfile

VERSION = "v5.5.7"
ZIP_PREFIX = "SoT30_v5.5.7/"
MANIFEST_SHA = "0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff"
INSTRUCTIONS_SHA = "73dcfe4e726cf436d27bf555b33055c5e0bd040d3aaeea85446b50df6dce8deb"
SUMS_SHA = "d6bfbc25992274f96a856101e354f534423fd181eaf0e50b9ebd2a7fad3106b3"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verify(src: pathlib.Path, is_zip: bool) -> bool:
    problems = []
    archive = zipfile.ZipFile(src) if is_zip else None
    prefix = ZIP_PREFIX if archive else ""
    try:
        if archive:
            all_names = archive.namelist()
            if len(all_names) != len(set(all_names)):
                problems.append("duplicate ZIP members")
            for entry in archive.infolist():
                rest = entry.filename.removeprefix(prefix)
                rel = pathlib.PurePosixPath(rest)
                if (not entry.filename.startswith(prefix) or rest.startswith("/")
                        or ".." in rel.parts or "\\" in rest):
                    problems.append("unsafe ZIP path: " + entry.filename)
                if (entry.external_attr >> 16) & 0o170000 == stat.S_IFLNK:
                    problems.append("symlink: " + entry.filename)
            corrupt = archive.testzip()
            if corrupt:
                problems.append("ZIP CRC error: " + corrupt)

        def read(rel: str) -> bytes:
            if archive:
                return archive.read(prefix + rel)
            path = src.joinpath(*rel.split("/"))
            if path.is_symlink():
                raise ValueError("symlink instead of file: " + str(path))
            return path.read_bytes()

        manifest_raw = read("support/MANIFEST.json")
        if digest(manifest_raw) != MANIFEST_SHA:
            problems.append("manifest differs from pinned source")
        manifest = json.loads(manifest_raw)
        if manifest.get("package_version") != VERSION:
            problems.append("wrong version")
        expected_files = manifest.get("files", [])
        if len(expected_files) != 30 or manifest.get("knowledge_file_count") != 30:
            problems.append("expected 30 Knowledge files")
        if sum(item["bytes"] for item in expected_files) != 4054425:
            problems.append("wrong expected corpus bytes")

        for item in expected_files:
            rel = item["path"]
            raw = read(rel)
            if len(raw) != item["bytes"] or digest(raw) != item["sha256"]:
                problems.append("content differs: " + rel)

        special = {
            "support/PROJECT_INSTRUCTIONS_SOT30.md": INSTRUCTIONS_SHA,
            "support/SHA256SUMS": SUMS_SHA,
        }
        for rel, expected_sha in special.items():
            if digest(read(rel)) != expected_sha:
                problems.append("support differs: " + rel)
        if len(read("support/PROJECT_INSTRUCTIONS_SOT30.md")) != 8200:
            problems.append("instructions size differs")

        checksums = read("support/SHA256SUMS").decode("utf-8").splitlines()
        if len(checksums) != 32:
            problems.append("expected 32 checksum lines")
        for line in checksums:
            expected_hash, rel = line.split("  ", 1)
            if digest(read(rel)) != expected_hash:
                problems.append("checksum file mismatch: " + rel)

        expected_paths = {item["path"] for item in expected_files}
        expected_paths.update({"support/MANIFEST.json", *special})
        if archive:
            real_paths = {n.removeprefix(prefix) for n in all_names if not n.endswith("/")}
        else:
            real_paths = {p.relative_to(src).as_posix() for p in src.rglob("*") if p.is_file()}
        if real_paths != expected_paths:
            problems.append("inventory mismatch (missing/extra): "
                            + str(sorted(real_paths ^ expected_paths)[:10]))

        if problems:
            print("SOT30_INTEGRITY_FAIL")
            for issue in problems:
                print(" -", issue)
            return False

        print(f"SOT30_INTEGRITY_PASS version={VERSION} knowledge=30/30 "
              "sha256sums=32/32 corpus_bytes=4054425 members=33/33")
        print(f"PINNED_MANIFEST_SHA256={MANIFEST_SHA}")
        return True
    except (OSError, KeyError, ValueError, zipfile.BadZipFile) as exc:
        print("SOT30_INTEGRITY_FAIL", type(exc).__name__, str(exc))
        return False
    finally:
        if archive:
            archive.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument("--zip", dest="zipfile", type=pathlib.Path)
    choice.add_argument("--directory", type=pathlib.Path)
    args = parser.parse_args()
    sys.exit(0 if verify(args.zipfile or args.directory, bool(args.zipfile)) else 1)
