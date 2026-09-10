# syntax=docker/dockerfile:1.7
#
# Commit-image Dockerfile (SMALL form) — builds mpv-player/mpv's OSS-Fuzz harnesses on top of the
# org C/C++ base image (ghcr.io/mayhemheroes/base). mpv's OSS-Fuzz build.sh (adapted in mayhem/build.sh)
# vendors FFmpeg + libplacebo + libass from source and pulls expat/fontconfig/freetype2/fribidi/
# harfbuzz/lcms2/uchardet/xxhash as meson wrap subprojects. Everything network-fetched is pre-cached
# HERE, under /opt/mpv-deps (docker build has network); mayhem/build.sh only ever COPIES from that
# cache, so it re-runs fully offline (§6.2 item 9 / §6.5) — all the actual build steps (ffmpeg configure
# + make, mpv meson setup + compile) live in build.sh, not here, so the offline re-run gate exercises
# the real pipeline.
#
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile \
#       --build-arg COMMIT=$(git rev-parse HEAD) \
#       [--build-arg SOURCE_DATE_EPOCH=$(date +%s)] \
#       -t mpv-commit:test .

FROM ghcr.io/savantenvs/base:20260621

ARG COMMIT
ARG REPO_URL
# SOURCE_DATE_EPOCH stays an ARG (available to build.sh); never bake it as ENV — an empty value
# breaks clang. build.sh unsets it when empty.
ARG SOURCE_DATE_EPOCH

# Sanitizer override: the base exports SANITIZER_FLAGS (ASan+UBSan, halting) as the default.
# Declare it WITHOUT a default so an unset arg lets the base ENV flow through; --build-arg overrides.
ARG SANITIZER_FLAGS
ARG MAYHEM_JOBS=

USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true; \
    apt-get update && apt-get install -yq --no-install-recommends \
      nasm pkgconf gperf zip python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Meson is pinned in its own venv. Originally pinned to 1.7.2 to dodge
# https://github.com/mesonbuild/meson/issues/14524 (backend_max_links broken in 1.8.0); that bug was
# fixed for 1.8.1, so we've moved to 1.11.2 — libass's fontconfig fallback subproject (fetched fresh
# from WrapDB below) now requires meson >= 1.11.0.
ENV PYTHONDONTWRITEBYTECODE=1
ENV VIRTUAL_ENV=/opt/mayhem-venv
ENV PATH="$VIRTUAL_ENV/bin:$PATH"
RUN python3 -m venv "$VIRTUAL_ENV" && \
    "$VIRTUAL_ENV/bin/pip" --no-cache-dir install meson==1.11.2 ninja && \
    chown -R 2000:2000 "$VIRTUAL_ENV"

# Pre-fetch (network, HERE at docker-build time only) everything mayhem/build.sh needs to compile
# fully offline: FFmpeg source, libplacebo + libass (subprojects mpv wants at mpv/subprojects/*), and
# the libass regression-test font baked into the fontconfig sysroot so libass/harfbuzz fuzzing paths
# can actually shape+render text without depending on real system fonts. This is a READ-ONLY cache
# under /opt — build.sh copies from it, never fetches.
RUN mkdir -p /opt/mpv-deps /opt/mpv-assets && cd /opt/mpv-deps && \
    git clone --depth 1 https://github.com/FFmpeg/FFmpeg ffmpeg && \
    git clone --depth 1 --recursive https://github.com/haasn/libplacebo libplacebo && \
    git clone --depth 1 https://github.com/libass/libass libass && \
    curl -fL https://github.com/libass/libass-tests/raw/613d615deaa48863ce6bd731762696a186c6fd17/regression/.fonts/FansubBlock-CFF.otf \
      -o /opt/mpv-assets/FansubBlock-CFF.otf && \
    chmod -R a+rX /opt/mpv-deps /opt/mpv-assets
# Writable by mayhem: build.sh snapshots the meson-wrap subprojects (expat/fontconfig/freetype2/
# fribidi/harfbuzz/lcms2/uchardet/xxhash) here right after fetching them, once, with network — see
# the matching restore in build.sh (§6.2 item 16: subprojects/ is gitignored, wiped by a clean).
RUN mkdir -p /opt/mpv-wrap-cache && chown -R 2000:2000 /opt/mpv-wrap-cache

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN bash mayhem/build.sh

# Snapshot the meson-wrap packagecache tarballs build.sh just fetched (network, first run only)
# into the writable cache under /opt, so a later offline rebuild on a git-clean-ed tree (§6.2 item
# 16, which removes gitignored packagecache/ but not the untracked-not-ignored .wrap files) can
# restore + extract them locally instead of re-fetching from WrapDB.
RUN mkdir -p /opt/mpv-wrap-cache/packagecache && \
    cp subprojects/packagecache/* /opt/mpv-wrap-cache/packagecache/

# Run mpv's behavioral KAT oracle at build time (a failing oracle fails the image build).
RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
