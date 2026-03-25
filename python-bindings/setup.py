from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py
from setuptools.command.sdist import sdist

ROOT_DIR = Path(__file__).resolve().parent
VERSION = ROOT_DIR.joinpath("VERSION").read_text(encoding="utf-8").strip()


def copy_tree(src: Path, dst: Path) -> None:
    for path in src.rglob("*"):
        rel = path.relative_to(src)
        target = dst / rel
        if path.is_dir():
            target.mkdir(parents=True, exist_ok=True)
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)


def ensure_bundled_core(base_dir: Path) -> None:
    repo_root = ROOT_DIR.parent
    bundled_root = base_dir / "localmip_core"
    bundled_root.mkdir(parents=True, exist_ok=True)
    copy_tree(repo_root / "src", bundled_root / "src")
    shutil.copy2(repo_root / "LICENSE", base_dir / "LICENSE")


class CMakeExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name, sources=[])


class CMakeBuild(build_ext):
    def build_extension(self, ext: Extension) -> None:
        ext_fullpath = Path(self.get_ext_fullpath(ext.name)).resolve()
        extdir = ext_fullpath.parent
        build_temp = Path(self.build_temp).resolve() / ext.name
        build_temp.mkdir(parents=True, exist_ok=True)

        cfg = "Debug" if self.debug else "Release"
        cmake_args = [
            "cmake",
            "-S",
            str(Path(__file__).resolve().parent),
            "-B",
            str(build_temp),
            f"-DCMAKE_BUILD_TYPE={cfg}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-Dpybind11_DIR={self._get_pybind11_cmake_dir()}",
            "-Wno-dev",
        ]
        build_args = ["cmake", "--build", str(build_temp), "--config", cfg]

        subprocess.run(cmake_args, check=True)
        subprocess.run(build_args, check=True)

        built_module = next(build_temp.glob("localmip_py*.so"), None)
        if built_module is None:
            raise RuntimeError(f"Built extension not found in {build_temp}")

        extdir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built_module, ext_fullpath)

    @staticmethod
    def _get_pybind11_cmake_dir() -> str:
        import pybind11

        return pybind11.get_cmake_dir()


class BundledSdist(sdist):
    def make_release_tree(self, base_dir: str, files: Iterable[str]) -> None:
        super().make_release_tree(base_dir, files)
        ensure_bundled_core(Path(base_dir))


class BundledBuildPy(build_py):
    def run(self) -> None:
        super().run()
        ensure_bundled_core(Path(self.build_lib))


setup(
    name="localmip",
    version=VERSION,
    description="Python bindings for the Local-MIP solver",
    long_description=Path("README.md").read_text(encoding="utf-8"),
    long_description_content_type="text/markdown",
    packages=[],
    py_modules=[],
    ext_modules=[CMakeExtension("localmip_py")],
    cmdclass={
        "build_ext": CMakeBuild,
        "sdist": BundledSdist,
        "build_py": BundledBuildPy,
    },
    license="MIT",
    license_files=("LICENSE",),
    zip_safe=False,
)
