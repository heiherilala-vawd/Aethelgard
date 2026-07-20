#!/usr/bin/env python3
"""
Carte de prévisualisation des biomes — Aethelgard.
Utilise les mêmes paramètres et algorithmes que GenerationDefaults.h.
Modifie la section PARAMS pour tester de nouveaux réglages.

Usage:
    pip install -r requirements.txt
    python map_preview.py [--size 1024] [--seed 0] [--out preview.png]
"""

import math
import random
import argparse
import sys
from dataclasses import dataclass
from typing import Tuple

import noise  # pip install noise
from PIL import Image

# ═══════════════════════════════════════════════════════════════════════════════
# PARAMÈTRES  —  modifie cette section pour tester de nouveaux réglages
# Doit correspondre à GenerationDefaults.h
# ═══════════════════════════════════════════════════════════════════════════════

PARAMS = {
    # Bruits — Macro
    "macro_scale": 0.0002,
    "macro_amplitude": 130.0,
    "macro_octaves": 2,
    "macro_persistence": 0.7,
    "macro_lacunarity": 4.0,
    # Bruits — BaseShape
    "base_shape_scale": 0.001,
    "base_shape_amplitude": 15.0,
    "base_shape_persistence": 0.5,
    "base_shape_lacunarity": 2.0,
    # Bruits — Méso
    "meso_scale": 0.025,
    "meso_amplitude": 8.0,
    "meso_octaves": 2,
    "meso_persistence": 0.5,
    "meso_lacunarity": 2.0,
    # Bruits — Micro
    "micro_scale": 0.08,
    "micro_amplitude": 3.0,
    "micro_octaves": 1,
    "micro_persistence": 0.5,
    "micro_lacunarity": 2.0,
    # Élévation
    "global_elevation": 75.0,
    # Zonage
    "sea_level": 70.0,
    "mountain_start": 110.0,
    "max_height": 200.0,
    # Voronoï (séparation biomes)
    "voronoi_scale": 0.002,
    # Eau
    "lake_threshold": 95.0,
    "lake_depth": 6,
    "sea_depth_slope": 0.05,
    "sea_max_depth": 25.0,
    "lake_depth_slope": 0.05,
    # Montagne
    "mountain_shape_scale": 0.03,
    "mountain_shape_amplitude": 90.0,
    "mountain_shape_persistence": 0.5,
    "mountain_shape_lacunarity": 2.0,
    # Collines
    "hill_scale": 0.1,
    "hill_amplitude": 2.0,
    # Divers
    "lake_noise_scale": 0.01,
    "mountain_stone_threshold": 120.0,
    # Hauteurs admissibles par biome (zone médiane)
    "plains_min_height": 1.0,
    "plains_max_height": 110.0,
    "desert_min_height": 1.0,
    "desert_max_height": 95.0,
    "forest_min_height": 1.0,
    "forest_max_height": 110.0,
}

# Couches de bruit (mêmes indices que ENoiseLayer)
N_MACRO = 0
N_BASESHAPE = 1
N_MESO = 2
N_MICRO = 3
N_VORONOI = 4
N_MOUNTAINSHAPE = 5
N_HILLS = 6
N_LAKE = 7

BIOME_HEIGHT_RANGES = None  # built from PARAMS at startup


def _build_ranges(p: dict):
    global BIOME_HEIGHT_RANGES
    BIOME_HEIGHT_RANGES = [
        (p["plains_min_height"], p["plains_max_height"]),
        (p["desert_min_height"], p["desert_max_height"]),
        (p["mountain_start"],    p["max_height"]),
        (p["forest_min_height"], p["forest_max_height"]),
    ]


# ═══════════════════════════════════════════════════════════════════════════════
# BRUITS  (reproduisent le comportement C++)
# ═══════════════════════════════════════════════════════════════════════════════

_offset_cache = {}


def _make_offset(seed: int) -> Tuple[float, float]:
    rng = random.Random(seed)
    return (rng.random() * 10000.0, rng.random() * 10000.0)


def perlin_octaves(x: float, y: float, scale: float,
                   octaves: int, seed: int,
                   persistence: float = 0.5,
                   lacunarity: float = 2.0) -> float:
    value = 0.0
    amplitude = 1.0
    max_amp = 0.0
    freq = 1.0

    for i in range(octaves):
        key = seed + i * 7919
        if key not in _offset_cache:
            _offset_cache[key] = _make_offset(key)
        ox, oy = _offset_cache[key]

        value += amplitude * noise.pnoise2(
            x * scale * freq + ox,
            y * scale * freq + oy,
            octaves=1, repeatx=1024, repeaty=1024, base=0
        )
        max_amp += amplitude
        amplitude *= persistence
        freq *= lacunarity

    return value / max_amp


def voronoi_select(wx: int, wy: int, scale: float, seed: int) -> int:
    """Voronoi → biome zone mediane. Map {0,1,2} → {Plains=0,Desert=1,Forest=3}."""
    cx = int(math.floor(wx * scale))
    cy = int(math.floor(wy * scale))
    h = cx * 73856093 + cy * 19349663 + seed * 83492791
    rng = random.Random(h)
    raw = rng.randint(0, 2)
    return [0, 1, 3][raw]


# ═══════════════════════════════════════════════════════════════════════════════
# GÉNÉRATION  (reproduit ComputeBaseHeight + ComputeColumnAt)
# ═══════════════════════════════════════════════════════════════════════════════

@dataclass
class ColumnResult:
    height: float
    biome: int
    water_surface: int


def compute_base_height(wx: int, wy: int, p: dict, seed: int) -> float:
    macro = perlin_octaves(float(wx), float(wy),
                           p["macro_scale"], p["macro_octaves"],
                           seed + N_MACRO * 7919,
                           p["macro_persistence"], p["macro_lacunarity"])
    baseshape = abs(perlin_octaves(float(wx), float(wy),
                                   p["base_shape_scale"], 1,
                                   seed + N_BASESHAPE * 7919,
                                   p["base_shape_persistence"], p["base_shape_lacunarity"]))
    meso = perlin_octaves(float(wx), float(wy),
                          p["meso_scale"], p["meso_octaves"],
                          seed + N_MESO * 7919,
                          p["meso_persistence"], p["meso_lacunarity"])
    micro = perlin_octaves(float(wx), float(wy),
                           p["micro_scale"], p["micro_octaves"],
                           seed + N_MICRO * 7919,
                           p["micro_persistence"], p["micro_lacunarity"])

    return (p["global_elevation"]
            + macro * p["macro_amplitude"]
            + baseshape * p["base_shape_amplitude"]
            + meso * p["meso_amplitude"]
            + micro * p["micro_amplitude"])


def compute_column(wx: int, wy: int, p: dict, seed: int) -> ColumnResult:
    height = compute_base_height(wx, wy, p, seed)
    height = max(1.0, min(height, p["max_height"]))

    # ---- MER ----
    if height < p["sea_level"]:
        raw = p["sea_level"] - height
        if raw <= p["sea_max_depth"]:
            depth = raw
        else:
            depth = p["sea_max_depth"] + (raw - p["sea_max_depth"]) * p["sea_depth_slope"]
        height = p["sea_level"] - depth
        height = max(1.0, min(height, p["max_height"]))
        ws = int(min(max(p["sea_level"], 0), 255))
        return ColumnResult(height=height, biome=0, water_surface=ws)

    # ---- ZONE MÉDIANE ----
    if height < p["mountain_start"]:
        biome = voronoi_select(wx, wy, p["voronoi_scale"],
                               seed + N_VORONOI * 7919)
        pre_hill = height

        # Collines (Plaines=0, Desert=1)
        if biome in (0, 1):
            hill = abs(perlin_octaves(float(wx), float(wy),
                                      p["hill_scale"], 1,
                                      seed + N_HILLS * 7919))
            height += hill * p["hill_amplitude"]

        # Lacs
        lake_noise = perlin_octaves(float(wx), float(wy),
                                    p["lake_noise_scale"], 1,
                                    seed + N_LAKE * 7919)
        ws = 0
        if lake_noise < -0.25 and pre_hill < p["lake_threshold"] and biome != 1:
            lake_surface = pre_hill
            lake_floor = height - p["lake_depth"] - p["lake_depth_slope"] * 4.0
            height = max(1.0, min(lake_floor, p["max_height"]))
            ws = int(min(max(lake_surface, 0), 255))

        # Blend biome height range (doux, pas de clamp dur)
        min_h, max_h = BIOME_HEIGHT_RANGES[biome]
        if height < min_h:
            height = height + (min_h - height) * 0.5
        elif height > max_h:
            height = height + (max_h - height) * 0.5
        height = max(1.0, min(height, p["max_height"]))

        return ColumnResult(height=height, biome=biome, water_surface=ws)

    # ---- MONTAGNE ----
    mfactor = (height - p["mountain_start"]) / (p["max_height"] - p["mountain_start"])
    mfactor = max(0.0, min(mfactor, 1.0))
    shape = perlin_octaves(float(wx), float(wy),
                           p["mountain_shape_scale"], 2,
                           seed + N_MOUNTAINSHAPE * 7919,
                           p["mountain_shape_persistence"], p["mountain_shape_lacunarity"])
    height += shape * p["mountain_shape_amplitude"] * mfactor * mfactor
    height = max(1.0, min(height, p["max_height"]))
    return ColumnResult(height=height, biome=2, water_surface=0)


# ═══════════════════════════════════════════════════════════════════════════════
# RENDU
# ═══════════════════════════════════════════════════════════════════════════════

def biome_color(biome: int, height: float, water: int, p: dict) -> Tuple[int, int, int]:
    if water > 0:
        return (30, 100, 200)
    if height < p["sea_level"]:
        return (30, 80, 160)
    if biome == 0:   # Plaines
        return (34, 139, 34)
    elif biome == 1: # Désert
        return (210, 180, 100)
    elif biome == 2: # Montagne
        if height > p["mountain_stone_threshold"]:
            return (160, 160, 160)
        else:
            return (80, 130, 60)
    else:            # Forêt
        return (20, 100, 20)


def generate_map(size: int, p: dict, seed: int) -> Image.Image:
    _build_ranges(p)
    img = Image.new("RGB", (size, size))
    pix = img.load()

    world_range = 32768
    step = world_range / size

    print(f"Génération {size}×{size} pixels  (seed={seed}) ...")

    for py in range(size):
        if py % 128 == 0:
            print(f"  ligne {py}/{size}")
        wy = int((py - size / 2) * step)
        for px in range(size):
            wx = int((px - size / 2) * step)
            col = compute_column(wx, wy, p, seed)
            c = biome_color(col.biome, col.height, col.water_surface, p)
            pix[px, py] = c

    return img


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Prévisualisation biomes Aethelgard")
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--out", type=str, default="carte.png")
    args = parser.parse_args()

    img = generate_map(args.size, PARAMS, args.seed)
    img.save(args.out)
    print(f"\nCarte sauvegardée : {args.out}")
    print(f"Modifie la section PARAMS dans {sys.argv[0]} pour tester de nouveaux réglages.")


if __name__ == "__main__":
    main()
